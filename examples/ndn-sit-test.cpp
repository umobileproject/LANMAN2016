/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

// ndn-simple-with-link-failure.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

// for LinkStatusControl::FailLinks and LinkStatusControl::UpLinks
#include "ns3/ndnSIM/helper/ndn-link-control-helper.hpp"

// for removing fib entries
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"

// for scheduling application-level events (e.g., SendPacket)
#include "ns3/ndnSIM/apps/ndn-consumer.hpp"
#include "ns3/ndnSIM/apps/ndn-consumer-cbr.hpp"
#include "ns3/ndnSIM/apps/ndn-consumer-sit.hpp"
#include "ns3/application.h"
#include "ns3/ptr.h"

// for random number generator
#include <random>

// for Rocketfuel topology reading
#include "ns3/ndnSIM/utils/topology/rocketfuel-map-reader.hpp"

// for generating Zipf distributed content
#include "ns3/ndnSIM/apps/ndn-consumer-zipf-mandelbrot.hpp"

namespace ns3 {

/**
 * This scenario simulates a very simple network topology:
 *
 *          (0)                         (1)                      (5)                         (2)
 *      +----------+     1Mbps      +---------+      1Mbps   +---------+     1Mbps      +----------+
 *      | consumer1| <------------> | router1 | <----------->| router2 | <------------> | producer |
 *      +----------+     10ms       +---------+      10ms    +---------+     10ms       +----------+
 *                                                                ^
 *                                                                |
 *                                                                |
 *                                                                v
 *                                                           +---------+
 *                                                           | router3 | (4)
 *                                                           +---------+
 *                                                                ^
 *                                                                |
 *                                                                |
 *                                                                v
 *                                                          +----------+
 *                                                          | consumer2| (3)
 *                                                          +----------+
 *
 *
 * Consumer requests data from producer with frequency 10 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-simple-with-link-failure
 */

template<typename T>
void set_new_lambda(std::exponential_distribution<T> *exp_dis, T val)
{
  typename std::exponential_distribution<T>::param_type new_lambda(val);
  exp_dis->param(new_lambda);
}

template<typename TargetType>
TargetType convert(const std::string& value) {
    TargetType converted;
    std::istringstream stream(value);
    
    stream >> converted;

    return converted;
}

int
main(int argc, char* argv[])
{
  //Parameters of the simulation (to be read from the command line)
  int num_contents;
  double connection_rate;
  double disconnection_rate;
  double initialization_period_length;
  double observation_period_length;
  double zipf_exponent;
  int cache_size;
  std::string topology_file;

  std::cout<<"Number of arguments: "<<argc<<"\n";
  if(argc < 9)
  {
    std::cout<<"Invalid number of parameters: "<<argc<<", Expecting 9\n";
    exit(0);
  }

  // setting default parameters for PointToPoint user node links
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("2ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("20"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd; 
  cmd.AddValue ("num_contents", "Number of contents available", num_contents);
  cmd.AddValue ("connection_rate", "Rate at which connection arrive <0-1>", connection_rate);
  cmd.AddValue ("disconnection_rate", "Rate at which users disconnect <0-1>", disconnection_rate);
  cmd.AddValue ("initialization_period_length", "Length of initialization period in seconds (double var)", initialization_period_length);
  cmd.AddValue ("observation_period_length", "Length of observation period in seconds (double var)", observation_period_length);
  cmd.AddValue ("zipf_exponent", "Content popularity dist. zipf exponent", zipf_exponent);
  cmd.AddValue ("cache_size", "Size of the cache on routers", cache_size);
  cmd.AddValue ("topology_file", "Name of the topology file", topology_file);
  cmd.Parse(argc, argv);
  
  std::cout<<"Params\n";
  std::cout<<"num_contents "<<num_contents<<"\n";
  std::cout<<"connection_rate "<<connection_rate<<"\n";
  std::cout<<"disconnection_rate "<<disconnection_rate<<"\n";
  std::cout<<"initialization_period_length "<<initialization_period_length<<"\n";
  std::cout<<"observation_period_length "<<observation_period_length<<"\n";
  std::cout<<"zipf_exponent "<<zipf_exponent<<"\n";
  std::cout<<"cache_size "<<cache_size<<"\n";
  std::cout<<"topology_file "<<topology_file<<"\n";
  std::cout<<"End_of_Params\n";

// Prepare the Topology
  // Read Rocketfuel topology and set producer 
  RocketfuelParams params;
  params.averageRtt = 2.0;
  params.clientNodeDegrees = 2;
  params.minb2bDelay = "1ms";
  params.minb2bBandwidth = "10Mbps";
  params.maxb2bDelay = "6ms";
  params.maxb2bBandwidth = "100Mbps";
  params.minb2gDelay = "1ms";
  params.minb2gBandwidth = "10Mbps";
  params.maxb2gDelay = "2ms";
  params.maxb2gBandwidth = "50Mbps";
  params.ming2cDelay = "1ms";
  params.ming2cBandwidth = "1Mbps";
  params.maxg2cDelay = "3ms";
  params.maxg2cBandwidth = "10Mbps";

  RocketfuelMapReader topo_reader("", 10);
  std::string topo_file_name = "/home/onur/Downloads/rocketfuel_maps_cch/" + topology_file;
  topo_reader.SetFileName(topo_file_name);
  NodeContainer nodes = topo_reader.Read(params, true, true);

  // Read network (infrastructure) topology from a file 
  /*
  AnnotatedTopologyReader topologyReader("", 10);
  std::string topo_file_name = "src/ndnSIM/examples/topologies/" + topology_file;
  topologyReader.SetFileName(topo_file_name);
  NodeContainer nodes = topologyReader.Read();*/
  std::cout << "Number of infrastructure nodes: "<<nodes.GetN() <<"\n"; 
  uint32_t num_infrastructure_nodes = nodes.GetN();

  //Attach a (consumer) node to each router in the topology
  std::map<int, int > app_to_node; //maps app index to access node index
  std::map<int, int > access_to_router; //maps access node index to the next hop router index
  uint32_t num_connected = 0;
  // for each of the n infrastructure node attach a user node
  // n-1 customer nodes and a producer node
  NodeContainer consumer_nodes, producer_node, infrastructure_nodes;
  consumer_nodes.Create(num_infrastructure_nodes-1);
  producer_node.Create(1);
  nodes.Add(consumer_nodes);
  nodes.Add(producer_node);
  PointToPointHelper p2p;
  std::cout <<"Num total nodes: "<<nodes.GetN()<<"\n";
  for(uint32_t i = 0; i < num_infrastructure_nodes; i++)
  {
    p2p.Install(nodes.Get(i), nodes.Get(i+num_infrastructure_nodes));
	 app_to_node.insert(std::make_pair(i, i+num_infrastructure_nodes));
    access_to_router.insert(std::make_pair(i+num_infrastructure_nodes, i));
  }
  for(uint32_t i = 0; i < num_infrastructure_nodes; i++)
  {
    infrastructure_nodes.Add(nodes.Get(i));
  }
// Install NDN stack on all nodes
  ndn::StackHelper ndnHelperCaching;
  ndnHelperCaching.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", std::to_string(cache_size));
  ndnHelperCaching.Install(infrastructure_nodes);
  
  ndn::StackHelper ndnHelperInfCaching;
  ndnHelperInfCaching.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", std::to_string(num_contents));
  ndnHelperInfCaching.Install(consumer_nodes);
  ndnHelperInfCaching.Install(producer_node);

  // Set BestRoute strategy
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Installing applications 
  std::string prefix = "/prefix";
  ApplicationContainer consumer_apps;
  for(uint32_t i = 0; i < consumer_nodes.GetN(); i++)
  {
    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerSit");
    consumerHelper.SetPrefix(prefix);
    // install consumer app on node i
    consumer_apps.Add(consumerHelper.Install(consumer_nodes.Get(i)));
  }
  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix(prefix);
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(producer_node); // last node
  
  // Add /prefix origins to ndn::GlobalRouter
  ndnGlobalRoutingHelper.AddOrigins(prefix, producer_node);

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();
  
  /****************************************************************/
  
  //Setup Simulation Events (connection, disconnection, etc)

  // Content Distribution 
  ndn::ConsumerZipfMandelbrot content_dist(num_contents, 0, zipf_exponent);

  /***  Initialization Period: ***/
  double connect_time = 0.2;
  double disconnect_time = 0.2;
  std::random_device rd; 
  std::exponential_distribution<double> rng_exp_con (connection_rate);
  std::exponential_distribution<double> rng_exp_dis (disconnection_rate);
  std::mt19937 rnd_gen (rd ()); //initialize the random number generator
  // The number of each content connected at each node
  std::map<int, int> connected_content[consumer_apps.GetN()];
  do 
  {
	 uint32_t app_indx = rnd_gen()%(consumer_apps.GetN());
	 std::cout << "Picked a random node: "<<app_indx<<"\n";
    ns3::Application *app_ptr = PeekPointer(consumer_apps.Get(app_indx));
    ndn::ConsumerSit *cons = reinterpret_cast<ndn::ConsumerSit *>(app_ptr);
    uint32_t content_indx = content_dist.GetNextSeq();
	 std::cout << "Picked a random content: "<<content_indx<<"\n";
	 std::cout << "Picked a random time: "<<connect_time<<"\n\n";
    Simulator::Schedule(Seconds(connect_time), &ndn::Consumer::SendPacketWithSeq, cons, content_indx);
	 num_connected++;
    connect_time = connect_time + rng_exp_con(rnd_gen);
	 // Bookkeeping of connected contents, their locations and numbers
	 connected_content[app_indx][content_indx]++;
  }while(connect_time < initialization_period_length);

//Disconnect Producer from the topology and notify all routers
  //Remove all the FIB table entries for /prefix from all nodes
  const ndn::Name n("ndn://" + prefix); 
  for(uint32_t indx = 0; indx < num_infrastructure_nodes; indx++)
  {
    Simulator::Schedule(Seconds(initialization_period_length), (void (*)(Ptr<Node>, const ndn::Name&)) (&ndn::FibHelper::RemoveRoute), nodes.Get(indx), n);
  }
  
  /***  Observation Period: ***/ 
  connect_time = initialization_period_length + 0.2;
  disconnect_time = connect_time;
  double connect_time_next;
	 
  std::cout << "\n\nBeginning of Observation Period: \n\n";
  
  while(connect_time < observation_period_length + initialization_period_length)
  {
	 uint32_t app_indx = rnd_gen()%(consumer_apps.GetN());
	 std::cout << "Picked a random node: "<<app_indx<<"\n";
    ns3::Application *app_ptr = PeekPointer(consumer_apps.Get(app_indx));
    ndn::ConsumerSit *cons = reinterpret_cast<ndn::ConsumerSit *>(app_ptr);
    uint32_t content_indx = content_dist.GetNextSeq();
	 std::cout << "Picked a random content: "<<content_indx<<"\n";
	 std::cout << "Picked a random time: "<<connect_time<<"\n\n";
    Simulator::Schedule(Seconds(connect_time), &ndn::Consumer::SendPacketWithSeq, cons, content_indx);
    connect_time_next = connect_time + rng_exp_con(rnd_gen);
	 //while(disconnect_time < connect_time_next)
    while(0)
	 { //randomly pick a connected content and disconnect
	   if(0 == num_connected)
		{
		  std::cout<<"ERROR: Out of connected content \n";
		  break;
		}
		uint32_t app_indx;
	   do
		{
        app_indx = rnd_gen()%(consumer_apps.GetN());
		}while(connected_content[app_indx].begin() == connected_content[app_indx].end());
		auto it = connected_content[app_indx].begin();
		std::advance(it, rnd_gen() % connected_content[app_indx].size());
		it->second = it->second - 1;
		if(0 == it->second)
		{
		  const ndn::Name content("ndn://" + prefix + std::to_string(it->first));
        int access_node = app_to_node[app_indx];
		  int router_node = access_to_router[access_node];
        std::cout<<"Removing from SIT table: "<< it->first<<" at node: "<<app_indx<<"residing at node: "<<access_node<<" at time: "<<disconnect_time<<"\n\n";
        Simulator::Schedule(Seconds(disconnect_time), (void (*)(Ptr<Node>, const ndn::Name&, Ptr<Node>)) (&ndn::FibHelper::RemoveRoute), nodes.Get(router_node), content, nodes.Get(access_node));
		  connected_content[app_indx].erase(it);
		}
		num_connected--;
      std::cout<<"Removing content: "<< it->first<<" from node: "<<app_indx<<" at time: "<<disconnect_time<<"\n\n";
      double lambda;
      if(0 == num_connected)
	     lambda = disconnection_rate*1;
      else
	     lambda = disconnection_rate*num_connected;
	   set_new_lambda(&rng_exp_dis, lambda);
      std::cout<< "Settng lambda to: "<<rng_exp_dis.lambda()<<"\n";
	   disconnect_time = disconnect_time + rng_exp_dis(rnd_gen);
      std::cout<<"New disconnection time: "<<disconnect_time<<"\n";
	 }//End of disconnections
	 connect_time = connect_time_next;
  }//End of Observation Period
         
  Simulator::Stop(Seconds(observation_period_length + initialization_period_length));

  Simulator::Run();
  Simulator::Destroy();
  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}

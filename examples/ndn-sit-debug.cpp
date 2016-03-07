
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

#include <chrono> //for sleep_for call
#include <thread> 

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

// for LinkStatusControl::FailLinks and LinkStatusControl::UpLinks
#include "ns3/ndnSIM/helper/ndn-link-control-helper.hpp"

// for removing fib entries
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"

// for removing cache entries
#include "ns3/ndnSIM/model/cs/ndn-content-store.hpp"

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

#include "ns3/log.h"

// for obtaining forwarder of a node
#include "ns3/ndnSIM/model/ndn-l3-protocol.hpp"

//string comparison case insensitive
#include <boost/algorithm/string.hpp>

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

NS_LOG_COMPONENT_DEFINE ("SitTest");

template<typename T>
void set_new_lambda(std::exponential_distribution<T> *exp_dis, T val)
{
  typename std::exponential_distribution<T>::param_type new_lambda(val);
  exp_dis->param(new_lambda);
}

template<typename TargetType>
TargetType convert(const std::string& value) 
{
    TargetType converted;
    std::istringstream stream(value);
    
    stream >> converted;

    return converted;
}

uint32_t get_cost(NodeContainer &nodes, uint32_t app_indx, uint32_t producer_indx)
{
  uint32_t cost;
  Ptr<Node> n = nodes.Get(app_indx);
  Ptr<ndn::L3Protocol> p =  ndn::L3Protocol::getL3Protocol(n);
  shared_ptr<nfd::Forwarder> f = p->getForwarder();
  ndn::Name name("/prefix");
  name.appendNumber(producer_indx);
  NS_LOG_INFO("About to get cost to "<<name<<" from "<<app_indx);
  NS_LOG_DEBUG("About to get cost to "<<name<<" from "<<app_indx);
  if(f->getFib().findLongestPrefixMatch(name)->hasNextHops())
    cost = f->getFib().findLongestPrefixMatch(name)->getNextHops()[0].getCost();
  else
  {
    NS_LOG_INFO("Cost to "<<name<<" is not known, set to 0 ");
    NS_LOG_DEBUG("Cost to "<<name<<" is not known, set to 0 ");
    cost = 0;
  }
  NS_LOG_INFO("Cost to "<<name<<" is "<<cost);
  NS_LOG_DEBUG("Cost to "<<name<<" is "<<cost);

  return cost;
}

void Schedule_Send(ApplicationContainer consumer_apps, uint32_t app_indx, double connect_time, uint32_t producer_indx, uint32_t scoped_downstream_counter, uint32_t content_indx, uint32_t num_chunks)
{
  double interpacket = 0.008192; //num. of secs btw outgoing packets (i.e. 1024bytes/10_Mbits/sec)
    
  ns3::Application *app_ptr = PeekPointer(consumer_apps.Get(app_indx));
  if (!static_cast<bool> (app_ptr) )
    NS_LOG_INFO("app pointer is null ");
  
  ndn::ConsumerSit *cons = reinterpret_cast<ndn::ConsumerSit *>(app_ptr);
  if (!static_cast<bool> (cons) )
    NS_LOG_INFO("cons is null ");
  //NS_LOG_INFO("App_indx: "<<app_indx<<" producer_indx: "<<producer_indx<<" num_chunks "<<num_chunks<<" connect_time "<<connect_time);
  
  for (uint32_t chunk = content_indx; chunk < content_indx + num_chunks; chunk++)
  {
    NS_LOG_DEBUG("Sending from "<<app_indx<<" to "<<content_indx<<" at "<<connect_time);
    NS_LOG_INFO("Sending from "<<app_indx<<" to "<<content_indx<<" at "<<connect_time);
    Simulator::Schedule(Seconds(connect_time), &ndn::Consumer::SendPacketWithSeq, cons, producer_indx, chunk, scoped_downstream_counter); 
    connect_time += interpacket;
  }
}

//NodeContainer nodes;

// Run with: NS_LOG=ndn.Consumer=info:SitTest=info:ndn.cs.Lru=info:nfd.FibManager=info:nfd.Forwarder=info:nfd.Cfib=info:nfd.FibEntry=info
int
main(int argc, char* argv[])
{
  LogComponentEnable("nfd.Forwarder", LOG_PREFIX_ALL);  //print time, node , etc. information for each log
  LogComponentEnable("nfd.FibManager", LOG_PREFIX_ALL); 
  LogComponentEnable("nfd.Cfib", LOG_PREFIX_ALL); 
  LogComponentEnable("nfd.BestRouteStrategy2", LOG_PREFIX_ALL); 
  LogComponentEnable("nfd.MulticastStrategy", LOG_PREFIX_ALL); 
  LogComponentEnable("nfd.PickOneStrategy", LOG_PREFIX_ALL); 
  LogComponentEnable("nfd.FibEntry", LOG_PREFIX_ALL);  
  LogComponentEnable("ndn.Consumer", LOG_PREFIX_ALL); 
  LogComponentEnable("ndn.cs.Lru", LOG_PREFIX_ALL); 
  LogComponentEnable("SitTest", LOG_PREFIX_ALL);  
  LogComponentEnable("ndn.cs.ProbabilityImpl", LOG_PREFIX_ALL);  
  //Parameters of the simulation (to be read from the command line)
  int num_contents;
  double connection_rate;
  double simulation_length;
  double zipf_exponent;
  int cache_size;
  std::string topology_file;
  uint32_t scoped_downstream_counter = 0;
  double probability = 1; //for probabilistic
  uint32_t num_chunks = 1;
  std::string strategy;
  uint32_t sit_size = 0;

  if(argc < 12)
  {
    std::cout<<"Invalid number of parameters: "<<argc<<", Expecting 10\n";
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
  cmd.AddValue ("simulation_length", "Length of the simulation in seconds (double var)", simulation_length);
  cmd.AddValue ("zipf_exponent", "Content popularity dist. zipf exponent", zipf_exponent);
  cmd.AddValue ("cache_size", "Size of the cache on routers", cache_size);
  cmd.AddValue ("topology_file", "Name of the topology file", topology_file);
  cmd.AddValue ("scoped_downstream_counter", "Scope in terms of multicast branching factor, the range of search", scoped_downstream_counter);
  cmd.AddValue ("probability", "Probability of caching at each router", probability);
  cmd.AddValue ("num_chunks", "Number of chunks each flow requests", num_chunks);
  cmd.AddValue ("strategy", "Forwarding strategy: send to all or one", strategy);
  cmd.AddValue ("sit_size", "SIT table size", sit_size);
  cmd.Parse(argc, argv);
  
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

 /*
  RocketfuelMapReader topo_reader("", 10);
  std::string topo_file_name = "/home/uceeoas/maps/" + topology_file;
  topo_reader.SetFileName(topo_file_name);
  NodeContainer nodes = topo_reader.Read(params, true, true);
 */
  // Read network (infrastructure) topology from a file 
//  /*
  AnnotatedTopologyReader topologyReader("", 10);
  std::string topo_file_name = "src/ndnSIM/examples/topologies/" + topology_file;
  topologyReader.SetFileName(topo_file_name);
  NodeContainer nodes = topologyReader.Read();
//  */
//

  NS_LOG_INFO("Params");
  NS_LOG_INFO("num_contents "<<num_contents);
  NS_LOG_INFO("connection_rate "<<connection_rate);
  NS_LOG_INFO("simulation_length "<<simulation_length);
  NS_LOG_INFO("zipf_exponent "<<zipf_exponent);
  NS_LOG_INFO("cache_size "<<cache_size);
  NS_LOG_INFO("topology_file "<<topology_file);
  NS_LOG_INFO("scoped_downstream_counter "<<scoped_downstream_counter);
  NS_LOG_INFO("Probability of caching "<<probability);
  NS_LOG_INFO("Number of chunks "<<num_chunks);
  NS_LOG_INFO("Strategy: "<<strategy);
  NS_LOG_INFO("Sit_size: "<<sit_size);
  NS_LOG_INFO("End_of_Params");

  NS_LOG_INFO("Number_of_infrastructure_nodes: "<<nodes.GetN()); 

  std::map<int, int > app_to_node; //maps app index to access node index
  std::map<int, int > access_to_router; //maps access node index to the next hop router index
  uint32_t num_connected = 0; //number of connected applications/users
  uint32_t num_contents_requested_init=0;  //number of unique content items requested during initialization

// Install Content Store: Infrastructure nodes get limited storage
  ndn::StackHelper ndnHelperCaching;
  if(probability == 1)
  {
    ndnHelperCaching.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", std::to_string(cache_size));
  }
  else
  {
    ndnHelperCaching.SetOldContentStore("ns3::ndn::cs::Probability::Lru", "MaxSize", std::to_string(cache_size), "CacheProbability", std::to_string(probability));
  }
  ndnHelperCaching.Install(nodes);
  
  // Set forwarding strategy
  if (boost::iequals(strategy, "ALL"))
  {
    ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/multicast"); //multicast strategy
    NS_LOG_INFO("Multicast Strategy");
  }
  else
  {
    ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/pickone");
    NS_LOG_INFO("PickOne Strategy");
  }
  
  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Installing applications 
  ApplicationContainer consumer_apps;
  std::string prefix = "/prefix";
  for(uint32_t i = 0; i < nodes.GetN(); i++)
  {
    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerSit");
    consumerHelper.SetPrefix(prefix);
    // install consumer app on node i
    consumer_apps.Add(consumerHelper.Install(nodes.Get(i)));
  }
  // Producers at each node
  ApplicationContainer producer_apps;
  for(uint32_t i = 0; i < nodes.GetN(); i++)
  {
    ndn::AppHelper producerHelper("ns3::ndn::Producer");
    // Producer will reply to all requests starting with /prefix
    ndn::Name n ("/prefix");
    n.appendNumber(i);
    producerHelper.SetPrefix(n.toUri());
    NS_LOG_INFO("Name set to: "<<n);
    producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
    producer_apps.Add(producerHelper.Install(nodes.Get(i))); // last node
    // Add /prefix origins to ndn::GlobalRouter
    ndnGlobalRoutingHelper.AddOrigins(n.toUri(), nodes.Get(i));
  }
  
  //set the SIT table capacity of each node 
  ///*
  if(sit_size > 0)
  {
    for(uint32_t i = 0; i < nodes.GetN(); i++)
    {
      Ptr<Node> n = nodes.Get(i);
      Ptr<ndn::L3Protocol> p =  ndn::L3Protocol::getL3Protocol(n);
      shared_ptr<nfd::Forwarder> f = p->getForwarder();
      f->setSitCapacity(sit_size);
    }
  }//*/

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();
  std::this_thread::sleep_for(std::chrono::seconds(2));

  /****************************************************************/
  //Setup Simulation Events (connection, disconnection, etc)

  // Content Distribution 
  ndn::ConsumerZipfMandelbrot content_dist(num_contents, 0, zipf_exponent);

  /***  Initialization Period: ***/
  NS_LOG_INFO("Beginning of Initialization Period");
  double connect_time = 0.2;
  std::random_device rd; 
  std::exponential_distribution<double> rng_exp_con (connection_rate);
  std::mt19937 rnd_gen (rd ()); //initialize the random number generator
  // The number of each content connected at each node
  std::map<int, int> requested_content;
  uint32_t content_indx, producer_indx, app_indx, cost;
  /*
  do 
  {
    content_indx = content_dist.GetNextSeq(); 
    producer_indx = content_indx%(producer_apps.GetN());
    app_indx = rnd_gen()%(consumer_apps.GetN());
    cost = get_cost(nodes, app_indx, producer_indx);
	 Schedule_Send(consumer_apps, app_indx, connect_time, producer_indx, cost + scoped_downstream_counter, content_indx, num_chunks);
	 num_connected++;
    connect_time = connect_time + rng_exp_con(rnd_gen);
    
    if(!requested_content[content_indx])
    {
      requested_content[content_indx]++;
      num_contents_requested_init++;
    }
  }while(connect_time < simulation_length);
   */
///*
    content_indx = 17; //content_dist.GetNextSeq(); 
    producer_indx = content_indx%(producer_apps.GetN());
    app_indx = 0; //rnd_gen()%(consumer_apps.GetN());
    cost = get_cost(nodes, app_indx, producer_indx);
	 Schedule_Send(consumer_apps, app_indx, connect_time, producer_indx, cost + scoped_downstream_counter, content_indx, num_chunks);
	 num_connected++;
    connect_time = connect_time + rng_exp_con(rnd_gen) + 1;
    
    content_indx = 17; //content_dist.GetNextSeq(); 
    producer_indx = content_indx%(producer_apps.GetN());
    app_indx = 6; //rnd_gen()%(consumer_apps.GetN());
    cost = get_cost(nodes, app_indx, producer_indx);
	 Schedule_Send(consumer_apps, app_indx, connect_time, producer_indx, cost + scoped_downstream_counter, content_indx, num_chunks);
	 num_connected++;
    connect_time = connect_time + rng_exp_con(rnd_gen) + 1;
    
    content_indx = 17; //content_dist.GetNextSeq(); 
    producer_indx = content_indx%(producer_apps.GetN());
    app_indx = 7; //rnd_gen()%(consumer_apps.GetN());
    cost = get_cost(nodes, app_indx, producer_indx);
	 Schedule_Send(consumer_apps, app_indx, connect_time, producer_indx, cost + scoped_downstream_counter, content_indx, num_chunks);
	 num_connected++;
    connect_time = connect_time + rng_exp_con(rnd_gen);
    
    connect_time = 10.0;
    content_indx = 17; //content_dist.GetNextSeq(); 
    producer_indx = content_indx%(producer_apps.GetN());
    app_indx = 7; //rnd_gen()%(consumer_apps.GetN());
    cost = get_cost(nodes, app_indx, producer_indx);
	 Schedule_Send(consumer_apps, app_indx, connect_time, producer_indx, cost + scoped_downstream_counter, content_indx, num_chunks);
	 num_connected++;
    
  // */
  
  NS_LOG_INFO("Number of connected applications during initialization: "<<num_connected);
  NS_LOG_INFO("Number of contents requested during initialization: "<<num_contents_requested_init);
  
  Simulator::Stop(Seconds(simulation_length+1));

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



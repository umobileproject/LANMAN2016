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

int
main(int argc, char* argv[])
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("20"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(6);

  // Connecting nodes using two links
  PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(1));
  p2p.Install(nodes.Get(1), nodes.Get(5));
  p2p.Install(nodes.Get(5), nodes.Get(2));
  p2p.Install(nodes.Get(5), nodes.Get(4));
  p2p.Install(nodes.Get(4), nodes.Get(3));

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelperCaching;
  ndnHelperCaching.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize", "10000");
  //Edge caching (install on nodes 1 and 4) Note: There is NFD running on nodes 0 and 3 as well!
  
  //ndnHelperCaching.Install(nodes.Get(0));
  //ndnHelperCaching.Install(nodes.Get(3));

  ndn::StackHelper ndnHelperNoCaching;
  ndnHelperNoCaching.SetOldContentStore("ns3::ndn::cs::Nocache");
  ndnHelperNoCaching.Install(nodes.Get(1));
  ndnHelperNoCaching.Install(nodes.Get(5));
  ndnHelperNoCaching.Install(nodes.Get(2));
  ndnHelperNoCaching.Install(nodes.Get(4));
  ndnHelperNoCaching.Install(nodes.Get(0));
  ndnHelperNoCaching.Install(nodes.Get(3));

  //ndnHelper.SetDefaultRoutes(true);
  //ndnHelper.InstallAll();

   // Set BestRoute strategy
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Installing applications

  std::string prefix = "/prefix";

  // Consumer 1
  ndn::AppHelper consumerHelper1("ns3::ndn::ConsumerSit");
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper1.SetPrefix(prefix);
  consumerHelper1.SetAttribute("Frequency", StringValue("1")); // 10 interests a second
  consumerHelper1.SetAttribute("StartSeq", IntegerValue(1));
  ApplicationContainer appcontainer1 = consumerHelper1.Install(nodes.Get(0));                        // node 0
  Ptr<Application> consumer_app1 = appcontainer1.Get(0);

  // Consumer 2
  ndn::AppHelper consumerHelper2("ns3::ndn::ConsumerSit");
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper2.SetPrefix(prefix);
  consumerHelper2.SetAttribute("Frequency", StringValue("1")); // 10 interests a second
  consumerHelper2.SetAttribute("StartSeq", IntegerValue(0));
  ApplicationContainer appcontainer2 = consumerHelper2.Install(nodes.Get(3));                        // node 3
  Ptr<Application> consumer_app2 = appcontainer2.Get(0);

  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix(prefix);
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(nodes.Get(2)); // last node
  Ptr<Node> producer = nodes.Get(2);

  // The failure of the link connecting consumer and router will start from seconds 10.0 to 15.0
  //Simulator::Schedule(Seconds(10.0), ndn::LinkControlHelper::FailLink, nodes.Get(2), nodes.Get(5));
  //Simulator::Schedule(Seconds(15.0), ndn::LinkControlHelper::UpLink, nodes.Get(2), nodes.Get(5));

  const ndn::Name n("ndn://" + prefix); 
  std::cout<<n<<"\n";

  // Add /prefix origins to ndn::GlobalRouter
  ndnGlobalRoutingHelper.AddOrigins(prefix, producer);

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();

  // consumer app1 sends an interest for 12 at time 2
  ns3::Application *app_ptr = GetPointer(consumer_app1);
  ndn::ConsumerSit *cons = reinterpret_cast<ndn::ConsumerSit *>(app_ptr);
  Simulator::Schedule(Seconds(2.0), &ndn::Consumer::SendPacketWithSeq, cons, 12);

  //Remove FIB and SIT to disconnect the Producer at time 4
  Simulator::Schedule(Seconds(4.0), (void (*)(Ptr<Node>, const ndn::Name&, Ptr<Node>)) (&ndn::FibHelper::RemoveRoute), nodes.Get(5), n, nodes.Get(2));

  // consumer app2 sends an interest for 12 at time 6
  app_ptr = GetPointer(consumer_app2);
  cons = reinterpret_cast<ndn::ConsumerSit *>(app_ptr);
  Simulator::Schedule(Seconds(6.0), &ndn::Consumer::SendPacketWithSeq, cons, 12);
  
  ndn::Name n1("ndn://" + prefix); 
  n1.appendSequenceNumber(12);
  std::cout<<n1<<"\n";
  //Remove the SIT table entry from app1 (node 0's)  SIT entry
  Simulator::Schedule(Seconds(8.0), (void (*)(Ptr<Node>, const ndn::Name&)) (&ndn::FibHelper::RemoveRoute), nodes.Get(0), n1);

  // consumer app2 sends an interest for 12 at time 6
  app_ptr = GetPointer(consumer_app2);
  cons = reinterpret_cast<ndn::ConsumerSit *>(app_ptr);
  Simulator::Schedule(Seconds(10.0), &ndn::Consumer::SendPacketWithSeq, cons, 12);

  //ndn::FibHelper::RemoveRoute(nodes.Get(5), n, nodes.Get(2));
  Simulator::Stop(Seconds(20.0));

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

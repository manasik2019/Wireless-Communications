/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mobility-model.h"
#include "ns3/csma-module.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/internet-module.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/wifi-mac-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/olsr-module.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"

#include <iostream>
// Default Network Topology
//
//  Wifi 10.1.1.0
//           
//  *            *   
//  |            |     
// n0(transm)   n1(receiver)    
           
        
using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("LAB2");


int
main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nWifi = 6;

  CommandLine cmd;
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.Parse (argc,argv);
  if (nWifi > 18)
    {
      std::cout << "Number of wifi nodes " << nWifi << 
                   " specified exceeds the mobility bounding box" << std::endl;
      exit (1);
    }
  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }
/////////////////////////////Nodes/////////////////////////////  
//Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
  
  NodeContainer ap;
  NodeContainer stas; 
  ap.Create (1);
  stas.Create (2);
  
 /////////////////////////////Wi-Fi part///////////////////////////// 

 //RngSeedManager::SetSeed(10);

  Ptr<YansWifiChannel> wifiChannel = CreateObject <YansWifiChannel> ();  //create a pointer for channel object
  Ptr<TwoRayGroundPropagationLossModel> lossModel = CreateObject<TwoRayGroundPropagationLossModel> (); //create a pointer for propagation loss model
  wifiChannel->SetPropagationLossModel (lossModel); // install propagation loss model
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();      
  wifiChannel->SetPropagationDelayModel (delayModel); // install propagation delay model
  
  //Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2200"));
 // disable fragmentation
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  
  YansWifiPhyHelper phy = YansWifiPhyHelper();
  phy.SetChannel (wifiChannel);
  phy.Set("TxPowerEnd", DoubleValue(16));
  phy.Set("TxPowerStart", DoubleValue(16));
  phy.Set("RxSensitivity", DoubleValue(-80));
//  phy.Set("CcaMode1Threshold", DoubleValue(-99));
  phy.Set("ChannelNumber", UintegerValue(7));
  WifiHelper wifi = WifiHelper();
  wifi.SetStandard(WIFI_STANDARD_80211b);
  
  Ssid ssid = Ssid ("wifi-default");

  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",StringValue ("DsssRate1Mbps"), "ControlMode",StringValue ("DsssRate1Mbps"));
  
  WifiMacHelper mac = WifiMacHelper();
  // setup ap.
  NetDeviceContainer apDevices;
  mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
  apDevices=wifi.Install (phy, mac, ap);
  // setup stas.
 mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false));
  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, stas);
 /////////////////////////////Deployment///////////////////////////// 
      MobilityHelper mobility;
      Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
      positionAlloc->Add (Vector (0.0, 0.0, 1.0));
      positionAlloc->Add (Vector (502.2, 0.0, 1.0));
      mobility.SetPositionAllocator (positionAlloc);
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install (stas);
      Ptr<ListPositionAllocator> positionAllocAP = CreateObject<ListPositionAllocator> ();
      positionAllocAP->Add (Vector (251.1, 0.0, 1.0));
      mobility.SetPositionAllocator (positionAllocAP);
      mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
      mobility.Install (ap);
  
/////////////////////////////Stack of protocols///////////////////////////// 
  // Set up internet stack
  InternetStackHelper stack;
  stack.Install (ap);
  stack.Install (stas);
  Ipv4AddressHelper address;
 /////////////////////////////Ip addresation/////////////////////////////  
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer wifiInterfaces;
  Ipv4InterfaceContainer wifiAPInterface;
   
  wifiAPInterface  = address.Assign (apDevices);  
  wifiInterfaces = address.Assign (staDevices);
  
/////////////////////////////Application part///////////////////////////// 
   
  uint16_t dlPort = 1000;
  //uint16_t dlPort1 = 2000;
   
   ApplicationContainer onOffApp;
    OnOffHelper onOffHelper("ns3::UdpSocketFactory", InetSocketAddress(wifiAPInterface.GetAddress (0), dlPort)); //OnOffApplication, UDP traffic, Please refer the ns-3 API
    onOffHelper.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=5000]"));
    onOffHelper.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    onOffHelper.SetAttribute ("StartTime", TimeValue (Seconds (1.000000)));
    onOffHelper.SetAttribute("DataRate", DataRateValue(DataRate("20.0Mbps"))); //Traffic Bit Rate
    onOffHelper.SetAttribute("PacketSize", UintegerValue(1000));
    onOffApp.Add(onOffHelper.Install(stas.Get(0)));  

    // ApplicationContainer onOffApp2;
    // OnOffHelper onOffHelper2("ns3::UdpSocketFactory", InetSocketAddress(wifiAPInterface.GetAddress (0), dlPort1)); //OnOffApplication, UDP traffic, Please refer the ns-3 API
    // onOffHelper2.SetAttribute("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=5000]"));
    // onOffHelper2.SetAttribute("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    // onOffHelper.SetAttribute ("StartTime", TimeValue (Seconds (1.000001)));
    // onOffHelper2.SetAttribute("DataRate", DataRateValue(DataRate("20.0Mbps"))); //Traffic Bit Rate
    // onOffHelper2.SetAttribute("PacketSize", UintegerValue(1000));
    // onOffApp2.Add(onOffHelper2.Install(stas.Get(1)));
 
    
     //Receiver socket on AP
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

    Ptr<Socket> recvSink = Socket::CreateSocket (ap.Get (0), tid);
    InetSocketAddress local = InetSocketAddress (wifiAPInterface.GetAddress(0), dlPort);
    bool ipRecvTos = true;
    recvSink->SetIpRecvTos (ipRecvTos);
    bool ipRecvTtl = true;
    recvSink->SetIpRecvTtl (ipRecvTtl);
    recvSink->Bind (local);


    // TypeId tid2 = TypeId::LookupByName ("ns3::UdpSocketFactory");
    // Ptr<Socket> recvSink2 = Socket::CreateSocket (ap.Get (0), tid2);
    // InetSocketAddress local2 = InetSocketAddress (wifiAPInterface.GetAddress (0), dlPort1);
    // bool ipRecvTos2 = true;
    // recvSink2->SetIpRecvTos (ipRecvTos2);
    // bool ipRecvTtl2 = true;
    // recvSink2->SetIpRecvTtl (ipRecvTtl2);
    // recvSink2->Bind (local2);

////////////////////////////////////////////////////////////
 std::cout << "IP address" << wifiInterfaces.GetAddress (0) << std::endl;
 std::cout << "IP address" << wifiAPInterface.GetAddress (0) << std::endl;
 
// double distance = ->GetDistanceFrom (mobil2);
// std::cout << distance <<  " Distance:"<< std::endl; //out system loss


// //  // 8. Install FlowMonitor on all nodes
//    FlowMonitorHelper flowmon;
//    Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  Simulator::Stop (Seconds (100.0));
/////////////////////////////PCAP tracing/////////////////////////////   
   phy.EnablePcap ("WIFI_STA", stas, true); 
   phy.EnablePcap ("WIFI_AP", ap, true); 
  Simulator::Run ();

  // 10. Print per flow statistics
  // monitor->CheckForLostPackets ();
  // Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  // FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  // for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
  //   {
  //     // first 2 FlowIds are for ECHO apps, we don't want to display them
  //     //
  //     // Duration for throughput measurement is 9.0 seconds, since
  //     //   StartTime of the OnOffApplication is at about "second 1"
  //     // and
  //     //   Simulator::Stops at "second 10".
  //     if (i->first > 2)
  //       {
  //         Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
  //         std::cout << "Flow " << i->first - 2 << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
  //         std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
  //         std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
  //         std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";
  //         std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
  //         std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
  //         std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";
  //       }
  //   }

  Simulator::Destroy ();
return 0;
};
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
#include "ns3/mobility-module.h"
#include "ns3/mobility-model.h"
#include "ns3/csma-module.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/internet-module.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/wifi-mac-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"
#include <iostream>

// Default Network Topology
//
//  Wifi 10.1.1.0
//           
//  *            *   
//  |            |     
// n0(transm)   n1(receiver)

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LAB1");

int main (int argc, char *argv[])
{
    bool verbose = true;
    uint32_t nWifi = 2;
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
    NodeContainer wifiStaNodes;
    wifiStaNodes.Create (nWifi);

    /////////////////////////////Wi-Fi part///////////////////////////// 
    Ptr<YansWifiChannel> channel = CreateObject<YansWifiChannel> ();  //create a pointer for channel object
    Ptr<TwoRayGroundPropagationLossModel> lossModel = CreateObject<TwoRayGroundPropagationLossModel> (); //create a pointer for propagation loss model
    channel->SetPropagationLossModel (lossModel); // install propagation loss model
    channel->SetPropagationDelayModel (CreateObject <ConstantSpeedPropagationDelayModel> ()); // install propagation delay model 
    
    Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));

    YansWifiPhyHelper phy = YansWifiPhyHelper ();
    phy.SetChannel (channel);
    phy.Set("TxPowerEnd", DoubleValue(16));
    phy.Set("TxPowerStart", DoubleValue(16));
    phy.Set("RxSensitivity", DoubleValue(-80));
    //  phy.Set("CcaMode1Threshold", DoubleValue(-99));
    phy.Set("ChannelNumber", UintegerValue(36));
    phy.Set("RxGain", DoubleValue(0));
    phy.Set("TxGain", DoubleValue(0));

    WifiHelper wifi = WifiHelper();
    wifi.SetStandard (WIFI_STANDARD_80211a);
    //wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", 
    //"DataMode",StringValue ("DsssRate11Mbps"), "ControlMode",StringValue ("DsssRate11Mbps"), "RtsCtsThreshold", UintegerValue(3000));
    // wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", 
    //"DataMode",StringValue ("OfdmRate54Mbps"), "ControlMode",StringValue ("OfdmRate54Mbps"), "RtsCtsThreshold", UintegerValue(3000));

    ///////////////////////////////////////////////////////////c
    wifi.SetRemoteStationManager ("ns3::ArfWifiManager", "SuccessThreshold",StringValue("1") );   
  
    //////////////////////////////////////////////////////////

    WifiMacHelper mac = WifiMacHelper();
    mac.SetType ("ns3::AdhocWifiMac");

    /////////////////////////////Devices///////////////////////////// 
    NetDeviceContainer staDevices;
    staDevices = wifi.Install (phy, mac, wifiStaNodes);
    /////////////////////////////Deployment///////////////////////////// 
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (0.0, 0.0, 1.0));
    positionAlloc->Add (Vector (251.1, 0.0, 1.0)); //251.1, 219.7, 188.32, 156.93, 125.55, 94.16, 62.77,31.38 for two way ground propagation model

    mobility.SetPositionAllocator (positionAlloc);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (wifiStaNodes);

    /////////////////////////////Stack of protocols/////////////////////////////
    InternetStackHelper stack;
    stack.Install (wifiStaNodes);
    Ipv4AddressHelper address;
    /////////////////////////////Ip addresation/////////////////////////////  
    address.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer wifiInterfaces;
    wifiInterfaces = address.Assign (staDevices);

    /////////////////////////////Application part///////////////////////////// 
    UdpEchoServerHelper echoServer (9);
    ApplicationContainer serverApps = echoServer.Install (wifiStaNodes.Get (nWifi-1));
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (30.0)); // was 30
    UdpEchoClientHelper echoClient (wifiInterfaces.GetAddress (nWifi-1), 9);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (15)); // was 15
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1000));
    ApplicationContainer clientApps =   echoClient.Install (wifiStaNodes.Get (0));
    clientApps.Start (Seconds (2.0));
    clientApps.Stop (Seconds (20.0)); // was 20

    Ptr<MobilityModel> mobil = wifiStaNodes.Get(0)->GetObject<MobilityModel> ();
    Ptr<MobilityModel> mobil2 = wifiStaNodes.Get(1)->GetObject<MobilityModel> ();

    double powrx;
    powrx=lossModel->CalcRxPower(16,mobil, mobil2); 
    std::cout << powrx <<  " "<< std::endl; //out system loss
    double distance;
    distance= mobil->GetDistanceFrom(mobil2);
    std::cout << distance << " " << std::endl; 
    //double man;
    //man = GetTxPowerStart(mobil);
    //std::cout << man << " " << std::endl;
    // output distance between mobil 1 & mobil 2
    //Ptr<TwoRayGroundPropagationLossModel> propmode = wifiStaNodes.Get(0)->GetObject<TwoRayGroundPropagationLossModel> ();
    //double pos;
    //pos=propmode->GetSystemLoss();
    //std::cout << pos <<  " distance "<< std::endl; 
    //Ptr<YansWifiPhyHelper> propmode =  wifiStaNodes.Get(0)->GetObject<YansWifiPhyHelper> ();
    //std::cout << propmode <<  " "<< std::endl; 
    //Time m_dalay_m_m2= propmode->GetDelay(mobil, mobil2);
    //TypeId propid = propmode->GetInstanceTypeId();
    //std1threshold::cout << propid.GetName()  <<  " distance "<< std::endl;

    Simulator::Stop (Seconds (30.0)); //was 30
/////////////////////////////PCAP tracing/////////////////////////////   
    phy.EnablePcap ("LAB1", staDevices .Get (nWifi-1), true);
    phy.EnablePcap ("LAB1", staDevices .Get (0), true); 
    Simulator::Run ();
    Simulator::Destroy ();
    return 0;
};

    
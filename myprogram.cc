#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include <cstdio>
#include <cstring>

NS_LOG_COMPONENT_DEFINE("labExample");

using namespace ns3;

void ReceivePacket(Ptr<Socket> socket) {
  char buf[100];
  static int count = 1;
  sprintf(buf, "Received %d packet(s)!", count++);
  NS_LOG_UNCOND(buf);
}

static void GenerateTraffic(Ptr<Socket> socket, uint32_t pktSize, uint32_t pktCount, Time pktInterval) {
  if(pktCount > 0) {
      socket->Send(Create<Packet>(pktSize));
      Simulator::Schedule(pktInterval, &GenerateTraffic, socket, pktSize,pktCount-1, pktInterval);
    }
  else
      socket->Close();
}

int main(int argc, char *argv[]) {
  std::string phyMode("DsssRate1Mbps");

  int nodeNumber = 4;
  uint32_t packetNumber = 40;
  char buf[100];

  CommandLine cmd;
  cmd.AddValue("nodeNum", "Number of nodes", nodeNumber);
  cmd.AddValue("packetNum", "Number of packets", packetNumber);

  cmd.Parse(argc, argv);

  Time interPacketInterval = Seconds(1.0);

  NodeContainer c;
  c.Create(nodeNumber);

  sprintf(buf, "Create %d nodes", nodeNumber);
  NS_LOG_UNCOND(buf);
  memset(buf, 0, 100);

  WifiHelper wifi;
  wifi.SetStandard(WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default();
  wifiPhy.Set("RxGain", DoubleValue(0)); 
  wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss("ns3::FixedRssLossModel","Rss", DoubleValue(-80));
  wifiPhy.SetChannel(wifiChannel.Create());

  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                               "DataMode",StringValue(phyMode), "ControlMode",StringValue(phyMode));
  
  wifiMac.SetType("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, c);
  
  sprintf(buf, "Make Connections");
  NS_LOG_UNCOND(buf);
  memset(buf, 0, 100);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add(Vector(0.0, 0.0, 0.0));
  positionAlloc->Add(Vector(5.0, 5.0, 0.0));
  positionAlloc->Add(Vector(10.0, 10.0, 0.0));
  positionAlloc->Add(Vector(15.0, 15.0, 0.0));
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(c);

  InternetStackHelper internet;
  internet.Install(c);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign(devices);

  sprintf(buf, "Set Ipv4 Address");
  NS_LOG_UNCOND(buf);
  memset(buf, 0, 100);

  TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
  //Ptr<Socket> recvSink = Socket::CreateSocket(c.Get(0), tid);
  //InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 80);
  //recvSink->Bind(InetSocketAddress(80));
  //recvSink->SetRecvCallback(MakeCallback(&ReceivePacket));

  Ptr<Socket> recvSink2 = Socket::CreateSocket(c.Get(2), tid);
  InetSocketAddress local2 = InetSocketAddress(i.GetAddress(2), 80);
  recvSink2->Bind(local2);
  recvSink2->SetRecvCallback(MakeCallback(&ReceivePacket));

  sprintf(buf, "Make recvSinks");
  NS_LOG_UNCOND(buf);
  memset(buf, 0, 100);

  Ptr<Socket> source = Socket::CreateSocket(c.Get(1), tid);
  InetSocketAddress remote = InetSocketAddress(i.GetAddress(0), 80);
//  source->SetAllowBroadcast(true);
  source->Connect(remote);

  Ptr<Socket> source2 = Socket::CreateSocket(c.Get(3), tid);
  InetSocketAddress remote2 = InetSocketAddress(i.GetAddress(2), 80);
//  source2->SetAllowBroadcast(true);
  source2->Connect(remote2);

  sprintf(buf, "%x", i.GetAddress(2).Get());
  NS_LOG_UNCOND(buf);
  memset(buf, 0, 100);

  sprintf(buf, "Make Source");
  NS_LOG_UNCOND(buf);
  memset(buf, 0, 100);

  wifiPhy.EnablePcap("labExample", devices);

  Simulator::ScheduleWithContext(source->GetNode()->GetId(), Seconds(1.0), &GenerateTraffic, 
                                 source, 1000, packetNumber, interPacketInterval);

  AnimationInterface anim("wireless-animation.xml");
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}


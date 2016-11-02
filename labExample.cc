#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"

NS_LOG_COMPONENT_DEFINE("labExample");

using namespace ns3;

void ReceivePacket(Ptr<Socket> socket) {
  NS_LOG_UNCOND("Received one packet!");
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
  int nodeNumber = 2;
  uint32_t packetNumber = 40;

  Time interPacketInterval = Seconds(1.0);

  NodeContainer c;
  c.Create(nodeNumber);

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

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add(Vector(0.0, 0.0, 0.0));
  positionAlloc->Add(Vector(5.0, 0.0, 0.0));
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(c);

  InternetStackHelper internet;
  internet.Install(c);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign(devices);

  TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket(c.Get(0), tid);
  InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 80);
  recvSink->Bind(local);
  recvSink->SetRecvCallback(MakeCallback(&ReceivePacket));

  Ptr<Socket> source = Socket::CreateSocket(c.Get(1), tid);
  InetSocketAddress remote = InetSocketAddress(Ipv4Address("255.255.255.255"), 80);
  source->SetAllowBroadcast(true);
  source->Connect(remote);

  wifiPhy.EnablePcap("labExample", devices);

  Simulator::ScheduleWithContext(source->GetNode()->GetId(), Seconds(1.0), &GenerateTraffic, 
                                 source, 1000, packetNumber, interPacketInterval);

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}


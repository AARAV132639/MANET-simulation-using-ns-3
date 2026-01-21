#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/aodv-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

// Fixed malicious node
uint32_t maliciousNodeId = 5;

// Blackhole: drop all received packets
bool
BlackholeReceive(Ptr<NetDevice> device,
                 Ptr<const Packet> packet,
                 uint16_t protocol,
                 const Address &source)
{
    return false; // silently drop
}

int main (int argc, char *argv[])
{
    uint32_t nNodes = 30;
    double simTime = 40.0;

    // Create nodes
    NodeContainer nodes;
    nodes.Create(nNodes);

    // ---------- FIXED TOPOLOGY (NO MOBILITY) ----------
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    Ptr<ListPositionAllocator> pos = CreateObject<ListPositionAllocator>();

    // Linear chain forcing node 5 in the path
   pos->Add(Vector(0.0,    0.0, 0.0));   // node 0
pos->Add(Vector(300.0,  0.0, 0.0));   // node 1
pos->Add(Vector(600.0,  0.0, 0.0));   // node 2
pos->Add(Vector(900.0,  0.0, 0.0));   // node 3
pos->Add(Vector(1200.0, 0.0, 0.0));   // node 4
pos->Add(Vector(1500.0, 0.0, 0.0));   // node 5 (BLACKHOLE)


    for (uint32_t i = 6; i < nNodes; ++i)
    {
        pos->Add(Vector(300 + i * 20, 0.0, 0.0));
    }

    mobility.SetPositionAllocator(pos);
    mobility.Install(nodes);

    // ---------- WIFI AD-HOC ----------
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);

    YansWifiPhyHelper phy;
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    // ---------- ROUTING ----------
    AodvHelper aodv;
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(nodes);

    // ---------- IP ADDRESSING ----------
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    // ---------- TRAFFIC ----------
    UdpEchoServerHelper server(9);
    ApplicationContainer serverApp = server.Install(nodes.Get(0));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(simTime));

    UdpEchoClientHelper client(interfaces.GetAddress(0), 9);
    client.SetAttribute("MaxPackets", UintegerValue(50));
    client.SetAttribute("Interval", TimeValue(Seconds(0.5)));
    client.SetAttribute("PacketSize", UintegerValue(64));

    ApplicationContainer clientApp = client.Install(nodes.Get(1));
    clientApp.Start(Seconds(10.0)); // allow route convergence
    clientApp.Stop(Seconds(simTime));

    // ---------- BLACKHOLE ATTACK ----------
    Ptr<Node> maliciousNode = nodes.Get(maliciousNodeId);
    for (uint32_t i = 0; i < maliciousNode->GetNDevices(); ++i)
    {
        Ptr<NetDevice> dev = maliciousNode->GetDevice(i);
        dev->SetReceiveCallback(MakeCallback(&BlackholeReceive));
    }

    // ---------- FLOW MONITOR ----------
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> flowMonitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    // ---------- METRICS ----------
    flowMonitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());

    auto stats = flowMonitor->GetFlowStats();

    double tx = 0, rx = 0, delay = 0, throughput = 0;

    for (const auto &flow : stats)
    {
        tx += flow.second.txPackets;
        rx += flow.second.rxPackets;
        delay += flow.second.delaySum.GetSeconds();

        if (flow.second.timeLastRxPacket.GetSeconds() > 0)
        {
            throughput +=
                (flow.second.rxBytes * 8.0) /
                (flow.second.timeLastRxPacket.GetSeconds() -
                 flow.second.timeFirstTxPacket.GetSeconds());
        }
    }

    double pdr = (tx > 0) ? (rx / tx) * 100.0 : 0.0;
    double avgDelay = (rx > 0) ? (delay / rx) : 0.0;

    std::cout << "\n========== FIXED-NODE BLACKHOLE METRICS ==========\n";
    std::cout << "Total Tx Packets: " << tx << "\n";
    std::cout << "Total Rx Packets: " << rx << "\n";
    std::cout << "Packet Delivery Ratio (PDR): " << pdr << " %\n";
    std::cout << "Average End-to-End Delay: " << avgDelay << " s\n";
    std::cout << "Aggregate Throughput: " << throughput / 1000 << " kbps\n";
    std::cout << "=================================================\n";

    Simulator::Destroy();
    return 0;
}


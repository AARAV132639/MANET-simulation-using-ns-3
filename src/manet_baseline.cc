#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/aodv-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main (int argc, char *argv[])
{
    uint32_t nNodes = 30;
    double simTime = 40.0;

    // 1. Create nodes
    NodeContainer nodes;
    nodes.Create(nNodes);

    // 2. Mobility model (Random Waypoint)
    MobilityHelper mobility;
    mobility.SetMobilityModel(
        "ns3::RandomWaypointMobilityModel",
        "Speed", StringValue("ns3::UniformRandomVariable[Min=1.0|Max=5.0]"),
        "Pause", StringValue("ns3::ConstantRandomVariable[Constant=1.0]"),
        "PositionAllocator",
        StringValue("ns3::RandomRectanglePositionAllocator")
    );

    mobility.SetPositionAllocator(
    "ns3::RandomRectanglePositionAllocator",
    "X", StringValue("ns3::UniformRandomVariable[Min=0|Max=300]"),
    "Y", StringValue("ns3::UniformRandomVariable[Min=0|Max=300]")
);

    mobility.Install(nodes);

    // 3. WiFi Ad-hoc
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);

    YansWifiPhyHelper phy;
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    // 4. Routing protocol (AODV)
    AodvHelper aodv;
    InternetStackHelper internet;
    internet.SetRoutingHelper(aodv);
    internet.Install(nodes);

    // 5. IP addressing
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    // 6. UDP Echo (traffic)
    UdpEchoServerHelper server(9);
    ApplicationContainer serverApp = server.Install(nodes.Get(29));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(simTime));

    UdpEchoClientHelper client(interfaces.GetAddress(29), 9);

    client.SetAttribute("MaxPackets", UintegerValue(50));
    client.SetAttribute("Interval", TimeValue(Seconds(0.5)));
    client.SetAttribute("PacketSize", UintegerValue(64));

    ApplicationContainer clientApp = client.Install(nodes.Get(1));
    clientApp.Start(Seconds(10.0));
    clientApp.Stop(Seconds(simTime));

    Simulator::Stop(Seconds(simTime));

        FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> flowMonitor = flowHelper.InstallAll();

        Simulator::Run();

    flowMonitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());

    std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();

    double totalTxPackets = 0;
    double totalRxPackets = 0;
    double totalDelay = 0;
    double totalThroughput = 0;

    for (auto const &flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);

        totalTxPackets += flow.second.txPackets;
        totalRxPackets += flow.second.rxPackets;
        totalDelay += flow.second.delaySum.GetSeconds();

        if (flow.second.timeLastRxPacket.GetSeconds() > 0)
        {
            double throughput =
                (flow.second.rxBytes * 8.0) /
                (flow.second.timeLastRxPacket.GetSeconds() -
                 flow.second.timeFirstTxPacket.GetSeconds());

            totalThroughput += throughput;
        }
    }

    double pdr = (totalTxPackets > 0)? (totalRxPackets / totalTxPackets) * 100.0: 0;

    double avgDelay = (totalRxPackets > 0)? (totalDelay / totalRxPackets): 0;

    std::cout << "\n========== BASELINE MANET METRICS ==========\n";
    std::cout << "Total Tx Packets: " << totalTxPackets << "\n";
    std::cout << "Total Rx Packets: " << totalRxPackets << "\n";
    std::cout << "Packet Delivery Ratio (PDR): " << pdr << " %\n";
    std::cout << "Average End-to-End Delay: " << avgDelay << " s\n";
    std::cout << "Aggregate Throughput: " << totalThroughput / 1000 << " kbps\n";
    std::cout << "===========================================\n";

   
    Simulator::Destroy();

    return 0;
}

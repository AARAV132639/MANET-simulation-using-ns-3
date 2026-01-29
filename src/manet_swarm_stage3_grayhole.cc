#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

/*
 STAGE 3B: Grayhole attack during patrol swarm
 - Node 2 becomes malicious at 45s
 - Drops packets probabilistically (p = 0.3)
*/

// ----- Swarm globals -----
Ptr<Node> leaderNode;
NodeContainer followerNodes;
//adding patrol constants
double patrolSize=300.0; //300X300 area
double patrolSpeed=10.0;//m/s

uint32_t maliciousNodeId = 2;
double dropProbability = 0.3;

// Random variable for grayhole behavior
Ptr<UniformRandomVariable> randVar = CreateObject<UniformRandomVariable>();

// Formation offsets
static Vector tightOffsets[6] = {
    Vector(-40.0,  0.0, 0.0),
    Vector( 40.0,  0.0, 0.0),
    Vector(  0.0, 40.0, 0.0),
    Vector(  0.0,-40.0, 0.0),
    Vector(-30.0, 30.0, 0.0),
    Vector( 30.0,-30.0, 0.0)
};

static Vector wideOffsets[6] = {
    Vector(-90.0,  0.0, 0.0),
    Vector( 90.0,  0.0, 0.0),
    Vector(  0.0, 90.0, 0.0),
    Vector(  0.0,-90.0, 0.0),
    Vector(-65.0, 65.0, 0.0),
    Vector( 65.0,-65.0, 0.0)
};

Vector *currentOffsets = tightOffsets;

// ----- Grayhole receive callback -----
bool
GrayholeReceive(Ptr<NetDevice> device,
                Ptr<const Packet> packet,
                uint16_t protocol,
                const Address &from)
{
    double r = randVar->GetValue();

    if (r < dropProbability)
    {
        // drop packet
        return false;
    }

    // forward packet normally
    return true;
}

// ----- Activate grayhole attack -----
void
ActivateGrayhole()
{
    Ptr<Node> maliciousNode = NodeList::GetNode(maliciousNodeId);

    for (uint32_t i = 0; i < maliciousNode->GetNDevices(); ++i)
    {
        Ptr<NetDevice> dev = maliciousNode->GetDevice(i);
        dev->SetReceiveCallback(MakeCallback(&GrayholeReceive));
    }

    std::cout << "[INFO] Grayhole attack activated at "
              << Simulator::Now().GetSeconds()
              << "s on node " << maliciousNodeId
              << " (p=" << dropProbability << ")"
              << std::endl;
}

// ----- Update follower positions -----
void
UpdateFollowerPositions()
{
    Vector leaderPos =
        leaderNode->GetObject<MobilityModel>()->GetPosition();

    for (uint32_t i = 0; i < followerNodes.GetN(); ++i)
    {
        followerNodes.Get(i)
            ->GetObject<MobilityModel>()
            ->SetPosition(leaderPos + currentOffsets[i]);
    }

    Simulator::Schedule(Seconds(2.0), &UpdateFollowerPositions);
}

// ----- Formation switching -----
void SwitchToWide()  { currentOffsets = wideOffsets; }
void SwitchToTight() { currentOffsets = tightOffsets; }

// ----- Leader velocity -----
void
SetLeaderVelocity(Vector v)
{
    leaderNode->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(v);
}

int main(int argc, char *argv[])
{
    uint32_t nNodes = 7;
    double simTime = 90.0;

    NodeContainer nodes;
    nodes.Create(nNodes);

    leaderNode = nodes.Get(0);
    for (uint32_t i = 1; i < nNodes; ++i)
        followerNodes.Add(nodes.Get(i));

    // ----- Mobility -----
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(nodes);

    leaderNode->GetObject<MobilityModel>()->SetPosition(Vector(0,0,0));

    // Leader patrol: 300x300 square
Simulator::Schedule(Seconds(0.0),
    &SetLeaderVelocity, Vector(patrolSpeed, 0.0, 0.0));   // Right

Simulator::Schedule(Seconds(patrolSize / patrolSpeed),
    &SetLeaderVelocity, Vector(0.0, patrolSpeed, 0.0));   // Up

Simulator::Schedule(Seconds(2 * patrolSize / patrolSpeed),
    &SetLeaderVelocity, Vector(-patrolSpeed, 0.0, 0.0));  // Left

Simulator::Schedule(Seconds(3 * patrolSize / patrolSpeed),
    &SetLeaderVelocity, Vector(0.0, -patrolSpeed, 0.0));  // Down


    // Formation dynamics
    Simulator::Schedule(Seconds(30.0), &SwitchToWide);
    Simulator::Schedule(Seconds(60.0), &SwitchToTight);

    Simulator::Schedule(Seconds(1.0), &UpdateFollowerPositions);

    // ----- Wi-Fi ad-hoc -----
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);

    YansWifiPhyHelper phy;
    phy.SetChannel(YansWifiChannelHelper::Default().Create());

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    // ----- Internet -----
    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.4.1.0", "255.255.255.0");
    ipv4.Assign(devices);

    // ----- Heartbeat traffic -----
    UdpEchoServerHelper server(9);
    server.Install(leaderNode).Start(Seconds(1.0));

    UdpEchoClientHelper client(
        leaderNode->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), 9);

    client.SetAttribute("Interval", TimeValue(Seconds(2.0)));
    client.SetAttribute("PacketSize", UintegerValue(64));

    for (uint32_t i = 1; i < nNodes; ++i)
        client.Install(nodes.Get(i)).Start(Seconds(2.0));

    // ----- Activate grayhole mid-patrol -----
    Simulator::Schedule(Seconds(45.0), &ActivateGrayhole);

     Simulator::Stop(Seconds(simTime));

    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> flowMonitor = flowHelper.InstallAll();

    Simulator::Run();
    flowMonitor->CheckForLostPackets();

Ptr<Ipv4FlowClassifier> classifier =
    DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());

std::map<FlowId, FlowMonitor::FlowStats> stats =
    flowMonitor->GetFlowStats();

double totalTx = 0;
double totalRx = 0;
double totalDelay = 0;
double totalThroughput = 0;

for (const auto &flow : stats)
{
    totalTx += flow.second.txPackets;
    totalRx += flow.second.rxPackets;
    totalDelay += flow.second.delaySum.GetSeconds();

    if (flow.second.timeLastRxPacket.GetSeconds() > 0)
    {
        double thr =
            (flow.second.rxBytes * 8.0) /
            (flow.second.timeLastRxPacket.GetSeconds() -
             flow.second.timeFirstTxPacket.GetSeconds());

        totalThroughput += thr;
    }
}

double pdr = (totalTx > 0) ? (totalRx / totalTx) * 100.0 : 0.0;
double avgDelay = (totalRx > 0) ? (totalDelay / totalRx) : 0.0;

std::cout << "\n===== SWARM BASELINE METRICS =====\n";
std::cout << "Tx Packets: " << totalTx << "\n";
std::cout << "Rx Packets: " << totalRx << "\n";
std::cout << "PDR: " << pdr << " %\n";
std::cout << "Avg Delay: " << avgDelay << " s\n";
std::cout << "Throughput: " << totalThroughput / 1000 << " kbps\n";
std::cout << "=================================\n";

//collecting data
flowMonitor->SerializeToXmlFile(
    "grayhole_swarm.xml",
    true,   // enable histograms
    true    // enable probes
);
    Simulator::Destroy();
    return 0;
}
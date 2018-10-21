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

// Network topology
//
//       n0 ----------- n1
//            500 Kbps
//             5 ms
//
// - Flow from n0 to n1 using BulkSendApplication.
// - Tracing of queues and packet receptions to file "tcp-bulk-send.tr"
//   and pcap tracing available when tracing is turned on.

#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/stats-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpBulkSendExample");

void ThroughputMonitor(ns3::FlowMonitorHelper *fmhelper, ns3::Ptr<FlowMonitor> flowMon, Gnuplot2dDataset *dataset, double Throughput, double xtime);

/* Detect if the output is to a new file that needs a header or not. */
bool newFile = true;

/**
 * Callback method to log changes to the congestion window.
 */
static void
CwndChange(uint32_t oldCwnd, uint32_t newCwnd) {
    NS_LOG_UNCOND(Simulator::Now().GetSeconds() << "\t" << newCwnd);

    // Write to a file
    std::ofstream myfile;
    if (newFile) {
        myfile.open("cubic-cwnd.log");
        newFile = false;
    } else {
        myfile.open("cubic-cwnd.log", std::ios::out | std::ios::app);
    }
    myfile << Simulator::Now().GetSeconds() << "," << newCwnd << "\n";
    myfile.close();

}

int main(int argc, char *argv[]) {
    
    // Set the TCP variant to use.
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpCubic::GetTypeId()));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1000));
    // LogComponentEnable( "TcpCubic", LOG_LEVEL_INFO);

    bool tracing = false;
    uint32_t maxBytes = 0;

    //
    // Allow the user to override any of the defaults cat
    // run-time, via command-line arguments
    //
    CommandLine cmd;
    cmd.AddValue("tracing", "Flag to enable/disable tracing", tracing);
    cmd.AddValue("maxBytes",
            "Total number of bytes for application to send", maxBytes);
    cmd.Parse(argc, argv);


    //
    // Explicitly create the nodes required by the topology (shown above).
    //
    NS_LOG_INFO("Create nodes.");
    NodeContainer nodes;
    nodes.Create(2);

    NS_LOG_INFO("Create channels.");

    // Setup the connection between the nodes.
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1000Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));
    
    
    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    //
    // Install the internet stack on the nodes
    //
    InternetStackHelper internet;
    internet.Install(nodes);

    //
    // We've got the "hardware" in place.  Now we need to add IP addresses.
    //
    NS_LOG_INFO("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign(devices);

    NS_LOG_INFO("Create Applications.");


    uint16_t port = 9; // well-known echo port number
    
    //
    // Create a PacketSinkApplication and install it on node 1
    //
    PacketSinkHelper sink("ns3::TcpSocketFactory",
            InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(nodes.Get(1));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(20.0));
    
    // Set the TCP congestion control algorithm to use.
    TypeId tid = TypeId::LookupByName ("ns3::TcpCubic");
    Config::Set ("/NodeList/*/$ns3::TcpL4Protocol/SocketType", TypeIdValue (tid));
    Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());

    // Trace changes to the congestion window.
     ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindowCubic", MakeCallback (&CwndChange));    
    //
    // Create a BulkSendApplication and install it on node 0
    //
    BulkSendHelper source("ns3::TcpSocketFactory",
            InetSocketAddress(i.GetAddress(1), port));
    // Set the amount of data to send in bytes.  Zero is unlimited.
    source.SetAttribute("MaxBytes", UintegerValue(maxBytes));
    ApplicationContainer sourceApps = source.Install(nodes.Get(0));
    sourceApps.Start(Seconds(1.0));
    sourceApps.Stop(Seconds(30.0));


    //
    // Set up tracing if enabled
    //
    if (tracing) {
        AsciiTraceHelper ascii;
        pointToPoint.EnableAsciiAll(ascii.CreateFileStream("tcp-bulk-send.tr"));
        pointToPoint.EnablePcapAll("tcp-bulk-send", false);
    }

    //
    // Now, do the actual simulation.
    //
    NS_LOG_INFO("Run Simulation.");
    NS_LOG_INFO("Done.");
    //
    // Gnuplot
    //
    std::string fileName = "TimeVSThroughput";
    std::string graphicsFileName = fileName + ".png";
    std::string plotFileName = fileName + ".plt";
    std::string plotTitle = "Throughput vs Time";
    std::string dataTitle = "Throughput";

    Gnuplot gnuplot(graphicsFileName);
    gnuplot.SetTitle(plotTitle);
    gnuplot.SetTerminal("png");
    gnuplot.SetLegend("Time (s)", "Throughput (Mbps)");

    Gnuplot2dDataset dataset;
    dataset.SetTitle("Throughput (Mbps)");
    dataset.SetStyle(Gnuplot2dDataset::LINES); //LINES, POINTS, DOTS, IMPULSES, STEPS

    double Throughput = 0;
    double xtime = 0;

    FlowMonitorHelper flowmon; // install flow monitor on all the nodes
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Schedule(Seconds(1), &ThroughputMonitor, &flowmon, monitor, &dataset, Throughput, xtime);
    NS_LOG_INFO("Run Simulation");

    Simulator::Stop(Seconds(30)); // Run simulation for 60 seconds
    Simulator::Run();

    NS_LOG_UNCOND("Flow Monitor Statistics: ");

    monitor->CheckForLostPackets();
    //   ThroughputMonitor(&flowmon, monitor, &dataset, Throughput, xtime);
    // 3. Print per flow statistics

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        std::cout << "Flow " << i->first - 1 << " (" << t.sourceAddress << " -> " << t.destinationAddress
                << ")\n";
        std::cout << " Total Tx Bytes: " << i->second.txBytes << "\n";
        std::cout << " Total Rx Bytes: " << i->second.rxBytes << "\n";
        std::cout << " Duration      : " << i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds();
        std::cout << " Average Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i ->second.timeFirstTxPacket.GetSeconds()) / 1024 / 1024 << " Mbps \n";
    }
    NS_LOG_UNCOND("Done");

    gnuplot.AddDataset(dataset);
    std::ofstream plotFile(plotFileName.c_str());
    gnuplot.GenerateOutput(plotFile);
    plotFile.close();
    monitor->SerializeToXmlFile("tcp-bulk-send.flowmon", true, true);
    Simulator::Destroy();
    return 0;
}

void ThroughputMonitor(ns3::FlowMonitorHelper *fmhelper, ns3::Ptr<FlowMonitor> flowMon, Gnuplot2dDataset *dataset, double Throughput, double xtime) {

    std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMon->GetFlowStats();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (fmhelper->GetClassifier());
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = flowStats.begin(); iter != flowStats.end(); ++iter) {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
        if (iter->first == 1) {
            NS_LOG_UNCOND("Flow IF: " << iter->first << "  Src Addr " << t.sourceAddress << "Dst Addr " << t.destinationAddress);
            NS_LOG_UNCOND("Tx Pkts= " << iter->second.txPackets);
            NS_LOG_UNCOND("Rx Pkts= " << iter->second.rxPackets);
            NS_LOG_UNCOND("Duration		: " << iter->second.timeLastRxPacket.GetSeconds() - iter->second.timeFirstTxPacket.GetSeconds());
            Throughput = iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds() - iter->second.timeFirstTxPacket.GetSeconds()) / 1024 / 1024;
            NS_LOG_UNCOND("Throughput: " << Throughput << "Mbps");
            dataset->Add((double) xtime, (double) Throughput);
            xtime++;
        }
    }
    Simulator::Schedule(Seconds(1), &ThroughputMonitor, fmhelper, flowMon, dataset, Throughput, xtime);


}
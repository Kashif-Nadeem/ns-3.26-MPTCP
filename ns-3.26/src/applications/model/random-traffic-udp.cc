/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: George F. Riley<riley@ece.gatech.edu>
//

// ns3 - On/Off Data Source Application class
// George F. Riley, Georgia Tech, Spring 2007
// Adapted from ApplicationOnOff in GTNetS.

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "random-traffic-udp.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "my-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RandomTrafficUdp");

NS_OBJECT_ENSURE_REGISTERED (RandomTrafficUdp);

TypeId
RandomTrafficUdp::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RandomTrafficUdp")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<RandomTrafficUdp> ()
    
    .AddAttribute ("Remote", "The address of the destination",
                   AddressValue (),
                   MakeAddressAccessor (&RandomTrafficUdp::m_peer),
                   MakeAddressChecker ())
    .AddAttribute ("MaxBytes", 
                   "The total number of bytes to send. Once these bytes are sent, "
                   "no packet is sent again, even in on state. The value zero means "
                   "that there is no limit.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RandomTrafficUdp::m_maxBytes),
                   MakeUintegerChecker<uint64_t> ())
    .AddAttribute ("Protocol", "The type of protocol to use.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&RandomTrafficUdp::m_tid),
                   MakeTypeIdChecker ())
     .AddAttribute ("PacketInterval", "A RandomVariableStream used to pick the length of interval "
				   "(in seconds) between consecutive packet transmissions ",
				   StringValue ("ns3::ExponentialRandomVariable[Mean=0.1]"),
				   MakePointerAccessor (&RandomTrafficUdp::m_PacketInterval),
				   MakePointerChecker <RandomVariableStream>())
      .AddAttribute ("PacketSize", "A RandomVariableStream used to pick the size of generated packets "
				   "(in bytes)",
				   StringValue ("ns3::ExponentialRandomVariable[Mean=512]"),
				   MakePointerAccessor (&RandomTrafficUdp::m_PacketSize),
				   MakePointerChecker <RandomVariableStream>())
      .AddAttribute ("Deadline", "A RandomVariableStream used to set the deadline of a packet "
				   "(in seconds)",
				   StringValue ("ns3::ExponentialRandomVariable[Mean=0.150]"),
				   MakePointerAccessor (&RandomTrafficUdp::m_Deadline),
				   MakePointerChecker <RandomVariableStream>())
      
      .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&RandomTrafficUdp::m_txTrace),
					 "ns3::Packet::TracedCallback")

  ;
  return tid;
}


RandomTrafficUdp::RandomTrafficUdp ()
  : m_socket (0),
    m_connected (false),
    m_totBytes (0)
{
  NS_LOG_FUNCTION (this);
}

RandomTrafficUdp::~RandomTrafficUdp()
{
  NS_LOG_FUNCTION (this);
}

void 
RandomTrafficUdp::SetMaxBytes (uint64_t maxBytes)
{
  NS_LOG_FUNCTION (this << maxBytes);
  m_maxBytes = maxBytes;
}

Ptr<Socket>
RandomTrafficUdp::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

int64_t 
RandomTrafficUdp::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_PacketInterval->SetStream (stream);
  m_PacketSize->SetStream (stream + 1);
  m_Deadline->SetStream (stream + 2);
  return 3;
}

void
RandomTrafficUdp::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  // chain up
  Application::DoDispose ();
}

// Application Methods
void RandomTrafficUdp::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);

  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      if (Inet6SocketAddress::IsMatchingType (m_peer))
        {
          m_socket->Bind6 ();
        }
      else if (InetSocketAddress::IsMatchingType (m_peer) ||
               PacketSocketAddress::IsMatchingType (m_peer))
        {
          m_socket->Bind ();
        }
      m_socket->Connect (m_peer);
      m_socket->SetAllowBroadcast (true);
      m_socket->ShutdownRecv ();

      m_socket->SetConnectCallback (
        MakeCallback (&RandomTrafficUdp::ConnectionSucceeded, this),
        MakeCallback (&RandomTrafficUdp::ConnectionFailed, this));
    }
  
  // If we are not yet connected, there is nothing to do here
  // The ConnectionComplete upcall will start timers at that time
  //if (!m_connected) return;
  StartSending ();
}

void RandomTrafficUdp::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);

  if(m_socket != 0)
    {
      m_socket->Close ();
    }
  else
    {
      NS_LOG_WARN ("RandomTrafficUdp found null socket to close in StopApplication");
    }
}

// Event handlers
void RandomTrafficUdp::StartSending ()
{
  NS_LOG_FUNCTION (this);
  ScheduleNextTx ();  // Schedule the send packet event
  
}

// Private helpers
void RandomTrafficUdp::ScheduleNextTx ()
{
  NS_LOG_FUNCTION (this);

  if (m_maxBytes == 0 || m_totBytes < m_maxBytes)
    {
      
          Time nextTime;
      // Time till next packet
    	  nextTime = Seconds (m_PacketInterval->GetValue ());
        std::cout<<" Packet Interval  "<< nextTime<<std::endl;
          NS_ASSERT_MSG (nextTime.IsPositive (),
    					 "Packet interval is unexpectedly zero or negative. "
    					 "Please verify the configuration of "
    					 "`ns3::RandomTrafficUdp::PacketInterval` "
    					 "random variable.");
     
      NS_LOG_LOGIC ("nextTime = " << nextTime);
      m_sendEvent = Simulator::Schedule (nextTime,
                                         &RandomTrafficUdp::SendPacket, this);
    }
  else
    { // All done, cancel any pending events
      StopApplication ();
    }
}

uint32_t c=1;
void RandomTrafficUdp::SendPacket ()
{
  NS_LOG_FUNCTION (this);

 // NS_ASSERT (m_sendEvent.IsExpired ());
  uint32_t packetSize;
    packetSize = m_PacketSize->GetInteger ();
    std::cout<<" packet size: "<< packetSize <<std::endl;
	  //NS_ASSERT_MSG (packetSize > 0, 
            //            "Packet size was unexpectedly zero." 
              //          "Please verify the configuration of `ns3::RandomTrafficUdp::PacketSize` "
		//	                          "random variable.");
   Ptr<Packet> packet = Create<Packet> (packetSize);
  // Tag random deadline; exponentially distributed
    DeadlineTag dl;
    Time d = Seconds(m_Deadline->GetValue());
    dl.SetDeadline (Simulator::Now () + d);
    packet->AddByteTag (dl);
  
    std::cout<<"random time: " << d<<std::endl;
    std::cout<<"Sending time: \n"<< Simulator::Now ();
    std::cout<<"attached deadline: "<< dl.GetDeadline()<<std::endl;

  m_txTrace (packet);
  m_socket->Send (packet);
  m_totBytes += packetSize;

  if (InetSocketAddress::IsMatchingType (m_peer))
    { 
      std::cout<<" RandomTrafficUdp sending packet "<<c<<" at "<<Simulator::Now ().GetSeconds ()
           <<" of size "<<  packet->GetSize()<<" Total bytes sent " <<m_totBytes << std::endl; c++;
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                   << "s random-traffic application sent "
                   <<  packet->GetSize () << " bytes to "
                   << InetSocketAddress::ConvertFrom(m_peer).GetIpv4 ()
                   << " port " << InetSocketAddress::ConvertFrom (m_peer).GetPort ()
                   << " total Tx " << m_totBytes << " bytes");
    }
  else if (Inet6SocketAddress::IsMatchingType (m_peer))
    { 
      std::cout<<" RandomTrafficUdp sending packet "<<c<<" at "<<Simulator::Now ().GetSeconds ()
           <<" of size "<<  packet->GetSize()<<" Total bytes sent " <<m_totBytes << std::endl; c++;
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                   << "s random-traffic application sent "
                   <<  packet->GetSize () << " bytes to "
                   << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6 ()
                   << " port " << Inet6SocketAddress::ConvertFrom (m_peer).GetPort ()
                   << " total Tx " << m_totBytes << " bytes");
    }

   
  ScheduleNextTx ();
}


void RandomTrafficUdp::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("RandomTrafficUdp Connection succeeded");
  m_connected = true;
  StartSending ();
}

void RandomTrafficUdp::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("RandomTrafficUdp, Connection Failed");
}


} // Namespace ns3

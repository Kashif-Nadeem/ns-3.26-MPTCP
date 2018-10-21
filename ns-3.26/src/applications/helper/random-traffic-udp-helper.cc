/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
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
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "random-traffic-udp-helper.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/random-variable-stream.h"
#include "ns3/random-traffic-udp.h"

namespace ns3 {

RandomTrafficUdpHelper::RandomTrafficUdpHelper (std::string protocol, Address address)
{
  m_factory.SetTypeId (RandomTrafficUdp::GetTypeId ());
  m_factory.Set ("Protocol", StringValue (protocol));
  m_factory.Set ("Remote", AddressValue (address));
}

void 
RandomTrafficUdpHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
RandomTrafficUdpHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
RandomTrafficUdpHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
RandomTrafficUdpHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
RandomTrafficUdpHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<Application> ();
  node->AddApplication (app);

  return app;
}

int64_t
RandomTrafficUdpHelper::AssignStreams (NodeContainer c, int64_t stream)
{
  int64_t currentStream = stream;
  Ptr<Node> node;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      node = (*i);
      for (uint32_t j = 0; j < node->GetNApplications (); j++)
        {
          Ptr<RandomTrafficUdp> randomtraffic = DynamicCast<RandomTrafficUdp> (node->GetApplication (j));
          if (randomtraffic)
            {
              currentStream += randomtraffic->AssignStreams (currentStream);
            }
        }
    }
  return (currentStream - stream);
}

void 
RandomTrafficUdpHelper::InitializeRandomVariables ()
{
  m_factory.Set ("PacketInterval", StringValue ("ns3::ExponentialRandomVariable[Mean=0.1]"));
  m_factory.Set ("PacketSize", StringValue ("ns3::ExponentialRandomVariable[Mean=512]"));
  m_factory.Set ("Deadline", StringValue ("ns3::ExponentialRandomVariable[Mean=0.150]"));
}

} // namespace ns3

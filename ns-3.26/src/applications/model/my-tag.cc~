#include "ns3/tag.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/object.h"
#include "ns3/random-variable-stream.h"
#include <iostream>
#include "my-tag.h"
#include "ns3/nstime.h"
#include "ns3/command-line.h"

namespace ns3 {



TypeId 
MyTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MyTag")
    .SetParent<Tag> ()
    .AddConstructor<MyTag> ()
    .AddAttribute ("SimpleValue",
                   "A simple value",
                   EmptyAttributeValue (),
                   MakeUintegerAccessor (&MyTag::GetSimpleValue),
                   MakeUintegerChecker<uint8_t> ())
  ;
  return tid;
}
TypeId 
MyTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t 
MyTag::GetSerializedSize (void) const
{
  return 1;
}
void 
MyTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_simpleValue);
}
void 
MyTag::Deserialize (TagBuffer i)
{
  m_simpleValue = i.ReadU8 ();
}
void 
MyTag::Print (std::ostream &os) const
{
  os << "v=" << (uint8_t)m_simpleValue;
}
void 
MyTag::SetSimpleValue (uint8_t value)
{

  m_simpleValue = value;
}
uint8_t 
MyTag::GetSimpleValue (void) const
{
  return m_simpleValue;
}

//----------------------------------------------------------------------
//-- TimestampTag
//------------------------------------------------------
TypeId 
TimestampTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("TimestampTag")
    .SetParent<Tag> ()
    .AddConstructor<TimestampTag> ()
    .AddAttribute ("Timestamp",
                   "Some momentous point in time!",
                   EmptyAttributeValue (),
                   MakeTimeAccessor (&TimestampTag::GetTimestamp),
                   MakeTimeChecker ())
  ;
  return tid;
}
TypeId 
TimestampTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t 
TimestampTag::GetSerializedSize (void) const
{
  return 16;
}
void 
TimestampTag::Serialize (TagBuffer i) const
{
  int64_t t = m_timestamp.GetNanoSeconds ();
  i.Write ((const uint8_t *)&t, 8);
}
void 
TimestampTag::Deserialize (TagBuffer i)
{
  int64_t t;
  i.Read ((uint8_t *)&t, 8);
  m_timestamp = NanoSeconds (t);
}

void
TimestampTag::SetTimestamp (Time time)
{
  m_timestamp = time;
}
Time
TimestampTag::GetTimestamp (void) const
{
  return m_timestamp;
  
}


void 
TimestampTag::Print (std::ostream &os) const
{
  os << "ts=" << m_timestamp;
}


//----------------------------------------------------------------------
//-- DeadlineTag
//------------------------------------------------------
TypeId 
DeadlineTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("DeadlineTag")
    .SetParent<Tag> ()
    .AddConstructor<DeadlineTag> ()
    .AddAttribute ("Deadline",
                   "Some momentous point in time!",
                   EmptyAttributeValue (),
                   MakeTimeAccessor (&DeadlineTag::GetDeadline),
                   MakeTimeChecker ())
  ;
  return tid;
}
TypeId 
DeadlineTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t 
DeadlineTag::GetSerializedSize (void) const
{
  return 8;
}
void 
DeadlineTag::Serialize (TagBuffer i) const
{
  int64_t t = m_deadline.GetNanoSeconds ();
  i.Write ((const uint8_t *)&t, 8);
}
void 
DeadlineTag::Deserialize (TagBuffer i)
{
  int64_t t;
  i.Read ((uint8_t *)&t, 8);
  m_deadline = NanoSeconds (t);
}

void
DeadlineTag::SetDeadline (Time time)
{
  m_deadline = time;
}
Time
DeadlineTag::GetDeadline (void) const
{
  return m_deadline;
}

void 
DeadlineTag::Print (std::ostream &os) const
{
  os << "d=" << m_deadline;
}


}

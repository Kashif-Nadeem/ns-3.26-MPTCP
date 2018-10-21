
#ifndef MY_TAG_H
#define MY_TAG_H

#include "ns3/tag.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/application.h"
#include "ns3/double.h"
#include "ns3/random-variable-stream.h"
#include <iostream>

namespace ns3 {

class MyTag : public Tag
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  // these are our accessors to our tag structure
  void SetSimpleValue (uint8_t value);
  uint8_t GetSimpleValue (void) const;
private:
  uint8_t m_simpleValue;
};


//------------------------------------------------------
class TimestampTag : public Tag {
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);

  // these are our accessors to our tag structure
  void SetTimestamp (Time time);
  Time GetTimestamp (void) const;
  void Print (std::ostream &os) const;

private:
  Time m_timestamp;
  // end class TimestampTag
};

//------------------------------------------------------
class DeadlineTag : public Tag {
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);

  // these are our accessors to our tag structure
  void SetDeadline (Time time);
  Time GetDeadline (void) const;

  void Print (std::ostream &os) const;

private:
  Time m_deadline;

  // end class TimestampTag
};
}
#endif /* MY_TAG_H */

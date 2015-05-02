#ifndef __XBEE_UNIT_MANAGER_H_INCLUDED__
#define __XBEE_UNIT_MANAGER_H_INCLUDED__

#include "Message.h"

#include <memory>
#include <sys/time.h>


class XBeeCommManager;

class XBeeUnitManager
{
public:
  XBeeUnitManager(XBeeCommManager* pCommManager);
  virtual ~XBeeUnitManager() {}

protected:
  // Potentially blocking call; will wait for CommManager to say
  // it's ready before sending
  // Returns 0 on success
  virtual int send(std::shared_ptr<Message> msg);

  // Potentially blocking call; will wait for CommManager
  // to say it's ready before sending, and will wait for CommManager
  // to reply to an internally created ThreadQueue that the send
  // is complete, before returning
  // Will not block for longer than msTimeout
  // Returns 0 on success
  // Returns ETIMEOUT if timeout elapses
  virtual int sendAndWaitForAck(std::shared_ptr<Message> msg,
				timespec* timeout);

  XBeeCommManager* m_pCommManager;  

};


#endif

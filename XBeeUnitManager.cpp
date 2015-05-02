#include "XBeeUnitManager.h"

#include "XBeeCommManager.h"

#include <iostream>
#include <sys/time.h>


XBeeUnitManager::XBeeUnitManager(XBeeCommManager* pCommManager)
  : m_pCommManager(pCommManager)
{
  
}

int XBeeUnitManager::send(std::shared_ptr<Message> msg)
{
  timespec t;
  while (!m_pCommManager->readyToSend(&t))
  {
    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &t, NULL);
  }
  m_pCommManager->sendMessage(msg);
  return 0;
}

int XBeeUnitManager::sendAndWaitForAck(std::shared_ptr<Message> msg,
				       timespec* timeout)
{
  timespec t;
  while (!m_pCommManager->readyToSend(&t))
  {
    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &t, NULL);
  }
  std::shared_ptr<XBeeCommManager::HandlerQueue> 
    pAckQueue(new XBeeCommManager::HandlerQueue);
  int id = 1;
  m_pCommManager->sendMessageWithAck(msg, id, pAckQueue);
  std::shared_ptr<Message> ackMsg;
  while (1) 
  {
    int e = pAckQueue->getMessage(ackMsg, timeout);
//    std::cout << dtime() << " getMessage returned " << e << std::endl;
    if (e == ETIMEDOUT)
      return ETIMEDOUT;
    MessageSentAck* pAck = dynamic_cast<MessageSentAck*>(ackMsg.get());
    if (pAck != 0)
    {
      if (pAck->getMessageId() == id)
      {
//	std::cout << dtime() << " got the ack!" << std::endl;
	return 0;
      }
    }
  }
 
  return 0;
}

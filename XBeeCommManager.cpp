#include "XBeeCommManager.h"

#include "PortService.h"
#include "MessageFactory.h"
#include "util.h"
#include "ModemStatus.h"

#include <fcntl.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string.h>
#include <sys/time.h>
#include <sstream>

XBeeCommManager::XBeeCommManager(std::string portName,
				 ConfigFile* pConfig)
  : m_pCatchallHandler(0)
  , m_config(*pConfig)
  , m_port(0)
{
  std::string defaultport = m_config.getParam("PROCESS_CONFIG",
					      "PORTNAME",
					      "/dev/ttyUSB0");
  if (portName == "")
    portName = defaultport;
  
  m_portName = portName;
  
  m_estimatedSendReady.tv_sec = 0;
  m_estimatedSendReady.tv_nsec = 0;
}

XBeeCommManager::~XBeeCommManager()
{
}

void XBeeCommManager::registerHandler(const std::vector<unsigned char>& address,
				      HandlerQueue* queue)
{
  std::lock_guard<std::recursive_mutex> g(m_mutex);
  m_handlers[address] = queue;
}

void XBeeCommManager::registerCatchallHandler(HandlerQueue* pHandler)
{
  m_pCatchallHandler = pHandler;
}


void XBeeCommManager::sendMessage(std::shared_ptr<Message> msg)
{
  m_sendQueue.putMessage(msg);
}

void XBeeCommManager::sendMessageWithAck(std::shared_ptr<Message> msg,
					 int id,
					 std::shared_ptr<HandlerQueue> responseQueue)
{
  std::lock_guard<std::recursive_mutex> g(m_notifyMapMutex);
  m_handlersToNotify[msg.get()] = 
    std::make_pair(id, responseQueue);
//    std::pair<int, std::shared_ptr<ThreadQueue<Message>>>(id, responseQueue);
  sendMessage(msg);
}

void XBeeCommManager::activate()
{
  m_thread = std::thread(&XBeeCommManager::threadFunc, this);  
}

void XBeeCommManager::join()
{
  m_thread.join();  
}

bool XBeeCommManager::readyToSend(timespec* willBeReadyAt)
{
  std::lock_guard<std::recursive_mutex> g(m_estimatedSendReadyMutex);
  if (m_estimatedSendReady.tv_sec == 0 &&
      m_estimatedSendReady.tv_nsec == 0)
    return true;
  
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  if (m_estimatedSendReady.tv_sec < now.tv_sec ||
      (m_estimatedSendReady.tv_sec == now.tv_sec &&
       m_estimatedSendReady.tv_nsec < now.tv_nsec))
    return true;

  if (willBeReadyAt != NULL)
  {
    willBeReadyAt->tv_sec = m_estimatedSendReady.tv_sec;
    willBeReadyAt->tv_nsec = m_estimatedSendReady.tv_nsec;
  }
  return false;
}

void XBeeCommManager::threadFunc()
{
  std::stringstream ss;
  ss << "/bin/stty -F " << m_portName << " raw";
  ::system(ss.str().c_str());

  m_port = open(m_portName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
  if (m_port == -1)
  {
    std::cout << "Failed to open port " << m_portName;
    exit(-1);
  }

  PortService p(m_port, m_config);
  MessageFactory factory(m_config);

  while (1)
  {
    struct timespec t;
    // We want to check the queue without blocking
    t.tv_sec = 0;
    t.tv_nsec = 0;
    std::shared_ptr<Message> msg;
    int e = m_sendQueue.getMessage(msg, &t);
    if (e == 0)
    {
      // We got a request to send a message
      std::vector<unsigned char> outData = msg->getPacket();
      int we = p.write(outData);

      // Update out estimated time until next available to send
      {
	// Push the stack so this guard is released once we're done
	// with our time work
	std::lock_guard<std::recursive_mutex> g(m_estimatedSendReadyMutex);
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	// 9600 baud =~ 960 bytes/sec =~ 1ms/byte.
	long int nsecToSend = outData.size() * 1e6 * 5; // Assume 1ms per byte
	if (nsecToSend < 1e8)
	  nsecToSend = 1e8;  // Never wait less than 100ms.
	if (m_estimatedSendReady < now)
	  m_estimatedSendReady = tsNsAdd(now, nsecToSend);
	else
	  m_estimatedSendReady = tsNsAdd(m_estimatedSendReady, nsecToSend);
      }	
      
      // Notify the handler for this message if one exists
      {
	// Push the stack so this guard is released once we're done
	std::lock_guard<std::recursive_mutex> g(m_notifyMapMutex);
	if (m_handlersToNotify.find(msg.get()) != m_handlersToNotify.end())
	{
	  int id = m_handlersToNotify[msg.get()].first;
	  std::shared_ptr<Message> ack(new MessageSentAck(id));
	  m_handlersToNotify[msg.get()].second->putMessage(ack);
	  m_handlersToNotify.erase(msg.get());
	}
      }

      int d = m_config.getParam("DEBUG", "DUMP_OUTGOING_PACKETS", 0);
      if (d != 0)
      {
	std::ofstream os("/var/tmp/xbdump.out", std::ios_base::app);
	for (auto it = outData.begin(); it != outData.end(); it++)
	  os << *it;
	for (int i = 0; i < 4; i++)
	  os << '\0';
      }

      
      if (we < 0)
	std::cout << __FILE__ << ":" << __LINE__ 
		  << ": write returned " << we 
		  << " (" << strerror(we) << ")" << std::endl;
      
    }
    else if (e == ETIMEDOUT)
    {
      // There was no message
    }
    else
    {
      // Some kinda error
      std::cout << __FILE__ << ":" << __LINE__ 
		<< ": getMessage returned " << e 
		<< " (" << strerror(e) << ")" << std::endl;
    }


    std::vector<unsigned char> data;
    struct timeval tv, tvnow;
    gettimeofday(&tvnow, NULL);
    tv = tvSub(tsToTv(m_estimatedSendReady), tvnow);
    if (tv.tv_sec < 0 ||
	(tv.tv_sec == 0 &&
	 tv.tv_usec < 0))
    {
      tv.tv_sec = 0;
      tv.tv_usec = 1e5; // Wait 100ms if the estimated ready is in the past
    }
//    tv.tv_sec = 0;
//    tv.tv_usec = 1e5; // 100ms 
    int retval = p.service(data, &tv);
    if (retval != 0 && retval != 1)
    {
      std::cout << __FILE__ << ":" << __LINE__
		<< "Call to PortService::service ERROR, returned "
		<< retval << std::endl;
    }
    else if (retval != 1)
    {
      std::shared_ptr<Message> a;
      int r = factory.createMessage(data, a);
      if (r == 0)
      {
	if (m_pCatchallHandler != 0)
	{
	  m_pCatchallHandler->putMessage(a);
	}

        {
	  std::lock_guard<std::recursive_mutex> g(m_mutex);
	  if (m_handlers.find(a->getAddress()) != m_handlers.end())
	  {
	    m_handlers[a->getAddress()]->putMessage(a);
	  }
	}
      }
    }
  }  
  return;
  
}

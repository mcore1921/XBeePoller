#ifndef __XBEE_COMM_MANAGER_H_INCLUDED__
#define __XBEE_COMM_MANAGER_H_INCLUDED__

#include "ThreadQueue.h"
#include "Message.h"
#include "Config.h"

#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <map>

class PortService;
class ConfigFile;

class XBeeCommManager
{
public:
  typedef ThreadQueue<std::shared_ptr<Message>> HandlerQueue;

  XBeeCommManager(std::string portname,
		  ConfigFile* pConfig);
  virtual ~XBeeCommManager();

  void registerHandler(const std::vector<unsigned char>& address,
		       HandlerQueue* queue);
  void registerCatchallHandler(HandlerQueue* pHandler);

  // Simple message send with no "it's been sent" notification
  void sendMessage(std::shared_ptr<Message> msg);

  // More fancy message send, with an attached ThreadQueue
  // The ThreadQueue will have a MessageSentAck message put onto it
  // when the message is written to the port.  The MessageSentAck's 
  // messageId field will be populated with the id argument provided
  // to this function.
  void sendMessageWithAck(std::shared_ptr<Message> msg,
			  int id,
			  std::shared_ptr<HandlerQueue> responseQueue);
  
  int getSerialFD() { return m_port; }

  void activate();
  void join();

  // Returns true if a send request will be accepted
  // Note this is no guarantee thanks to concurrency issues
  // If it's not ready and the arg is not null, arg will be
  // populated with the time at which we will be ready
  bool readyToSend(timespec* willBeReadyAt = NULL);

private:
  void threadFunc();

  std::recursive_mutex m_mutex;
  std::map<std::vector<unsigned char>, HandlerQueue*> m_handlers;
  HandlerQueue* m_pCatchallHandler;
  ConfigFile m_config;
  HandlerQueue m_sendQueue;

  timespec m_estimatedSendReady;
  std::recursive_mutex m_estimatedSendReadyMutex;

  std::recursive_mutex m_notifyMapMutex;
  std::map<Message*, 
	   std::pair<int, std::shared_ptr<HandlerQueue>>> m_handlersToNotify;

  std::string m_portName;
  int m_port;

  std::thread m_thread;

};

#endif

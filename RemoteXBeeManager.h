#ifndef __REMOTE_XBEE_MANAGER_H_INCLUDED__
#define __REMOTE_XBEE_MANAGER_H_INCLUDED__

#include "Message.h"
#include "ThreadQueue.h"
#include "RemoteATCommand.h"
#include "RemoteCommandResponse.h"
#include "Config.h"
#include "XBeeUnitManager.h"

#include <thread>
#include <map>

class XBeeCommManager;
class DataManager;

// Manage a single remote XBee, including:
// 0) Attempting to wake the remote xbee (in case of long sleeps)
// 1) Gathering all (interesting) parameters about the remote XBee
// 2) Setting any parameters that don't match what we want
//    (including a WR to make them permanent)
// 3) Monitoring ongoing communication of incoming sample data
//    and notifying data manager when they arrive

class RemoteXBeeManager : public XBeeUnitManager
{
public:
  typedef ThreadQueue<std::shared_ptr<Message>> HandlerQueue;
  
  RemoteXBeeManager(XBeeCommManager* pCommManager,
		    const std::vector<unsigned char>& address,
		    DataManager* pDataManager,
		    ConfigFile config);
  virtual ~RemoteXBeeManager();
  
  HandlerQueue* getQueue();
  std::vector<unsigned char> getAddress();

  void activate();
  void join();

  std::string paramsDescriptionString();

  typedef enum { UNINITIALIZED = 1,
		 PARAMS_KNOWN = 2,
		 PARAMS_SET = 3,
		 INITIALIZED = 4 } InitStatus;

  InitStatus getInitStatus();

private:
  void threadFunc();

  // Sends cmd, and waits for a response of the same command type
  // Populates the response argument with the response
  // Timeout needs to be long enough that XBee Tx timeout will
  // hit (preferably several times) before it times out.  
  // This function will retry transmission in response to an error
  // returned by the coordinator.
  // The XBee Tx timeout for an end device is:
  //   3 * (50 * NH) + (1.2 * SP)  (Note SP is in 10ms steps)
  // NH is 30 by default.  SP is typically 2500 for this app, but
  // you should check.  So, in theory, timeout should be
  // ~ 3 * (50*30) 1500ms + (1.2 * 25000) 30sec = 3 * 31.5s = ~95 sec
  // ... but now give that 2 or 3 tries.  So, ~300 sec is good.
  // Returns 0 on success
  // Returns ETIMEOUT if timeout elapses
  int sendAndWaitForResponse(RemoteATCommand cmd,
			     RemoteCommandResponse& response,
			     timespec* timeout);

  void getParams();
  int getParam(std::string param,
	       RemoteCommandResponse& rcr);

  void setParams();
  int setParam(std::string param,
	       int value,
	       RemoteCommandResponse& rcr);

  void drainMessages();

  std::vector<unsigned char> m_address;
  DataManager* m_pDataManager;
  HandlerQueue m_incomingQueue;

  timeval m_lastReceiveTime;
  std::recursive_mutex m_paramsMutex;
  std::map<std::string, int> m_intParams;
  std::map<std::string, std::string> m_stringParams;

  ConfigFile m_config;
  std::string m_name;
  double m_sensorCal;
  int m_lastAIOSample;
  unsigned char m_frameId;
  int m_writeCounter;

  InitStatus m_initStatus;

  std::thread m_thread;
};


#endif

#ifndef __COORDINATOR_XBEE_MANAGER_H_INCLUDED__
#define __COORDINATOR_XBEE_MANAGER_H_INCLUDED__

#include "Message.h"
#include "ThreadQueue.h"
#include "ATCommand.h"
#include "ATCommandResponse.h"
#include "Config.h"
#include "XBeeUnitManager.h"

#include <thread>
#include <map>

class XBeeCommManager;
class DataManager;

class CoordinatorXBeeManager : public XBeeUnitManager
{
public:
  typedef ThreadQueue<std::shared_ptr<Message>> HandlerQueue;
  
  CoordinatorXBeeManager(XBeeCommManager* pCommManager,
			 DataManager* pDataManager,
			 ConfigFile config);
  virtual ~CoordinatorXBeeManager();
  
  HandlerQueue* getQueue();

  void activate();
  void join();

  std::string paramsDescriptionString();

  typedef enum { UNINITIALIZED = 1,
		 RESET_COMPLETE = 2,
		 PARAMS_KNOWN = 3,
		 PARAMS_SET = 4,
		 INITIALIZED = 5 } InitStatus;

  InitStatus getInitStatus();

private:
  void threadFunc();

  // Sends cmd, and waits for a response of the same command type
  // Populates the response argument with the response
  // Returns 0 on success
  // Returns ETIMEOUT if timeout elapses
  int sendAndWaitForResponse(ATCommand cmd,
			     ATCommandResponse& response,
			     timespec* timeout);

  void getParams();
  int getParam(std::string param,
	       ATCommandResponse& rcr);

  void updateParams();
  int updateParam(std::string param,
		  std::vector<unsigned char> arg,
		  ATCommandResponse& rcr);

  void resetCoordinator();

  void debugDrainMessages();

  DataManager* m_pDataManager;
  HandlerQueue m_incomingQueue;

  timeval m_lastReceiveTime;
  std::recursive_mutex m_paramsMutex;
  std::map<std::string, int> m_intParams;
  std::map<std::string, std::string> m_stringParams;

  ConfigFile m_config;
  unsigned char m_frameId;

  InitStatus m_initStatus;

  std::thread m_thread;
};

#endif

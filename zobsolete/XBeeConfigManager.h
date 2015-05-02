#ifndef __XBEE_CONFIG_MANAGER_H_INCLUDED__
#define __XBEE_CONFIG_MANAGER_H_INCLUDED__

#include "ThreadQueue.h"
#include "Message.h"
#include "Config.h"

#include <memory>
#include <vector>
#include <time.h>

class XBeeCommManager;

// Parameters for a single XBee
class XBeeConfigParams
{
public:
  XBeeConfigParams();
  std::string m_address;
  struct timeval m_lastCommunicateTime;
  struct timeval m_lastSendTime;
  int m_queryOutstanding;
  int m_lastQueryFrameId;
  int m_lastAI0Sample;
  std::map<std::string, std::string> m_stringParams;
  std::map<std::string, int> m_intParams;
};

class XBeeConfigManager
{
public:
  XBeeConfigManager(XBeeCommManager* pCommManager,
		    ConfigFile cf);
  
  // Give the config object a chance to do something, if it wants
  void heartbeat();

  // Returns 0 on success, nonzero on failure
  int handleMessage(std::shared_ptr<Message> pMessage);

private:
  void writeStatusFile();
  void doSensorSearch();
  void doInit();

  // Check to see if there are any desired parameters
  // for the argument module that we don't yet have.
  // If there are, and if we haven't sent anything in a 
  // reasonable period of time, ask for one of them
  void getMissingParameters(const std::vector<unsigned char>& addressData);

  // Check to see if there are any desired parameters
  // for the coordinator that we don't yet have.
  // If there are, and if we haven't sent anything in a 
  // reasonable period of time, ask for one of them
  void getCoordinatorParameters();

  // No-op until the coordinator parameters are known
  // If the parameters don't match what's desired, request a change
  void updateCoordinatorParameters();
  
  // No-op until the sensor parameters are known
  // If the parameters don't match what's desired, request a change
  void updateSensorParameters();

  XBeeCommManager* m_pCommManager;
  ConfigFile m_config;  
  
  // Map keyed off an address 
  std::map<std::vector<unsigned char>, XBeeConfigParams> m_params;
  XBeeConfigParams m_coordinatorParams;

  struct timeval m_lastStatusWrite;
  struct timeval m_lastSensorSearch;
  struct timeval m_lastPacketSend;
  struct timeval m_initTime;
  int m_initDone;
};


#endif

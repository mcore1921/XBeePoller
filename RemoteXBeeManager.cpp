#include "RemoteXBeeManager.h"

#include "DataManager.h"
#include "XBeeCommManager.h"
#include "RemoteATCommand.h"
#include "RemoteCommandResponse.h"
#include "IOSample.h"
#include "util.h"

#include <iostream>
#include <iomanip>
#include <sys/time.h>
#include <sstream>
#include <random>


#define RXBMLOG std::setprecision(3) << std::fixed << dtime() << " RXBM " << macAddrString(m_address) << ":" 

RemoteXBeeManager::RemoteXBeeManager(XBeeCommManager* pCommManager,
				     const std::vector<unsigned char>& address,
				     DataManager* pDataManager,
				     ConfigFile config)
  : XBeeUnitManager(pCommManager)
  , m_address(address)
  , m_pDataManager(pDataManager)
  , m_config(config)
{
  gettimeofday(&m_lastReceiveTime, NULL);
  
  m_name = m_config.getParam("SENSORS",
			     macAddrString(m_address),
			     "UNKNOWN");

  m_sensorCal = m_config.getParam("CALIBRATIONS",
				  macAddrString(m_address),
				  1.0);

  m_lastAIOSample = 0;
  m_frameId = 0;

  m_initStatus = UNINITIALIZED;
  m_writeCounter = 0;
}

RemoteXBeeManager::~RemoteXBeeManager()
{
}
  
RemoteXBeeManager::HandlerQueue* RemoteXBeeManager::getQueue()
{
  return &m_incomingQueue;
}

std::vector<unsigned char> RemoteXBeeManager::getAddress()
{
  return m_address;
}

void RemoteXBeeManager::activate()
{
  m_thread = std::thread(&RemoteXBeeManager::threadFunc, this);  
}

void RemoteXBeeManager::join()
{
  m_thread.join();
}

std::string RemoteXBeeManager::paramsDescriptionString()
{
  std::lock_guard<std::recursive_mutex> g(m_paramsMutex);

  struct timeval tvnow;
  gettimeofday(&tvnow, NULL);

  std::stringstream os;
  // Dump sensor config parameters and status
  
  double degC = (((double)m_lastAIOSample 
		  * 1000.0 / 1023.0 * 1.2) 
		 - 500.0) * 0.1 * m_sensorCal;
  double degF = (degC * 9.0 / 5.0) + 32;
  os << macAddrString(m_address) << " " 
     << std::fixed << std::setw(8) << std::setfill('0') << std::right 
     << std::setprecision(3)
     << tvdiff(tvnow, m_lastReceiveTime) << " "
     << m_initStatus << " "
     << std::setw(3) << std::setfill(' ') << (int) m_frameId << " "
     << m_sensorCal << " "
     << std::setw(3) << std::setfill(' ') << m_lastAIOSample << " "
     << m_writeCounter << " "
     << "(" << std::setprecision(1) << std::setfill(' ') << std::right
     << std::setw(5) << degC << " C) (" 
     << std::setw(5) << degF << " F) "
     << m_name
     << std::endl;
  int loc = 0;

  // Map for items that should be printed in hex
  std::map<std::string, int> hexVals;
  hexVals["HV"] = 1;
  hexVals["VR"] = 1;
  for (auto& intpair : m_intParams)
  {
    os << "  " << intpair.first << " ";
    if (hexVals[intpair.first] == 1)
    {
      os << "0x" << std::setw(4) << std::setfill('0')
	 << std::left << std::hex << intpair.second;
    }
    else
    {
      os << std::setw(6) << std::setfill(' ') 
	 << std::left << std::dec << intpair.second;
    }
    if (++loc > 4)
    {
      os << std::endl;
      loc = 0;
    }
  }
  if (loc != 0) os << std::endl;
  loc = 0;
  for (auto& stringpair : m_stringParams)
  {
    os << "  " << stringpair.first
       <<  " " << std::setw(17) << std::setfill(' ') << stringpair.second;
    if (++loc > 2)
    {
      os << std::endl;
      loc = 0;
    }
  }
  if (loc != 0) os << std::endl;

  os << std::endl;

  return os.str();
}

RemoteXBeeManager::InitStatus RemoteXBeeManager::getInitStatus()
{
  return m_initStatus;
}

void RemoteXBeeManager::threadFunc()
{
  std::hash<std::thread::id> h1;
  std::minstd_rand0 rgen(h1(m_thread.get_id()));
  m_frameId = rgen() % 256;
  if (m_frameId == 0) m_frameId = 1;

  getParams();

  m_initStatus = PARAMS_KNOWN;

  setParams();

  m_initStatus = PARAMS_SET;

  m_initStatus = INITIALIZED;

  drainMessages();
}


int RemoteXBeeManager::sendAndWaitForResponse(RemoteATCommand cmd,
					      RemoteCommandResponse& response,
					      timespec* timeout)
{
  std::shared_ptr<Message> pCmd(new RemoteATCommand(cmd));
  int e1;
  while (1)
  {
    e1 = sendAndWaitForAck(pCmd,
			   timeout); 
    if (e1 == 0)
      break;
    else if (e1 == ETIMEDOUT)
      return ETIMEDOUT;
  }
  std::cout << RXBMLOG << " Sent a command " 
	    << cmd.getCommand()[0] << cmd.getCommand()[1] 
	    << " frameId " << std::dec << cmd.getFrameId() << std::endl;
  std::shared_ptr<Message> pResponse;
  while (1)
  {
    e1 = m_incomingQueue.getMessage(pResponse, timeout);
    if (e1 == 0)
    {
      gettimeofday(&m_lastReceiveTime, NULL);
      RemoteCommandResponse* p = 
	dynamic_cast<RemoteCommandResponse*>(pResponse.get());
      if (p != 0)
      {
	unsigned char cmdFrameId = 
	  ((RemoteATCommand*)pCmd.get())->getFrameId();
	if (p->getFrameId() == cmdFrameId &&
	    p->getCommand() == cmd.getCommand())
	{
	  if (p->getStatus() != 0)
	  {
	    std::cout << RXBMLOG << " Got a status " 
		      << (int) p->getStatus() << " in response to "
		      << cmd.getCommand()[0] << cmd.getCommand()[1]
		      << " frame " << std::dec << (int) cmdFrameId
		      << ".  Retrying." << std::endl;
	    unsigned char newFrameId = 
	      ((RemoteATCommand*)pCmd.get())->getFrameId()+1;
	    if (newFrameId == 0) newFrameId = 1;
	    ((RemoteATCommand*)pCmd.get())->setFrameId(newFrameId);
	    m_frameId = newFrameId;
	    // Send again
	    while (1)
	    {
	      e1 = sendAndWaitForAck(pCmd,
				     timeout); 
	      if (e1 == 0)
		break;
	      else if (e1 == ETIMEDOUT)
		return ETIMEDOUT;
	    }

	    continue;
	  }
	  else
	  {
	    response = *p;
	    return 0;
	  }
	}
	else 
	{
	  std::cout << RXBMLOG << " Command or Frame mismatch. "
		    << "Expecting " 
		    << cmd.getCommand()[0] << cmd.getCommand()[1]
		    << " frame " << (int) cmd.getFrameId() <<", got "
		    << p->getCommand()[0] << p->getCommand()[1]
		    << " frame " << (int) p->getFrameId() 
		    << ".  Continuing to wait for proper response." 
		    << std::endl;
	}
	    
      }
      else
      {
	std::cout << RXBMLOG << " Got a " 
		  << pResponse->type() << " message when waiting for "
		  << cmd.getCommand()[0] << cmd.getCommand()[1]
		  << " response.  Discarding." << std::endl;
      }
    }
    else if (e1 == ETIMEDOUT)
    {
      std::cout << RXBMLOG << " Timeout on response to " 
		<< cmd.getCommand()[0] << cmd.getCommand()[1] 
		<< " frame " << std::dec << cmd.getFrameId() << std::endl;
      return ETIMEDOUT;
    }
  }
}
  
void RemoteXBeeManager::getParams()
{
  std::vector<std::string> intParamList = 
    { "D0", "D1", "SM", "SP", "SN", "IR", "ST", "VR", "HV", "%V" };
  std::vector<std::string> stringParamList =
    { "SH" , "SL", "DD" };

  for (auto& param : intParamList)
  {
    RemoteCommandResponse rcr;
    getParam(param, rcr);
    {
      // Make a stack layer so we can use a guard
      std::lock_guard<std::recursive_mutex> g(m_paramsMutex);
      m_intParams[param] = rcr.getArg();
    }
    std::cout << RXBMLOG << " IntParam " << param << ":" << rcr.getArg() 
	      << " frame " << (int) rcr.getFrameId() << std::endl;
  }
  
  for (auto& param : stringParamList)
  {
    RemoteCommandResponse rcr;
    getParam(param, rcr);
    {
      // Make a stack layer so we can use a guard
      std::lock_guard<std::recursive_mutex> g(m_paramsMutex);
      m_stringParams[param] = macAddrString(rcr.getArgData());
    }
    std::cout << RXBMLOG << " StringParam " << param 
	      << ":" << macAddrString(rcr.getArgData()) 
	      << " frame " << (int) rcr.getFrameId() << std::endl;
  }

}

int RemoteXBeeManager::getParam(std::string param,
				RemoteCommandResponse& rcr)
{
  std::vector<unsigned char> paramData;
  paramData.push_back(param[0]);
  paramData.push_back(param[1]);
  std::vector<unsigned char> arg;
  RemoteATCommand a(paramData,
		    m_address,
		    0, m_frameId, arg);
  m_frameId++;  if (m_frameId == 0) m_frameId = 1;

  timespec timeout, now;
  clock_gettime(CLOCK_REALTIME, &now);
  int timeoutsec = m_config.getParam("REMOTE_XBEE_MGR_CONFIG",
				     "GET_PARAM_TIMEOUT",
				     300); // See header file for discussion
  timeout = tsMsAdd(now, timeoutsec*1e3);

  while (1)
  {
    int e = sendAndWaitForResponse(a, rcr, &timeout);
    if (e == ETIMEDOUT)
    {
      std::cout << RXBMLOG << " Timeout (" << timeoutsec << "s) waiting for "
		<< param << " response.  Retrying." << std::endl;
      a.setFrameId(m_frameId);
      m_frameId++;  if (m_frameId == 0) m_frameId = 1;
      timeout = tsMsAdd(timeout, timeoutsec*1e3);
    }
    else if (e != 0)
    {
      std::cout << RXBMLOG << " Unknown error " << e << " waiting for "
		<< param << " response.  Bailing." << std::endl;
      return e;
    }
    else 
    {
      return 0;
    }
  }
}

void RemoteXBeeManager::setParams()
{
  bool anySet = false;
  // Note that order of application is important; we'll put these
  // into an ordered container instead of a map
  std::vector<std::pair<std::string, int>> intParams;
  intParams.push_back(std::make_pair("D0", 2));
  intParams.push_back(std::make_pair("D1", 0));
  intParams.push_back(std::make_pair("ST", 5000));
  intParams.push_back(std::make_pair("SN", 6));
  intParams.push_back(std::make_pair("SM", 4));
  intParams.push_back(std::make_pair("SP", 2500));
  intParams.push_back(std::make_pair("IR", 250));
/*
  std::map<std::string, int> intParamsMap;
  // TODO : make this config-file read
  intParamsMap["D0"] = 2;
  intParamsMap["D1"] = 0;
  intParamsMap["SM"] = 4;
  intParamsMap["SP"] = 2500;
  intParamsMap["SN"] = 6;
  intParamsMap["IR"] = 250;
  intParamsMap["ST"] = 5000;
*/

  for (auto& param : intParams)
  {
    if (m_intParams.find(param.first) != m_intParams.end() &&
	m_intParams[param.first] != param.second)
    {
      std::cout << RXBMLOG << " IntParam " << param.first << ":" 
		<< " was " << m_intParams[param.first] << ", setting to "
		<< param.second << std::endl;
      anySet = true;
      RemoteCommandResponse rcr;
      setParam(param.first, param.second, rcr);

      // Now that we've set it, query to make sure it's set
      getParam(param.first, rcr);
      {
	// Make a stack layer so we can use a guard
	std::lock_guard<std::recursive_mutex> g(m_paramsMutex);
	m_intParams[param.first] = rcr.getArg();
      }
      std::cout << RXBMLOG << " IntParam " << param.first 
		<< ":" << rcr.getArg() 
		<< " frame " << (int) rcr.getFrameId() << std::endl;
    }
  }

  // If any parameters have been changed, write them to flash
  if (anySet)
  {
    std::cout << RXBMLOG << " Writing new params" << std::endl;
    RemoteCommandResponse rcr;
    setParam("WR", 0, rcr);
    m_writeCounter++;
  }

/*  
  for (auto& param : stringParamList)
  {
    RemoteCommandResponse rcr;
    getParam(param, rcr);
    {
      // Make a stack layer so we can use a guard
      std::lock_guard<std::recursive_mutex> g(m_paramsMutex);
      m_stringParams[param] = macAddrString(rcr.getArgData());
    }
    std::cout << RXBMLOG << " StringParam " << param 
	      << ":" << macAddrString(rcr.getArgData()) 
	      << " frame " << (int) rcr.getFrameId() << std::endl;
  }
*/
}

int RemoteXBeeManager::setParam(std::string param,
				int value,
				RemoteCommandResponse& rcr)
{
  std::vector<unsigned char> paramData;
  paramData.push_back(param[0]);
  paramData.push_back(param[1]);
  std::vector<unsigned char> arg;
  arg.push_back((value & 0xFF00) >> 8);
  arg.push_back(value & 0xFF);

  RemoteATCommand a(paramData,
		    m_address,
		    2,    // apply changes immediately
		    m_frameId, arg);
  m_frameId++;  if (m_frameId == 0) m_frameId = 1;

  timespec timeout, now;
  clock_gettime(CLOCK_REALTIME, &now);
  int timeoutsec = m_config.getParam("REMOTE_XBEE_MGR_CONFIG",
				     "SET_PARAM_TIMEOUT",
				     300); // See header file for discussion
  timeout = tsMsAdd(now, timeoutsec*1e3);

  while (1)
  {
    int e = sendAndWaitForResponse(a, rcr, &timeout);
    if (e == ETIMEDOUT)
    {
      std::cout << RXBMLOG << " Timeout (" << timeoutsec << "s) waiting for "
		<< param << " response.  Retrying." << std::endl;
      a.setFrameId(m_frameId);
      m_frameId++;  if (m_frameId == 0) m_frameId = 1;
      timeout = tsMsAdd(timeout, timeoutsec*1e3);
    }
    else if (e != 0)
    {
      std::cout << RXBMLOG << " Unknown error " << e << " waiting for "
		<< param << " response.  Bailing." << std::endl;
      return e;
    }
    else 
    {
      std::cout << RXBMLOG << " Set param " << param << " to value "
		<< value << ".  Re-querying." << std::endl;
      return 0;
    }
  }
}

void RemoteXBeeManager::drainMessages()
{
  std::shared_ptr<Message> pMsg;
  do {
    int e = m_incomingQueue.getMessage(pMsg, 30*1e3);
    if (e == 0)
    {
      gettimeofday(&m_lastReceiveTime, NULL);
      RemoteCommandResponse* pResponse =
	dynamic_cast<RemoteCommandResponse*>(pMsg.get());
      if (pResponse != 0)
      {
	std::cout << std::setprecision(3) << std::fixed << dtime() 
		  << " RXBM: " 
		  << macAddrString(pResponse->getAddress()) 
		  << ":" 
		  << pResponse->getCommand()[0]
		  << pResponse->getCommand()[1] 
		  << std::endl;
	continue;
      }
      IOSample* pSample =
	dynamic_cast<IOSample*>(pMsg.get());
      if (pSample != 0)
      {
	std::cout << RXBMLOG << " sample " << std::dec
		  << pSample->getAnalogValue() << std::endl;
	// Compute the degF
	double degC = (((double)pSample->getAnalogValue() 
			* 1000.0 / 1023.0 * 1.2) 
		       - 500.0) * 0.1 * m_sensorCal;
	double degF = (degC * 9.0 / 5.0) + 32;

	m_pDataManager->handleIOSample(m_address,
				       pSample->getAnalogValue(),
				       m_name,
				       degF);

	m_lastAIOSample = pSample->getAnalogValue();
	continue;
      }
      std::cout << std::setprecision(3) << std::fixed << dtime() 
		<< "RXBM: got message "
		<< pMsg->type() << std::endl;
    }
  } while (1);

}

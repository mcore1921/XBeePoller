#include "CoordinatorXBeeManager.h"

#include "XBeeCommManager.h"
#include "ATCommand.h"
#include "ATCommandResponse.h"
#include "ModemStatus.h"
#include "util.h"

#include <iostream>
#include <iomanip>
#include <sys/time.h>
#include <sstream>
#include <random>


#define CXBMLOG std::setprecision(3) << std::fixed << dtime() << " CXBM:" 

CoordinatorXBeeManager::CoordinatorXBeeManager(XBeeCommManager* pCommManager,
					       DataManager* pDataManager,
					       ConfigFile config)
  : XBeeUnitManager(pCommManager)
  , m_pDataManager(pDataManager)
  , m_config(config)
{
  gettimeofday(&m_lastReceiveTime, NULL);
  
  m_frameId = 0;

  m_initStatus = UNINITIALIZED;
}

CoordinatorXBeeManager::~CoordinatorXBeeManager()
{
}
  
CoordinatorXBeeManager::HandlerQueue* CoordinatorXBeeManager::getQueue()
{
  return &m_incomingQueue;
}

void CoordinatorXBeeManager::activate()
{
  m_thread = std::thread(&CoordinatorXBeeManager::threadFunc, this);  
}

void CoordinatorXBeeManager::join()
{
  m_thread.join();
}

std::string CoordinatorXBeeManager::paramsDescriptionString()
{
  std::lock_guard<std::recursive_mutex> g(m_paramsMutex);

  struct timeval tvnow;
  gettimeofday(&tvnow, NULL);

  std::stringstream os;
  // Dump config parameters and status
  os << std::setw(24) << std::left << "Coordinator "
     << std::fixed << std::setw(8) << std::setfill('0') << std::right 
     << std::setprecision(3)
     << tvdiff(tvnow, m_lastReceiveTime) << " "
     << m_initStatus << " "
     << std::setw(3) << std::setfill(' ') << (int) m_frameId << " "
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

CoordinatorXBeeManager::InitStatus CoordinatorXBeeManager::getInitStatus()
{
  return m_initStatus;
}

void CoordinatorXBeeManager::threadFunc()
{
  std::hash<std::thread::id> h1;
  std::minstd_rand0 rgen(h1(m_thread.get_id()));
  m_frameId = rgen() % 256;
  if (m_frameId == 0) m_frameId = 1;

  resetCoordinator();

  m_initStatus = RESET_COMPLETE;

  getParams();

  m_initStatus = PARAMS_KNOWN;

  updateParams();

  m_initStatus = PARAMS_SET;

  m_initStatus = INITIALIZED;

  debugDrainMessages();
}

void CoordinatorXBeeManager::resetCoordinator()
{
  // We expect that, after a FR command, the modem will
  // respond with ModemStatus 1 and then ModemStatus 6.
  // For some reason this doesn't always happen.
  // In this function we will keep retrying the FR
  // until we get those two responses within the timeout
  typedef enum  { INIT, 
		  WATCHDOG_TIMER_RESET,
		  COORDINATOR_STARTED } resetCoordinatorState;

  std::shared_ptr<Message> pMsg(new ATCommand("FR", 100));
  
  int timeoutsec = 10;

  while (1)
  {
    timespec timeout, now;
    clock_gettime(CLOCK_REALTIME, &now);
    timeout = tsMsAdd(now, timeoutsec*1e3);

    resetCoordinatorState state = INIT;
    std::cout << CXBMLOG << " Restarting coordinator." << std::endl;
    m_pCommManager->sendMessage(pMsg);
    std::shared_ptr<Message> response;
    // inner loop so that we keep waiting in case of spurious signals
    while (1)
    {
      int e = m_incomingQueue.getMessage(response, &timeout);
      if (e == 0)
      {
	gettimeofday(&m_lastReceiveTime, NULL);
	ModemStatus* pMS = dynamic_cast<ModemStatus*>(response.get());
	if (pMS != 0)
	{
	  if (state == INIT && pMS->getStatus() == 1)
	  {
	    std::cout << CXBMLOG << " Watchdog Timer Reset complete." 
		      << std::endl;
	    state = WATCHDOG_TIMER_RESET;
	  }
	  else if (state == WATCHDOG_TIMER_RESET &&
		   pMS->getStatus() == 6)
	  {
	    std::cout << CXBMLOG << " Coordinator started.  Reset complete."
		      << std::endl;
	    state = COORDINATOR_STARTED;
	    return;
	  }
	}
      }
      else if (e == ETIMEDOUT)
      {
	// Break out of the inner loop and resend the command
	// if we time out
	std::cout << CXBMLOG << " Reset command timed out.  Retrying."
		  << std::endl;
	break;
      }
    }
  }
}


int CoordinatorXBeeManager::sendAndWaitForResponse(ATCommand cmd,
						   ATCommandResponse& response,
						   timespec* timeout)
{
  std::shared_ptr<Message> pCmd(new ATCommand(cmd));
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
  std::cout << CXBMLOG << " Sent a command " 
	    << cmd.getCommand()[0] << cmd.getCommand()[1] 
	    << " frameId " << std::dec << cmd.getFrameId() << std::endl;
  std::shared_ptr<Message> pResponse;
  while (1)
  {
    e1 = m_incomingQueue.getMessage(pResponse, timeout);
    if (e1 == 0)
    {
      gettimeofday(&m_lastReceiveTime, NULL);
      ATCommandResponse* p = 
	dynamic_cast<ATCommandResponse*>(pResponse.get());
      if (p != 0)
      {
	if (p->getFrameId() == cmd.getFrameId() &&
	    p->getCommand() == cmd.getCommand())
	{
	  if (p->getStatus() != 0)
	  {
	    std::cout << CXBMLOG << " Got a status " 
		      << (int) p->getStatus() << " in response to "
		      << cmd.getCommand()[0] << cmd.getCommand()[1]
		      << " frame " << std::dec << cmd.getFrameId()
		      << ".  Retrying." << std::endl;

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
      }
      std::cout << CXBMLOG << " Got a " 
		<< pResponse->type() << " message when waiting for "
		<< cmd.getCommand()[0] << cmd.getCommand()[1]
		<< " response.  Discarding." << std::endl;
    }
    else if (e1 == ETIMEDOUT)
    {
      std::cout << CXBMLOG << " Timeout on response to " 
		<< cmd.getCommand()[0] << cmd.getCommand()[1] 
		<< " frame " << std::dec << cmd.getFrameId() << std::endl;
      return ETIMEDOUT;
    }
  }
}
  
void CoordinatorXBeeManager::getParams()
{
  std::vector<std::string> intParamList = 
    { "ID", "SP", "SN", "VR", "HV" };
  std::vector<std::string> stringParamList =
    { "SH" , "SL", "DD" };

  for (auto& param : intParamList)
  {
    ATCommandResponse rcr;
    getParam(param, rcr);
    {
      // Make a stack layer so we can use a guard
      std::lock_guard<std::recursive_mutex> g(m_paramsMutex);
      m_intParams[param] = rcr.getArg();
    }
    std::cout << CXBMLOG << " IntParam " << param << ":" << rcr.getArg() 
	      << " frame " << (int) rcr.getFrameId() << std::endl;
  }
  
  for (auto& param : stringParamList)
  {
    ATCommandResponse rcr;
    getParam(param, rcr);
    {
      // Make a stack layer so we can use a guard
      std::lock_guard<std::recursive_mutex> g(m_paramsMutex);
      m_stringParams[param] = macAddrString(rcr.getArgData());
    }
    std::cout << CXBMLOG << " StringParam " << param 
	      << ":" << macAddrString(rcr.getArgData()) 
	      << " frame " << (int) rcr.getFrameId() << std::endl;
  }

}

int CoordinatorXBeeManager::getParam(std::string param,
				     ATCommandResponse& rcr)
{
  std::vector<unsigned char> paramData;
  paramData.push_back(param[0]);
  paramData.push_back(param[1]);
  std::vector<unsigned char> arg;
  ATCommand a(paramData,
	      m_frameId,
	      arg);
  m_frameId++;  if (m_frameId == 0) m_frameId = 1;

  timespec timeout, now;
  clock_gettime(CLOCK_REALTIME, &now);
  int timeoutsec = 10;
  timeout = tsMsAdd(now, timeoutsec*1e3);

  while (1)
  {
    int e = sendAndWaitForResponse(a, rcr, &timeout);
    if (e == ETIMEDOUT)
    {
      std::cout << CXBMLOG << " Timeout (" << timeoutsec << "s) waiting for "
		<< param << " response.  Retrying." << std::endl;
      a.setFrameId(m_frameId);
      m_frameId++;  if (m_frameId == 0) m_frameId = 1;
      timeout = tsMsAdd(timeout, timeoutsec*1e3);
    }
    else if (e != 0)
    {
      std::cout << CXBMLOG << " Unknown error " << e << " waiting for "
		<< param << " response.  Bailing." << std::endl;
      return e;
    }
    else 
    {
      return 0;
    }
  }
}

void CoordinatorXBeeManager::updateParams()
{
  // Note this currently only works for 2-byte settings
  // More work is needed if we want other sizes
  std::map<std::string, int> desiredSettings;
  desiredSettings["SP"] = m_config.getParam("DESIRED_COORDINATOR_PARAMS",
					    "SP",
					    2500);
  desiredSettings["SN"] = m_config.getParam("DESIRED_COORDINATOR_PARAMS",
					    "SN",
					    36);
  

  for (auto& desiredPair : desiredSettings)
  {
    bool doUpdate;
    {
      // Make a stack layer so we can use a guard
      std::lock_guard<std::recursive_mutex> g(m_paramsMutex);
      doUpdate = (m_intParams.find(desiredPair.first) == m_intParams.end() ||
		  m_intParams[desiredPair.first] != desiredPair.second);
    }
    if (doUpdate)
    {
      std::vector<unsigned char> arg;
      arg.push_back((desiredPair.second & 0xFF00) >> 8);
      arg.push_back(desiredPair.second & 0xFF);
      ATCommandResponse rcr;
      updateParam(desiredPair.first, arg, rcr);
      std::cout << CXBMLOG << " Set IntParam " << desiredPair.first 
		<< ":" << rcr.getArg() 
		<< " frame " << (int) rcr.getFrameId() 
		<< ", now re-querying" << std::endl;
      getParam(desiredPair.first, rcr);
      {
	// Make a stack layer so we can use a guard
	std::lock_guard<std::recursive_mutex> g(m_paramsMutex);
	m_intParams[desiredPair.first] = rcr.getArg();
      }
      std::cout << CXBMLOG << " IntParam " << desiredPair.first
		<< ":" << rcr.getArg() 
		<< " frame " << (int) rcr.getFrameId() << std::endl;
    }
  }
}

int CoordinatorXBeeManager::updateParam(std::string param,
					std::vector<unsigned char> arg,
					ATCommandResponse& rcr)
{
  std::vector<unsigned char> paramData;
  paramData.push_back(param[0]);
  paramData.push_back(param[1]);
  ATCommand a(paramData,
	      m_frameId,
	      arg);
  m_frameId++;  if (m_frameId == 0) m_frameId = 1;

  timespec timeout, now;
  clock_gettime(CLOCK_REALTIME, &now);
  int timeoutsec = 10;
  timeout = tsMsAdd(now, timeoutsec*1e3);

  while (1)
  {
    int e = sendAndWaitForResponse(a, rcr, &timeout);
    if (e == ETIMEDOUT)
    {
      std::cout << CXBMLOG << " Timeout (" << timeoutsec << "s) waiting for "
		<< param << " response.  Retrying." << std::endl;
      a.setFrameId(m_frameId);
      m_frameId++;  if (m_frameId == 0) m_frameId = 1;
      timeout = tsMsAdd(timeout, timeoutsec*1e3);
    }
    else if (e != 0)
    {
      std::cout << CXBMLOG << " Unknown error " << e << " waiting for "
		<< param << " response.  Bailing." << std::endl;
      return e;
    }
    else 
    {
      return 0;
    }
  }
}


void CoordinatorXBeeManager::debugDrainMessages()
{
  std::shared_ptr<Message> pMsg;
  do {
    int e = m_incomingQueue.getMessage(pMsg, 30*1e3);
    if (e == 0)
    {
      gettimeofday(&m_lastReceiveTime, NULL);
      ATCommandResponse* pResponse =
	dynamic_cast<ATCommandResponse*>(pMsg.get());
      if (pResponse != 0)
      {
	std::cout << CXBMLOG << " ATCommandResponse "
		  << pResponse->getCommand()[0]
		  << pResponse->getCommand()[1] 
		  << std::endl;
	continue;
      }
      ModemStatus* pModemStatus = 
	dynamic_cast<ModemStatus*>(pMsg.get());
      if (pModemStatus != 0)
      {
	std::cout << CXBMLOG << " ModemStatus "
		  << pModemStatus->getStatus() 
		  << std::endl;
	continue;
      }
/* The coordinator manager is a catchall - this will spam if we log it.
      std::cout << CXBMLOG << " got message "
		<< pMsg->type() << std::endl;
*/
    }
  } while (1);

}

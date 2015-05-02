#include "MessageHandler.h"
#include "IOSample.h"
#include "RemoteCommandResponse.h"
#include "ATCommand.h"
#include "ATCommandResponse.h"
#include "SQLConnector.h"
#include "XBeeConfigManager.h"
#include "util.h"

#include <iomanip>
#include <iostream>
#include <sys/time.h>
#include <fstream>

MessageHandler::MessageHandler(XBeeConfigManager* pManager,
			       ConfigFile cf)
  : m_config(cf)
{
  m_pSqlConnector = new SQLConnector(cf);
  m_pConfigManager = pManager;
}

MessageHandler::~MessageHandler()
{
  delete m_pSqlConnector;
}

// Returns 0 on success, nonzero on failure
int MessageHandler::handleMessage(std::shared_ptr<Message> pMessage)
{
  std::string type = pMessage->type();
  if (type == "IOSample")
  {
    return handleIOSample(pMessage);
  }
  else if (type == "RemoteCommandResponse")
  {
    return handleRemoteCommandResponse(pMessage);
  }
  else if (type == "ATCommandResponse")
  {
    return handleATCommandResponse(pMessage);
  }
  else if (type == "ATCommand" ||
	   type == "RemoteATCommand")
  {
    std::cout << __FILE__ << ":" << __LINE__ 
	      << ": MessageHandler::handleMessage given " 
	      << type << " msg. "
	      << "This is confusing." << std::endl;
    return -1;
  }
  else
  {
    std::cout << __FILE__ << ":" << __LINE__ 
	      << ": MessageHandler::handleMessage given unknown msg type "
	      << type << std::endl;
    return -1;
  }

  return 0;
}

int MessageHandler::handleIOSample(std::shared_ptr<Message> pMessage)
{
  // Let the config manager know about this message
  m_pConfigManager->handleMessage(pMessage);

  // Process the sample and add to the database
  IOSample sample = *(dynamic_cast<IOSample*>(pMessage.get()));
  double descaled = sample.getAnalogValue() * (1000.0/1023.0) * 1.2;
  double celsius = (descaled - 500.0) * 0.1;
  double faren = celsius * 9.0 / 5.0 + 32.0;
  std::string serno = macAddrString(sample.getSenderAddress());
    
  std::cout << std::setprecision(3) << std::fixed << dtime() << " ";
  std::cout << serno << " "
	    << (int) sample.getAnalogValue() << " "
	    << faren << " "
	    << celsius << std::endl;
    
  if (m_pSqlConnector->testSensorName(serno) != 0)
  {
    std::cout << dtime() << " (" << __FILE__ << ":" << __LINE__ << ")"
	      << "Sensor does not exist; creating: " 
	      << serno << std::endl;
    m_pSqlConnector->makeSensorName(serno);
  }

  std::string locationName = m_config.getParam("SENSORS", serno, "UNKNOWN");
  if (locationName == "UNKNOWN")
  {
    std::cout << dtime() << " (" << __FILE__ << ":" << __LINE__ << ")"
	      << "Location for sensor " << serno 
	      << " is not in config file; not entering data." << std::endl;
  }
  else if (m_pSqlConnector->testLocationName(locationName))
  {
    std::cout << dtime() << " (" << __FILE__ << ":" << __LINE__ << ")"
	      << "Location does not exist; creating: " 
	      << locationName << std::endl;
    m_pSqlConnector->makeLocationName(locationName);
  }

  timeval tv;
  gettimeofday(&tv, NULL);
  m_pSqlConnector->putVal(serno, locationName, 
			  tv.tv_sec, sample.getAnalogValue());
  return 0;
}

int MessageHandler::handleRemoteCommandResponse(std::shared_ptr<Message> pMessage)
{
  RemoteCommandResponse rcr = 
    *(dynamic_cast<RemoteCommandResponse*>(pMessage.get()));

/*
  std::ofstream of("/var/tmp/rcr.out", std::ios::app);
  of << dtime() << " " 
     << macAddrString(rcr.getAddress()) << " "
     << rcr.getCommand() << " "
     << rcr.getArg() << " "
     << (int)rcr.getFrameId() << " "
     << (int)rcr.getStatus() << std::endl;
*/

  return m_pConfigManager->handleMessage(pMessage);
}

int MessageHandler::handleATCommandResponse(std::shared_ptr<Message> pMessage)
{
  ATCommandResponse rcr = 
    *(dynamic_cast<ATCommandResponse*>(pMessage.get()));

/*
  std::ofstream of("/var/tmp/atcr.out", std::ios::app);
  of << dtime() << " " 
     << rcr.getCommand() << " "
     << rcr.getArg() << " "
     << (int)rcr.getFrameId() << " "
     << (int)rcr.getStatus() << std::endl;
*/

  return m_pConfigManager->handleMessage(pMessage);
}

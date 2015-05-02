#include "XBeeConfigManager.h"
#include "ATCommand.h"
#include "ATCommandResponse.h"
#include "RemoteATCommand.h"
#include "RemoteCommandResponse.h"
#include "IOSample.h"
#include "util.h"
#include "XBeeCommManager.h"

#include <fstream>
#include <sys/time.h>
#include <time.h>
#include <iomanip>
#include <iostream>


XBeeConfigParams::XBeeConfigParams()
{
  tvinit(m_lastCommunicateTime);
  tvinit(m_lastSendTime);
  m_queryOutstanding = 0;
  m_lastQueryFrameId = 0;
  m_lastAI0Sample = 0.0;
}

XBeeConfigManager::XBeeConfigManager(XBeeCommManager* pCommManager,
				     ConfigFile cf)
  : m_config(cf)
{
  m_pCommManager = pCommManager;

  // Initialize timing
  tvinit(m_lastStatusWrite);
  tvinit(m_lastSensorSearch);
  tvinit(m_lastPacketSend);
  gettimeofday(&m_initTime, NULL);
  m_initDone = 0;
}

// Give the config object a chance to do something, if it wants
void XBeeConfigManager::heartbeat()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);

  double initDelay = m_config.getParam("XBEE_CONFIG_MANAGER",
				       "INIT_DELAY",
				       1.0);
  if (m_initDone == 0 && 
      tvdiff(tv, m_initTime) > initDelay)
  {
    m_initDone = 1;
    doInit();
  }

  double statusFreq = m_config.getParam("XBEE_CONFIG_MANAGER",
					"STATUS_UPDATE_FREQUENCY",
					1.0);
  if (tvdiff(tv, m_lastStatusWrite) > statusFreq)
  {
    m_lastStatusWrite = tv;
    writeStatusFile();
  }

  // Check to see if there are any missing coordinator parameters
  getCoordinatorParameters();

  // Then, update coordinator parameters if necessary
  updateCoordinatorParameters();

  // Last, update remote sensor parameters if necessary
  updateSensorParameters();

  // Actually, maybe we should never do searches, and instead
  //   just talk when the sensors phone home naturally...
  int doSearch = m_config.getParam("XBEE_CONFIG_MANAGER",
				   "DO_XBEE_SEARCH",
				   0);
  double searchFreq = m_config.getParam("XBEE_CONFIG_MANAGER",
					"SENSOR_SEARCH_FREQUENCY",
					60.0);
  if (doSearch && tvdiff(tv, m_lastSensorSearch) > searchFreq)
  {
    m_lastSensorSearch = tv;
    doSensorSearch();
  }
}

// Returns 0 on success, nonzero on failure
int XBeeConfigManager::handleMessage(std::shared_ptr<Message> pMessage)
{
  // Do some general work for any type of message
  std::vector<unsigned char> addressData;
  int frameId = -1;
  if (pMessage->type() == std::string("RemoteCommandResponse"))
  {
    RemoteCommandResponse rcr = 
      *(dynamic_cast<RemoteCommandResponse*>(pMessage.get()));
    addressData = rcr.getAddress();
    frameId = rcr.getFrameId();
  }
  else if (pMessage->type() == std::string("IOSample"))
  {
    IOSample sample = *(dynamic_cast<IOSample*>(pMessage.get()));
    addressData = sample.getSenderAddressData();
    if (m_params.find(addressData) != m_params.end())
      m_params[addressData].m_lastAI0Sample = sample.getAnalogValue();
  }

  if (pMessage->type() == std::string("ATCommandResponse"))
  {
    ATCommandResponse atcr = 
      *(dynamic_cast<ATCommandResponse*>(pMessage.get()));
    frameId = atcr.getFrameId();
    struct timeval tvnow;
    gettimeofday(&tvnow, NULL);
    m_coordinatorParams.m_lastCommunicateTime = tvnow;
    if (frameId == m_coordinatorParams.m_lastQueryFrameId)
    {
      m_coordinatorParams.m_queryOutstanding = 0;
    }    
  }
  else
  {
    if (m_params[addressData].m_address == "")
      m_params[addressData].m_address = macAddrString(addressData);
    struct timeval tvnow;
    gettimeofday(&tvnow, NULL);
    m_params[addressData].m_lastCommunicateTime = tvnow;
    if (frameId == m_params[addressData].m_lastQueryFrameId)
    {
      m_params[addressData].m_queryOutstanding = 0;
    }
  }

  // Now process specific message types
  if (pMessage->type() == std::string("RemoteCommandResponse"))
  {
    RemoteCommandResponse rcr = 
      *(dynamic_cast<RemoteCommandResponse*>(pMessage.get()));

    std::string cmd = { rcr.getCommand()[0], rcr.getCommand()[1] };
    if (cmd == "SH" ||
	cmd == "SL" ||
	cmd == "DH" ||
	cmd == "DL")
    {
      std::string addr = macAddrString(rcr.getArgData());
      m_params[rcr.getAddress()].m_stringParams[cmd] = addr;

      std::cout << std::setprecision(3) << std::fixed << dtime()  
		<< " " << cmd << ":" << addr << " Reported from "
		<< macAddrString(addressData) << std::endl;
    }
    else
    {
      // Don't just add; if this is an ACK to a set request,
      // the data will be empty.  This act will cause us to insert
      // a 0 into our map for this parameter, which will force
      // us into an infinite loop of setting and never discovering
      // it worked.
      if (rcr.getArgData().size() > 0)
	m_params[rcr.getAddress()].m_intParams[cmd] = rcr.getArg();

      std::cout << std::setprecision(3) << std::fixed << dtime()  
		<< " " << cmd << ":" << rcr.getArg() << " Reported from "
		<< macAddrString(addressData) << std::endl;
    }
  }
  else if (pMessage->type() == std::string("IOSample"))
  {
    IOSample sample = *(dynamic_cast<IOSample*>(pMessage.get()));
    // Nothing more to do here for now...
  }
  else if (pMessage->type() == std::string("ATCommandResponse"))
  {
    ATCommandResponse atca =
      *(dynamic_cast<ATCommandResponse*>(pMessage.get()));

    // Don't just add; if this is an ACK to a set request,
    // the data will be empty.  This act will cause us to insert
    // a 0 into our map for this parameter, which will force
    // us into an infinite loop of setting and never discovering
    // it worked.
    std::string atcacmd;
    atcacmd.push_back(atca.getCommand()[0]);
    atcacmd.push_back(atca.getCommand()[1]);
    if (atca.getArgData().size() > 0)
      m_coordinatorParams.m_intParams[atcacmd] = atca.getArg();
    
    std::cout << std::setprecision(3) << std::fixed << dtime()  
	      << " " << atcacmd << ":" << atca.getArg() 
	      << " (" << (int) atca.getStatus() << ")"
	      << " Reported from coordinator" << std::endl;
  }

  // Now that we know this sensor is talking, get parameters from
  // it if any are needed
  if (addressData.size() > 0)
    getMissingParameters(addressData);
      
  return 0;
}

void XBeeConfigManager::writeStatusFile()
{
  std::string filename = m_config.getParam("XBEE_CONFIG_MANAGER",
					   "STATUS_FILENAME",
					   "/var/tmp/xbcfg.out");
  time_t now = time(NULL);
  std::string timestring = ctime(&now);
  struct timeval tvnow;
  gettimeofday(&tvnow, NULL);
  time_t starttime = m_initTime.tv_sec;
  std::string starttimestring = ctime(&starttime);
  int loc = 0; // LineOutputCounter

  std::ofstream os(filename.c_str());
  os << std::setprecision(3) << std::fixed << dtime()  
     << " CONFIGURATION " << timestring; // timestring includes newline
  os << "          " << "Process active time: " << tvdiff(tvnow, m_initTime)
     << ", started " << starttimestring; // timestring includes newline

  // Dump coordinator config parameters
  os << "Coordinator " 
     << m_coordinatorParams.m_queryOutstanding << " "
     << m_coordinatorParams.m_lastQueryFrameId << std::endl;
  loc = 0;
  for (auto cit = m_coordinatorParams.m_intParams.begin();
       cit != m_coordinatorParams.m_intParams.end(); cit++)
  {
    os << "  " << cit->first 
       <<  " " << std::setw(5) << std::setfill(' ') 
       << std::left << cit->second;
    if (++loc > 4)
    {
      os << std::endl;
      loc = 0;
    }
  }
  if (loc != 0) os << std::endl;
  loc = 0;
  os << "  Num Sensors: " << m_params.size() << std::endl;
  os << std::endl;

  // Dump sensor config parameters and status
  for (auto mit = m_params.begin(); mit != m_params.end(); mit++)
  {
    XBeeConfigParams* pParams = &(mit->second);
    std::string locationName = m_config.getParam("SENSORS", 
						 pParams->m_address, 
						 "UNKNOWN");
    double sensorCal = m_config.getParam("CALIBRATIONS",
					 pParams->m_address,
					 1.0);
					 
    double degC = (((double)pParams->m_lastAI0Sample 
		    * 1000.0 / 1023.0 * 1.2) 
		   - 500.0) * 0.1 * sensorCal;
    double degF = (degC * 9.0 / 5.0) + 32;
    os << pParams->m_address << " " 
       << std::setw(8) << std::setfill('0') << std::right
       << tvdiff(tvnow, pParams->m_lastCommunicateTime) << " "
       << pParams->m_queryOutstanding << " "
       << pParams->m_lastQueryFrameId << " "
       << pParams->m_lastAI0Sample << " "
       << "(" << std::setprecision(1) << std::setfill(' ') << std::right
       << std::setw(5) << degC << " C) (" 
       << std::setw(5) << degF << " F) "
       << locationName << std::endl;
    loc = 0;
    for (auto iit = pParams->m_intParams.begin();
	 iit != pParams->m_intParams.end(); iit++)
    {
      os << "  " << iit->first 
	 <<  " " << std::setw(5) << std::setfill(' ') 
	 << std::left << iit->second;
      if (++loc > 4)
      {
	os << std::endl;
	loc = 0;
      }
    }
    if (loc != 0) os << std::endl;
    loc = 0;
    for (auto sit = pParams->m_stringParams.begin();
	 sit != pParams->m_stringParams.end(); sit++)
    {
      os << "  " << sit->first 
	 <<  " " << std::setw(16) << std::setfill(' ') << sit->second;
      if (++loc > 2)
      {
	os << std::endl;
	loc = 0;
      }
    }
    if (loc != 0) os << std::endl;

    os << std::endl;
  }
  
}

void XBeeConfigManager::doSensorSearch()
{
  std::vector<unsigned char> SL = { 'S', 'L' };
  std::vector<unsigned char> bcast = { 0x00, 0x00, 0x00, 0x00,
				       0x00, 0x00, 0xFF, 0xFF };
  std::vector<unsigned char> arg;

  RemoteATCommand* prcmd = new RemoteATCommand(SL, bcast, 0, 20, arg);
  m_pCommManager->sendMessage(std::shared_ptr<Message>(prcmd));
}


void XBeeConfigManager::doInit()
{
/*
  ATCommand commandSN("SN", 1);
  m_pSendQueue->putMessage(commandSN.getPacket());
  commandSN = ATCommand("SP", 2);
  m_pSendQueue->putMessage(commandSN.getPacket());
  commandSN = ATCommand("SH", 3);
  m_pSendQueue->putMessage(commandSN.getPacket());
  commandSN = ATCommand("SL", 4);
  m_pSendQueue->putMessage(commandSN.getPacket());
  std::vector<unsigned char> sn = { 'S', 'N' };
  std::vector<unsigned char> bcast = { 0x00, 0x00, 0x00, 0x00,
				       0x00, 0x00, 0xFF, 0xFF };
  std::vector<unsigned char> arg;
  RemoteATCommand rcmd(sn, bcast, 0, 10, arg);
  m_pSendQueue->putMessage(rcmd.getPacket());
  std::vector<unsigned char> sp = { 'S', 'P' };
  rcmd = RemoteATCommand(sp, bcast, 0, 11, arg);
//  m_pSendQueue->putMessage(rcmd.getPacket());
*/  

}

// Check to see if there are any desired parameters
// for the argument module that we don't yet have.
// If there are, and if we haven't sent anything in a 
// reasonable period of time, ask for one of them
void XBeeConfigManager::getMissingParameters(const std::vector<unsigned char>& addressData)
{
  std::vector<std::string> intCommandList = 
    { "D0", "D1", "SM", "SP", "SN", "IR", "ST" };
  std::vector<std::string> stringCommandList =
    { "DH" , "DL" };

  for (auto it = intCommandList.begin(); it != intCommandList.end(); it++)
  {
    if (m_params[addressData].m_intParams.find(*it) ==
	m_params[addressData].m_intParams.end())
    {
      // We don't yet know this parameter for this device.
      // Check timing...
      struct timeval tv;
      gettimeofday(&tv, NULL);
      
      // Delay this long in between asking each parameter of the same
      // XBee
      double delay = m_config.getParam("XBEE_CONFIG_MANAGER",
				       "PARAM_QUERY_DELAY",
				       1.0);
      bool remoteDelayOK = 
	(tvdiff(tv, m_params[addressData].m_lastSendTime) > delay);
      // Delay this long in between sending any packets to the
      // XBee coordinator
      double packetDelay = m_config.getParam("XBEE_CONFIG_MANAGER",
					     "PACKET_SEND_DELAY",
					     1.0);
      bool coordDelayOK = 
	(tvdiff(tv, m_lastPacketSend) > packetDelay);
      // Allow this long for a previously sent request to be
      // replied to before sending it again
      double retrydelay = m_config.getParam("XBEE_CONFIG_MANAGER",
					    "PARAM_QUERY_RETRY_DELAY",
					    60.0);
      bool remoteBusyOK =
	(m_params[addressData].m_queryOutstanding == 0) ||
	(tvdiff(tv, m_params[addressData].m_lastSendTime) > retrydelay);
      if (remoteDelayOK && coordDelayOK && remoteBusyOK)
      {
	m_params[addressData].m_lastSendTime = tv;
	m_lastPacketSend = tv;
	std::vector<unsigned char> cmd;
	std::vector<unsigned char> arg;
	cmd.push_back((*it)[0]);
	cmd.push_back((*it)[1]);
	m_params[addressData].m_lastQueryFrameId++;
	if (m_params[addressData].m_lastQueryFrameId > 255)
	  m_params[addressData].m_lastQueryFrameId = 0;
	std::cout << std::setprecision(3) << std::fixed << dtime()  
		  << " Requesting " << *it << " from "
		  << macAddrString(addressData) << std::endl;
	m_params[addressData].m_queryOutstanding = 1;
	RemoteATCommand* prcmd = 
	  new RemoteATCommand(cmd, 
			      addressData, 
			      0, 
			      m_params[addressData].m_lastQueryFrameId, 
			      arg);
	m_pCommManager->sendMessage(std::shared_ptr<Message>(prcmd));
      }
    }
  }
}

// Check to see if there are any desired parameters
// for the coordinator that we don't yet have.
// If there are, and if we haven't sent anything in a 
// reasonable period of time, ask for one of them
void XBeeConfigManager::getCoordinatorParameters()
{
  std::vector<std::string> intCommandList = 
    { "SP", "SN", "ID" };

  for (auto it = intCommandList.begin(); it != intCommandList.end(); it++)
  {
    if (m_coordinatorParams.m_intParams.find(*it) ==
	m_coordinatorParams.m_intParams.end())
    {
      // We don't yet know this parameter for this device.
      // Check timing...
      struct timeval tv;
      gettimeofday(&tv, NULL);
      
      // Delay this long betweent the start of the process
      // and our first query
      double initDelay = m_config.getParam("XBEE_CONFIG_MANAGER",
					   "COORDINATOR_INIT_DELAY",
					   0.5);
      bool initDelayOK =
	(tvdiff(tv, m_initTime) > initDelay);

      // Delay this long in between sending any packets to the
      // XBee coordinator
      double packetDelay = m_config.getParam("XBEE_CONFIG_MANAGER",
					     "PACKET_SEND_DELAY",
					     1.0);
      bool coordDelayOK = 
	(tvdiff(tv, m_lastPacketSend) > packetDelay);
      // Allow this long for a previously sent request to be
      // replied to before sending it again
      double retrydelay = m_config.getParam("XBEE_CONFIG_MANAGER",
					    "PARAM_QUERY_RETRY_DELAY",
					    60.0);
      bool busyOK =
	(m_coordinatorParams.m_queryOutstanding == 0) ||
	(tvdiff(tv, m_coordinatorParams.m_lastSendTime) > retrydelay);
      if (initDelayOK && coordDelayOK && busyOK)
      {
	m_coordinatorParams.m_lastSendTime = tv;
	m_lastPacketSend = tv;
	m_coordinatorParams.m_lastQueryFrameId++;
	if (m_coordinatorParams.m_lastQueryFrameId > 255)
	  m_coordinatorParams.m_lastQueryFrameId = 0;
	std::cout << std::setprecision(3) << std::fixed << dtime()  
		  << " Requesting " << *it 
		  << " from coordinator" << std::endl;
	m_coordinatorParams.m_queryOutstanding = 1;
	ATCommand* pacmd = 
	  new ATCommand(it->c_str(), m_coordinatorParams.m_lastQueryFrameId);
	m_pCommManager->sendMessage(std::shared_ptr<Message>(pacmd));
      }
    }
  }
}

void XBeeConfigManager::updateCoordinatorParameters()
{
  // Check for an override that tells us to leave things alone
  if (m_config.getParam("XBEE_CONFIG_MANAGER",
			"SET_CONTROLLER_PARAMS",
			1) != 1)
    return;

  std::map<std::string, int> desiredSettings;
  desiredSettings["SP"] = m_config.getParam("XBEE_CONFIG_MANAGER",
					    "COORDINATOR_SP",
					    2500);
  desiredSettings["SN"] = m_config.getParam("XBEE_CONFIG_MANAGER",
					    "COORDINATOR_SN",
					    36);
  struct timeval tv;
  gettimeofday(&tv, NULL);
  std::vector<std::string> toDelete;

  for (auto it = desiredSettings.begin(); 
       it != desiredSettings.end(); it++)
  {
    if (m_coordinatorParams.m_intParams.find(it->first) != 
	m_coordinatorParams.m_intParams.end())
    {
      if (m_coordinatorParams.m_intParams[it->first] !=
	  it->second)
      {
	// Delay this long in between sending any packets to the
	// XBee coordinator
	double packetDelay = m_config.getParam("XBEE_CONFIG_MANAGER",
					       "PACKET_SEND_DELAY",
					       1.0);
	bool coordDelayOK = 
	  (tvdiff(tv, m_lastPacketSend) > packetDelay);
	// Allow this long for a previously sent request to be
	// replied to before sending it again
	double retrydelay = m_config.getParam("XBEE_CONFIG_MANAGER",
					      "PARAM_QUERY_RETRY_DELAY",
					      60.0);
	bool busyOK =
	  (m_coordinatorParams.m_queryOutstanding == 0) ||
	  (tvdiff(tv, m_coordinatorParams.m_lastSendTime) > retrydelay);
	
	if (coordDelayOK && busyOK)
	{
	  m_coordinatorParams.m_lastSendTime = tv;
	  m_lastPacketSend = tv;
	  m_coordinatorParams.m_lastQueryFrameId++;
	  if (m_coordinatorParams.m_lastQueryFrameId > 255)
	    m_coordinatorParams.m_lastQueryFrameId = 0;
	  std::vector<unsigned char> arg;
	  arg.push_back((it->second & 0xFF00) >> 8);
	  arg.push_back(it->second & 0xFF);
	  std::cout << std::setprecision(3) << std::fixed << dtime()  
		    << " Requesting " << it->first << ":" << it->second
		    << " from coordinator" << std::endl;
	  m_coordinatorParams.m_queryOutstanding = 1;
	  ATCommand* pacmd = 
	    new ATCommand(it->first.c_str(), 
			  m_coordinatorParams.m_lastQueryFrameId,
			  arg);
	  m_pCommManager->sendMessage(std::shared_ptr<Message>(pacmd));
	  toDelete.push_back(it->first);
	}
      }
    }
  }
  // Now we have to purge our knowledge of the old setting.
  // The ATCommandResponse to this request will not have any
  // data saying what the new value is.  We need to forget that
  // we know it, so we ask again.
  for (auto it = toDelete.begin(); it != toDelete.end(); it++)
    m_coordinatorParams.m_intParams.erase(*it);
}
	  
void XBeeConfigManager::updateSensorParameters()
{
  // Check for an override that tells us to leave things alone
  if (m_config.getParam("XBEE_CONFIG_MANAGER",
			"SET_SENSOR_PARAMS",
			1) != 1)
    return;

  std::map<std::string, int> desiredSettings;
  desiredSettings["D0"] = m_config.getParam("XBEE_CONFIG_MANAGER",
					    "SENSOR_D0",
					    2);
  desiredSettings["D1"] = m_config.getParam("XBEE_CONFIG_MANAGER",
					    "SENSOR_D1",
					    0);
  desiredSettings["IR"] = m_config.getParam("XBEE_CONFIG_MANAGER",
					    "SENSOR_IR",
					    250);
  desiredSettings["SM"] = m_config.getParam("XBEE_CONFIG_MANAGER",
					    "SENSOR_SM",
					    4);
  desiredSettings["SP"] = m_config.getParam("XBEE_CONFIG_MANAGER",
					    "SENSOR_SP",
					    2500);
  desiredSettings["SN"] = m_config.getParam("XBEE_CONFIG_MANAGER",
					    "SENSOR_SN",
					    6);
  desiredSettings["ST"] = m_config.getParam("XBEE_CONFIG_MANAGER",
					    "SENSOR_ST",
					    5000);
  struct timeval tv;
  gettimeofday(&tv, NULL);
  std::vector<std::string> toDelete;

  for (auto& setting : desiredSettings) 
  {
    for (auto& sensorPair : m_params)
    {
      auto& remoteAddr = sensorPair.first;
      auto& remoteParams = sensorPair.second;
      if (remoteParams.m_intParams.find(setting.first) != 
	  remoteParams.m_intParams.end() &&
	  remoteParams.m_intParams[setting.first] != setting.second)
      {
	// Delay this long in between sending any packets to the
	// XBee coordinator
	double packetDelay = m_config.getParam("XBEE_CONFIG_MANAGER",
					       "PACKET_SEND_DELAY",
					       1.0);
	bool coordDelayOK = 
	  (tvdiff(tv, m_lastPacketSend) > packetDelay);
	// Allow this long for a previously sent request to be
	// replied to before sending it again
	double retrydelay = m_config.getParam("XBEE_CONFIG_MANAGER",
					      "PARAM_QUERY_RETRY_DELAY",
					      60.0);
	bool busyOK =
	  (remoteParams.m_queryOutstanding == 0) ||
	  (tvdiff(tv, remoteParams.m_lastSendTime) > retrydelay);

	if (coordDelayOK && busyOK)
	{
	  remoteParams.m_lastSendTime = tv;
	  m_lastPacketSend = tv;
	  remoteParams.m_lastQueryFrameId++;
	  if (remoteParams.m_lastQueryFrameId > 255)
	    remoteParams.m_lastQueryFrameId = 0;
	  std::vector<unsigned char> arg;
	  arg.push_back((setting.second & 0xFF00) >> 8);
	  arg.push_back(setting.second & 0xFF);
	  std::vector<unsigned char> cmd;
	  cmd.push_back(setting.first.at(0));
	  cmd.push_back(setting.first.at(1));
	  std::cout << std::setprecision(3) << std::fixed << dtime()  
		    << " Requesting " << setting.first 
		    << ":" << setting.second
		    << " from sensor " << macAddrString(remoteAddr)
		    << std::endl;
	  remoteParams.m_queryOutstanding = 1;
	  RemoteATCommand* pacmd = 
	    new RemoteATCommand(cmd,
				remoteAddr,
				0x02,       // Apply changes
				remoteParams.m_lastQueryFrameId,
				arg);
	  m_pCommManager->sendMessage(std::shared_ptr<Message>(pacmd));
	  toDelete.push_back(setting.first);
	}
      }
      // Now we have to purge our knowledge of the old setting.
      // The RemoteCommandResponse to this request will not have any
      // data saying what the new value is.  We need to forget that
      // we know it, so we ask again.
      for (auto& del : toDelete)
	remoteParams.m_intParams.erase(del);

    }
  }

}

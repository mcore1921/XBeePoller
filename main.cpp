
#include "util.h"
#include "Config.h"
#include "IOSample.h"
#include "PortService.h"
#include "ATCommand.h"
#include "RemoteATCommand.h"
#include "MessageFactory.h"
#include "XBeeCommManager.h"
#include "RemoteXBeeManager.h"
#include "CoordinatorXBeeManager.h"
#include "DataManager.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fstream>
#include <sys/time.h>
#include <boost/program_options.hpp>
#include <memory>
#include <unistd.h>
#include <fcntl.h>

#define HEX( x ) std::setw(2) << std::setfill('0') << std::hex << (int)( x )

namespace po = boost::program_options;

void dumpConfigurationMap(CoordinatorXBeeManager* coordinator,
                          const std::vector<RemoteXBeeManager*>& managers,
			  DataManager* pDataManager,
			  ConfigFile config );
// Call this in its own thread to start the managers slowly over time
void activateManagers(CoordinatorXBeeManager* pCoordinator,
		      std::vector<RemoteXBeeManager*>& mgrs);

int main(int argc, char** argv)
{
  ConfigFile configFile;
  configFile.open("/etc/XBeeThermClient/XBeeThermPollerConfig");
  std::string defaultport = configFile.getParam("PROCESS_CONFIG",
					  "PORTNAME",
					  "/dev/ttyUSB0");

  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("port", 
     po::value<std::string>()->default_value(defaultport), 
     "XBee Port");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help"))
  {
    std::cout << desc << std::endl;
    return 0;
  }

  {
    struct stat as;
    if (stat(vm["port"].as<std::string>().c_str(),
	     &as) == -1)
    {
      std::cout << "Stat on '" << vm["port"].as<std::string>()
		<< "' returned -1, errno " << errno 
		<< " (" << strerror(errno) << ")" << std::endl;
      return -1;
    }
    else if (!S_ISCHR(as.st_mode))
    {
      std::cout << "Error, '" << vm["port"].as<std::string>()
		<< "' is not a character device" << std::endl;
      return -1;
    }
  }

  // Make all the objects we'll need
  DataManager dm(configFile);
  dm.activate();

  std::string portName = vm["port"].as<std::string>().c_str();
  XBeeCommManager cmgr(portName, &configFile);
  cmgr.activate();

  CoordinatorXBeeManager coordinatorManager(&cmgr, &dm, configFile);
  cmgr.registerCatchallHandler(coordinatorManager.getQueue());

  std::vector<RemoteXBeeManager*> remoteManagers;
  int numsensors = configFile.getParam("SENSORS",
				       "NUM_SENSORS", 
				       0);
  for (int i = 0; i < numsensors; i++)
  {
    std::stringstream ss;
    ss << "SENSOR_" << i;
    std::vector<unsigned char> taddr = 
      macAddrData(configFile.getParam("SENSORS",
				      ss.str(),
				      ""));
    RemoteXBeeManager* pMgr = new RemoteXBeeManager(&cmgr, taddr, 
						    &dm, configFile);
    cmgr.registerHandler(pMgr->getAddress(),
			 pMgr->getQueue());
    remoteManagers.push_back(pMgr);
 
  }

  // Start up our objects (use a helper thread for this)
  std::thread starterThread(&activateManagers, 
			    &coordinatorManager,
			    remoteManagers);
  starterThread.detach();

  // Start our (infinite) config file dumping loop
  while (1)
  {
    dumpConfigurationMap(&coordinatorManager, remoteManagers, 
			 &dm, configFile);
    sleep(1);
    
    // Check that the port is still available
    if (fcntl(cmgr.getSerialFD(), F_GETFL) == -1 && errno != EBADF)
    {
      std::cout << __FILE__ << ":" << __LINE__ 
		<< ": Port closed unexpectedly (" << cmgr.getSerialFD()
		<< ")" << std::endl;
      std::cout << "Calling exit." << std::endl;
      exit(-1);
    }

    struct stat as;
    if (stat(vm["port"].as<std::string>().c_str(),
	     &as) == -1)
    {
      std::cout << "Stat on '" << vm["port"].as<std::string>()
		<< "' returned -1, errno " << errno 
		<< " (" << strerror(errno) << ")" << std::endl;
      std::cout << "Calling exit." << std::endl;
      exit(-1);
    }
    else if (!S_ISCHR(as.st_mode))
    {
      std::cout << "Error, '" << vm["port"].as<std::string>()
		<< "' is not a character device" << std::endl;
      std::cout << "Calling exit." << std::endl;
      exit(-1);
    }
    
  }

  return 0;
}  

void activateManagers(CoordinatorXBeeManager* pCoordinator,
		      std::vector<RemoteXBeeManager*>& mgrs)
{

  // First activate the coordinator
  {
    pCoordinator->activate();
    
    timespec mgrDelay = tsMake(300, 0);
    timespec delayInterval = tsMake(0, 5e8);
    
    timespec now, mgrTimeout;
    clock_gettime(CLOCK_REALTIME, &now);
    mgrTimeout = tsAdd(now, mgrDelay);
    
    do 
    {
      nanosleep(&delayInterval, NULL);
      clock_gettime(CLOCK_REALTIME, &now);
    }
    while (pCoordinator->getInitStatus() != 
	   CoordinatorXBeeManager::INITIALIZED &&
	   tsDiff(mgrTimeout, now) > 0);
  }

  // Then activate the remotes
  {
    // start delay - give the coordinator a sec (well, half a sec)
    // after its config is finished
    timespec initialDelay = tsMake(0, 5e8);
    timespec mgrDelay = tsMake(60, 0);
    timespec delayInterval = tsMake(0, 5e8);
    
    timespec now, mgrTimeout;
    clock_gettime(CLOCK_REALTIME, &now);
    mgrTimeout = tsAdd(now, mgrDelay);
    
    nanosleep(&initialDelay, NULL);
    for (auto pMgr : mgrs)
    {
      pMgr->activate();
      do 
      {
	nanosleep(&delayInterval, NULL);
	clock_gettime(CLOCK_REALTIME, &now);
      }
      while (pMgr->getInitStatus() != RemoteXBeeManager::INITIALIZED &&
	     tsDiff(mgrTimeout, now) > 0);
      mgrTimeout = tsAdd(now, mgrDelay);
    }
  }

  return;
}

void dumpConfigurationMap(CoordinatorXBeeManager* pCoordinator,
			  const std::vector<RemoteXBeeManager*>& managers,
			  DataManager* pDataManager,
			  ConfigFile config )
{
  static timeval firstCallTime;
  static int firstcall = 0;

  if (firstcall == 0)
  {
    gettimeofday(&firstCallTime, NULL);
    firstcall = 1;
  }

  std::string filename = config.getParam("XBEE_CONFIG_MANAGER",
					 "STATUS_FILENAME",
					 "/var/tmp/xbcfg.out");
  time_t now = time(NULL);
  std::string timestring = ctime(&now);
  struct timeval tvnow;
  gettimeofday(&tvnow, NULL);
  time_t starttime = firstCallTime.tv_sec;
  std::string starttimestring = ctime(&starttime);


  std::ofstream os(filename.c_str());
  os << std::setw(9) << std::setfill('0') 
     << std::setprecision(3) << std::fixed << dtime()  
     << " " << timestring; // timestring includes newline
  os << "Process active time: " << tvdiff(tvnow, firstCallTime)
     << ", started " << starttimestring; // timestring includes newline
  os << std::endl;

  os << pCoordinator->paramsDescriptionString();

  for (auto& pManager : managers)
  {
    os << pManager->paramsDescriptionString();
  }

  os << pDataManager->dataDescriptionString();
}



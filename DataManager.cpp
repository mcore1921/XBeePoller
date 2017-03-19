#include "DataManager.h"

#include "SQLConnector.h"
#include "util.h"

#include <iostream>
#include <iomanip>
#include <sys/time.h>
#include <chrono>
#include <sstream>
#include <unistd.h>

#define DMLOG std::setprecision(3) << std::fixed << dtime() << " DM:" 

DataManager::DataManager(ConfigFile cf)
  : m_config(cf)
{
  m_pSqlConnector = new SQLConnector(cf);
}

DataManager::~DataManager()
{
  delete m_pSqlConnector;
}

void DataManager::activate()
{
  m_thread = std::thread(&DataManager::threadFunc, this);
}

void DataManager::join()
{
  m_thread.join();
}

int DataManager::handleIOSample(std::vector<unsigned char> address,
				int value,
				std::string name,
				double degF)
{
  std::lock_guard<std::recursive_mutex> g(m_SQLMutex);
  {
    // Open a stack layer for the guard
    std::lock_guard<std::recursive_mutex> g(m_thermMapMutex);
    if (m_thermMap.find(name) == m_thermMap.end())
    {
      m_thermMap[name] = std::make_pair(1, degF);
    }
    else
    {
      auto i = m_thermMap[name];
      double newVal = 
	(((double)i.first * i.second) + (double) degF) / (double) (i.first+1);
      m_thermMap[name] = std::make_pair(i.first+1, newVal);
    }
  }

  return 0;
}

std::string DataManager::dataDescriptionString()
{
  std::lock_guard<std::recursive_mutex> g(m_thermMapMutex);
  int n_minutes = m_config.getParam("DATA_MANAGER",
				    "DATA_COLLATION_RATE_MINUTES",
				    5);

  std::stringstream ss;
  ss << "Data Manager      Sec until wake: " << secUntilNextWake(n_minutes)
     << std::endl;
  unsigned int numEntriesToShow = 5;
  if (m_thermEntries.size() < numEntriesToShow)
  {
    for (auto& s : m_thermEntries)
    {
      ss << s << std::endl;
    }
  }
  else
  {
    for (auto sIt = m_thermEntries.end() - numEntriesToShow;
	 sIt != m_thermEntries.end(); sIt++)
    {
      ss << *sIt << std::endl;
    }
  }
  return ss.str();
}


int DataManager::secUntilNextWake(int intervalInMinutes)
{
  int N = intervalInMinutes;
  time_t now = time(NULL);
  tm tmNow;
  localtime_r(&now, &tmNow);
  
  // Figure out how long we need to wait so that we're sleeping until a
  // number of clock minutes % N on the current hour = 0.  In other 
  // words, if 60 % N = 0, we want to be sure that we are running our
  // cleanup at the top of every hour.  Figure out how to do that.
  int nextMinute = (N * (tmNow.tm_min / N)) + N ;
  tm waitUntil = tmNow;
  waitUntil.tm_sec = 0;
  waitUntil.tm_min = nextMinute;
  time_t secWaitUntil = mktime(&waitUntil);
  int secToSleep = secWaitUntil - now;

  char buf[26];
  asctime_r(&waitUntil, buf);
  std::string waitUntilStr(buf);
  while (waitUntilStr.find('\n') != std::string::npos)
    waitUntilStr.erase(waitUntilStr.find('\n'), 1);

  return secToSleep;
}

void DataManager::threadFunc()
{
  // Every N minutes, wake and transfer all of the values in
  // m_thermMap to the database.  Then clear m_thermMap and start over.
  int n_minutes = m_config.getParam("DATA_MANAGER",
				    "DATA_COLLATION_RATE_MINUTES",
				    5);

  while (1)
  {
    int secToSleep = secUntilNextWake(n_minutes);
    std::cout << DMLOG << " Init - Sleeping " << secToSleep << "seconds."
	      << std::endl;
    sleep (secToSleep);

    time_t now = time(NULL);
    char buf[26];
    ctime_r(&now, buf);
    std::string nowStr(buf);
    while (nowStr.find('\n') != std::string::npos)
      nowStr.erase(nowStr.find('\n'), 1);
    
    // Open a stack layer for the guard
    {
      std::lock_guard<std::recursive_mutex> g(m_thermMapMutex);
      // Hokey here - make it a string for now, but later this will be
      // insertions into SQL table
      std::stringstream ss;
      ss << nowStr << " ";
      
      std::vector<std::pair<std::string, double>> vals;
      for (auto& i : m_thermMap)
      {
	ss << i.first << " " << i.second.second << "F ";
	vals.push_back(std::make_pair(i.first, i.second.second));
      }
      m_thermEntries.push_back(ss.str());

      m_pSqlConnector->putVals(now, vals);
      // We've processed the data for this time chunk; now clear it
      m_thermMap.clear();
    }
  }
 
  return;
}

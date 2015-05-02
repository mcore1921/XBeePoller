#ifndef __DATA_MANAGER_H_ALREADY_INCLUDED__
#define __DATA_MANAGER_H_ALREADY_INCLUDED__

#include "ThreadQueue.h"
#include "Message.h"
#include "Config.h"

#include <memory>
#include <mutex>
#include <thread>

class SQLConnector;
class SQLConnectorB;

class DataManager
{
public:
  typedef ThreadQueue<std::shared_ptr<Message>> HandlerQueue;

  DataManager(ConfigFile cf);
  virtual ~DataManager();

  int handleIOSample(std::vector<unsigned char> address,
		     int value,
		     std::string name,
		     double degF);
  
  std::string dataDescriptionString();

  void activate();
  void join();

private:
  virtual void threadFunc();
  
  int secUntilNextWake(int intervalInMinutes);

  SQLConnector* m_pSqlConnector;
  SQLConnectorB* m_pSqlConnectorB;
  std::recursive_mutex m_SQLMutex;
  ConfigFile m_config;

  // Table column header mapped to count / value
  //   (count so that we can effectively average as new samples come in)
  std::map<std::string, std::pair<int, double>> m_thermMap;
  std::recursive_mutex m_thermMapMutex;
  std::thread m_thread;

  // For debugging
  std::vector<std::string> m_thermEntries;
};


#endif

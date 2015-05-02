#ifndef _SQL_CONNECTOR_H_INCLUDED_
#define _SQL_CONNECTOR_H_INCLUDED_

#include "Config.h"

#include <string>

namespace sql
{
class Driver;
class Connection;
}

class SQLConnector
{
public:
  SQLConnector(ConfigFile cf);
  virtual ~SQLConnector();

  // All functions return nonzero on failure, zero on success
  int testSensorName(std::string sensorName);
  int makeSensorName(std::string sensorName);
  int testLocationName(std::string locationName);
  int makeLocationName(std::string locationName);
//  int checkConnection();
  
//  int putTemperature(float temperature, std::string sensor, std::string location);
//  int putVal(float temperature, 
//  float calval, 
//	     std::string sensor);
  int putVal(std::string sensor,
	     std::string location,
	     long int time, 
	     int value);

private:
  sql::Driver* m_pDriver;
  sql::Connection* m_pConnection;
  ConfigFile m_config;
};


#endif

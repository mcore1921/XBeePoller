#include "SQLConnector.h"
#include "util.h"

#include "cppconn/driver.h" 
#include "cppconn/resultset.h" 
#include "cppconn/statement.h" 
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <time.h>

SQLConnector::SQLConnector(ConfigFile cf)
{
  m_config = cf;
  m_pDriver = get_driver_instance();
  m_pConnection = m_pDriver->connect("localhost", "mcore", "matt");
  m_pConnection->setSchema("xbtherm");
}

SQLConnector::~SQLConnector()
{
  delete m_pConnection;
}

int SQLConnector::testSensorName(std::string sensorName)
{
  {
    std::stringstream ss;
    ss << "SELECT * from sensors";
    std::unique_ptr<sql::Statement> pStatement(m_pConnection->createStatement());
    std::unique_ptr<sql::ResultSet> pResults(pStatement->executeQuery(ss.str()));
    
    pResults->beforeFirst();
    while (pResults->next())
    {
      if (pResults->getString("address") == sensorName)
	return 0;
    }
  }
  return -1;
}

int SQLConnector::makeSensorName(std::string sensorName)
{
  std::stringstream ss;
  ss << "INSERT INTO sensors (address) VALUES (\"" << sensorName << "\");";
  std::unique_ptr<sql::Statement> pStatement(m_pConnection->createStatement());
  bool result = pStatement->execute(ss.str());
  if (!result)
    return -1;
  
  return 0;
}

int SQLConnector::testLocationName(std::string locationName)
{
  {
    std::stringstream ss;
    ss << "SELECT * from locations";
    std::unique_ptr<sql::Statement> pStatement(m_pConnection->createStatement());
    std::unique_ptr<sql::ResultSet> pResults(pStatement->executeQuery(ss.str()));

    pResults->beforeFirst();
    while (pResults->next())
    {
      if (pResults->getString("name") == locationName)
	return 0;
    }
  }
  return -1;
}
  
int SQLConnector::makeLocationName(std::string locationName)
{
  std::stringstream ss;
  ss << "INSERT INTO locations (name) VALUES (\"" << locationName << "\");";
  std::unique_ptr<sql::Statement> pStatement(m_pConnection->createStatement());
  bool result = pStatement->execute(ss.str());
  if (!result)
    return -1;
  
  return 0;
}

/*
int SQLConnector::putTemperature(float temperature, std::string sensor, std::string location)
{
  std::stringstream ss;
  ss << "INSERT INTO therm (time, temperature, loc_id, sensor_id) "
     << "VALUES ( NOW(), " << temperature << ", "
     << "( SELECT loc_id FROM loc WHERE location LIKE \'" << location << "\'), "
     << "( SELECT sensor_id FROM sensor WHERE sensor LIKE \'" << sensor << "\'));";

  std::cout << dtime() << " (" << __FILE__ << ":" << __LINE__ << ")"
	    << "Adding temperature: " << temperature 
	    << ", sensor: " << sensor
	    << ", location: " << location << std::endl;

  mysqlpp::Query q = m_pConnection->query(ss.str());
  bool result = q.exec();
  
  if (!result)
    return -1;

  return 0;
}

*/

int SQLConnector::putVal(std::string sensor,
			 std::string location,
			 long int time, 
			 int value)
{
  struct tm* t = localtime(&time);
  char buf[50];
  snprintf(buf, 50, "%04d-%02d-%02d %02d:%02d:%02d",
	   t->tm_year+1900, t->tm_mon+1, t->tm_mday,
	   t->tm_hour, t->tm_min, t->tm_sec);

  std::stringstream ss;
  ss << "INSERT INTO readings (sensor, location, time, value) "
     << "VALUES ( " 
     << " (SELECT id FROM sensors WHERE address = '" << sensor << "'),"
     << " (SELECT id FROM locations WHERE name = '" << location << "'),"
     << "'" << buf << "',"
     << value << ")";

  if (m_config.getParam("SQL_CONNECTOR", "DEBUG_OUTPUT", 0) != 0)
  {
    std::cout << dtime() << " (" << __FILE__ << ":" << __LINE__ << ")"
	      << "Adding: " << ss.str() << std::endl;
  }

  std::unique_ptr<sql::Statement> pStatement(m_pConnection->createStatement());
  bool result = pStatement->execute(ss.str());
  if (!result)
    return -1;

  return 0;
}

/*
int SQLConnector::checkConnection()
{
  if (m_pConnection->ping())
  {
    return 0;
  }
  else
  {
    return -1;
  }
}
*/

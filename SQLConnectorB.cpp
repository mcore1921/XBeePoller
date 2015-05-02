#include "SQLConnectorB.h"
#include "util.h"

#include "cppconn/driver.h" 
#include "cppconn/resultset.h" 
#include "cppconn/statement.h" 
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdio.h>
#include <time.h>

#define SQLLOG std::setprecision(3) << std::fixed << dtime() << " SQLB:"

SQLConnectorB::SQLConnectorB(ConfigFile cf)
{
  m_config = cf;
  m_pDriver = get_driver_instance();

  std::string hostName = m_config.getParam("SQL_PARAMETERS",
					   "HOSTNAME",
					   "localhost");
  std::string userName = m_config.getParam("SQL_PARAMETERS",
					   "USERNAME",
					   "xbtherm");
  std::string userPass = m_config.getParam("SQL_PARAMETERS",
					   "PASSWORD",
					   "therm");
  std::string database = m_config.getParam("SQL_PARAMETERS",
					   "DATABASE",
					   "therm");

  m_pConnection = m_pDriver->connect(hostName, userName, userPass);
  m_pConnection->setSchema(database);
}

SQLConnectorB::~SQLConnectorB()
{
  delete m_pConnection;
}

/*
int SQLConnectorB::testSensorName(std::string sensorName)
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

int SQLConnectorB::makeSensorName(std::string sensorName)
{
  std::stringstream ss;
  ss << "INSERT INTO sensors (address) VALUES (\"" << sensorName << "\");";
  std::unique_ptr<sql::Statement> pStatement(m_pConnection->createStatement());
  bool result = pStatement->execute(ss.str());
  if (!result)
    return -1;
  
  return 0;
}

int SQLConnectorB::testLocationName(std::string locationName)
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
  
int SQLConnectorB::makeLocationName(std::string locationName)
{
  std::stringstream ss;
  ss << "INSERT INTO locations (name) VALUES (\"" << locationName << "\");";
  std::unique_ptr<sql::Statement> pStatement(m_pConnection->createStatement());
  bool result = pStatement->execute(ss.str());
  if (!result)
    return -1;
  
  return 0;
}
*/
/*
int SQLConnectorB::putTemperature(float temperature, std::string sensor, std::string location)
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
 /*
int SQLConnectorB::putVal(std::string sensor,
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
 */

int SQLConnectorB::putVals(long int time,
	      const std::vector<std::pair<std::string, double>>& vals)
{
  // If there are no values, bomb immediately
  if (vals.size() == 0)
  {
    std::cout << SQLLOG << " putVals given no data, bailing out" << std::endl;
    return -1;
  }

  // First check whether we have a table
  std::string tableName = m_config.getParam("SQL_PARAMETERS",
					    "TABLE_NAME",
					    "temperatures");
  std::vector<std::string> colNames;
  for (auto& i : vals)
    colNames.push_back(i.first);

  if (!doesTableExist(tableName))
  {
    if (createTable(tableName, colNames) != 0)
    {
      std::cout << SQLLOG << " createTable failed, bailing out" << std::endl;
      return -1;
    }
  }

  if (!doTableColumnsWork(tableName, colNames))
  {
    if (makeTableColumnsWork(tableName, colNames) != 0)
    {
      std::cout << SQLLOG << " makeTableColumnsWork failed, bailing out" 
		<< std::endl;
      return -1;
    }
  }

  if (addRow(tableName, time, vals) != 0)
  {
    std::cout << SQLLOG << " addRow failed." << std::endl;
    return -1;
  }

  return 0;
}
			   
bool SQLConnectorB::doesTableExist(std::string tableName)
{
  std::stringstream ss;
  ss << "show tables;";
  std::shared_ptr<sql::Statement> pStatement(m_pConnection->createStatement());
  std::shared_ptr<sql::ResultSet> pResults(pStatement->executeQuery(ss.str()));
    
  pResults->beforeFirst();
  while (pResults->next())
  {
    if (pResults->getString(1) == tableName)
      return true;
  }
  return false;
}

int SQLConnectorB::createTable(std::string tableName,
			      const std::vector<std::string>& colNames)
{
  if (colNames.size() == 0)
  {
    std::cout << SQLLOG << " createTable provided empty colNames" 
	      << std::endl;
    return -1;
  }

  std::vector<std::string> colNamesLocal(colNames);
  std::stringstream ss;
  ss << "create table " << tableName << "\n"
     << "  (id INT AUTO_INCREMENT PRIMARY KEY,\n"
     << "   time              TIMESTAMP,\n";
  std::string last = colNamesLocal.back();
  colNamesLocal.pop_back();
  for (auto& colName : colNamesLocal)
  {
    ss << "   " << colName << "   FLOAT,\n";
  }
  ss << "   " << last << "   FLOAT );";
  std::shared_ptr<sql::Statement> pStatement(m_pConnection->createStatement());
  bool result = pStatement->execute(ss.str());

  if (!result)
  {
    std::cout << SQLLOG << " created table with:" << std::endl;
    std::cout << ss.str() << std::endl;
  }
  else
  {
    std::cout << SQLLOG << " error creating table with:" << std::endl;
    std::cout << ss.str() << std::endl;
    return -1;
  }
   
  ss.str(std::string());
  ss << "CREATE INDEX " << tableName << "_time_index ON " 
     << tableName << " (time);";
  result = pStatement->execute(ss.str());
  if (!result)
  {
    std::cout << SQLLOG << " created index with:" << std::endl;
    std::cout << ss.str() << std::endl;
  }
  else
  {
    std::cout << SQLLOG << " error creating index with:" << std::endl;
    std::cout << ss.str() << std::endl;
    return -1;
  }

  return 0;
}

bool SQLConnectorB::doTableColumnsWork(std::string tableName, 
				   const std::vector<std::string>& colNames)
{
  std::stringstream ss;
  ss << "describe " << tableName << ";";
  std::shared_ptr<sql::Statement> pStatement(m_pConnection->createStatement());
  std::shared_ptr<sql::ResultSet> pResults(pStatement->executeQuery(ss.str()));

  std::map<std::string, int> colsFound;
  for (auto& colName : colNames)
    colsFound[colName] = 0;
  
  pResults->beforeFirst();
  while (pResults->next())
  {
    for (auto& colName : colNames)
    {
      if (pResults->getString("Field") == colName)
	colsFound[colName] = 1;
    }
  }
  
  for (auto& colPair : colsFound)
  {
    if (colPair.second == 0)
      return false;
  }

  return true;
}

int SQLConnectorB::makeTableColumnsWork(std::string tableName, 
				   const std::vector<std::string>& colNames)
{
  std::stringstream ss;
  ss << "describe " << tableName << ";";
  std::shared_ptr<sql::Statement> pStatement(m_pConnection->createStatement());
  std::shared_ptr<sql::ResultSet> pResults(pStatement->executeQuery(ss.str()));

  std::map<std::string, int> colsFound;
  for (auto& colName : colNames)
    colsFound[colName] = 0;
  
  pResults->beforeFirst();
  while (pResults->next())
  {
    for (auto& colName : colNames)
    {
      if (pResults->getString("Field") == colName)
	colsFound[colName] = 1;
    }
  }
  
  for (auto& colPair : colsFound)
  {
    if (colPair.second == 0)
    {
      // Add the column
      ss.str(std::string());
      ss << "ALTER TABLE " << tableName << " ADD " 
	 << colPair.first << " FLOAT;";
      bool result = pStatement->execute(ss.str());
      if (!result)
      {
	std::cout << SQLLOG << " added column with:" << std::endl;
	std::cout << ss.str() << std::endl;
      }
      else
      {
	std::cout << SQLLOG << " error adding column with:" << std::endl;
	std::cout << ss.str() << std::endl;
	return -1;
      }
    }
  }

  return 0;
}

int SQLConnectorB::addRow(std::string tableName,
			  long int time,
		 const std::vector<std::pair<std::string, double>>& vals)
{
  struct tm* t = localtime(&time);
  char buf[50];
  snprintf(buf, 50, "%04d-%02d-%02d %02d:%02d:%02d",
	   t->tm_year+1900, t->tm_mon+1, t->tm_mday,
	   t->tm_hour, t->tm_min, t->tm_sec);
  std::string timestring(buf);

  std::vector<std::pair<std::string, double>> valsCopy(vals);
  std::pair<std::string, double> lastVal = valsCopy.back();
  valsCopy.pop_back();

  std::stringstream ss;
  ss << "INSERT INTO " << tableName << " ( time, ";
  for (auto& val : valsCopy)
  {
    ss << val.first << ", ";
  }
  ss << lastVal.first << " ) ";
  ss << "VALUES ( '" << timestring << "', ";
  for (auto& val : valsCopy)
  {
    ss << val.second << ", ";
  }
  ss << lastVal.second << ");";

  std::shared_ptr<sql::Statement> pStatement(m_pConnection->createStatement());
  bool result = pStatement->execute(ss.str());
  
  if (!result)
  {
    std::cout << SQLLOG << " added row with:" << std::endl;
    std::cout << ss.str() << std::endl;
  }
  else
  {
    std::cout << SQLLOG << " error adding row with:" << std::endl;
    std::cout << ss.str() << std::endl;
    return -1;
  }
  
  return 0;

}


/*
int SQLConnectorB::checkConnection()
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

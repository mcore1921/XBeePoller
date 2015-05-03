#include "SQLConnector.h"
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

SQLConnector::SQLConnector(ConfigFile cf)
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

SQLConnector::~SQLConnector()
{
  delete m_pConnection;
}

int SQLConnector::putVals(long int time,
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
			   
bool SQLConnector::doesTableExist(std::string tableName)
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

int SQLConnector::createTable(std::string tableName,
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

bool SQLConnector::doTableColumnsWork(std::string tableName, 
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

int SQLConnector::makeTableColumnsWork(std::string tableName, 
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

int SQLConnector::addRow(std::string tableName,
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


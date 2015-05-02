#ifndef _SQL_CONNECTOR_B_H_INCLUDED_
#define _SQL_CONNECTOR_B_H_INCLUDED_

#include "Config.h"

#include <string>
#include <vector>

namespace sql
{
class Driver;
class Connection;
}

class SQLConnectorB
{
public:
  SQLConnectorB(ConfigFile cf);
  virtual ~SQLConnectorB();

  int putVals(long int time,
	      const std::vector<std::pair<std::string, double>>& vals);

private:
  
  bool doesTableExist(std::string tableName);
  // Makes the table, and makes an index on the time column.
  // Returns 0 on success, nonzero on failure
  int createTable(std::string tableName, 
		  const std::vector<std::string>& colNames);

  // True if the table with name tableName has columns matching each
  // string in colNames
  bool doTableColumnsWork(std::string tableName, 
			  const std::vector<std::string>& colNames);
  // Updates the table columns by adding columns for any that are missing
  // from colNames
  // Returns 0 on success, nonzero on failure.
  int makeTableColumnsWork(std::string tableName, 
			   const std::vector<std::string>& colNames);
  
  
  int addRow(std::string tableName,
	     long int time,
	     const std::vector<std::pair<std::string, double>>& vals);
  
  sql::Driver* m_pDriver;
  sql::Connection* m_pConnection;
  ConfigFile m_config;
};


#endif

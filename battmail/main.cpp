#include "Payload.h"
#include "SendMail.h"

#include "driver/mysql_connection.h"
#include "cppconn/driver.h" 
#include "cppconn/exception.h" 
#include "cppconn/resultset.h" 
#include "cppconn/resultset_metadata.h"
#include "cppconn/statement.h" 

#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <sstream>
#include <string>
#include <boost/program_options.hpp>
#include <map>
#include <iomanip>

#include "Config.h"

#define ll(p,d); if (loglevel > p) {d;}

int loglevel = 0;

namespace po = boost::program_options;

int main (int argc, char** argv)
{
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("loglevel", po::value<int>(), "set log level (higher is more logging)")
    ("duration", po::value<double>()->default_value(2.0), "Hours with no update before notification")
    ("to", po::value<std::string>(), "destination email address")
    ("cc", po::value<std::string>(), "additional email address")
    ("nomail", "Suppress sending mail")
    ("configfile", po::value<std::string>()->default_value("/etc/XBeeThermClient/BattMailConfig"), "configuration file")
    ;
//  po::positional_options_description p;
//  p.add("to", 1);

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
//  po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(),
//	    vm);
  po::notify(vm);

  if (vm.count("help"))
  {
    std::cout << "Usage: " << argv[0] << std::endl;
    std::cout << desc << std::endl;
    return 0;
  }
  if (vm.count("loglevel"))
  {
    loglevel = vm["loglevel"].as<int>();
    ll(1, std::cout << "Setting loglevel to " << loglevel << std::endl);
  }
  if (vm.count("nomail"))
  {
    ll(1, std::cout << "Suppressing mail send" << std::endl);
  }
  
  ConfigFile cf;
  cf.open(vm["configfile"].as<std::string>().c_str());
  std::string destAddress = cf.getParam("MAIL_CONFIG",
					"DESTINATION_ADDRESS",
					"");
  std::string ccAddress = cf.getParam("MAIL_CONFIG",
				      "CC_ADDRESS",
				      "");
  std::string fromAddress = cf.getParam("MAIL_CONFIG",
					"FROM_ADDRESS",
					"");
  std::string hostName = cf.getParam("SQL_PARAMETERS",
				     "HOSTNAME",
				     "localhost");
  std::string userName = cf.getParam("SQL_PARAMETERS",
				     "USERNAME",
				     "xbtherm");
  std::string userPass = cf.getParam("SQL_PARAMETERS",
				     "PASSWORD",
				     "therm");
  std::string database = cf.getParam("SQL_PARAMETERS",
				     "DATABASE",
				     "therm");
  std::string tableName = cf.getParam("SQL_PARAMETERS",
				      "TABLE_NAME",
				      "temperatures");

  sql::Driver *driver;
  sql::Connection *con;
  sql::Statement *stmt;
  sql::ResultSet *res;
  
  driver = get_driver_instance();
  con = driver->connect(hostName, userName, userPass);
  con->setSchema(database);

  double numhours = vm["duration"].as<double>();
  if (vm.count("duration"))
  {
    ll(1, std::cout << "Setting duration to " << numhours << std::endl);
  }
  int numsecs = numhours * 3600.0;
  ll (1, std::cout << "Duration is also " << numsecs 
      << " seconds." << std::endl);

  std::stringstream ss;
  // First figure out what our columns are
  ss << "SELECT * from " << tableName << " order by id desc limit 1";
  ll(5, std::cout << ss.str() << std::endl);
    
  stmt = con->createStatement();
  res = stmt->executeQuery(ss.str());
  
  sql::ResultSetMetaData* md = res->getMetaData();
  std::vector<std::string> locations;
  std::map<std::string, int> durations;
  for (unsigned int i = 3; i <= md->getColumnCount(); i++)
  {
    locations.push_back(std::string(md->getColumnName(i)));
  }
  delete res;

  for (std::vector<std::string>::iterator it = locations.begin();
       it != locations.end(); it++)
  {
    std::stringstream ss;
    ss << "select unix_timestamp(now()) - unix_timestamp(a.time) from "
       << "(select time, " << *it << " from " << tableName 
       << " where " << *it << " is not null order by time desc limit 1)"
       << " as a";
    ll(5, std::cout << ss.str() << std::endl);
    res = stmt->executeQuery(ss.str());
    res->next();
    durations[*it] = res->getInt(1);
    delete res;
  }


  std::map<std::string, int>  lates;
  for (std::map<std::string, int>::iterator it = durations.begin();
       it != durations.end(); it++)
  {
    ll(0, 
       std::cout << "Time since " << it->first << " update: " 
       << std::setw(2) << std::setfill('0') 
       << it->second / 3600 << ":"
       << std::setw(2) << std::setfill('0') 
       << (it->second % 3600) / 60 << ":"
       << std::setw(2) << std::setfill('0') 
       << (it->second % 60) << " ("
       << it->second << ")." << std::endl);

    if (it->second > numsecs)
      lates[it->first] = it->second;
  }

  ll (2, std::cout << lates.size() << " total 'late' stations" << std::endl);

  CEXBTherm::Payload* pPL = 0;
  if (lates.size() == durations.size())
  {
    // Something's wrong; nothing is coming in
    int minlate = lates.begin()->second;
    for (std::map<std::string, int>::iterator it = lates.begin();
	 it != lates.end(); it++)
      if (it->second < minlate) minlate = it->second;

    std::stringstream ss;
    ss << "No therm data has been recieved for " 
       << std::setw(2) << std::setfill('0') 
       << minlate / 3600 << ":"
       << std::setw(2) << std::setfill('0') 
       << (minlate % 3600) / 60 << ":"
       << std::setw(2) << std::setfill('0') 
       << (minlate % 60) << "." << std::endl;
    ss << "Check the server and/or hub XBee station." << std::endl;
    pPL = new CEXBTherm::Payload(destAddress, 
				 fromAddress,
				 "XBTherm Warning: No therm data", 
				 ss.str().c_str());
  }
  else if (lates.size() > 0)
  {
    // Some batteries need checking
    std::stringstream ss1;
    ss1 << "XBTherm Battery: ";
    std::stringstream ss2;
    ss2 << "The following stations have not responded for more than "
	<< numhours << " hours." << std::endl;
    ss2 << "Please check their batteries." << std::endl;
    
    for (std::map<std::string, int>::iterator it = lates.begin();
	 it != lates.end(); it++)
    {
      ss1 << it->first << " ";
      ss2 << it->first << " (" 
	  << std::setw(2) << std::setfill('0') 
	  << it->second / 3600 << ":"
	  << std::setw(2) << std::setfill('0') 
	  << (it->second % 3600) / 60 << ":"
	  << std::setw(2) << std::setfill('0') 
	  << (it->second % 60) << " since last report)." << std::endl;
    }
    pPL = new CEXBTherm::Payload(destAddress, 
				 fromAddress,
				 ss1.str().c_str(),
				 ss2.str().c_str());
    
  }

  if (pPL != 0)
  {
    if (ccAddress != "")
    {
      pPL->cc(ccAddress);
    }
    
    ll (2, pPL->dump(std::cout));
    
    if (vm.count("nomail") == 0)
      sendmail(destAddress.c_str(),
	       fromAddress.c_str(),
	       ccAddress.c_str(),
	       pPL->get(),
	       &cf);
  }

  delete pPL;
  delete stmt;
  delete con;

}

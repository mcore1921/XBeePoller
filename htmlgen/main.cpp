#include "DataForHtml.h"
#include "GenDataForHtml.h"
#include "FormatForHtml.h"

#include "driver/mysql_connection.h"
#include "cppconn/driver.h" 
#include "cppconn/exception.h" 
#include "cppconn/resultset.h" 
#include "cppconn/statement.h" 

#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <sstream>
#include <string>
#include <boost/program_options.hpp>

#define ll(p,d) if (loglevel > p) {d}

int loglevel = 0;

namespace po = boost::program_options;

int main (int argc, char** argv)
{
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("loglevel", po::value<int>(), "set log level (higher is more logging)")
    ("days", po::value<int>(), "Number of days of lookback")
    ("nosql", "don't actually run the SQL query")
    ("template", po::value<std::string>()->default_value(std::string("template_dg.html")), 
     "template file to use")
    ("database", po::value<std::string>()->default_value(std::string("therm")),
     "SQL database to use")
    ("table", po::value<std::string>()->default_value(std::string("temperatures")))
    ("sqlhostname", po::value<std::string>()->default_value(std::string("localhost")), "SQL Hostname")
    ("sqlusername", po::value<std::string>()->default_value(std::string("xbtherm")), "SQL Username")
    ("sqlpass", po::value<std::string>()->default_value(std::string("therm")),
     "SQL Password");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help"))
  {
    std::cout << desc << std::endl;
    return 0;
  }
  if (vm.count("loglevel"))
  {
    loglevel = vm["loglevel"].as<int>();
    ll(1, std::cout << "Setting loglevel to " << loglevel << std::endl;)
  }

  std::string templatefile = vm["template"].as<std::string>();
  std::string database = vm["database"].as<std::string>();
  std::string table = vm["table"].as<std::string>();
  std::string hostname = vm["sqlhostname"].as<std::string>();
  std::string username = vm["sqlusername"].as<std::string>();
  std::string pass = vm["sqlpass"].as<std::string>();
  sql::Driver *driver;
  sql::Connection *con;
  sql::Statement *stmt;
  sql::ResultSet *res;
  
  driver = get_driver_instance();
  con = driver->connect(hostname, username, pass);
  con->setSchema(database);

  int numdays = 1;
  if (vm.count("days"))
  {
    numdays = vm["days"].as<int>();
    ll(1, std::cout << "Setting days to " << numdays << std::endl;)
  }

  std::stringstream ss;
  ss << "SELECT "
//     << "time, mattsdesktop, hallwaythermostat, backporch, "
//     << " garage, masterbedroom, nursery, kegerator "
     << " * "
     << "from " << table << " where time > now() - interval "
     << numdays << " day";
  ll(5, std::cout << ss.str() << std::endl;)
  
  if (vm.count("nosql"))
    return 0;

  stmt = con->createStatement();
  res = stmt->executeQuery(ss.str());
  
  // Here we have the data.  Let's translate it into our format
  // so we can massage a little bit.
  DataForHtml dfh;
  genDataForHtml(res, dfh);

  FormatForHtml ffh;
  ffh.readTemplate(templatefile);
  ffh.doFormatting(dfh);
  std::cout << ffh.getOutput() << std::endl;

  delete res;
  delete stmt;
  delete con;

}



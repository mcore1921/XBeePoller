#include "Config.h"
#include "util.h"

#include <sstream>
#include <fstream>
#include <iostream>

/*
Maybe something like this?
:SENSORS
NUM_SENSORS 1
SENSOR_0 00-13-a2-00-40-a8-36-49
# more sensors here...
:00-13-a2-00-40-a8-36-49
SQLNAME HallwayThermostat
DISPLAYNAME Hallway Thermostat
CALIBRATION 1.034
:00-13-a2-00-40-a8-36-49_PARAMS
SN 6
SP 2500
IR 250

XBeeConfig::XBeeConfig(std::string address, ConfigFile cf)
{
  m_address = address;
  m_sqlName = cf.getParam(m_address,
			  "SQLNAME",
			  "");
  m_displayName = cf.getParam(m_address,
			      "DISPLAYNAME",
			      "");
  m_sensorCal = cf.getParam(m_address,
			    "CALIBRATION",
			    1.0);

  // I know this is expensive, but who cares
  // it's also safe, and we're only planning to do it during init
  // plus computers are fast anyway
  auto valsCopy = cf.m_vals;
  for (auto& stringPair : valsCopy[m_address])
  {
    std::stringstream ss(stringPair.second);
    int i;
    ss >> i;
    if (!ss.fail())
      m_intParams[stringPair.first] = i;
  }
}
*/


int ConfigFile::open(std::string filename)
{
  std::ifstream inputFile(filename.c_str());
  if (!inputFile)
    return -1;

  std::string line;
  std::string state = "NONE";
  while (! (!getline(inputFile, line)))
  {
    if (line.length() == 0)
      continue;
    else if (line[0] == '#') // comment
      continue;
    else if (line[0] == ':')
    {
      state = line.substr(1);
    }
    else 
    {
      std::stringstream ss(line);
      std::string t1, t2;
      ss >> t1;
      if (ss)
      {
	t2 = line.substr(t1.length()+1);
	m_vals[state][t1]=t2;
      }
    }
  }

  return 0;
}

std::string ConfigFile::getParam(const std::string& group,
				 const std::string& param, 
				 const std::string& default_val)
{
  if (m_vals.find(group) == m_vals.end())
    return default_val;
  if (m_vals.find(group)->second.find(param) == 
      m_vals.find(group)->second.end())
    return default_val;
  else
    return m_vals[group][param];
}

int ConfigFile::getParam(const std::string& group,
			 const std::string& param, 
			 int default_val)
{
  std::string s;
  if (m_vals.find(group) == m_vals.end())
    return default_val;
  if (m_vals.find(group)->second.find(param) == 
      m_vals.find(group)->second.end())
    return default_val;
  else
  {
    s = m_vals[group][param];
    std::stringstream ss(s);
    int i;
    ss >> i;
    return i;
  }
}

double ConfigFile::getParam(const std::string& group,
			    const std::string& param, 
			    double default_val)
{
  std::string s;
  if (m_vals.find(group) == m_vals.end())
    return default_val;
  if (m_vals.find(group)->second.find(param) == 
      m_vals.find(group)->second.end())
    return default_val;
  else
  {
    s = m_vals[group][param];
    std::stringstream ss(s);
    double d;
    ss >> d;
    return d;
  }
}


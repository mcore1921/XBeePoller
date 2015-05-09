#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <iostream>

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <fstream>

#include "InvertMatrix.h"

typedef std::pair<std::string, std::map<double, double>> Sensor;

// Returns 0 on success
int readConfig(std::string configFile, std::vector<Sensor>& sensors)
{
  sensors.clear();

  std::ifstream inputFile(configFile.c_str());
  if (!inputFile)
    return -1;

  std::string line;
  while (! (!getline(inputFile, line)))
  {
    if (line.length() == 0)
      continue;
    else if (line[0] == '#') // comment
      continue;
    else if (line[0] == ':')
    {
      sensors.push_back(std::make_pair(line.substr(1), 
				       std::map<double, double>()));
    }
    else if (sensors.size() > 0)
    {
      std::stringstream ss(line);
      double val, temperature;
      ss >> val >> temperature;
      if (ss)
      {
	sensors.back().second[val] = temperature;
      }
    }
  }
  return 0;
}

void debugReadConfig(std::vector<Sensor> sensors)
{
  for (auto& s : sensors)
  {
    std::cout << s.first << std::endl;
    for (auto& v : s.second)
    {
      std::cout << "  " << v.first << " " << v.second << std::endl;
    }
  }
    
}  

std::vector<double> computeGainAndOffset(Sensor s)
{
  using namespace boost::numeric::ublas;
  int numrows = s.second.size();

  matrix<double> a(numrows,2);
  vector<double> b(numrows);
  int c = 0;
  for (auto& v : s.second)
  {
    a(c, 0) = v.first;
    a(c, 1) = 1;
    b(c) = v.second;
    c++;
  }
  matrix<double> aT(2,numrows);
  aT = trans(a);
  matrix<double> aTa(2,2);
  aTa = prod(aT, a);
  matrix<double> aTaI(2,2);
  InvertMatrix(aTa, aTaI);
  vector<double> aTb (2);
  aTb = prod(aT, b);
  vector<double> x (2);
  x = prod(aTaI,aTb);

  std::vector<double> r;
  r.push_back(x(0));
  r.push_back(x(1));
  return r;
}

int main (int argc, char** argv)
{
  if (argc < 2)
  {
    std::cout << "usage: " << argv[0] << " <calib input file>" << std::endl;
    std::cout << "Calib input file format:" << std::endl;
    std::cout << ":sensor1_name" << std::endl;
    std::cout << "analog_val temperature" << std::endl;
    std::cout << "analog_val temperature" << std::endl;
    std::cout << "analog_val temperature" << std::endl;
    std::cout << ":sensor2_name" << std::endl;
    std::cout << "analog_val temperature" << std::endl;
    std::cout << "analog_val temperature" << std::endl;
    std::cout << "... etc.  At least 2 rows required per sensor." << std::endl;
    std::cout << "As many rows as desired may be provided; best-fit is performed." << std::endl;
    exit(-1);
  }
  
  std::string filename = argv[1];
  std::vector<Sensor> sensors;
  int e = readConfig(filename, sensors);
  if (e != 0)
  {
    std::cout << "Failed to read input file " << filename << std::endl;
    exit(-1);
  }

  std::stringstream s1, s2;
  s1 << ":CALIBRATIONS_GAIN" << std::endl;
  s2 << ":CALIBRATIONS_OFFSET" << std::endl;
  for (auto& s : sensors)
  {
    std::vector<double> go = computeGainAndOffset(s);
    s1 << s.first << " " << go[0] << std::endl;
    s2 << s.first << " " << go[1] << std::endl;
  }
  std::cout << s1.str() << s2.str();

  return 0;
}

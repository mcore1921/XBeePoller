#ifndef __CONFIG_H_INCLUDED__
#define __CONFIG_H_INCLUDED__

#include <string>
#include <map>

/* Once I'm ready to implement remote sensor param settings again,
   I'll ressurect this commented block
class ConfigFile;

class XBeeConfig
{
public:
  XBeeConfig(std::string address, ConfigFile cf);
  std::string m_address;
  std::string m_sqlName;
  std::string m_displayName;
  double m_sensorCal;
  std::map<std::string, int> m_intParams;
};  
*/

class ConfigFile
{
friend class XBeeConfig;
public:
  ConfigFile() {}
  int open(std::string filename);
  std::string getParam(const std::string& group,
		       const std::string& param, 
		       const std::string& default_val);
  int getParam(const std::string& group,
	       const std::string& param, 
	       int default_val);
  double getParam(const std::string& group,
		  const std::string& param, 
		  double default_val);

private:
  std::map<std::string, std::map<std::string, std::string> > m_vals;
};

#endif

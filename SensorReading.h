#ifndef _SENSOR_READING_H_INC_
#define _SENSOR_READING_H_INC_

#include <string>
#include <ostream>

class SensorReading
{
public:
  SensorReading();
  SensorReading(std::string serno, double time, float analog, float cal);

  int parseWixelLine(std::string line);

  std::string m_serno;
  // Seconds (including fractional seconds) since epoch
  double m_time;
  float m_analog;
  float m_cal;
};

std::ostream& operator<<(std::ostream& o, const SensorReading& s);

#endif

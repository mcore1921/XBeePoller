#include "SensorReading.h"


SensorReading::SensorReading() : m_time(0), m_analog(0), m_cal(0) 
{}

SensorReading::SensorReading(std::string serno, 
			     double time, 
			     float analog, 
			     float cal)
  : m_serno(serno), 
    m_time(time), 
    m_analog(analog), 
    m_cal(cal) 
{}

std::ostream& operator<<(std::ostream& o, const SensorReading& s)
{
  o << "(" << s.m_serno 
    << ") [" << s.m_time 
    << "] " << s.m_analog 
    << " " << s.m_cal;
  return o;
}

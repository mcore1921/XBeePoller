#include <sys/time.h>
#include <time.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include "util.h"

#define HEX( x ) std::setw(2) << std::setfill('0') << std::hex << (int)( x )

double dtime()
{
  timeval tv;
  gettimeofday(&tv, NULL);
  
  tm tm;
  time_t t = time(NULL);
  localtime_r(&t, &tm);

  return ((double)((tm.tm_hour*3600)+(tm.tm_min*60)+(tm.tm_sec))) +
    ((double)tv.tv_usec*1.0e-6);
}

std::string macAddrString(const unsigned char* addr)
{
  std::stringstream ss;
  for (int i = 0; i < 7; i++)
    ss << HEX(addr[i]) << "-";
  ss << HEX(addr[7]);
  return ss.str();
}

std::string macAddrString(const std::vector<unsigned char> addr)
{
  std::stringstream ss;
  if (addr.size() > 0)
  {
    for (auto it = addr.begin(); it != addr.end()-1; it++)
      ss << HEX(*it) << "-";
    ss << HEX(addr[addr.size()-1]);
  }
  return ss.str();
}

std::vector<unsigned char> macAddrData(const std::string in_addr)
{
  std::string addr = in_addr;
  while (addr.find("-") != std::string::npos)
    addr.replace(addr.find("-"), 1, " ");
  std::stringstream ss(addr);
  std::vector<unsigned char> r;
  while (ss.good())
  {
    int a;
    ss >> std::hex >> a;
    if (!ss.fail())
      r.push_back(a);
  }
  return r;
}


void debugPacketInfo(const unsigned char* data)
{
  int pos = 1;

  std::cout << "Sender's address (64b): ";
  for (int i = 1; i < 8; i++)
    std::cout << HEX(data[pos++]) << "-";
  std::cout << HEX(data[pos++]) << std::endl;
  std::cout << "Sender's address (16b): " << HEX(data[pos++]); 
  std::cout << HEX(data[pos++]) << std::endl;
  std::cout << "Receive Options: " << HEX(data[pos++]) << std::endl;
  std::cout << "Number of Samples: " << HEX(data[pos++]) << std::endl;
  std::cout << "Digital Channel Mask: " << HEX(data[pos++]);
  std::cout << HEX(data[pos++]) << std::endl;
  std::cout << "Analog Channel Mask: " << HEX(data[pos++]) << std::endl;
  if (data[13] != 0 || data[14] != 0)
    std::cout << "Digital Samples: " << HEX(data[pos++]);
  std::cout << HEX(data[pos++]) << std::endl;
  std::cout << "Analog Sample: " << HEX(data[pos++]);
  std::cout << HEX(data[pos++]) << std::endl;
  std::cout << "Accounted for " << std::dec << pos << " bytes." << std::endl;

}


double tvdiff(const struct timeval& a, const struct timeval& b)
{
  return (((double) a.tv_usec - (double) b.tv_usec) * 1.0e-6) 
    + (double) a.tv_sec - (double) b.tv_sec;
}

void tvinit(struct timeval& a)
{
  a.tv_sec = 0;
  a.tv_usec = 0;
}

timeval tsToTv(const timespec& a)
{
  timeval b;
  b.tv_sec = a.tv_sec;
  b.tv_usec = a.tv_nsec / 1e3;
  return b;
}

timeval tvSub(const timeval& a, const timeval& b)
{
  timeval c;
  c.tv_sec = a.tv_sec - b.tv_sec;
  c.tv_usec = a.tv_usec - b.tv_usec;
  if (c.tv_usec < 0)
  {
    c.tv_usec += 1e6;
    c.tv_sec -= 1;
  }
  return c;
}

bool operator<(const struct timespec& a, const struct timespec& b)
{
  return (a.tv_sec < b.tv_sec ||
	  (a.tv_sec == b.tv_sec &&
	   a.tv_nsec < b.tv_nsec));
}

timespec tsMake(long int sec, long int nsec)
{
  timespec a;
  a.tv_sec = sec;
  a.tv_nsec = nsec;
  return a;
}

timespec tsAdd(timespec a, timespec b)
{
  timespec c;
  c.tv_sec = a.tv_sec + b.tv_sec;
  c.tv_nsec = a.tv_nsec + b.tv_nsec;
  if (c.tv_nsec >= 1e9)
  {
    c.tv_sec += 1;
    c.tv_nsec -= 1e9;
  }
  return c;
}

timespec tsMsAdd(timespec a, long int msec)
{
  a.tv_sec += msec / (long int) 1e3;
  a.tv_nsec += (msec % (long int) 1e3) * (long int) 1e6;
  if (a.tv_nsec >= (long int) 1e9)
  {
    a.tv_sec += 1;
    a.tv_nsec -= (long int) 1e9;
  }
  return a;
}

timespec tsNsAdd(timespec a, long int nsec)
{
  a.tv_sec += nsec / (int) 1e9;
  a.tv_nsec += nsec % (long int) 1e9;
  if (a.tv_nsec >= 1e9)
  {
    a.tv_sec += 1;
    a.tv_nsec -= 1e9;
  }
  return a;
}

double tsDiff(timespec a, timespec b)
{
  return (((double) a.tv_nsec - (double) b.tv_nsec) * 1.0e-9) 
    + (double) a.tv_sec - (double) b.tv_sec;
}

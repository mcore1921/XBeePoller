#ifndef _UTIL_H_INCLUDED_
#define _UTIL_H_INCLUDED_

#include <string>
#include <vector>

double dtime();
std::string macAddrString(const unsigned char* addr);
std::string macAddrString(const std::vector<unsigned char> addr);
std::vector<unsigned char> macAddrData(const std::string addr);
void debugPacketInfo(const unsigned char* data);

double tvdiff(const struct timeval& a, const struct timeval& b);
void tvinit(struct timeval& a);
timeval tsToTv(const timespec& a);
timeval tvSub(const timeval& a, const timeval& b);

bool operator<(const struct timespec& a, const struct timespec& b);
timespec tsMake(long int sec, long int nsec);
timespec tsAdd(timespec a, timespec b);
timespec tsMsAdd(timespec a, long int msec);
timespec tsNsAdd(timespec a, long int nsec);
double tsDiff(timespec a, timespec b);

#endif

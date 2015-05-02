#ifndef __MODEM_STATUS_H_DEFINED__
#define __MODEM_STATUS_H_DEFINED__

#include "Message.h"

#include <vector>

class ModemStatus : public Message
{
public:
  virtual ~ModemStatus() {}
  virtual const char* type() { return "ModemStatus"; }
  ModemStatus(std::vector<unsigned char> frame);

  int getStatus() { return m_status; }
  
private:
  int m_status;

};

#endif

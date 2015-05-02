#include "MessageFactory.h"
#include "IOSample.h"
#include "RemoteCommandResponse.h"
#include "ATCommandResponse.h"
#include "ModemStatus.h"
#include "util.h"

#include <iostream>
#include <iomanip>
#include <fstream>

#define HEX( x ) std::setw(2) << std::setfill('0') << std::hex << (int)( x )

MessageFactory::MessageFactory(ConfigFile cf)
  : m_config(cf)
{
}


int MessageFactory::createMessage(const std::vector<unsigned char>& frame,
				  std::shared_ptr<Message>& msg)
{
  msg.reset();
  if (frame[0] == 0x92)
  {
    std::shared_ptr<Message> a(new IOSample(frame));
    msg = a;
    return 0;
  }
  else if (frame[0] == 0x88)
  {
    std::shared_ptr<Message> a(new ATCommandResponse(frame));
    msg = a;
    return 0;
  }
  else if (frame[0] == 0x97)
  {
    std::shared_ptr<Message> a(new RemoteCommandResponse(frame));
    msg = a;
    return 0;
  }
  else if (frame[0] == 0x8a)
  {
    std::shared_ptr<Message> a(new ModemStatus(frame));
    msg = a;
    return 0;
  }
  else
  {
    std::cout << std::setprecision(3) << std::fixed << dtime() 
	      << " Unknown packet type: " << HEX(frame[0]) << std::endl;
    if (m_config.getParam("DEBUG", "DUMP_UNKNOWN_PACKET_TYPES", 0) != 0)
    {
      std::ofstream os("/var/tmp/xbdump.in", std::ios_base::app);
      for (auto it = frame.begin(); it != frame.end(); it++)
	os << *it;
      for (int i = 0; i < 4; i++)
	os << '\0';
    }
    return -1;
  }
  return -1;
}

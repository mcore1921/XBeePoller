#include "ATCommandResponse.h"

#include <iostream>
#include <iomanip>

#define HEX( x ) std::setw(2) << std::setfill('0') << std::hex << (int)( x )

ATCommandResponse::ATCommandResponse(std::vector<unsigned char> frame)
{
  setData(frame);
}

int ATCommandResponse::setData(std::vector<unsigned char> frame)
{
  // Check frame type
  if (frame[0] != 0x88)
  {
    std::cout << __FILE__ << ":" << __LINE__ 
	      << "ATCommandResponse::setData(): Unknown type (" 
	      << HEX(frame[0]) << ")" << std::endl;
    return -1;
  }
  // Check size
  if (frame.size() < 5)
  {
    std::cout << __FILE__ << ":" << __LINE__ 
	      << "ATCommandResponse::setData(): Frame not large "
	      << "enough!  Expected at least 5 bytes, size " 
	      << frame.size() << std::endl;
    return -1;
  }
    

  // Frame ID is next
  m_frameId = frame[1];
  for (int i = 0; i < 2; i++)
  {
    m_command.push_back(frame[i+2]); // command is bytes 2 and 3
  }
  m_status = frame[4]; // Status is byte 4 (starting from 0)
  // Any remaining bytes are the arg, starting at 15
  m_arg = 0;
  for (unsigned int i = 5; i < frame.size(); i++)
  {
    m_argData.push_back(frame[i]);
    m_arg = m_arg << 8;
    m_arg += frame[i];
  }

  return 0;
}

std::vector<unsigned char> ATCommandResponse::getCommand()
{
  return m_command;
}
std::vector<unsigned char> ATCommandResponse::getArgData()
{
  return m_argData;
}
int ATCommandResponse::getArg()
{
  return m_arg;
}
unsigned char ATCommandResponse::getStatus()
{
  return m_status;
}
unsigned char ATCommandResponse::getFrameId()
{
  return m_frameId;
}

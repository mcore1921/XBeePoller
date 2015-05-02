#include "RemoteCommandResponse.h"

#include <iostream>
#include <iomanip>

#define HEX( x ) std::setw(2) << std::setfill('0') << std::hex << (int)( x )

RemoteCommandResponse::RemoteCommandResponse(std::vector<unsigned char> frame)
{
  setData(frame);
}

int RemoteCommandResponse::setData(std::vector<unsigned char> frame)
{
  // Check frame type
  if (frame[0] != 0x97)
  {
    std::cout << __FILE__ << ":" << __LINE__ 
	      << "RemoteCommandResponse::setData(): Unknown type (" 
	      << HEX(frame[0]) << ")" << std::endl;
    return -1;
  }
  // Check size
  if (frame.size() < 15)
  {
    std::cout << __FILE__ << ":" << __LINE__ 
	      << "RemoteCommandResponse::setData(): Frame not large "
	      << "enough!  Expected at least 15 bytes, size " 
	      << frame.size() << std::endl;
    return -1;
  }
    

  // Frame ID is next
  m_frameId = frame[1];
  for (int i = 0; i < 8; i++) 
  {
    m_address.push_back(frame[i+2]); // 64-bit addr starts at byte 2
  }
  for (int i = 0; i < 2; i++)
  {
    m_command.push_back(frame[i+12]); // command is bytes 12 and 13
  }
  m_status = frame[14]; // Status is byte 14 (starting from 0)
  // Any remaining bytes are the arg, starting at 15
  m_arg = 0;
  for (unsigned int i = 15; i < frame.size(); i++)
  {
    m_argData.push_back(frame[i]);
    m_arg = m_arg << 8;
    m_arg += frame[i];
  }

  return 0;
}

std::vector<unsigned char> RemoteCommandResponse::getCommand()
{
  return m_command;
}
std::vector<unsigned char> RemoteCommandResponse::getAddress()
{
  return m_address;
}
std::vector<unsigned char> RemoteCommandResponse::getArgData()
{
  return m_argData;
}
int RemoteCommandResponse::getArg()
{
  return m_arg;
}
unsigned char RemoteCommandResponse::getStatus()
{
  return m_status;
}
unsigned char RemoteCommandResponse::getFrameId()
{
  return m_frameId;
}

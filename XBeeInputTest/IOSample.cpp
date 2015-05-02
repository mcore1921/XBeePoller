#include "IOSample.h"
#include <string.h>

#include <iostream>
#include <iomanip>

#define HEX( x ) std::setw(2) << std::setfill('0') << std::hex << (int)( x )

IOSample::IOSample(const unsigned char* senderAddress,
		   int value)
{
  memcpy(m_senderAddress, senderAddress, 8);
  m_value = value;
}
 
IOSample::IOSample(const std::vector<unsigned char>& data)
{
  setData(data.data());
}

IOSample::IOSample(const unsigned char* data)
{
  setData(data);
}

int IOSample::setData(const unsigned char* data)
{
  // Check packet type
  if (data[0] != 0x92)
  {
    std::cout << __FILE__ << ":" << __LINE__ 
	      << "IOSample::setData(): Unknown type (" 
	      << HEX(data[0]) << ")" << std::endl;
    return -1;
  }

  // Many assumptions here, but the biggest is that we have exactly
  // 1 analog sample in the packet and 0 digital samples.
  for (int i = 0; i < 8; i++) 
  {
    m_senderAddress[i] = data[1+i]; // 64-bit addr starts at byte 1
  }
  m_value = (data[16] << 8) + data[17];  // 2-byte value starts at byte 16

  return 0;
}

#include "ModemStatus.h"

#include <iostream>
#include <iomanip>

#define HEX( x ) std::setw(2) << std::setfill('0') << std::hex << (int)( x )

ModemStatus::ModemStatus(std::vector<unsigned char> frame)
{
  // Check frame type
  if (frame[0] != 0x8a)
  {
    std::cout << __FILE__ << ":" << __LINE__ 
	      << "ModemStatus::setData(): Unknown type (" 
	      << HEX(frame[0]) << ")" << std::endl;
  }
  // Check size
  if (frame.size() < 2)
  {
    std::cout << __FILE__ << ":" << __LINE__ 
	      << "ModemStatus::setData(): Frame not large "
	      << "enough!  Expected at least 5 bytes, size " 
	      << frame.size() << std::endl;
  }

  // Status is next
  m_status = frame[1];

}

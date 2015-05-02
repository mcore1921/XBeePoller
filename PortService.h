#ifndef __PORT_SERVICE_H_DEFINED__
#define __PORT_SERVICE_H_DEFINED__

#include "Config.h"

#include <vector>

class PortService
{
public:
  PortService(int fd, ConfigFile cf);

  // Blocking call
  // Waits on the port until data arrives
  // Constructs the data into a buffer, with the following rules:
  // 1) Any data prior to the first packet header (0x7e) is discarded
  //    Note this applies only to the first service() call; subsequent
  //    calls use any residual data from prior calls instead of discarding
  // 2) After header, the size is determined from the next two bytes.
  //    MSB comes first.
  // 3) At least <size> bites are then read, plus one byte for checksum.
  //    Any residual data is not discarded.
  //    The data between the size and the checksum is the frame data.
  // 4) The frame data is checked against the checksum.  
  //    All bytes of the frame are added to the checksum.  The result
  //    should be 0xFF.
  // 5) Provided the checksum passes, the frame is put into the 
  //    pass-by-ref argument.
  // 6) Any subsequent call to service() will begin using any residual
  //    data collected at the end of this call.
  //
  // service will return once an underlying select() call has consumed
  //   all time allocated by timeout, if not sooner.
  // Returns 0 on success, 1 on timeout, negative on error.
  int service(std::vector<unsigned char>& data,
	      struct timeval* timeout);

  // Writes the data to the port.
  // Not much thinking about it, either.  Just write.
  // Returns whatever write() returns.
  int write(const std::vector<unsigned char>& data);

private:

  // Resets m_buffer to a new vector, dropping any junk characters 
  // that are in the front of the current m_buffer.  The first packet header
  // and any chars following it will be retained.
  // Returns 0 on success, nonzero on failure.
  int consumeJunkChars();

  // Blocking call
  // Reads a full packet from the input FD.
  // Uses select to prevent unnecessary delays and/or CPU usage
  // Assumes that either:
  // 1) m_buffer is empty, or
  // 2) the first char of m_buffer is a packet header
  // Stops when:
  // 1) The first char in m_buffer is a packet header
  // 2) The next two chars, which describe the size, are well
  //    formed.  According to the datasheet, the maximum payload
  //    size appears to be never more than 84 bytes.  Let's be healthy
  //    and assume it can get as big as 200.
  // 3) <size>+1 packets have been read.
  // Does not look at checksum; this function exists primarly
  // to be the blocker while waiting for input.
  // Returns zero on success, 1 on timeout, negative on failure.
  int waitForPacket(struct timeval* timeout);

  // Blocking call
  // Returns 0 on succes, 1 on timeout, negative on failure
  // argument is populated with any retrieved data
  int getData(std::vector<unsigned char>& data,
	      struct timeval* timeout);

  // Verifies the data packet sitting in the front of m_buffer
  // passes its internal checksum.
  // Assumes that m_buffer contains a full packet.
  // Returns zero on successful checksum verification, nonzero on failure.
  int verifyChecksum();
  
  // Pulls a frame off m_buffer.
  // Also resets m_buffer to a new vector containing only the data
  // following the frame.
  // Assumes the frame is well-formed.
  int pullFrame(std::vector<unsigned char>& frame);

  int m_fdSer;
  std::vector<unsigned char> m_buffer;
  ConfigFile m_cf;

};


#endif

#include "PortService.h"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <fstream>

#include <sys/select.h>
#include <string.h>
#include <unistd.h>

#define HEXC( x ) std::setw(2) << std::setfill('0') << std::hex << (int)( x&0xFF )

PortService::PortService(int fd, ConfigFile cf)
{
  m_fdSer = fd;
  m_cf = cf;
}

int PortService::service(std::vector<unsigned char>& data,
			 struct timeval* timeout)
{
  int retval;

  // Loop forever.
  // This allows for checksum failures to start over from
  // scratch.
  // We expect something to return out when a frame is available.
  while (1)
  {

    // First, clean up any residual state of the buffer
    consumeJunkChars();

    // Now, wait for a packet
    retval = waitForPacket(timeout);
    if (retval == 1)
      return 1;
    else if (retval != 0)
    {
      std::cout << "PortService::service: ERROR waitForPacket returned " 
		<< retval << std::endl;
      return -1;
    }
    
    retval = verifyChecksum();
    if (retval != 0)
    {
      std::cout << "PortService::service: checksum failed."
		<< " Discarding packet and waiting for another." 
		<< std::endl;
      // Just reset the first character so it is no longer 0x7e
      // Trust the loop to consume junk characters off the start.
      m_buffer[0] = 0;
    }
    else
    {
      // Checksum passes
      std::vector<unsigned char> frame;
      retval = pullFrame(frame);
      if (retval != 0)
      {
	std::cout << "PortService::service: ERROR pullFrame returned "
		  << retval << std::endl;
	return -1;
      }
      data = frame;
      return 0;
    }
  }
}

int PortService::write(const std::vector<unsigned char>& data)
{
  return ::write(m_fdSer, &data[0], data.size());
}


// Resets m_buffer to a new vector, dropping any junk characters 
// that are in the front of the current m_buffer.  The first packet header
// and any chars following it will be retained.
// Returns 0 on success, nonzero on failure.
int PortService::consumeJunkChars()
{
  // If already empty, done.
  if (m_buffer.size() == 0)
    return 0;

  // If the first packet is a header, done.
  if (m_buffer[0] == 0x7e)
    return 0;

  // Otherwise, find the first instance of header.
  std::vector<unsigned char>::iterator it;
  it = std::find(m_buffer.begin(), m_buffer.end(), 0x7e);

  // If the header isn't in the vector, purge the vector, done.
  if (it == m_buffer.end())
  {
    m_buffer.clear();
    return 0;
  }

  // If we have a header but it's not first, create a new vector
  // starting at the header and including the rest of the buffer.
  // Assign the buffer to that new vector, done.
  std::vector<unsigned char> t(it, m_buffer.end());

  m_buffer = t;
  return 0;
}


int PortService::waitForPacket(struct timeval* timeout)
{
  // Blocking call
  // Reads a full packet from the input FD.
  // Uses select to prevent unnecessary delays and/or CPU usage
  // Assumes that either:
  // 1) m_buffer is empty, or
  // 2) the first char of m_buffer is a packet header
  if (m_buffer.size() > 0 &&
      m_buffer[0] != 0x7e)
  {
    std::cout << "PortService::waitForPacket: ERROR called when "
	      << "first char of buffer is not header.  It is: "
	      << HEXC(m_buffer[0]) << std::endl;
    return -1;
  }
  // Stops when:
  // 1) The first char in m_buffer is a packet header
  // 2) The next two chars, which describe the size, are well
  //    formed.  According to the datasheet, the maximum payload
  //    size appears to be never more than 84 bytes.  Let's be healthy
  //    and assume it can get as big as 200.
  // 3) <size>+1 packets have been read.
  // Does not look at checksum; this function exists primarly
  // to be the blocker while waiting for input.
  // Returns zero on success, nonzero on failure.
  
  // We'll stash the length value from the packet into this variable.
  // Note that for loop purposes, we'll read until we have this many
  // bytes + 4.  (1 for header, 2 for length itself, 1 for checksum)
  int length = 0;
  if (m_buffer.size() >= 3)
  {
    length = (m_buffer[1] << 8) + m_buffer[2];
  }

  while ((int)m_buffer.size() < length+4)
  {

    std::vector<unsigned char> data;
    int r = getData(data, timeout);
    if (r < 0)
    {
      std::cout << "PortService::waitForPacket: ERROR getData "
		<< "returned " << r << std::endl;
      return -1;
    }
    else if (r == 0)
    {
      // If we don't already have a header (from a prior loop)
      // then we need to drop chars until we get our first header
      if (m_buffer.size() > 0 && m_buffer[0] != 0x7e)
      {
	// Read the buffer until we find a header
	unsigned int i = 0;
	for (; i < data.size() && data[i] != 0x7e; i++);
	// Read the rest of the buffer (including the packet header)
	// into m_buffer
	for (; i < data.size(); i++)
	  m_buffer.push_back(data[i]);
      }
      else
      {
	// If we do have a header from a prior loop, then we just
	// keep all the characters
	for (unsigned int i = 0; i < data.size(); i++)
	  m_buffer.push_back(data[i]);
      }
    }
    else
    {
      // Read timeout
      return 1;
    }
    
    // At this point we're sure that the start of m_buffer
    // is the start of a packet.  Figure out the packet length.
    if (m_buffer.size() >= 3)
      length = (m_buffer[1] << 8) + m_buffer[2];

    if (length > 200)
    {
      std::cout << "PortService::waitForPacket: ERROR packet "
		<< "size is too large: " << length << std::endl;
      std::cout << "Overwriting packet header to prevent infinite "
		<< "loops." << std::endl;
      m_buffer[0] = 0;
      return -1;
    }
  }
  // At this point we've read enough bytes
  return 0;
}

int PortService::getData(std::vector<unsigned char>& data,
			 struct timeval* timeout)
{
  data.clear();

  fd_set readfs;
  fd_set writefs;
  fd_set exceptfs;
  FD_ZERO(&readfs);
  FD_ZERO(&writefs);
  FD_ZERO(&exceptfs);
  FD_SET(m_fdSer, &readfs);
  int nfds = m_fdSer;
    
  int r = select(nfds+1, &readfs, &writefs, &exceptfs, timeout);
  if (r == -1)
  {
    std::cout << "PortService::getData: ERROR select "
	      << "returned " << r << std::endl;
    return -1;
  }
	
  if (FD_ISSET(m_fdSer, &readfs))
  {
    FD_CLR(m_fdSer, &readfs);
    const int readsize = 1024;
    unsigned char buf[readsize];
    memset(buf, 0, readsize);
    int numread = read(m_fdSer, &buf, readsize);

    if (m_cf.getParam("DEBUG", "DUMP_ALL_XBEE_OUTPUT", 0) != 0)
    {
      std::ofstream os("/var/tmp/xbdump.raw", std::ios_base::app);
      for (int i = 0; i < numread; i++)
	os << buf[i];
    }
    
    data.assign(&buf[0], &buf[numread]);
    return 0;
  }
  else
  {
    //timeout
    return 1;
  }
}

// Verifies the data packet sitting in the front of m_buffer
// passes its internal checksum.
// Assumes that m_buffer contains a full packet.
// Returns zero on successful checksum verification, nonzero on failure.
int PortService::verifyChecksum()
{

  if (m_buffer.size() < 3)
  {
    std::cout << "PortService::verifyChecksum: ERROR length of "
	      << "buffer is obviously too small:" << m_buffer.size()
	      << std::endl;
    return -1;
  }
  int length = (m_buffer[1] << 8) + m_buffer[2];
  if ((int)m_buffer.size() < length+4) // length should not include sum, 
                                  // header or length itself
  {
    std::cout << "PortService::verifyChecksum: ERROR length of "
	      << "buffer is less than specified.  Specified:" 
	      << length << ", actual:" << m_buffer.size()
	      << std::endl;
    return -1;    
  }
  unsigned char sum = 0;
  for (int i = 0; i < length+1; i++) // Length does not include sum
  {
    sum += m_buffer[i+3];
  }
  if (sum != 0xFF)
  {
    std::cout << "PortService::verifyChecksum: Checksum failure."
	      << std::endl;
    return -1;
  }
  return 0;
}

// Pulls a frame off m_buffer.
// Also resets m_buffer to a new vector containing only the data
// following the frame.
// Assumes the frame is well-formed.
int PortService::pullFrame(std::vector<unsigned char>& frame)
{
  frame.clear();
  if (m_buffer.size() < 3)
  {
    std::cout << "PortService::verifyChecksum: ERROR length of "
	      << "buffer is obviously too small:" << m_buffer.size()
	      << std::endl;
    return -1;
  }
  int length = (m_buffer[1] << 8) + m_buffer[2];
  if ((int)m_buffer.size() < length+3) // length should include sum, 
                                    // but not header or length itself
  {
    std::cout << "PortService::verifyChecksum: ERROR length of "
	      << "buffer is less than specified.  Specified:" 
	      << length << ", actual:" << m_buffer.size()
	      << std::endl;
    return -1;    
  }
  for (int i = 0; i < length; i++) 
  {
    frame.push_back(m_buffer[i+3]);
  }

  // Purge the frame and associated protocol bytes
  std::vector<unsigned char> t = m_buffer;
  m_buffer.clear();
  m_buffer.assign(t.begin()+length+4, t.end());
  
  return 0;

}


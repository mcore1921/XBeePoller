#include "IOSample.h"
#include "ThreadQueue.h"

#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iomanip>
#include <pthread.h>

#define HEX( x ) std::setw(2) << std::setfill('0') << std::hex << (int)( x )

//#define DEBUG

std::vector<unsigned char> getPacket(int fd);
void parsePacket(const std::vector<unsigned char>& data);
void* servicePort(void* args);  

ThreadQueue<IOSample> g_inputQueue;

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " <ttyname>" << std::endl;
    return -1;
  }

  std::string portName(argv[1]);
  int ser;
  ser = open(portName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
  if (ser == -1)
  {
    std::cout << "Failed to open port " << portName;
    return -1;
  }

  pthread_attr_t attr;
  pthread_t tid;
  pthread_attr_init(&attr);
  pthread_create(&tid, &attr, &servicePort, (void*)ser);

  while (1)
  {
    IOSample a;
    timespec t;
    t.tv_sec = time(NULL) + (60*60*24*365); // (1 year from today)
    t.tv_nsec = 0;
    g_inputQueue.getMessage(a, &t);
    std::cout << "IOSample read addr:";
    for (int i = 0; i < 7; i++)
      std::cout << HEX(a.getSenderAddress()[i]) << "-";
    std::cout << HEX(a.getSenderAddress()[7]);
    std::cout << ", value " << a.getAnalogValue() << std::endl;
  }

  return 0;
}
    
std::vector<unsigned char> getPacket(int fd)
{
  // Read until we find the start char
  while (1)
  {
    char onechar = 0;
    read(fd, &onechar, 1);
    if (onechar == 0x7e)
      break;
  }

#ifdef DEBUG
  std::cout << "Read header" << std::endl;
#endif
  
  // Now read the size
  int size = 0;
  {
    std::vector<unsigned char> buf(2, '\0');
    int r = 0;
    while (1)
    {
      int v = read(fd, &buf[r], 2-r);
      if (v > 0)
	r += v;
      if (r == 2)
	break;
    }
    size = (buf[0] << 8) + buf[1];
  }

#ifdef DEBUG
  std::cout << "Read size of: " << size << std::endl;
#endif

  // Now read the frame data
  std::vector<unsigned char> data(size, '\0');
  {
    int r = 0;
    while (1)
    {
      int v = read(fd, &data[r], size-r);
      if (v > 0)
	r += v;
#ifdef DEBUG
      if (v > 0)
	std::cout << "Reading data; read # of bytes: " << r << std::endl;
#endif
      if (r == size)
	break;
    }
  }

#ifdef DEBUG
  std::cout << "Read data of size: " << size << std::endl;
#endif

  // Now read the checksum
  unsigned char checksum = 0;
  {
    int r = 0;
    while (1)
    {
      int v = read(fd, &checksum, 1-r);
      if (v > 0)
	r += v;
      if (r == 1)
	break;
    }
  }

  // Now check the checksum
  unsigned char sum = 0;
  for (unsigned int i = 0; i < data.size(); i++)
    sum += data[i];
  sum += checksum;
#ifdef DEBUG
  std::cout << "Checksum val: " << HEX(checksum)
	    << ", actual sum: " << HEX(sum)
	    << std::dec << std::endl;
#endif


  if (sum != 0xFF)
  {
    std::cout << "Checksum error" << std::endl;
    return std::vector<unsigned char>(0);
  }
  else
  {
#ifdef DEBUG
    std::cout << "Read a data frame of size " << data.size() << std::endl;
#endif
    return data;
  }
}

void parsePacket(const std::vector<unsigned char>& data)
{
  if (data[0] == 0x92)
  {
    IOSample a(data);
    std::cout << "IOSample read addr:";
    for (int i = 0; i < 7; i++)
      std::cout << HEX(a.getSenderAddress()[i]) << "-";
    std::cout << HEX(a.getSenderAddress()[7]);
    std::cout << ", value " << a.getAnalogValue() << std::endl;
  }
  else
  {
    std::cout << "Unknown packet type: " << HEX(data[0]) << std::endl;
  }

}

void debugPacketInfo(const unsigned char* data)
{
  int pos = 1;

  std::cout << "Sender's address (64b): ";
  for (int i = 1; i < 8; i++)
    std::cout << HEX(data[pos++]) << "-";
  std::cout << HEX(data[pos++]) << std::endl;
  std::cout << "Sender's address (16b): " << HEX(data[pos++]) 
	    << HEX(data[pos++]) << std::endl;
  std::cout << "Receive Options: " << HEX(data[pos++]) << std::endl;
  std::cout << "Number of Samples: " << HEX(data[pos++]) << std::endl;
  std::cout << "Digital Channel Mask: " << HEX(data[pos++]) << HEX(data[pos++])
	    << std::endl;
  std::cout << "Analog Channel Mask: " << HEX(data[pos++]) << std::endl;
  if (data[13] != 0 || data[14] != 0)
    std::cout << "Digital Samples: " << HEX(data[pos++]) << HEX(data[pos++]) 
	      << std::endl;
  std::cout << "Analog Sample: " << HEX(data[pos++]) << HEX(data[pos++]) 
	    << std::endl;
  std::cout << "Accounted for " << std::dec << pos << " bytes." << std::endl;

}

// Thread function to service the port
void* servicePort(void* arg)
{
  // Expect the file# of the open serial port as arg
  int ser = (int)arg;
  
  while (1)
  {
    std::vector<unsigned char> data = getPacket(ser);
    if (data[0] == 0x92)
    {
      IOSample a(data);
      g_inputQueue.putMessage(a);
    }
    else
    {
      std::cout << "Unknown packet type: " << HEX(data[0]) << std::endl;
    }
  }  
}

#ifndef __IO_SAMPLE_H_DEFINED__
#define __IO_SAMPLE_H_DEFINED__

#include <vector>

class IOSample
{
public:
  IOSample() {}
  IOSample(const unsigned char* senderAddress,
	   int value);
  IOSample(const std::vector<unsigned char>& data);
  IOSample(const unsigned char* data);

  // Returns 0 on success, nonzero on failure
  int setData(const unsigned char* data);

  const unsigned char* getSenderAddress() {return m_senderAddress;}
  // Pick from possibly up to 4 analog values
  // Note that not all of this is implemented yet, but I don't
  // want to have to come back and change things if I end up extending
  // the implementation...
  int getAnalogValue(int selector = 0) {return m_value;}

private:
  unsigned char m_senderAddress[8];
  // Only one value for now, but the public interface supports more
  int m_value;
};


#endif

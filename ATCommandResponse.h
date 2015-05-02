#ifndef __AT_COMMAND_RESPONSE_H_DEFINED__
#define __AT_COMMAND_RESPONSE_H_DEFINED__

#include "Message.h"

#include <vector>
#include <string>

class ATCommandResponse : public Message
{
public:
  virtual ~ATCommandResponse() {}
  virtual const char* type() { return "ATCommandResponse"; }
  // Set up an AT command frame from arguments
  // Note the frame is expected to have the start, length, and checksum
  //   already removed
  ATCommandResponse(std::vector<unsigned char> frame);
  ATCommandResponse() {}

  int setData(std::vector<unsigned char> frame);

  std::vector<unsigned char> getCommand();
  std::vector<unsigned char> getArgData();
  int getArg();
  unsigned char getStatus();
  unsigned char getFrameId();
  
private:
  std::vector<unsigned char> m_command;
  std::vector<unsigned char> m_argData;
  int m_arg;
  unsigned char m_status;
  unsigned char m_frameId;
};


#endif

#ifndef __REMOTE_COMMAND_RESPONSE_H_DEFINED__
#define __REMOTE_COMMAND_RESPONSE_H_DEFINED__

#include "Message.h"

#include <vector>
#include <string>

class RemoteCommandResponse : public Message
{
public:
  virtual ~RemoteCommandResponse() {}
  virtual const char* type() { return "RemoteCommandResponse"; }
  // Set up an AT command frame from arguments
  // Note the frame is expected to have the start, length, and checksum
  //   already removed
  RemoteCommandResponse(std::vector<unsigned char> frame);
  RemoteCommandResponse() {}

  int setData(std::vector<unsigned char> frame);

  std::vector<unsigned char> getCommand();
  virtual std::vector<unsigned char> getAddress();
  std::vector<unsigned char> getArgData();
  int getArg();
  unsigned char getStatus();
  unsigned char getFrameId();
  
private:
  std::vector<unsigned char> m_command;
  std::vector<unsigned char> m_address;
  std::vector<unsigned char> m_argData;
  int m_arg;
  unsigned char m_status;
  unsigned char m_frameId;
};


#endif

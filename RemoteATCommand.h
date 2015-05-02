#ifndef __REMOTE_AT_COMMAND_H_DEFINED__
#define __REMOTE_AT_COMMAND_H_DEFINED__

#include "Message.h"

#include <vector>

class RemoteATCommand : public Message
{
public:
  virtual ~RemoteATCommand() {}
  virtual const char* type() { return "RemoteATCommand"; }
  // Set up an AT command packet from arguments
  // All versions assume 64-bit address
  RemoteATCommand(const char* command,
		  const char* address,
		  const unsigned char options,
		  const unsigned char frameId);
  RemoteATCommand(const std::vector<unsigned char>& command,
		  const std::vector<unsigned char>& address,
		  const unsigned char options,
		  const unsigned char frameId,
		  const std::vector<unsigned char>& arg);

  // Returns a data packet
  virtual std::vector<unsigned char> getPacket();

  virtual std::vector<unsigned char> getAddress()
    { return m_address; }

  std::vector<unsigned char> getCommand()
    { return m_command; }
  int getFrameId()
    { return m_frameId; }
  void setFrameId(int frameId) { m_frameId = frameId; }

private:
  std::vector<unsigned char> m_command;
  std::vector<unsigned char> m_address;
  std::vector<unsigned char> m_arg;
  unsigned char m_options;
  unsigned char m_frameId;
};


#endif

#ifndef __AT_COMMAND_H_DEFINED__
#define __AT_COMMAND_H_DEFINED__

#include "Message.h"

#include <vector>

class ATCommand : public Message
{
public:
  virtual ~ATCommand() {}
  virtual const char* type() { return "ATCommand"; }
  // Set up an AT command packet from arguments
  ATCommand(const char* command,
	    const unsigned char frameId);
  ATCommand(const char* command,
	    const unsigned char frameId,
	    const std::vector<unsigned char>& arg);
  ATCommand(const std::vector<unsigned char>& command,
	    const unsigned char frameId,
	    const std::vector<unsigned char>& arg);

  // Returns a data packet
  virtual std::vector<unsigned char> getPacket();

  std::vector<unsigned char> getCommand() { return m_command; }
  void setFrameId(unsigned char id) { m_frameId = id; }
  int getFrameId() { return m_frameId; }

private:
  std::vector<unsigned char> m_command;
  std::vector<unsigned char> m_arg;
  unsigned char m_frameId;
};


#endif

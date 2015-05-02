#include "ATCommand.h"

ATCommand::ATCommand(const char* command,
		     const unsigned char frameId)
{
  m_command.push_back(command[0]);
  m_command.push_back(command[1]);
  m_frameId = frameId;
}

ATCommand::ATCommand(const char* command,
		     const unsigned char frameId,
		     const std::vector<unsigned char>& arg)
  : m_arg(arg)
  , m_frameId(frameId)
{
  m_command.push_back(command[0]);
  m_command.push_back(command[1]);
}

ATCommand::ATCommand(const std::vector<unsigned char>& command,
		     const unsigned char frameId,
		     const std::vector<unsigned char>& arg)
  : m_command(command)
  , m_arg(arg)
  , m_frameId(frameId)
{
}

// Returns a data packet
std::vector<unsigned char> ATCommand::getPacket()
{
  std::vector<unsigned char> p;
  p.push_back(0x7e); // packet header
  p.push_back(0);    // Create space for the lenght
  p.push_back(0);    // We will come back and set this correctly later
  p.push_back(0x08); // AT Command frame header
  p.push_back(m_frameId);
  p.push_back(m_command[0]);
  p.push_back(m_command[1]);
  for (auto it = m_arg.begin(); it != m_arg.end(); it++)
    p.push_back(*it);
  // Checksum
  unsigned char sum = 0;
  for (unsigned int i = 3; i < p.size(); i++)
    sum += p[i];
  sum = 0xFF - sum;
  p.push_back(sum);

  // Now go fix the length
  int length = p.size() - 4;
  p[1] = (length & 0xFF00) >> 8;
  p[2] = (length & 0x00FF);

  return p;
}

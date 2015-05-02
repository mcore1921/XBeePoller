#include "RemoteATCommand.h"

RemoteATCommand::RemoteATCommand(const char* command,
				 const char* address,
				 const unsigned char options,
				 const unsigned char frameId)
{
  m_command.push_back(command[0]);
  m_command.push_back(command[1]);
  for (int i = 0; i < 8; i++)
    m_address.push_back(address[i]);
  m_options = options;
  m_frameId = frameId;
}

RemoteATCommand::RemoteATCommand(const std::vector<unsigned char>& command,
				 const std::vector<unsigned char>& address,
				 const unsigned char options,
				 const unsigned char frameId,
				 const std::vector<unsigned char>& arg)
  : m_command(command)
  , m_address(address)
  , m_arg(arg)
{
  m_options = options;
  m_frameId = frameId;
}


// Returns a data packet
std::vector<unsigned char> RemoteATCommand::getPacket()
{
  std::vector<unsigned char> p;
  p.push_back(0x7e); // packet header
  p.push_back(0);    // Create space for the lenght
  p.push_back(0);    // We will come back and set this correctly later
  p.push_back(0x17); // Remote AT Command frame header
  p.push_back(m_frameId);
  for (auto it = m_address.begin(); it != m_address.end(); it++)
    p.push_back(*it);
  p.push_back(0xFF); // 16-bit address
  p.push_back(0xFE); // Protocol says to use 0xFFFE if it's unknown
  p.push_back(m_options);
  for (auto it = m_command.begin(); it != m_command.end(); it++)
    p.push_back(*it);
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


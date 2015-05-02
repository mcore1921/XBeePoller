#ifndef __MESSAGE_H_DEFINED__
#define __MESSAGE_H_DEFINED__

#include <vector>

class Message
{
public:
  virtual ~Message() {}
  virtual const char* type() = 0;
  virtual std::vector<unsigned char> getPacket()
    { return std::vector<unsigned char>(); }
  virtual std::vector<unsigned char> getAddress()
    { return std::vector<unsigned char>(); }
};

// Internal message type that is never used outside of this process
class MessageSentAck : public Message
{
public:
  MessageSentAck(int id) : messageId(id) {}
  virtual ~MessageSentAck() {}
  virtual const char* type() { return "MessageSentAck"; }
  int getMessageId() { return messageId; }
private:
  int messageId;
};

#endif

#ifndef __MESSAGE_FACTORY_H_INCLUDED__
#define __MESSAGE_FACTORY_H_INCLUDED__

#include "Message.h"
#include "Config.h"

#include <memory>
#include <vector>

class MessageFactory
{
public:
  MessageFactory(ConfigFile cf);

  // Returns 0 on success, nonzero on failure
  int createMessage(const std::vector<unsigned char>& frame,
		    std::shared_ptr<Message>& msg);

private:
  ConfigFile m_config;
};

#endif

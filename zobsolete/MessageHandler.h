#ifndef __MESSAGE_HANDLER_H_DEFINED__
#define __MESSAGE_HANDLER_H_DEFINED__

#include "Message.h"
#include "Config.h"

#include <memory>

class SQLConnector;
class XBeeConfigManager;

class MessageHandler
{
public:
  MessageHandler(XBeeConfigManager* pManager,
		 ConfigFile cf);
  ~MessageHandler();

  // Returns 0 on success, nonzero on failure
  int handleMessage(std::shared_ptr<Message> pMessage);

private:
  // Returns 0 on success, nonzero on failure
  int handleIOSample(std::shared_ptr<Message> pMessage);
  int handleRemoteCommandResponse(std::shared_ptr<Message> pMessage);
  int handleATCommandResponse(std::shared_ptr<Message> pMessage);

  SQLConnector* m_pSqlConnector;
  XBeeConfigManager* m_pConfigManager;
  ConfigFile m_config;
};

#endif

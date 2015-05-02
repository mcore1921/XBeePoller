#include "Payload.h"

#include <time.h>
#include <iostream>
#include <sstream>

CEXBTherm::Payload::Payload(const std::string& to,
			    const std::string& from,
			    const std::string& subject,
			    const std::string& body)
{
  time_t rawtime = time(NULL);
  struct tm * timeinfo;
  char buffer[256];
  
  timeinfo = localtime(&rawtime);
  
  strftime(buffer, 256, "%a, %d %b %Y %T %z ", timeinfo);
  
  std::stringstream ss;
  char* data;

  ss.str("");
  ss << "Date: " << std::string(buffer) << "\r\n\r\n"; // Don't understand why two newlines, but google seems to corrupt the to: field if I don't do this
  data = new char[ss.str().size()+1];
  data[ss.str().size()] = '\0';
  ss.str().copy(data, ss.str().size());
  m_payload.push_back(data);
  
  ss.str("");
  ss << "To: " << to << "\r\n";
  data = new char[ss.str().size()+1];
  data[ss.str().size()] = '\0';
  ss.str().copy(data, ss.str().size());
  m_payload.push_back(data);  
  
  ss.str("");
  ss << "From: " << from << "\r\n";
  data = new char[ss.str().size()+1];
  data[ss.str().size()] = '\0';
  ss.str().copy(data, ss.str().size());
  m_payload.push_back(data);

  // Use time(NULL) to get mostly unique message id
  ss.str("");
  ss << "Message-ID: <469db4b5-4d58-49b3-9444-23f88235891d-" << time(NULL)
     << "@macore.score.webfactional.com>\r\n";
  data = new char[ss.str().size()+1];
  data[ss.str().size()] = '\0';
  ss.str().copy(data, ss.str().size());
  m_payload.push_back(data);
  
  ss.str("");
  ss << "Subject: " << subject << "\r\n";
  data = new char[ss.str().size()+1];
  data[ss.str().size()] = '\0';
  ss.str().copy(data, ss.str().size());
  m_payload.push_back(data);

  ss.str("");
  ss << "\r\n";
  data = new char[ss.str().size()+1];
  data[ss.str().size()] = '\0';
  ss.str().copy(data, ss.str().size());
  m_payload.push_back(data);
  
  data = new char[body.size()+1];
  data[body.size()] = '\0';
  body.copy(data, body.size());
  m_payload.push_back(data);

  data = new char[1];
  data[0] = '\0';
  m_payload.push_back(data);

}

void CEXBTherm::Payload::cc(const std::string& cc)
{
  std::stringstream ss;
  char* data;

  ss << "Cc: " << cc << "\r\n";
  data = new char[ss.str().size()+1];
  data[ss.str().size()] = '\0';
  ss.str().copy(data, ss.str().size());
  
  for (std::vector<const char*>::iterator it = m_payload.begin();
       it != m_payload.end(); it++)
  {
    std::string t(*it);
    if (t.find("To:") != std::string::npos)
    {
      m_payload.insert(it+1, data);
      return;
    }
  }
}

void CEXBTherm::Payload::dump(std::ostream& os)
{
  const char** p = get();
  for (unsigned int i = 0; i < m_payload.size(); i++)
    os << p[i];
  
}

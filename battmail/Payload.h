#ifndef __b758716c_PAYLOAD_H_DEFINED__
#define __b758716c_PAYLOAD_H_DEFINED__

#include <vector>
#include <string>
#include <ostream>

namespace CEXBTherm {

class Payload 
{
public:
  // Initialize Date to now
  Payload(const std::string& to,
	  const std::string& from,
	  const std::string& subject,
	  const std::string& body);

  void cc(const std::string& cc);

  const char** get() {return m_payload.data();}

  void dump(std::ostream& os);

private:
  std::vector<const char*> m_payload;
};

} // end namespace

#endif

#ifndef __FORMAT_FOR_HTML_H_INCLUDED__
#define __FORMAT_FOR_HTML_H_INCLUDED__

#include "DataForHtml.h"
#include <string>

class FormatForHtml
{
public:
  int readTemplate(std::string filename);

  int doFormatting(const DataForHtml& dfh);

  std::string getOutput() {return m_output;}

private:
  void doSimpleStuff(const DataForHtml& dfh);
  void expandColumnLoops(const DataForHtml& dfh);
  void fillData(const DataForHtml& dfh);

  std::string m_template;  // Contents of the template file
  std::string m_output; // Results of formatting
};

#endif

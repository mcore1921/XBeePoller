#include "FormatForHtml.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

const std::string FER= "$foreachrow\n";
const std::string EFER="$endforeachrow\n";
const std::string RTS= "$rowtimeslash";
const std::string RTD= "$rowtimedash";
const std::string RCV= "$rowcolval";
const std::string RCVI="$rcvinstance";
const std::string FEC= "$foreachcol\n";
const std::string EFEC="$endforeachcol\n";
const std::string CNAM="$colname";
const std::string CNUM="$colnum";
const std::string CCV= "$curcolval";
const std::string DEL= "$delimeter";
const std::string HDTM="$humandatetime";
const std::string LDTM="$lastdatatime";


int FormatForHtml::readTemplate(std::string filename)
{
  std::ifstream t(filename.c_str());
  if (!t) return -1;
  std::stringstream ss;
  ss << t.rdbuf();
  m_template = ss.str();
  return 0;
}

int FormatForHtml::doFormatting(const DataForHtml& dfh)
{

  m_output = m_template;

  doSimpleStuff(dfh);
  expandColumnLoops(dfh);
  fillData(dfh);

  return 0;
}

void FormatForHtml::doSimpleStuff(const DataForHtml& dfh)
{
  while (m_output.find(HDTM) != std::string::npos)
  {
    m_output.replace(m_output.find(HDTM),
		     HDTM.length(),
		     dfh.m_longtime);
  }
  while (m_output.find(LDTM) != std::string::npos)
  {
    m_output.replace(m_output.find(LDTM),
		     LDTM.length(),
		     dfh.m_lastDataTime);
  }
}

void FormatForHtml::expandColumnLoops(const DataForHtml& dfh)
{
  while (m_output.find(FEC) != std::string::npos)
  {
    // Find appropriate bookends for the first column loop
    // NOTE: This is currently not able to support nested column loops!

    // Find and then remove the block that we're going to repeat.
    int start = m_output.find(FEC);
    int end = m_output.find(EFEC);
    int startplus = start + FEC.length();
    int endplus = end + EFEC.length();
    if ((unsigned int) end == std::string::npos)
    {
      std::cout << "Error: Imbalanced column loops." << std::endl;
      std::cout << "       Look for every $foreachcol to have an $endforeachcol" << std::cout;
      return;
    }
    std::string repeat = m_output.substr(startplus,
					 end-startplus);

    m_output.erase(start, endplus-start);

    for(std::vector<ColForHtml>::const_iterator it = dfh.m_cols.begin();
	it != dfh.m_cols.end(); it++)
    {
      std::string thisloop = repeat;
      while (thisloop.find(CNAM) != std::string::npos)
      {
	thisloop.replace(thisloop.find(CNAM),
			 std::string(CNAM).size(),
			 it->m_colName);
      }
      while (thisloop.find(CNUM) != std::string::npos)
      {
	std::stringstream ss;
	ss << it->m_refNum;
	thisloop.replace(thisloop.find(CNUM),
			 std::string(CNUM).size(),
			 ss.str());
      }
      while (thisloop.find(CCV) != std::string::npos)
      {
	std::stringstream ss;
	ss << std::fixed << std::setprecision(1) << it->m_curColValue;
	thisloop.replace(thisloop.find(CCV),
			 std::string(CCV).size(),
			 ss.str());
      }
      while (thisloop.find(DEL) != std::string::npos)
      {
	int pos = thisloop.find(DEL);
	int startquote = thisloop.find("\"",pos);
	int endquote = thisloop.find("\"", startquote+1);
	std::string del = thisloop.substr(startquote+1, 
					  endquote-(startquote+1));
	int enddel = thisloop.find(")", endquote);
	
	thisloop.erase(pos, enddel+1-pos);
	if (it+1 != dfh.m_cols.end())
	  thisloop.insert(pos, del);
      }
      while (thisloop.find(RCV) != std::string::npos)
      {
	std::stringstream ss;
	ss << RCVI << "(" << it->m_refNum << ")";
	thisloop.replace(thisloop.find(RCV),
			 RCV.length(),
			 ss.str());
      }
			 
      m_output.insert(start, thisloop);
      start += thisloop.length();
    }
  }
    
}

void FormatForHtml::fillData(const DataForHtml& dfh)
{
  while (m_output.find(FER) != std::string::npos)
  {
    // Find and then remove the block that we're going to repeat.
    int start = m_output.find(FER);
    int end = m_output.find(EFER);
    int startplus = start + FER.length();
    int endplus = end + EFER.length();
    if ((unsigned int) end == std::string::npos)
    {
      std::cout << "Error: Imbalanced row loops." << std::endl;
      std::cout << "       Look for every $foreachrow to have an $endforeachrow" << std::cout;
      return;
    }
    std::string repeat = m_output.substr(startplus,
					 end-startplus);

    m_output.erase(start, endplus-start);

    for(std::vector<RowForHtml>::const_iterator it = dfh.m_rows.begin();
	it != dfh.m_rows.end(); it++)
    {
      std::string thisloop = repeat;
      while (thisloop.find(RTS) != std::string::npos)
      {
	thisloop.replace(thisloop.find(RTS),
			 std::string(RTS).size(),
			 it->m_time);
      }	
      while (thisloop.find(RTD) != std::string::npos)
      {
	std::string dash = it->m_time;
	while (dash.find("/") != std::string::npos)
	  dash.replace(dash.find("/"), 1, "-");
	thisloop.replace(thisloop.find(RTD),
			 std::string(RTD).size(),
			 dash);
      }	
      while (thisloop.find(RCVI) != std::string::npos)
      {
	int pos = thisloop.find(RCVI);
	int startparen = thisloop.find("(",pos);
	int endparen = thisloop.find(")", startparen+1);
	std::string refnumstr = thisloop.substr(startparen+1, 
						endparen-(startparen+1));
	std::stringstream ss(refnumstr);
	int refnum;
	ss >> refnum;

	thisloop.erase(pos, endparen+1-pos);
	if (it->m_nullmask[refnum] == 0)
	{
	  thisloop.insert(pos, "null");
	}
	else
	{
	  std::stringstream ss;
	  ss << std::fixed << std::setprecision(1) << it->m_values[refnum];
	  thisloop.insert(pos, ss.str());
	}
      }
			 
      m_output.insert(start, thisloop);
      start += thisloop.length();
    }
  }

}



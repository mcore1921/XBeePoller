#ifndef __DATA_FOR_HTML_H_INCLUDED__
#define __DATA_FOR_HTML_H_INCLUDED__

#include <vector>
#include <string>
#include <map>

// Note that currently we only support doubles as the data type for
// the table being displayed...

class RowForHtml
{
public:
  std::string m_time;
  std::vector<double> m_values;
  std::vector<int> m_nullmask;  // 1 when a col is not null, 0 when it's null
};

class ColForHtml
{
public:
  int m_refNum;
  int m_colNum;
  std::string m_colName;
  double m_curColValue;
};

class DataForHtml
{
public:
  std::string m_longtime;
  std::string m_lastDataTime;
  std::vector<RowForHtml> m_rows;
  std::vector<ColForHtml> m_cols;
};


#endif

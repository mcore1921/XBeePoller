#include "GenDataForHtml.h"


int genDataForHtml(sql::ResultSet *res,
		   DataForHtml& data)
{
  // Make a new one, we'll set the input to this at the end
  DataForHtml d; 

  // Some working information:
  unsigned int numcols;

  sql::ResultSetMetaData * meta;
  meta = res->getMetaData();
  numcols = meta->getColumnCount();

  res->last();
  int refcount = 0;
  for (unsigned int i = 0; i < numcols; i++)
  {
    ColForHtml c;
    c.m_refNum = refcount++;
    c.m_colNum = refcount+1; // Because the 0th col is the time
    c.m_colName = meta->getColumnName(i+1);
    c.m_curColValue = res->getDouble(i+1);
    // Special handling - leave the "time" column off here
    if (c.m_colName == "time")
      refcount--;
    // Special handling - leave the "id" column off here
    else if (c.m_colName == "id")
      refcount--;
    else
      d.m_cols.push_back(c);
  }
  
  res->beforeFirst();
  while (res->next())
  {
    RowForHtml r;
    r.m_time = res->getString("time");
    while (r.m_time.find("-") != std::string::npos)
      r.m_time.replace(r.m_time.find("-"), 1, "/");
    for (unsigned int i = 1; i < numcols+1; i++)
    {
      // Special handling to skip time and ID columns
      if (meta->getColumnName(i) == "time" ||
	  meta->getColumnName(i) == "id")
	continue;
      
      if (!res->isNull(i)) 
      {
	r.m_values.push_back(res->getDouble(i));
	r.m_nullmask.push_back(1);
      }
      else
      {
	r.m_values.push_back(0);
	r.m_nullmask.push_back(0);
      }
    }
    d.m_rows.push_back(r);
  }

  time_t now = time(NULL);
  std::string timestring = ctime(&now);
  // Strip that trailing newline...
  d.m_longtime = timestring.substr(0, timestring.length()-1);

  d.m_lastDataTime = d.m_rows[d.m_rows.size()-1].m_time;

  data = d;
  return 0;
}

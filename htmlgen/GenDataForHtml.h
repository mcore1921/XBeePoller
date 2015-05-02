#ifndef __GEN_DATA_FOR_HTML_H_INCLUDED__
#define __GEN_DATA_FOR_HTML_H_INCLUDED__
#include "DataForHtml.h"
#include "cppconn/resultset.h" 

// Returns 0 on success, nonzero on failure
int genDataForHtml(sql::ResultSet *res,
		   DataForHtml& data);


#endif

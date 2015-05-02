#ifndef __638ef594_SEND_MAIL_H_INCLUDED__
#define __638ef594_SEND_MAIL_H_INCLUDED__

class ConfigFile;

// Blocks for some time, hopefully also sends a mail
int sendmail(const char* TO, const char* FROM, 
	     const char* CC, const char** p,
	     ConfigFile* pConfig);

#endif

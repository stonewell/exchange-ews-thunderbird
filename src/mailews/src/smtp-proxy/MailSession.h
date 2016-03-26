#ifndef _MAIL_SESSION_INCLUDED_
#define _MAIL_SESSION_INCLUDED_

#include "globals.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include "nsIFile.h"
#include "nsIOutputStream.h"
#include "nsIMsgIncomingServer.h"

class CMailSession
{
public:
	CMailSession(int client_soc, nsIMsgIncomingServer * pIncomingServer)
	{
		m_nStatus=SMTP_STATUS_INIT;
		m_socConnection=client_soc;
		m_nRcptCount=0;
        m_pIncomingServer = pIncomingServer;
	}
	

	int m_socConnection;
private:
    nsCOMPtr<nsIFile> m_File;
    nsCOMPtr<nsIOutputStream> m_OutputStream;
	unsigned int m_nStatus;
	int m_nRcptCount;
    nsCString m_DataBuffer;
    nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;

	int ProcessHELO(char *buf, int len);
	int ProcessRCPT(char *buf, int len);
	int ProcessMAIL(char *buf, int len);
	int ProcessRSET(char *buf, int len);
	int ProcessNOOP(char *buf, int len);
	int ProcessQUIT(char *buf, int len);
	int ProcessDATA(char *buf, int len);
	int ProcessNotImplemented(bool bParam=false);
	int ProcessDATAEnd(void);
	bool CreateNewMessage(void);
    int SendMail();
public:
	int ProcessCMD(char *buf, int len);
	int SendResponse(int nResponseType, const char * buf = NULL);
};
#endif //_MAIL_SESSION_INCLUDED_

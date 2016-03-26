#include "stdafx.h"
#include "MailSession.h"

#ifdef _WIN32
#include <errno.h>
#include <winsock2.h>
typedef int socklen_t;
#define strncasecmp strnicmp
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#define INVALID_SOCKET (-1)
#define closesocket close
#endif

#include "MailEwsLog.h"
#include <string>

#include "MailEwsMsgUtils.h"
#include "nsNetUtil.h"

int CMailSession::ProcessCMD(char *buf, int len)
{
	if(m_nStatus==SMTP_STATUS_DATA)
	{
		return ProcessDATA(buf,len);
	}
	else if(strncasecmp(buf,"HELO",4)==0)
	{
		return ProcessHELO(buf, len);
	}
	else if(strncasecmp(buf,"EHLO",4)==0)
	{
		return ProcessHELO(buf, len);
	}
	else if(strncasecmp(buf,"MAIL",4)==0)
	{
		return ProcessMAIL(buf, len);
	}
	else if(strncasecmp(buf,"RCPT",4)==0)
	{
		return ProcessRCPT(buf, len);
	}
	else if(strncasecmp(buf,"DATA",4)==0)
	{
		return ProcessDATA(buf,len);
	}
	else if(strncasecmp(buf,"QUIT",4)==0)
	{
		return ProcessQUIT(buf,len);
	}
	else 
		return ProcessNotImplemented(false);
}

int CMailSession::ProcessHELO(char *buf, int len)
{
	mailews_logger << ("Received HELO") << std::endl;

	m_nStatus=SMTP_STATUS_HELO;
	m_nRcptCount=0;

	CreateNewMessage();

	return SendResponse(250);
}

/*
  RCPT
  S: 250, 251
  F: 550, 551, 552, 553, 450, 451, 452
  E: 500, 501, 503, 421
*/

int CMailSession::ProcessRCPT(char *, int)
{
	if(m_nStatus!=SMTP_STATUS_HELO)
	{
		//503 Bad Command
		return SendResponse(503);
	}

	return SendResponse(250);
}


/*
  MAIL
  S: 250
  F: 552, 451, 452
  E: 500, 501, 421
*/
int CMailSession::ProcessMAIL(char *, int)
{
	if(m_nStatus!=SMTP_STATUS_HELO)
	{
		return SendResponse(503);
	}

	return SendResponse(250);
}

int CMailSession::ProcessRSET(char *buf, int len)
{
	mailews_logger << ("Received RSET\n------------------------------------\n")
	               << std::endl;
	m_nRcptCount=0;
	m_nStatus=SMTP_STATUS_HELO;

	CreateNewMessage();

	return SendResponse(220);
}

int CMailSession::ProcessNOOP(char *buf, int len)
{
	mailews_logger << ("Received NOOP\n")
	               << std::endl;
	return SendResponse(220);
}

int CMailSession::ProcessQUIT(char *buf, int len)
{
	//No need to reset cause we are destroying the thread
	mailews_logger << ("Quit session\n")
	               << std::endl;
	return SendResponse(221);
}

int CMailSession::ProcessDATA(char *buf, int len)
{
	if(m_nStatus!=SMTP_STATUS_DATA)
	{
		m_nStatus=SMTP_STATUS_DATA;
		return SendResponse(354);
	}

	//good client should send term in separate line
    if(!strncmp(buf, SMTP_DATA_TERMINATOR, strlen(SMTP_DATA_TERMINATOR)))
	{
		mailews_logger << ("Data End\n")
		               << std::endl;
		m_nStatus=SMTP_STATUS_DATA_END;
		return ProcessDATAEnd();
	}

    m_DataBuffer.Append(buf, len);

    if (m_OutputStream) {
        uint32_t bytesWritten;
        while(len > 0) {
            (void) m_OutputStream->Write(buf,
                                         len,
                                         &bytesWritten);
            len -= bytesWritten;
        }
    }

	return 220;
}

int CMailSession::ProcessNotImplemented(bool bParam)
{
	if (bParam) 
	{
		return SendResponse(504);
	}
	else return SendResponse(502);
}

extern bool IsSmtpProxyRunning();

int WriteToNetwork(int sockfd, void *buf, size_t len, int flags) {
    int bytes_write = 0;
    while(IsSmtpProxyRunning() && len > 0) {
        fd_set wset;
        struct timeval timeout;

        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        
        FD_ZERO(&wset);

        FD_SET(sockfd, &wset);

        int c = select(sockfd + 1, NULL, &wset, NULL, &timeout);

        if (c <= 0) {//timeout
            continue;
        }
    
        int cc = send(sockfd,
#ifdef _WIN32
                      (const char *)buf,
#else                      
                      buf,
#endif                      
                      len,
                      flags);

        if (cc <= 0)
            break;
        
        len -= cc;
        bytes_write += cc;
    }
    
    return bytes_write;
}

int CMailSession::SendResponse(int nResponseType, const char * extrabuf)
{
	char buf[4096];
	int len;
	if(nResponseType==220)
		sprintf(buf,"220 localhost Welcome to localhost mailews \r\n");
	else if(nResponseType==221)
		strcpy(buf,"221 Service closing transmission channel\r\n");
	else if (nResponseType==250) 
		strcpy(buf,"250 OK\r\n");
	else if (nResponseType==354)
		strcpy(buf,"354 Start mail input; end with <CRLF>.<CRLF>\r\n");
	else if(nResponseType==501)
		strcpy(buf,"501 Syntax error in parameters or arguments\r\n");		
	else if(nResponseType==502)
		strcpy(buf,"502 Command not implemented\r\n");		
	else if(nResponseType==503)
		strcpy(buf,"503 Bad sequence of commands\r\n");		
	else if(nResponseType==550)
		strcpy(buf,"550 No such user\r\n");
	else if(nResponseType==551)
		strcpy(buf,"551 User not local. Can not forward the mail\r\n");
	else if(nResponseType==552)
		sprintf(buf,"552 Can not send the mail, detail info:%s\r\n",
               (extrabuf ? extrabuf : ""));
	else
		sprintf(buf,"%d No description\r\n",nResponseType);

	len=(int)strlen(buf);

	mailews_logger << "Sending: "
	               << buf
	               << std::endl;

	WriteToNetwork(m_socConnection,buf,len,0);

	return nResponseType;
}

int CMailSession::ProcessDATAEnd(void)
{
	m_nStatus=SMTP_STATUS_HELO;

    if (m_OutputStream) {
        m_OutputStream->Close();
    }

    if (m_File) {
        nsCOMPtr <nsIFile> tmpFile1;
        m_File->Clone(getter_AddRefs(tmpFile1));

        m_File = do_QueryInterface(tmpFile1);
    }
    
	return SendMail();
}

bool CMailSession::CreateNewMessage(void)
{
    m_DataBuffer.AssignLiteral("");
    
    nsresult rv = MailEwsMsgCreateTempFile("ews_out_mail.tmp",
                                           getter_AddRefs(m_File));
    NS_ENSURE_SUCCESS(rv, false);

    rv = NS_NewLocalFileOutputStream(getter_AddRefs(m_OutputStream),
                                     m_File,
                                     PR_WRONLY | PR_CREATE_FILE,
                                     00600);
    NS_ENSURE_SUCCESS(rv, false);
    
	return true;
}

int CMailSession::SendMail() {
    nsCString err_msg;
    
    int ret = msg_server_send_mail(m_pIncomingServer,
                               m_DataBuffer,
                               err_msg);

    if (ret != EWS_SUCCESS) {
        //replace all \r and \n
        for(PRUint32 i=0;i < err_msg.Length(); i++) {
            if (err_msg[i] == '\r' ||
                err_msg[i] == '\n') {
                err_msg.Replace(i, i + 1, ' ');
            }
        }
        
        return SendResponse(552, err_msg.get());
    }

    return SendResponse(250);
}

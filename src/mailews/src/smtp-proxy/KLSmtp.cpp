#include "stdafx.h"
#include "MailSession.h"
#include <vector>
#include "libews.h"
#include "IMailEwsService.h"
#include "nsCOMPtr.h"

#include "MailEwsLog.h"

#ifdef _WIN32
#include <errno.h>
#include <winsock2.h>
typedef int socklen_t;
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

typedef std::vector<char> Buffer;
const int BufferSize = 2050;

static bool g_SmtpProxyRunning;

#define _SOCKNONBLOCK(fd) fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)|O_NONBLOCK);

void StopSmtpProxy() {
    g_SmtpProxyRunning = false;
}

bool IsSmtpProxyRunning() {
    return g_SmtpProxyRunning;
}

int RecvFromNetwork(int sockfd, void *buf, size_t len, int flags) {
    while(g_SmtpProxyRunning) {
        fd_set rset;
        struct timeval timeout;

        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        
        FD_ZERO(&rset);

        FD_SET(sockfd, &rset);

        int c = select(sockfd + 1, &rset, NULL, NULL, &timeout);

        if (c <= 0) {//timeout
            continue;
        }

        return recv(sockfd,
#ifdef _WIN32
                    (char *)buf,
#else
                    buf,
#endif                    
                    len,
                    flags);
    }
    
    return 0;
}

int ConnectionThread(CMailSession *pSession)
{
	int len;
	Buffer buf;
	
	pSession->SendResponse(220);	

	do {
        if (!g_SmtpProxyRunning)
            break;
        
        const size_t oldSize = buf.size();
        buf.resize(oldSize + BufferSize);
        
        len = RecvFromNetwork(pSession->m_socConnection,
                   &buf[oldSize],
                   BufferSize,
                   0);

        if (len <= 0)
            break;

        buf.resize(oldSize + len);
        Buffer::iterator it1, it2;

        while(true) {
            it1 = buf.begin();
            it2 = it1 + 1;
            
            while((it1 != buf.end()) && (it2 != buf.end())) {
                if ((*it1 == '\r') && (*it2 == '\n'))
                    break;
                it1++;
                it2++;
            }

            if ((it1 != buf.end()) && (it2 != buf.end())) {
                if(221 == pSession->ProcessCMD(&buf[0], it2 - buf.begin() + 1)) {
                    return 0;
                }
                buf.erase(buf.begin(), it2 + 1);
            } else {
                break;
            }
        } //get all lines

        if (!g_SmtpProxyRunning)
            break;
	} while(len > 0);
    
	return -1;
}

void AcceptConnections(int server_soc, nsIMsgIncomingServer * incomingServer)
{
	CMailSession *pSession;
	int soc_client;
    nsCOMPtr<nsIMsgIncomingServer> server(incomingServer);

    g_SmtpProxyRunning = true;
    
	while(g_SmtpProxyRunning)
	{
		sockaddr nm;
		socklen_t len=sizeof(sockaddr);
        fd_set rset;
        fd_set wset;
        struct timeval timeout;

        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        
        FD_ZERO(&rset);
        FD_ZERO(&wset);

        FD_SET(server_soc, &rset);
        FD_SET(server_soc, &wset);

        int c = select(server_soc + 1, &rset, &wset, NULL, &timeout);

        if (c <= 0)//timeout
            continue;

		if(INVALID_SOCKET == (soc_client=accept(server_soc,&nm,&len)))
		{
			mailews_logger << "Error: Invalid Soceket returned by accept(): "
			               << errno
			               << std::endl;
            continue;
		}

		pSession=new CMailSession(soc_client, server);

        ConnectionThread(pSession);

        // ewsService->SendMailWithMimeContent(&pSession->m_DataBuffer[0],
        //                                     pSession->m_DataBuffer.size(),
        //                                     nullptr);

        delete pSession;
        closesocket(soc_client);
	}
}


#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgIncomingServer.h"
#include "nsArrayUtils.h"
#include "nsMsgFolderFlags.h"
#include "msgCore.h"
#include "nsIMsgFolder.h"
#include "nsISmtpService.h"
#include "nsISmtpServer.h"
#include "nsMsgCompCID.h"
#include "nsIMsgAccountManager.h"
#include "nsMsgBaseCID.h"

#include "libews.h"

#include "MailEwsSmtpProxyTask.h"
#include "MailEwsErrorInfo.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"

#include "MailEwsLog.h"
#include "MailEwsCommon.h"

#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>

#define INVALID_SOCKET (-1)

#define closesocket close
#else
#include <errno.h>
#include <winsock2.h>

typedef int socklen_t;
#endif

extern void AcceptConnections(int server_soc, nsIMsgIncomingServer * server);

static bool get_sock_port(int sock, int & port) {
	struct sockaddr_in name;
	socklen_t namelen = sizeof(name);
	int err = getsockname(sock, (struct sockaddr*) &name, &namelen);

	if (err == -1) {
		return false;
	}

	port = ntohs(name.sin_port);

	return true;
}

class SmtpProxyDoneTask : public MailEwsRunnable
{
public:
	SmtpProxyDoneTask(nsIRunnable * runnable,
                      nsIMsgIncomingServer * pIncomingServer,
                      int port,
	                  nsISupportsArray * ewsTaskCallbacks)
		: MailEwsRunnable(ewsTaskCallbacks)
        , m_Runnable(runnable)
        , m_pIncomingServer(pIncomingServer)
        , m_Port(port)
	{
	}

	NS_IMETHOD Run() {
        nsresult rv = NS_OK;
        //update identity outgoin server
        rv = UpdateOutgoingServer(m_Port);
        NS_FAILED_WARN(rv);
	
		NotifyEnd(m_Runnable, NS_SUCCEEDED(rv) ? EWS_SUCCESS : EWS_FAIL);

		return NS_OK;
	}

private:
	nsCOMPtr<nsIRunnable> m_Runnable;
	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
	int m_Port;

    NS_IMETHODIMP UpdateOutgoingServer(int port) {
        nsresult rv = NS_OK;
	
        nsCOMPtr<nsIMsgAccountManager> accountMgr = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIMsgIdentity> identity;
        rv = accountMgr->GetFirstIdentityForServer(m_pIncomingServer,
                                                   getter_AddRefs(identity));
        NS_ENSURE_SUCCESS(rv, rv);
	
        if (identity)
        {
            nsCOMPtr<nsISmtpService> smtpService =
                    do_GetService(NS_SMTPSERVICE_CONTRACTID, &rv);
            NS_ENSURE_SUCCESS(rv, rv);

            nsCOMPtr<nsISmtpServer> defaultServer;
            nsCOMPtr<nsISmtpServer> server;

            rv = smtpService->GetDefaultServer(getter_AddRefs(defaultServer));
            NS_ENSURE_SUCCESS(rv, rv);

            rv = smtpService->GetServerByIdentity(identity,
                                                  getter_AddRefs(server));
            NS_ENSURE_SUCCESS(rv, rv);

            char * key;
            char * defaultKey;

            if (defaultServer) {
                rv = defaultServer->GetKey(&defaultKey);
                NS_ENSURE_SUCCESS(rv, rv);
            }

            if (server) {
                rv = server->GetKey(&key);
                NS_ENSURE_SUCCESS(rv, rv);
            }
		
            if (!server || !defaultServer || !strcmp(key, defaultKey)) {
                //create new smtp server
                rv = smtpService->CreateServer(getter_AddRefs(server));
                NS_ENSURE_SUCCESS(rv, rv);
                rv = server->GetKey(&key);
                NS_ENSURE_SUCCESS(rv, rv);
                identity->SetSmtpServerKey(nsCString(key));
            }

            server->SetHostname(nsCString("localhost"));
            server->SetPort(port);
        }
	
        return NS_OK;
    }
};

class DLL_LOCAL SmtpProxyRunTask : public nsRunnable
{
private:
	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
	int m_Socket;
	
public:
	SmtpProxyRunTask(nsIMsgIncomingServer * pIncomingServer,
	                 int socket)
			: m_pIncomingServer(pIncomingServer)
			, m_Socket(socket) {
	}

protected:
	virtual ~SmtpProxyRunTask() {
	}

	NS_IMETHOD Run() {
		AcceptConnections(m_Socket, m_pIncomingServer);
		closesocket(m_Socket);
		return NS_OK;
	}
};

SmtpProxyTask::SmtpProxyTask(nsIMsgIncomingServer * pIncomingServer,
                             nsIThread * smtpProxyThread,
                             nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
	, m_pIncomingServer(pIncomingServer)
	, m_SmtpProxyThread(smtpProxyThread) {
}

SmtpProxyTask::~SmtpProxyTask() {
}

NS_IMETHODIMP SmtpProxyTask::Run() {
	char buf[255] = {0};
	sockaddr_in soc_addr;
	struct hostent *lpHost = NULL;
	int soc = INVALID_SOCKET;
	int port = 0;
	nsCOMPtr<nsIRunnable> runnable;
	
	if (EWS_FAIL == NotifyBegin(this)) {
		return NS_OK;
	}
	
	//init
	soc = socket(PF_INET, SOCK_STREAM, 0) ;

	if(soc == INVALID_SOCKET)
	{
		sprintf(buf, "Error: Invalid socketQuiting with error code: %d\n",errno);
		goto Error;
	}

	lpHost = gethostbyname("localhost");

	soc_addr.sin_family=AF_INET;
	soc_addr.sin_port=htons(0);
	memmove((char *)&soc_addr.sin_addr.s_addr,
          (char *)lpHost->h_addr, 
	      lpHost->h_length);

	//bind
	if(bind(soc,(const struct sockaddr*)&soc_addr,sizeof(soc_addr)))
	{
		sprintf(buf, "Error: Can not bind socket. Another server running? Quiting with error code: %d\n",errno);

		goto Error;
	}

	if (!get_sock_port(soc, port)) {
		sprintf(buf, "Error: unable to get listen port");
		goto Error;
	}

	if(-1==listen(soc, SOMAXCONN))
	{
		sprintf(buf, "Error: Can not listen to socket, Quiting with error code: %d\n",errno);

		goto Error;
	}

    //update identity
    runnable = new SmtpProxyDoneTask(this,
                                     m_pIncomingServer,
                                     port,
                                     m_pEwsTaskCallbacks);
	NS_DispatchToMainThread(runnable);
    
	//run
	runnable = new SmtpProxyRunTask(m_pIncomingServer,
	                                soc);
	return m_SmtpProxyThread->Dispatch(runnable, nsIEventTarget::DISPATCH_NORMAL);
	
Error:
	NotifyError(this, EWS_FAIL, buf);

	if (soc != INVALID_SOCKET) closesocket(soc);

	return NS_ERROR_FAILURE;	
}


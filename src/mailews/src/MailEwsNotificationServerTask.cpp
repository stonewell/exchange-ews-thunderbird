#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgIncomingServer.h"
#include "nsArrayUtils.h"
#include "nsMsgFolderFlags.h"
#include "msgCore.h"
#include "nsIMsgFolder.h"

#include "libews.h"

#include "MailEwsNotificationServerTask.h"
#include "MailEwsErrorInfo.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"

#include "MailEwsLog.h"

NotificationServerTask::NotificationServerTask(MailEwsSubscriptionCallback * callback,
                                               nsIMsgIncomingServer * pIncomingServer,
                                               nsIThread * notificationThread,
                                               nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
    , m_Callback(callback)
	, m_pIncomingServer(pIncomingServer)
	, m_NotificationThread(notificationThread) {
}

NotificationServerTask::~NotificationServerTask() {
}

NS_IMETHODIMP NotificationServerTask::Run() {
	char * err_msg = NULL;

	if (EWS_FAIL == NotifyBegin(this)) {
		return NS_OK;
	}
	
	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer));

	ews_session * session = NULL;

	nsCOMPtr<IMailEwsService> ewsService;
	nsresult rv = ewsServer->GetService(getter_AddRefs(ewsService));

	nsresult rv1 = ewsService->GetNewSession(&session);
    if (NS_SUCCEEDED(rv)
	    && NS_SUCCEEDED(rv1)
	    && session) {
        //Create notification server
        char * url = NULL;

        rv = CreateNotificationServer(ewsService,
                                      session, &url, &err_msg);

        if (NS_SUCCEEDED(rv)) {
	        m_Callback->SetUrl(url);
        }

        NotifyError(this, NS_SUCCEEDED(rv) ? EWS_SUCCESS : EWS_FAIL, err_msg);
			
        if (err_msg) free(err_msg);
    } else {
        NotifyError(this, session ? EWS_FAIL : 401, err_msg);
    }

    ewsService->ReleaseSession(session);
	return NS_OK;
}

class DLL_LOCAL NotificationServerRunTask : public nsRunnable
{
public:
    nsCOMPtr<IMailEwsService> m_EwsService;
    ews_notification_server * m_Server;
	MailEwsSubscriptionCallback * m_Callback;
    NotificationServerRunTask(IMailEwsService * ews_service,
                              MailEwsSubscriptionCallback *callback,
                              ews_notification_server * server)
            : m_EwsService(ews_service)
            , m_Server(server)
            , m_Callback(callback) {
	}

protected:
    virtual ~NotificationServerRunTask() {
    }

	NS_IMETHOD Run() {
        if (!m_Server) {
            mailews_logger << "server is null" << std::endl;
        } else {
            int retcode = ews_notification_server_run(m_Server);
            if (retcode != EWS_SUCCESS) {
                mailews_logger << "server run fail" << std::endl;
            }

            m_Callback->SetNotificationServer(NULL);

            m_EwsService->StartNotificationServer(nullptr);
        }

		return NS_OK;
	}
};

NS_IMETHODIMP NotificationServerTask::CreateNotificationServer(IMailEwsService * ews_service,
                                                               ews_session * session,
                                                               char ** pp_url,
                                                               char ** err_msg) {
    ews_notification_server * server = NULL;
    int retcode = EWS_SUCCESS;
    nsCOMPtr<nsIRunnable> resultrunnable;
    
    server = ews_create_notification_server(session,
                                            m_Callback->GetEwsCallback(),
                                            NULL);
  
    retcode = ews_notification_server_bind(server);
    if (retcode != EWS_SUCCESS) {
	    if (err_msg)
		    *err_msg = strdup("server bind fail");
        goto END;
    }

    retcode = ews_notification_server_get_url(server, pp_url);
    if (retcode != EWS_SUCCESS) {
	    if (err_msg)
		    *err_msg = strdup("server get url fail");
        goto END;
    }

    m_Callback->SetNotificationServer(server);
    resultrunnable =
            new NotificationServerRunTask(ews_service, m_Callback, server);
    
    return m_NotificationThread->Dispatch(resultrunnable,
		                         nsIEventTarget::DISPATCH_NORMAL);
END:
    if (server)
        ews_free_notification_server(server);

    return NS_ERROR_FAILURE;
}


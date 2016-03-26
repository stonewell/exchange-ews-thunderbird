#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgIncomingServer.h"
#include "nsArrayUtils.h"

#include "libews.h"

#include "MailEwsSyncFolderTask.h"
#include "MailEwsErrorInfo.h"
#include "MailEwsSyncFolderUtils.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"

#include "MailEwsLog.h"

MailEwsTaskCallback::MailEwsTaskCallback()
	:m_pErrorInfo(new MailEwsErrorInfo) {
}

class SyncFolderDoneTask : public MailEwsRunnable
{
public:
	SyncFolderDoneTask(SyncFolderCallback * pTaskCallback,
	                   int result,
	                   nsISupportsArray * ewsTaskCallbacks,
	                   nsIRunnable * runnable)
		: MailEwsRunnable(ewsTaskCallbacks)
		, m_pTaskCallback(pTaskCallback)
		, m_Runnable(runnable)
		, m_Result(result)
	{
	}

	NS_IMETHOD Run() {
        mailews_logger << "sync folders done Task running"
                       << std::endl;
		if (m_pTaskCallback) {
            m_pTaskCallback->GetErrorInfo()->SetErrorCode(m_Result);
			m_pTaskCallback->TaskDone();
        }

		NotifyEnd(m_Runnable, m_Result);
		
        mailews_logger << "sync folders done Task done."
                       << std::endl;
		return NS_OK;
	}

private:
	SyncFolderCallback * m_pTaskCallback;
	nsCOMPtr<nsIRunnable> m_Runnable;
	int m_Result;
};


SyncFolderTask::SyncFolderTask(nsIMsgIncomingServer * pIncomingServer,
                               nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
	, m_pIncomingServer(pIncomingServer)
	, m_pSyncFolderCallback(NULL) {
}

SyncFolderTask::~SyncFolderTask() {
}
	
NS_IMETHODIMP SyncFolderTask::Run() {
	int ret = EWS_FAIL;
	char * err_msg = NULL;

	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer));

    ewsServer->GetSyncFolderCallback(&m_pSyncFolderCallback);
    
	nsresult rv = m_pSyncFolderCallback->Initialize();
	m_pSyncFolderCallback->Reset();
	m_pSyncFolderCallback->TaskBegin();
		
	if (EWS_FAIL == NotifyBegin(this)) {
		return NS_OK;
	}
	
	ews_session * session = NULL;

	nsCOMPtr<IMailEwsService> ewsService;
	ewsServer->GetService(getter_AddRefs(ewsService));

    mailews_logger << "Sync folders task running..."
                   << std::endl;
    
	nsresult rv1 = ewsService->GetNewSession(&session);

	if (NS_SUCCEEDED(rv)
	    && NS_SUCCEEDED(rv1)
	    && session) {
		ret = ews_sync_folder(session,
		                      m_pSyncFolderCallback->GetEwsSyncFolderCallback(),
		                      &err_msg);

		NotifyError(this, ret, err_msg);
			
		if (err_msg) free(err_msg);
	} else {
		NotifyError(this, session ? EWS_FAIL : 401, err_msg);
	}

	ewsService->ReleaseSession(session);
	
	nsCOMPtr<nsIRunnable> resultrunnable =
			new SyncFolderDoneTask(m_pSyncFolderCallback,
			                       ret,
			                       m_pEwsTaskCallbacks,
			                       this);
		
	NS_DispatchToMainThread(resultrunnable);

    mailews_logger << "Sync folders task done."
                   << std::endl;
	return NS_OK;
}




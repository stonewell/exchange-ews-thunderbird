#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgIncomingServer.h"
#include "nsIMsgFolder.h"
#include "nsArrayUtils.h"

#include "libews.h"

#include "MailEwsSyncItemTask.h"
#include "MailEwsErrorInfo.h"
#include "MailEwsSyncItemUtils.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"

#include "MailEwsLog.h"

class SyncItemDoneTask : public MailEwsRunnable
{
public:
	SyncItemDoneTask(SyncItemCallback * pTaskCallback,
                     int result,
                     nsISupportsArray * ewsTaskCallbacks,
                     nsIRunnable * runnable)
		: MailEwsRunnable(ewsTaskCallbacks)
		, m_pTaskCallback(pTaskCallback)
		, m_Runnable(runnable)
		, m_Result(result)
	{
        mailews_logger << "sync item done task created."
                       << std::endl;
	}

	NS_IMETHOD Run() {
        mailews_logger << "sync item done task running."
                       << std::endl;
		if (m_pTaskCallback)
			m_pTaskCallback->TaskDone(m_Result);

		NotifyEnd(m_Runnable, m_Result);
		
        mailews_logger << "sync item done task done."
                       << std::endl;
		return NS_OK;
	}

private:
	SyncItemCallback * m_pTaskCallback;
	nsCOMPtr<nsIRunnable> m_Runnable;
	int m_Result;
};


SyncItemTask::SyncItemTask(nsIMsgFolder * pFolder,
                           nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
	, m_pFolder(pFolder)
	, m_pSyncItemCallback(NULL)
    , m_SyncState("") {
}

SyncItemTask::~SyncItemTask() {
}
	
NS_IMETHODIMP SyncItemTask::Run() {
	int ret = EWS_FAIL;
	char * err_msg = NULL;

	nsCOMPtr<nsIMsgIncomingServer> server;

    nsresult rv = m_pFolder->GetServer(getter_AddRefs(server));
    NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(server));

    nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(m_pFolder));
    
    ewsFolder->GetSyncItemCallback(&m_pSyncItemCallback);

    if (m_pSyncItemCallback->IsRunning()) {
	    mailews_logger << "another sync item is running, quit...."
	              << std::endl;
	    return NS_OK;
    }

    nsCString folder_id;
    ewsFolder->GetFolderId(folder_id);

    if (folder_id.IsEmpty()) {
        return NS_OK;
    }

    m_pSyncItemCallback->SetRunning(true);
    
	if (NS_FAILED(ewsFolder->GetSyncState(m_SyncState))) {
        NS_ASSERTION(false, "Get Sync State Fail");
    }

    nsString folderName;
    m_pFolder->GetName(folderName);
    nsCString cFolderName;
    CopyUTF16toUTF8(folderName, cFolderName);
    
    mailews_logger << "sync item for folder:"
              << cFolderName.get()
              << std::endl;
    
    rv = m_pSyncItemCallback->Initialize();
	m_pSyncItemCallback->Reset();
	m_pSyncItemCallback->TaskBegin();
    m_pSyncItemCallback->SetSyncState(m_SyncState.get());

	if (EWS_FAIL == NotifyBegin(this)) {
		return NS_OK;
	}
	
	ews_session * session = NULL;

	nsCOMPtr<IMailEwsService> ewsService;
	ewsServer->GetService(getter_AddRefs(ewsService));

	nsresult rv1 = ewsService->GetNewSession(&session);

	if (NS_SUCCEEDED(rv)
	    && NS_SUCCEEDED(rv1)
	    && session) {

        if (!folder_id.IsEmpty()) {
            ret = ews_sync_items(session,
                                 folder_id.get(),
                                 m_pSyncItemCallback->GetEwsSyncItemCallback(),
                                 &err_msg);

            m_pSyncItemCallback->ProcessItems();
        } else {
            ret = EWS_SUCCESS;
        }

        if (ret == 292) {
            mailews_logger << "sync item for folder:"
                           << cFolderName.get()
                           << " not found."
                           << std::endl;

            ret = EWS_SUCCESS;
        }
        NotifyError(this, ret, err_msg);
			
        if (err_msg) free(err_msg);
    } else {
        NotifyError(this, session ? EWS_FAIL : 401, err_msg);
    }

	ewsService->ReleaseSession(session);
	
    mailews_logger << "sync item for folder:"
                   << cFolderName.get()
                   << " done."
                   << std::endl;
    
    nsCOMPtr<nsIRunnable> resultrunnable =
            new SyncItemDoneTask(m_pSyncItemCallback,
                                 ret,
                                 m_pEwsTaskCallbacks,
                                 this);
		
    NS_DispatchToMainThread(resultrunnable);

    return NS_OK;
}




#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgIncomingServer.h"
#include "nsArrayUtils.h"

#include "libews.h"

#include "MailEwsDeleteFolderTask.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"

#include "MailEwsLog.h"

class DeleteFolderDoneTask : public MailEwsRunnable
{
public:
	DeleteFolderDoneTask(nsIRunnable * runnable,
	                     int result,
	                     nsISupportsArray * ewsTaskCallbacks)
		: MailEwsRunnable(ewsTaskCallbacks)
		, m_Runnable(runnable)
		, m_Result(result)
	{
	}

	NS_IMETHOD Run() {
		NotifyEnd(m_Runnable, m_Result);

		return NS_OK;
	}

private:
	nsCOMPtr<nsIRunnable> m_Runnable;
	int m_Result;
};


DeleteFolderTask::DeleteFolderTask(nsISupportsArray * foldersToDelete,
                                   bool deleteNoTrash,
                                   nsIMsgIncomingServer * pIncomingServer,
                                   nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
	, m_pIncomingServer(pIncomingServer)
	, m_FoldersToDelete(foldersToDelete)
	, m_DeleteNoTrash(deleteNoTrash) { 
}

DeleteFolderTask::~DeleteFolderTask() {
}
	
NS_IMETHODIMP DeleteFolderTask::Run() {
	int ret = EWS_FAIL;
	char * err_msg = NULL;
	nsresult rv;
	
	if (EWS_FAIL == NotifyBegin(this)) {
		return NS_OK;
	}
	
	ews_session * session = NULL;
	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer, &rv));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<IMailEwsService> ewsService;
	rv = ewsServer->GetService(getter_AddRefs(ewsService));
	NS_ENSURE_SUCCESS(rv, rv);

	nsresult rv1 = ewsService->GetNewSession(&session);

	if (NS_SUCCEEDED(rv)
	    && NS_SUCCEEDED(rv1)
	    && session) {
		uint32_t folderCount = 0;
		nsCOMPtr<IMailEwsMsgFolder> curFolder;
		
		m_FoldersToDelete->Count(&folderCount);

		for (int32_t i = folderCount - 1; i >= 0; i--) {
			nsCOMPtr<nsISupports> _supports;
			rv = m_FoldersToDelete->GetElementAt(i, getter_AddRefs(_supports));
			curFolder = do_QueryInterface(_supports, &rv);
			if (NS_SUCCEEDED(rv)) {
				nsCString savedFolderId;
				rv = curFolder->GetFolderId(savedFolderId);
				if (NS_FAILED(rv))
					continue;
	
				nsCString change_key;
				rv = curFolder->GetChangeKey(change_key);
				if (NS_FAILED(rv))
					continue;

				err_msg = NULL;
				ret = ews_delete_folder_by_id(session,
				                              savedFolderId.get(),
				                              change_key.get(),
				                              m_DeleteNoTrash,
				                              &err_msg);

				if (ret != EWS_SUCCESS) {
					mailews_logger << "delete folder failed, code="
					          << ret
					          << ","
					          << err_msg
					          << std::endl;

                    if (ret == 189) {
                        //folder not found
                        ret = EWS_SUCCESS;
                    }
                    
					NotifyError(this, ret, err_msg);
                    
					if (ret == 401) {
                        if (err_msg) free(err_msg);
						break;
                    }
				}

				if (err_msg) free(err_msg);
			}

			if (ret == EWS_SUCCESS) {
				ewsService->SyncFolder(nullptr);
			}
		}
	} else {
		NotifyError(this, session ? EWS_FAIL : 401, err_msg);
	}

	ewsService->ReleaseSession(session);

	nsCOMPtr<nsIRunnable> resultrunnable =
			new DeleteFolderDoneTask(this, ret, m_pEwsTaskCallbacks);
		
	NS_DispatchToMainThread(resultrunnable);

	return NS_OK;
}


#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgIncomingServer.h"
#include "nsArrayUtils.h"

#include "libews.h"

#include "MailEwsGetFolderTask.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"

#include "MailEwsLog.h"

class GetFolderDoneTask : public MailEwsRunnable
{
public:
	GetFolderDoneTask(nsIRunnable * runnable,
	                  int result,
	                  const nsACString & folder_id,
	                  const nsACString & change_key,
	                  IMailEwsMsgFolder * folder,
	                  nsISupportsArray * ewsTaskCallbacks)
		: MailEwsRunnable(ewsTaskCallbacks)
		, m_Runnable(runnable)
		, m_Result(result)
		, m_FolderId(folder_id)
		, m_ChangeKey(change_key)
		, m_Folder(folder)
	{
	}

	NS_IMETHOD Run() {
		if (m_Result == EWS_SUCCESS) {
			m_Folder->SetFolderId(m_FolderId);
			m_Folder->SetChangeKey(m_ChangeKey);
		}
        
		NotifyEnd(m_Runnable, m_Result);

		return NS_OK;
	}

private:
	nsCOMPtr<nsIRunnable> m_Runnable;
	int m_Result;
	nsCString m_FolderId;
	nsCString m_ChangeKey;
	nsCOMPtr<IMailEwsMsgFolder> m_Folder;
};


GetFolderTask::GetFolderTask(nsIMsgIncomingServer * pIncomingServer,
                             const nsACString & folder_id,
                             int32_t folder_id_name,
                             IMailEwsMsgFolder * folder,
                             nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
	, m_pIncomingServer(pIncomingServer)
	, m_FolderId(folder_id)
	, m_Folder(folder)
	, m_FolderIdName(folder_id_name) {
}

GetFolderTask::~GetFolderTask() {
}
	
NS_IMETHODIMP GetFolderTask::Run() {
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

	nsCString _id;
	nsCString _change_key;
    
	if (NS_SUCCEEDED(rv)
	    && NS_SUCCEEDED(rv1)
	    && session) {
		const char * folder_id = m_FolderId.get();
		ews_folder * folder = NULL;

		if (m_FolderId.Length() == 0) {
			nsTArray<nsCString> * folderIds;
			rv = ewsService->GetFolderIdNamesToFolderId(&folderIds);

			if (NS_SUCCEEDED(rv) && folderIds) {
				nsCString empty("");
				nsCString sfolder_id = folderIds->SafeElementAt(m_FolderIdName,
				                                                empty);

				if (sfolder_id.Length() == 0) {
					NotifyError(this, session ? EWS_FAIL : 401,
					            "1:Unable to get folder id from id name");
					ewsService->ReleaseSession(session);
					return NS_ERROR_FAILURE;
				}

				folder_id = sfolder_id.get();
			} else {
				NotifyError(this, session ? EWS_FAIL : 401,
				            "2:Unable to get folder id from id name");
				ewsService->ReleaseSession(session);
				return NS_ERROR_FAILURE;
			}
		}
		
		ret = ews_get_folders_by_id(session,
		                            &folder_id,
		                            1,
		                            &folder,
		                            &err_msg);

		NotifyError(this, ret, err_msg);

		if (ret == EWS_SUCCESS) {
			_id.AppendLiteral(folder->id);
			_change_key.AppendLiteral(folder->change_key);
		}

		ews_free_folders(folder, 1);
	} else {
		NotifyError(this, session ? EWS_FAIL : 401, err_msg);
	}

	ewsService->ReleaseSession(session);
	if (err_msg) free(err_msg);

	nsCOMPtr<nsIRunnable> resultrunnable =
			new GetFolderDoneTask(this,
			                      ret,
			                      _id,
			                      _change_key,
			                      m_Folder,
			                      m_pEwsTaskCallbacks);
		
	NS_DispatchToMainThread(resultrunnable);

	return NS_OK;
}


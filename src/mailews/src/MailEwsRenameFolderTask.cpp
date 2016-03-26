#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgIncomingServer.h"
#include "nsArrayUtils.h"

#include "libews.h"

#include "MailEwsRenameFolderTask.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"

#include "MailEwsLog.h"

class RenameFolderDoneTask : public MailEwsRunnable
{
public:
	RenameFolderDoneTask(int result,
	                     nsIMsgFolder * folder,
	                     const nsAString & aNewName,
	                     nsIMsgWindow * msgWindow,
	                     nsISupportsArray * ewsTaskCallbacks,
	                     nsIRunnable * runnable)
		: MailEwsRunnable(ewsTaskCallbacks)
		, m_Result(result)
		, m_Folder(folder)
		, m_NewName(aNewName)
		, m_MsgWindow(msgWindow)
		, m_Runnable(runnable)
	{
	}

	NS_IMETHOD Run() {
		nsresult rv;
		nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(m_Folder, &rv));
    
		if (NS_FAILED(rv)) {
			NotifyError(m_Runnable, (int)rv, "unable to get ews folder");
		} else if (m_Result == EWS_SUCCESS) {
			rv = ewsFolder->ClientRename(m_NewName, m_MsgWindow);

			if (NS_FAILED(rv)) {
				NotifyError(m_Runnable, (int)rv, "unable to rename folder");
			}
		}

		NotifyEnd(m_Runnable, m_Result);
		return NS_OK;
	}

private:
	int m_Result;
	nsCOMPtr<nsIMsgFolder> m_Folder;
	nsString m_NewName;
	nsCOMPtr<nsIMsgWindow> m_MsgWindow;
	nsCOMPtr<nsIRunnable> m_Runnable;
};


RenameFolderTask::RenameFolderTask(nsIMsgFolder * folder,
                                   const nsAString & aNewName,
                                   nsIMsgWindow * msgWindow,
                                   nsIMsgIncomingServer * pIncomingServer,
                                   nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
	, m_pIncomingServer(pIncomingServer)
	, m_Folder(folder)
	, m_NewName(aNewName)
	, m_MsgWindow(msgWindow) { 
}

RenameFolderTask::~RenameFolderTask() {
}
	
NS_IMETHODIMP RenameFolderTask::Run() {
	int ret = EWS_FAIL;
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
		nsCOMPtr<IMailEwsMsgFolder> curFolder(do_QueryInterface(m_Folder));
		
		nsCString savedFolderId;
		rv = curFolder->GetFolderId(savedFolderId);
		if (NS_FAILED(rv)) {
			NotifyError(this, EWS_FAIL, "unable to get folder id");
			return NS_OK;
		}

		nsCString change_key;
		rv = curFolder->GetChangeKey(change_key);
		if (NS_FAILED(rv)) {
			NotifyError(this, EWS_FAIL, "unable to get folder change key");
			return NS_OK;
		}
  
		ews_folder * folder = (ews_folder *)malloc(sizeof(char) * sizeof(ews_folder));
		memset(folder, 0, sizeof(ews_folder));
		char * display_name = ToNewUTF8String(m_NewName);
		folder->display_name = strdup(display_name);
		folder->id = strdup(savedFolderId.get());
		folder->change_key = strdup(change_key.get());
		NS_Free(display_name);
		
		char * err_msg = NULL;

		ret = ews_update_folder(session,
		                            folder,
		                            &err_msg);

		if (ret != EWS_SUCCESS) {
			mailews_logger << "rename folder on server fail:"
			          << ret
			          << ","
			          << err_msg
			          << std::endl;
			NotifyError(this, ret, err_msg);
		}

		if (err_msg) free(err_msg);

		ews_free_folder(folder);
	} else {
		NotifyError(this, session ? EWS_FAIL : 401, NULL);
	}

	ewsService->ReleaseSession(session);
	
	nsCOMPtr<nsIRunnable> resultrunnable =
			new RenameFolderDoneTask(ret,
			                         m_Folder,
			                         m_NewName,
			                         m_MsgWindow,
			                         m_pEwsTaskCallbacks,
			                         this);
		
	NS_DispatchToMainThread(resultrunnable);

	return NS_OK;
}



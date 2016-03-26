#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgIncomingServer.h"
#include "nsArrayUtils.h"
#include "nsMsgFolderFlags.h"
#include "msgCore.h"

#include "libews.h"

#include "MailEwsCreateSubFolderTask.h"
#include "MailEwsErrorInfo.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"

#include "MailEwsLog.h"

class CreateSubFolderDoneTask : public MailEwsRunnable
{
public:
	CreateSubFolderDoneTask(nsIMsgFolder * parentFolder,
	                        int result,
	                        ews_folder * folder,
	                        const nsAString & folderName,
	                        nsISupportsArray * ewsTaskCallbacks,
	                        nsIRunnable * runnable)
		: MailEwsRunnable(ewsTaskCallbacks)
		, m_pParentFolder(parentFolder)
		, m_Result(result)
		, m_Folder(folder)
		, m_FolderName(folderName)
		, m_Runnable(runnable)
	{
	}

	NS_IMETHOD Run() {
		MOZ_ASSERT(NS_IsMainThread()); // This method is supposed to run on the main thread!
		nsresult rv = NS_OK;

		if (m_Result == EWS_SUCCESS) {
			nsCOMPtr<nsIMsgFolder> newFolder;

			mailews_logger << "new create folder:"
			          << m_Folder->id
			          << ","
			          << m_Folder->change_key
			          << std::endl;
	  
			rv = m_pParentFolder->AddSubfolder(m_FolderName,
			                                   getter_AddRefs(newFolder));
		
			if (rv == NS_MSG_FOLDER_EXISTS) {
				rv = m_pParentFolder->GetChildNamed(m_FolderName,
				                                    getter_AddRefs(newFolder));
			}

			if (newFolder) {
				nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(newFolder));
			
				ewsFolder->SetFolderId(nsCString(m_Folder->id));
				ewsFolder->SetChangeKey(nsCString(m_Folder->change_key ? m_Folder->change_key : ""));
				m_pParentFolder->NotifyItemAdded(newFolder);
			}
		}

		if (m_Folder) ews_free_folder(m_Folder);

		NotifyEnd(m_Runnable, m_Result);
		return NS_OK;
	}

private:
	nsCOMPtr<nsIMsgFolder> m_pParentFolder;
	nsCOMPtr<nsISupportsArray> m_pEwsTaskCallbacks;
	int m_Result;
	ews_folder * m_Folder;
	nsString m_FolderName;
	nsCOMPtr<nsIRunnable> m_Runnable;
};


CreateSubFolderTask::CreateSubFolderTask(nsIMsgFolder * parentFolder,
                                         const nsAString & folderName,
                               nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
	, m_pParentFolder(parentFolder)
	, m_FolderName(folderName) { 
}

CreateSubFolderTask::~CreateSubFolderTask() {
}

#define NS_FAILED_BREAK(res) \
	{nsresult __rv = res; \
	if (NS_FAILED(__rv)) { \
		NS_ENSURE_SUCCESS_BODY_VOID(res); \
		break; \
	}}
	
NS_IMETHODIMP CreateSubFolderTask::Run() {
	int ret = EWS_FAIL;
	char * err_msg = NULL;
	nsresult rv = NS_OK;
	ews_folder * folder = NULL;
	
	if (EWS_FAIL == NotifyBegin(this)) {
		return NS_OK;
	}
	
	do {
		nsCOMPtr<nsIMsgIncomingServer> server;
		rv = m_pParentFolder->GetServer(getter_AddRefs(server));
		NS_FAILED_BREAK(rv);

		nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(m_pParentFolder, &rv));
		NS_FAILED_BREAK(rv);
		
		ews_session * session = NULL;
		nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(server, &rv));
		NS_FAILED_BREAK(rv);

		nsCOMPtr<IMailEwsService> ewsService;
		rv = ewsServer->GetService(getter_AddRefs(ewsService));
		NS_FAILED_BREAK(rv);

		rv = ewsService->GetNewSession(&session);

		if (NS_SUCCEEDED(rv)
		    && session) {
			nsCString savedFolderId;
			rv = ewsFolder->GetFolderId(savedFolderId);
			NS_FAILED_BREAK(rv);

			nsCString change_key;
			rv = ewsFolder->GetChangeKey(change_key);
			NS_FAILED_BREAK(rv);
  
			folder = (ews_folder *)malloc(sizeof(char) * sizeof(ews_folder));
			memset(folder, 0, sizeof(ews_folder));
			char * display_name = ToNewUTF8String(m_FolderName);
			folder->display_name = strdup(display_name);
			NS_Free(display_name);
			
			ret = ews_create_folder(session,
			                        folder,
			                        savedFolderId.get(),
			                        &err_msg);
			NotifyError(this, ret, err_msg);
			
			if (err_msg) free(err_msg);
		} else {
			NotifyError(this, session ? EWS_FAIL : 401, err_msg);
		}

        ewsService->ReleaseSession(session);
	} while (false);

    if (folder) ews_free_folder(folder);

	nsCOMPtr<nsIRunnable> resultrunnable =
			new CreateSubFolderDoneTask(m_pParentFolder,
			                            ret,
			                            folder,
			                            m_FolderName,
			                            m_pEwsTaskCallbacks,
			                            this);
		
	NS_DispatchToMainThread(resultrunnable);

	return NS_OK;
}



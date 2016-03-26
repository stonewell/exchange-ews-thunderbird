#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgIncomingServer.h"
#include "nsArrayUtils.h"
#include "nsMsgFolderFlags.h"
#include "msgCore.h"
#include "nsComponentManagerUtils.h"
#include "nsIMutableArray.h"
#include "nsTArray.h"

#include "libews.h"

#include "MailEwsSyncFolderIdNameWithFolderIdTask.h"
#include "MailEwsErrorInfo.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"

#include "MailEwsLog.h"

class SyncFolderIdNameWithFolderIdDoneTask : public MailEwsRunnable
{
public:
	SyncFolderIdNameWithFolderIdDoneTask(
	                        int result,
	                        nsISupportsArray * ewsTaskCallbacks,
	                        nsIRunnable * runnable)
		: MailEwsRunnable(ewsTaskCallbacks)
		, m_Result(result)
		, m_Runnable(runnable)
	{
	}

	NS_IMETHOD Run() {
		NotifyEnd(m_Runnable, m_Result);
		return NS_OK;
	}

private:
	nsCOMPtr<nsISupportsArray> m_pEwsTaskCallbacks;
	int m_Result;
	nsCOMPtr<nsIRunnable> m_Runnable;
};


SyncFolderIdNameWithFolderIdTask::SyncFolderIdNameWithFolderIdTask(nsIMsgIncomingServer * incomingServer,
                                                                   nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
	, m_pIncomingServer(incomingServer) {

}

SyncFolderIdNameWithFolderIdTask::~SyncFolderIdNameWithFolderIdTask() {
}

#define NS_FAILED_BREAK(res) \
	{nsresult __rv = res; \
	if (NS_FAILED(__rv)) { \
		NS_ENSURE_SUCCESS_BODY_VOID(res); \
		break; \
	}}

static int folder_id_names[] = {
 EWSDistinguishedFolderIdName_calendar,
 EWSDistinguishedFolderIdName_contacts,
 EWSDistinguishedFolderIdName_deleteditems,
 EWSDistinguishedFolderIdName_drafts,
 EWSDistinguishedFolderIdName_inbox,
 EWSDistinguishedFolderIdName_journal,
 EWSDistinguishedFolderIdName_notes,
 EWSDistinguishedFolderIdName_outbox,
 EWSDistinguishedFolderIdName_sentitems,
 EWSDistinguishedFolderIdName_tasks,
 EWSDistinguishedFolderIdName_msgfolderroot,
 EWSDistinguishedFolderIdName_publicfoldersroot,
 EWSDistinguishedFolderIdName_root,
 EWSDistinguishedFolderIdName_junkemail,
 EWSDistinguishedFolderIdName_searchfolders,
 EWSDistinguishedFolderIdName_voicemail
};

NS_IMETHODIMP SyncFolderIdNameWithFolderIdTask::Run() {
	int ret = EWS_FAIL;
	char * err_msg = NULL;
	nsresult rv = NS_OK;
	ews_folder * folders = NULL;
	
	if (EWS_FAIL == NotifyBegin(this)) {
		return NS_OK;
	}
	
	do {
		ews_session * session = NULL;
		nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer, &rv));
		NS_FAILED_BREAK(rv);

		nsCOMPtr<IMailEwsService> ewsService;
		rv = ewsServer->GetService(getter_AddRefs(ewsService));
		NS_FAILED_BREAK(rv);

		rv = ewsService->GetNewSession(&session);
		nsTArray<nsCString> folder_ids;

		if (NS_SUCCEEDED(rv)
		    && session) {
			
			int ret =
					ews_get_folders_by_distinguished_id_name(session,
					                                         folder_id_names,
					                                         sizeof(folder_id_names)/ sizeof(int),
					                                         &folders,
					                                         &err_msg);
			if (ret == EWS_SUCCESS) {
                mailews_logger << "get folder id for name:";
				for (size_t i = 0; i < sizeof(folder_id_names) / sizeof(int); i++) {
                    mailews_logger << i;
					if (folders[i].id) {
						nsCString folderId(folders[i].id);
                        mailews_logger << ":" << folders[i].id;
						folder_ids.AppendElement(folderId);
					} else {
                        mailews_logger << ": no id found";
						folder_ids.AppendElement(nsCString(""));
					}
                    mailews_logger << std::endl;
				}
			} else {
                mailews_logger << "get folder id for name fail"
                               << std::endl;
				for (size_t i = 0; i < sizeof(folder_id_names) / sizeof(int); i++) {
					folder_ids.AppendElement(nsCString(""));
				}
			}
			NotifyError(this, ret, err_msg);
			
			if (err_msg) free(err_msg);
		} else {
			NotifyError(this, session ? EWS_FAIL : 401, err_msg);
		}

		ewsService->SetFolderIdNamesToFolderId(&folder_ids);

		if (folders)
			ews_free_folders(folders,
			                 sizeof(folder_id_names) / sizeof(int));
		
        ewsService->ReleaseSession(session);
	} while (false);

	nsCOMPtr<nsIRunnable> resultrunnable =
			new SyncFolderIdNameWithFolderIdDoneTask(ret,
			                            m_pEwsTaskCallbacks,
			                            this);
		
	NS_DispatchToMainThread(resultrunnable);

	return NS_OK;
}




#include "nsIMsgIncomingServer.h"
#include "nsIThread.h"

#include "MailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"

#include "nsThreadUtils.h"
#include "nsIProxyInfo.h"
#include "nsProxyRelease.h"
#include "nsDebug.h"
#include "nsIMsgWindow.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgMailSession.h"
#include "nsThreadUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsMsgBaseCID.h"
#include "nsMsgIncomingServer.h"
#include "nsIThreadManager.h"
#include "nsXPCOMCIDInternal.h"
#include "nsISupportsArray.h"

#include "MailEwsSyncFolderUtils.h"
#include "MailEwsErrorInfo.h"
#include "MailEwsCreateSubFolderTask.h"
#include "MailEwsRenameFolderTask.h"
#include "MailEwsDeleteFolderTask.h"
#include "MailEwsSyncFolderIdNameWithFolderIdTask.h"
#include "MailEwsGetFolderTask.h"
#include "MailEwsSyncItemTask.h"
#include "MailEwsItemTask.h"
#include "MailEwsMsgUtils.h"
#include "MailEwsSubscribePushNotificationTask.h"
#include "MailEwsSubscriptionCallback.h"
#include "MailEwsNotificationServerTask.h"
#include "MailEwsSmtpProxyTask.h"
#include "MailEwsSendMailWithMimeContentTask.h"
#include "MailEwsDeleteItemsTask.h"
#include "MailEwsMarkItemsReadTask.h"
#include "MailEwsResolveNamesTask.h"
#include "MailEwsSyncCalendarTask.h"
#include "MailEwsAddCalendarItemTask.h"
#include "MailEwsDeleteCalendarItemTask.h"
#include "MailEwsConvertCalendarItemIdToUidTask.h"
#include "MailEwsModifyCalendarItemTask.h"
#include "MailEwsSyncTodoTask.h"
#include "MailEwsAddTodoTask.h"
#include "MailEwsModifyTodoTask.h"

#include "MailEwsLock.h"

#include "MailEwsLog.h"

#define ALL_IN_MAIN_THREAD 0
#define PLAYBACK_TIMER_INTERVAL_IN_MS (60 * 1000) //1min

NS_IMPL_ISUPPORTS(MailEwsService, IMailEwsService, IMailEwsTaskCallback)

MailEwsService::MailEwsService()
{
	/* member initializers and constructor code */
    m_pSession = NULL;
    m_SessionParamsInitialized = false;
    m_IsShutdown = false;
    m_Lock = PR_NewLock();
	memset(&m_SessionParams, 0, sizeof(ews_session_params));
}

MailEwsService::~MailEwsService()
{
    PR_Lock(m_Lock);
    PR_Unlock(m_Lock);
	PR_DestroyLock(m_Lock);

	if (m_SessionParamsInitialized) {
		if (m_SessionParams.endpoint) free((char *)m_SessionParams.endpoint);
		if (m_SessionParams.user) free((char *)m_SessionParams.user);
		if (m_SessionParams.password) free((char *)m_SessionParams.password);
		if (m_SessionParams.email) free((char *)m_SessionParams.email);
	}
}

/* long Discovery (in ACString email, out ACString url, out ACString oab); */
NS_IMETHODIMP MailEwsService::Discovery(const nsACString & email,
                                        const nsACString & user,
                                        const nsACString & passwd, 
                                        nsACString & url,
                                        nsACString & oab,
                                        int32_t * auth,
                                        nsACString & errMsg,
                                        int32_t *_retval)
{
	ews_session_params params;
	char * endpoint = NULL;
	char * oab_url = NULL;
	char * err_msg = NULL;
    int auth_method = EWS_AUTH_NTLM;

	memset(&params, 0, sizeof(ews_session_params));
	params.domain = "";
	NS_CStringGetData(user, &params.user);
	NS_CStringGetData(passwd, &params.password);
	NS_CStringGetData(email, &params.email);
  
	*_retval = ews_autodiscover(&params,
	                            &endpoint, &oab_url,
                                &auth_method,
	                            &err_msg);

	if (!*_retval) {
		if (endpoint) url = (nsCString)endpoint;
		if (oab_url) oab = (nsCString)oab_url;
		if (err_msg) errMsg = nsCString(err_msg);
        if (auth) *auth = auth_method;
	}

	ews_autodiscover_free(&params, endpoint, oab_url, err_msg);

	return NS_OK;
}

NS_IMETHODIMP MailEwsService::QueueTask(nsIRunnable * runnable) {
	ProcessQueuedTasks();

#if ALL_IN_MAIN_THREAD
	NS_DispatchToMainThread(runnable);
    return NS_OK;
#else
    return m_WorkerThread->Dispatch(runnable, nsIEventTarget::DISPATCH_NORMAL);
#endif
}

/* void updateFolderProperties (in IMailEwsMsgFolder folder); */
NS_IMETHODIMP MailEwsService::UpdateFolderProperties(IMailEwsMsgFolder *folder,
                                                     IMailEwsTaskCallback *callback)
{
	nsCString folder_id;
	nsresult rv = folder->GetFolderId(folder_id);
	NS_ENSURE_SUCCESS(rv, rv);
	return UpdateFolderPropertiesWithId(folder_id,
	                                    folder,
	                                    callback);
}

/* void updateFolderPropertiesWithId (in ACString folder_id, in IMailEwsMsgFolder folder); */
NS_IMETHODIMP MailEwsService::UpdateFolderPropertiesWithId(const nsACString & folder_id,
                                                           IMailEwsMsgFolder *folder,
                                                           IMailEwsTaskCallback *callback) {
    return __UpdateFolderPropertiesWithId(folder_id,
                                          -1,
                                          folder,
                                          callback);
}

NS_IMETHODIMP MailEwsService::__UpdateFolderPropertiesWithId(const nsACString & folder_id,
                                                             int32_t folder_id_name,
                                                           IMailEwsMsgFolder *folder,
                                                           IMailEwsTaskCallback *callback) {
	nsresult rv = NS_OK;

	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);
	
	nsCOMPtr<nsIRunnable> task =
			new GetFolderTask(m_pIncomingServer,
			                  folder_id,
                              folder_id_name,
			                  folder,
			                  callbacks);

	rv = QueueTask(task);

	return rv;
}

/* void updateFolderPropertiesWithIdName (in long folder_id_name, in IMailEwsMsgFolder folder); */
NS_IMETHODIMP MailEwsService::UpdateFolderPropertiesWithIdName(int32_t folder_id_name,
                                                               IMailEwsMsgFolder *folder,
                                                               IMailEwsTaskCallback *callback)
{
	nsCString empty("");

    return __UpdateFolderPropertiesWithId(empty,
                                          folder_id_name,
                                          folder,
                                          callback);
}

class DLL_LOCAL PromptPasswordTask : public nsRunnable
{
public:
	PromptPasswordTask(MailEwsService * service,
                       nsIMsgIncomingServer * pIncomingServer,
	                   const nsCString & oldPassword)
			: m_pIncomingServer(pIncomingServer)
			, m_OldPassword(oldPassword)
            , m_EwsService(service) {
		
		nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer));
		ewsServer->SetIsPromptingPassword(true);
	}

    static bool m_Cancelled;
    
	NS_IMETHOD Run() {	
		nsresult rv;

        if (PromptPasswordTask::m_Cancelled) {
            mailews_logger << "prompt password cancelled" << std::endl;
            nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer));
            ewsServer->SetIsPromptingPassword(false);
            return NS_OK;
        }
        
		m_pIncomingServer->ForgetPassword();
		mailews_logger << "reset password, next time to prompt" << std::endl;

		nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer));

		rv = DoPromptPassword();
        if (NS_FAILED(rv)) {
            ewsServer->SetIsPromptingPassword(false);
        }
		NS_ENSURE_SUCCESS(rv, rv);

		ewsServer->SetIsPromptingPassword(false);

		nsCOMPtr<IMailEwsService> ewsService;

		ewsServer->GetService(getter_AddRefs(ewsService));
		ewsService->ProcessQueuedTasks();

		return rv;
	}

	NS_IMETHOD DoPromptPassword() {
		nsresult rv;
		//Prompt to get password
		nsCOMPtr<nsIMsgMailSession> mailSession = do_GetService(NS_MSGMAILSESSION_CONTRACTID, &rv);
		NS_ENSURE_SUCCESS(rv, rv);

		nsCOMPtr <nsIMsgWindow> msgWindow;
		rv = mailSession->GetTopmostMsgWindow(getter_AddRefs(msgWindow));
		NS_ENSURE_SUCCESS(rv, rv);

        nsString acctName;
        rv = m_pIncomingServer->GetPrettyName(acctName);
        NS_ENSURE_SUCCESS(rv, rv);

        nsString msg;
        msg.Append(NS_LITERAL_STRING("Invalid password of "));
        msg.Append(acctName);

		rv = m_pIncomingServer->GetPasswordWithUI(
		    msg,
		    NS_LITERAL_STRING("Input Password"),
		    msgWindow.get(),
		    m_OldPassword);

		if (rv == NS_OK) {
			mailews_logger << "get password:" << m_OldPassword.get() << std::endl;
            m_EwsService->m_SessionParamsInitialized = false;
		} else if (rv == NS_MSG_PASSWORD_PROMPT_CANCELLED) {
            PromptPasswordTask::m_Cancelled = true;
            return rv;
        }
		
		return NS_OK;
	}

private:
	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
	nsCString m_OldPassword;
    MailEwsService * m_EwsService;
};

void ResetPasswordPromptStatus() {
    PromptPasswordTask::m_Cancelled = false;
}

bool PromptPasswordTask::m_Cancelled = false;

NS_IMETHODIMP MailEwsService::CreateSession() {
    ews_session * pSession = NULL;

    if (m_IsShutdown)
        return NS_ERROR_FAILURE;
    
    nsresult rv = CreateSession(&pSession);

    if (NS_SUCCEEDED(rv)) {
        m_pSession = pSession;
    }

    return rv;
}

NS_IMETHODIMP
MailEwsService::InitSessionParams() {
	nsresult rv = NS_OK;
	nsCString password;
	nsCString username;
	nsCString endpoint;
    nsCString email;
    int auth_method;

    if (m_SessionParamsInitialized)
        return NS_OK;
    
	rv = m_pIncomingServer->GetPassword(password);
	NS_ENSURE_SUCCESS(rv, rv);
	rv = m_pIncomingServer->GetRealUsername(username);
	NS_ENSURE_SUCCESS(rv, rv);
	rv = m_pIncomingServer->GetCharValue("url", endpoint);
	NS_ENSURE_SUCCESS(rv, rv);
	rv = m_pIncomingServer->GetIntValue("auth", &auth_method);
	NS_ENSURE_SUCCESS(rv, rv);
	rv = m_pIncomingServer->GetCharValue("email", email);
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer));

	if (password.IsEmpty()) {
		//try load password from login manager
        rv = ewsServer->GetPasswordWithoutUI();
        nsresult rv1 = NS_OK;

        if (NS_SUCCEEDED(rv)) {
            rv1 = m_pIncomingServer->GetPassword(password);
        }
        
		if (!(NS_SUCCEEDED(rv) &&
		      NS_SUCCEEDED(rv1) &&
		      !password.IsEmpty())) {

            mailews_logger << "unable to get password, do async prompt password"
                      << ",rv:" << int(rv)
                      << ",rv1:" << int(rv1)
                      << std::endl;
            AsyncPromptPassword();
			return NS_ERROR_ABORT;
		} else {
			mailews_logger << "---------- got password from login manager ----"
			          << std::endl;
		}
	}

	memset(&m_SessionParams, 0, sizeof(ews_session_params));
	m_SessionParams.endpoint = strdup(endpoint.get());
	m_SessionParams.domain = "";
	m_SessionParams.user = strdup(username.get());
	m_SessionParams.password = strdup(password.get());
    m_SessionParams.auth_method = auth_method;
    m_SessionParams.email = strdup(email.get());

	mailews_logger << "------------ create session username:" << username.get()
	          << ", endpoint:" << endpoint.get()
	          << ", email:" << email.get()
	          << ", password:" << password.get()
	          << std::endl;
    m_SessionParamsInitialized = true;
    
    return NS_OK;
}

NS_IMETHODIMP MailEwsService::CreateSession(ews_session ** ppSession) {
	int ret = EWS_FAIL;
	char * err_msg = NULL;
    nsresult rv = NS_OK;

    *ppSession = NULL;

    if (m_IsShutdown)
        return NS_ERROR_FAILURE;

    rv = InitSessionParams();
	NS_ENSURE_SUCCESS(rv, rv);

	ret = ews_session_init(&m_SessionParams, ppSession, &err_msg);

	if (ret != EWS_SUCCESS) {
		MailEwsErrorInfo error(ret, err_msg);
		HandleEwsError(&error);
        *ppSession = NULL;
	}

	if (err_msg)
		free(err_msg);
	return ret == EWS_SUCCESS ? NS_OK : NS_ERROR_ABORT;
}

void MailEwsService::AsyncPromptPassword() {
	if (m_pSession) {
		ews_session_cleanup(m_pSession);
		m_pSession = NULL;
	}

	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer));
	
	bool promptingPassword = false;
	if (NS_SUCCEEDED(ewsServer->GetIsPromptingPassword(&promptingPassword))
	    && !promptingPassword) {
		nsCString password;

        m_pIncomingServer->GetPassword(password);

		nsCOMPtr<nsIRunnable> resultrunnable =
				new PromptPasswordTask(this, m_pIncomingServer, password);
		NS_DispatchToMainThread(resultrunnable);
	}
}

// NS_IMETHODIMP MailEwsService::GetSession(ews_session ** pp_session) {
// 	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer));
	
// 	bool promptingPassword = false;
// 	if (NS_FAILED(ewsServer->GetIsPromptingPassword(&promptingPassword))
// 	    || promptingPassword) {
//         mailews_logger << "Prompting Password" << std::endl;
//         *pp_session = NULL;
//         return NS_ERROR_ABORT;
//     }
    
// 	if (!m_pSession) {
// 		CreateSession();
// 	}

// 	*pp_session =  m_pSession;

// 	return NS_OK;
// }

NS_IMETHODIMP MailEwsService::GetNewSession(ews_session ** pp_session) {
	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer));
	
	bool promptingPassword = false;
	if (NS_FAILED(ewsServer->GetIsPromptingPassword(&promptingPassword))
	    || promptingPassword) {
        mailews_logger << "Prompting Password" << std::endl;
        *pp_session = NULL;
        return NS_ERROR_ABORT;
    }
    
    return CreateSession(pp_session);
}

NS_IMETHODIMP MailEwsService::SyncFolder(IMailEwsTaskCallback *callback) {
	nsresult rv = NS_OK;

	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);
	
	nsCOMPtr<nsIRunnable> syncFolderTask =
			new SyncFolderTask(m_pIncomingServer,
			                   callbacks);

	rv = QueueTask(syncFolderTask);
	
	return rv;
}

/* void initWithIncomingServer (in nsIMsgIncomingServer incomingServer); */
NS_IMETHODIMP MailEwsService::InitWithIncomingServer(nsIMsgIncomingServer *incomingServer)
{
	nsresult rv = NS_OK;
	m_pIncomingServer = do_QueryInterface(incomingServer, &rv);
	NS_ENSURE_SUCCESS(rv, rv);

    m_SubscriptionCallback.reset(new MailEwsSubscriptionCallback(m_pIncomingServer));

    // Create Playback Timer
    CreatePlaybackTimer();

	// To create a new thread, get the thread manager
	nsCOMPtr<nsIThreadManager> tm = do_GetService(NS_THREADMANAGER_CONTRACTID);

	rv = tm->NewThread(0, 0, getter_AddRefs(m_WorkerThread));
	NS_ENSURE_SUCCESS(rv, rv);

	rv = tm->NewThread(0, 0, getter_AddRefs(m_NotificationThread));
	NS_ENSURE_SUCCESS(rv, rv);
    
	rv = tm->NewThread(0, 0, getter_AddRefs(m_SmtpProxyThread));
	NS_ENSURE_SUCCESS(rv, rv);

	rv = tm->NewThread(0, 0, getter_AddRefs(m_CalendarThread));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	
	nsCOMPtr<nsIRunnable> task =
			new SyncFolderIdNameWithFolderIdTask(m_pIncomingServer,
			                   callbacks);

	rv = QueueTask(task);

	nsCOMPtr<nsIMsgFolder> rootFolder;
	rv = m_pIncomingServer->GetRootFolder(getter_AddRefs(rootFolder));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<IMailEwsMsgFolder> ewsRootFolder(do_QueryInterface(rootFolder, &rv));
	NS_ENSURE_SUCCESS(rv, rv);
	
	nsCString savedFolderId;
	rv = ewsRootFolder->GetFolderId(savedFolderId);

	nsCString change_key;
	nsresult rv2 = ewsRootFolder->GetChangeKey(change_key);
	
	if (NS_FAILED(rv) || savedFolderId.Length() == 0
	    || NS_FAILED(rv2)
	    || change_key.Length() == 0) {
		
		rv = UpdateFolderPropertiesWithIdName(EWSDistinguishedFolderIdName_msgfolderroot,
		                                      ewsRootFolder,
		                                      nullptr);
		NS_ENSURE_SUCCESS(rv, rv);
	}

	StartNotificationServer(nullptr);

    StartSmtpProxy(nullptr);
	
    MailEwsMsgCompose::CreateIdentity();

    //Do subscription
    nsTArray<nsCString> folderIds;
    GetAllFolderIds(folderIds);

    SubscribePushNotification(&folderIds, nullptr);
	return rv;
	
}

void MailEwsService::HandleEwsError(MailEwsErrorInfo * pError) {
	mailews_logger << "===============Error:" << pError->GetErrorCode() << "," << pError->GetErrorMessage() << std::endl;

	if (pError->GetErrorCode() == 401) {
		//Authorize fail, prompt for password
		AsyncPromptPassword();
	}
}

/* void onTaskBegin (); */
NS_IMETHODIMP MailEwsService::OnTaskBegin(nsIRunnable * runnable) {
	return m_IsShutdown ? NS_ERROR_FAILURE : NS_OK;
}

/* void onTaskDone (); */
NS_IMETHODIMP MailEwsService::OnTaskDone(nsIRunnable * runnable,
                                         int result,
                                         void * taskData) {
	return NS_OK;
}

/* void onTaskError (in long err_code, in ACString err_msg); */
NS_IMETHODIMP MailEwsService::OnTaskError(nsIRunnable * runnable,
                                          int32_t err_code,
                                          const nsACString & err_msg) {
    mailews_logger << "Task Error:"
              << err_code
              << ","
              << nsCString(err_msg).get()
              << std::endl;
    
    if (m_IsShutdown)
        return NS_OK;
    
    MailEwsLock l(m_Lock);
    
	if (err_code == 401) {
		m_TaskQueue.AppendObject(runnable);
		AsyncPromptPassword();
	} else if (err_code > 600) {
		//curl error
		mailews_logger << "Curl Error"
		          << std::endl;
		m_DelayedTaskQueue.AppendObject(runnable);
	} else if (err_code > 200 && err_code < 600) {
		//http error
		mailews_logger << "Http Error"
		          << std::endl;
		m_DelayedTaskQueue.AppendObject(runnable);
	} else if (err_code < 0) {
		//http error
		mailews_logger << "Timeout or no response"
		          << std::endl;
		m_DelayedTaskQueue.AppendObject(runnable);
	}
	
	return NS_OK;
}

NS_IMETHODIMP MailEwsService::ProcessQueuedTasks() {
	ews_session * session = NULL;

    if (m_IsShutdown)
        return NS_OK;

	nsresult rv = GetNewSession(&session);

	if (NS_FAILED(rv) ||
	    !session) {
        mailews_logger << "Process queued Task get session fail!" << std::endl;
		return NS_OK;
    }

	ReleaseSession(session);
	
	MailEwsLock l(m_Lock);
	
	for (PRInt32 i = 0; i < m_TaskQueue.Count(); i++) {
#if ALL_IN_MAIN_THREAD
		NS_DispatchToMainThread(m_TaskQueue[i]);
#else
		m_WorkerThread->Dispatch(m_TaskQueue[i],
		                         nsIEventTarget::DISPATCH_NORMAL);
#endif
	}

	m_TaskQueue.Clear();

	return NS_OK;
}

NS_IMETHODIMP MailEwsService::CreateSubFolder(nsIMsgFolder * parentFolder,
                                              const nsAString & folderName,
                                              IMailEwsTaskCallback *callback) {
	nsresult rv = NS_OK;

	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);
	
	nsCOMPtr<nsIRunnable> task =
			new CreateSubFolderTask(parentFolder,
			                        folderName,
			                        callbacks);

	rv = QueueTask(task);
	
	return rv;
}                                             

NS_IMETHODIMP MailEwsService::RenameFolder(nsIMsgFolder * folder,
                                           const nsAString & newName,
                                           nsIMsgWindow * msgWindow,
                                           IMailEwsTaskCallback *callback) {
	nsresult rv = NS_OK;

	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);
	
	nsCOMPtr<nsIRunnable> task =
			new RenameFolderTask(folder,
			                     newName,
			                     msgWindow,
			                     m_pIncomingServer,
			                     callbacks);

	rv = QueueTask(task);
	
	return rv;
}

NS_IMETHODIMP MailEwsService::DeleteFolders(nsISupportsArray * folders,
                                            bool deleteNoTrash,
                                            IMailEwsTaskCallback *callback) {
	nsresult rv = NS_OK;

	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);
	
	nsCOMPtr<nsIRunnable> task =
			new DeleteFolderTask(folders,
			                     deleteNoTrash,
			                     m_pIncomingServer,
			                     callbacks);

	rv = QueueTask(task);
	
	return rv;
}                                             

NS_IMETHODIMP MailEwsService::GetFolderIdNamesToFolderId(nsTArray<nsCString> ** folderIds) {
	*folderIds = &m_FolderIdNamesToFolderId;

	return NS_OK;
}

NS_IMETHODIMP MailEwsService::SetFolderIdNamesToFolderId(nsTArray<nsCString> * folderIds) {
	if (!folderIds) return NS_OK;
	
	for (PRUint32 i = 0; i < folderIds->Length(); i++) {
		m_FolderIdNamesToFolderId.AppendElement((*folderIds)[i]);
	}
	
	return NS_OK;
}

NS_IMETHODIMP MailEwsService::SyncItems(nsIMsgFolder * folder,
                                        IMailEwsTaskCallback *callback) {
	nsresult rv = NS_OK;

	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);
	
	nsCOMPtr<nsIRunnable> syncItemTask =
			new SyncItemTask(folder,
                             callbacks);

	rv = QueueTask(syncItemTask);
	
	return rv;
}

NS_IMETHODIMP MailEwsService::ProcessItems(nsIMsgFolder*,
                                           IMailEwsItemsCallback* itemsCallback,
                                           IMailEwsTaskCallback* callback) {
	nsresult rv = NS_OK;

	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);
	
	nsCOMPtr<nsIRunnable> itemTask =
			new ItemTask(itemsCallback,
                             callbacks);

	rv = QueueTask(itemTask);
	
	return rv;
}

NS_IMETHODIMP MailEwsService::StartNotificationServer(IMailEwsTaskCallback* callback) {
	nsresult rv = NS_OK;

    if (m_IsShutdown)
        return rv;

	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);
	
	nsCOMPtr<nsIRunnable> task =
			new NotificationServerTask(m_SubscriptionCallback.get(),
			                           m_pIncomingServer,
			                           m_NotificationThread,
			                           callbacks);

	rv = QueueTask(task);
	
	return rv;
}

NS_IMETHODIMP MailEwsService::SubscribePushNotification(nsTArray<nsCString> * folderIds,
                                                        IMailEwsTaskCallback* callback) {
	nsresult rv = NS_OK;

	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);
	
	nsCOMPtr<nsIRunnable> task =
			new SubscribePushNotificationTask(m_SubscriptionCallback.get(),
			                           m_pIncomingServer,
			                           folderIds,
			                           callbacks);

	rv = QueueTask(task);
	
	return rv;
}

NS_IMETHODIMP MailEwsService::Shutdown() {
	/* destructor code */
    mailews_logger << "destroy service" << std::endl;
    m_IsShutdown = true;

    /* shutdown worker thread before unsubscribe */
    /* curl should not be called in multiple thread */
    m_WorkerThread->Shutdown();
    
    mailews_logger << "Worker thread shutdown OK." << std::endl;
    
    if (m_SubscriptionCallback.get()) {
        mailews_logger << "unsubscribe" << std::endl;

        if (m_pSession) {
            nsTArray<ews_subscription *> * subscriptions =
                    m_SubscriptionCallback->GetEwsSubscriptions();

            int len = subscriptions->Length();
            for(int i=0;i<len;i++) {
                ews_unsubscribe(m_pSession,
                                subscriptions->ElementAt(i),
                                NULL);
            }
        }
        mailews_logger << "shutdown notification server" << std::endl;
        m_SubscriptionCallback->StopNotificationServer();
        mailews_logger << "shutdown notification server OK" << std::endl;
    }

    mailews_logger << "shutdown notification thread" << std::endl;
    m_NotificationThread->Shutdown();
    mailews_logger << "shutdown notification thread OK" << std::endl;

    mailews_logger << "clean up session" << std::endl;
	if (m_pSession)
		ews_session_cleanup(m_pSession);
    mailews_logger << "clean up session OK" << std::endl;

    mailews_logger << "shutdown smtp" << std::endl;
    extern void StopSmtpProxy();
    StopSmtpProxy();
    m_SmtpProxyThread->Shutdown();
    mailews_logger << "shutdown smtp OK" << std::endl;

    mailews_logger << "shutdown Calendar" << std::endl;
    m_CalendarThread->Shutdown();
    mailews_logger << "shutdown Calendar OK" << std::endl;

    MailEwsMsgCompose::ReleaseIdentity();
    mailews_logger << "Release identify" << std::endl;

    if (m_PlaybackTimer) {
        m_PlaybackTimer->Cancel();
    }
    mailews_logger << "Cancel Timer" << std::endl;
    
    MailEwsLock l(m_Lock);
    mailews_logger << "Acquire the Lock" << std::endl;
    
    m_IsShutdown = false;
    
    return NS_OK;
}    

NS_IMETHODIMP MailEwsService::StartSmtpProxy(IMailEwsTaskCallback* callback) {
	nsresult rv = NS_OK;

	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);
	
	nsCOMPtr<nsIRunnable> task =
			new SmtpProxyTask(m_pIncomingServer,
                              m_SmtpProxyThread,
                              callbacks);

	rv = QueueTask(task);
	
	return rv;
}

NS_IMETHODIMP MailEwsService::SendMailWithMimeContent(char * mimeContent,
                                                      int32_t length,
                                                      IMailEwsTaskCallback* callback) {
	nsresult rv = NS_OK;

	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);
	
	nsCOMPtr<nsIRunnable> task =
			new SendMailWithMimeContentTask(m_pIncomingServer,
			                                mimeContent,
			                                length,
			                                callbacks);

	rv = QueueTask(task);
	
	return rv;
}

NS_IMETHODIMP MailEwsService::DeleteItems(nsIMsgFolder * pFolder,
                                          nsTArray<nsCString> * itemIdsAndChangeKeys,
                                          bool deleteNoTrash,
                                          IMailEwsTaskCallback* callback) {
	nsresult rv = NS_OK;

	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);
	
	nsCOMPtr<nsIRunnable> task =
			new DeleteItemsTask(itemIdsAndChangeKeys,
			                    deleteNoTrash,
			                    pFolder,
			                    m_pIncomingServer,
			                    callbacks);

	rv = QueueTask(task);
	
	return rv;
}

NS_IMETHODIMP MailEwsService::MarkItemsRead(nsIMsgFolder * pFolder,
                                            nsTArray<nsCString> * itemIdsAndChangeKeys,
                                            bool isRead,
                                            IMailEwsTaskCallback* callback) {
	nsresult rv = NS_OK;

	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);
	
	nsCOMPtr<nsIRunnable> task =
			new MarkItemsReadTask(itemIdsAndChangeKeys,
			                      isRead ? 1 : 0,
			                      pFolder,
			                      m_pIncomingServer,
			                      callbacks);

	rv = QueueTask(task);
	
	return rv;
}

nsresult MailEwsService::CreatePlaybackTimer()
{
  nsresult rv = NS_OK;
  if (!m_PlaybackTimer) {
    m_PlaybackTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv),"failed to create playback timer");
    m_PlaybackTimer->InitWithFuncCallback(PlaybackTimerCallback,
                                          (void *)this,
                                          PLAYBACK_TIMER_INTERVAL_IN_MS,
                                          nsITimer::TYPE_REPEATING_SLACK);
  }
  return rv;
}

void MailEwsService::PlaybackTimerCallback(nsITimer *aTimer, void *aClosure)
{
	MailEwsService * service = (MailEwsService *)aClosure;
    
    if (service->m_IsShutdown)
        return;

    {
        MailEwsLock l(service->m_Lock);
        mailews_logger << "playback timer running:"
                       << service->m_DelayedTaskQueue.Count()
                       << " re-queued"
                       << std::endl;

        for (PRInt32 i = 0; i < service->m_DelayedTaskQueue.Count(); i++) {
            service->m_TaskQueue.AppendObject(service->m_DelayedTaskQueue[i]);
        }

        service->m_DelayedTaskQueue.Clear();
    }

	service->ProcessQueuedTasks();
}

NS_IMETHODIMP MailEwsService::ResolveNames(const nsACString & unresolvedEntry,
                                           nsIAbDirSearchListener * aListener,
                                           nsIAbDirectory * aParent,
                                           const nsACString & uuid,
                                           int32_t resultLimit,
                                           bool doSyncCall,
                                           int32_t searchContext,
                                           IMailEwsTaskCallback * callback) {
	nsresult rv = NS_OK;

	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);

	nsCOMPtr<nsIRunnable> task =
			new ResolveNamesTask(unresolvedEntry,
			                     aListener,
                                 aParent,
			                     uuid,
			                     resultLimit,
                                 doSyncCall && NS_IsMainThread(),
			                     m_pIncomingServer,
                                 searchContext,
			                     callbacks);

    if (doSyncCall) {
        return task->Run();
    } else {
        //rv = QueueTask(task);
        rv = m_CalendarThread->Dispatch(task, nsIEventTarget::DISPATCH_NORMAL);
    }
	
	return rv;
}
		
NS_IMETHODIMP MailEwsService::ReleaseSession(ews_session * session) {
	if (session) {
		ews_session_cleanup(session);
	}

    return NS_OK;
}

NS_IMETHODIMP
MailEwsService::SyncCalendar(IMailEwsCalendarItemCallback * calCallback,
                             IMailEwsTaskCallback* callback) {
	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);

	nsCOMPtr<nsIRunnable> runnable =
			new SyncCalendarTask(calCallback,
			                     m_pIncomingServer,
			                     callbacks);

	return QueueTask(runnable);
}

NS_IMETHODIMP
MailEwsService::AddCalendarItem(calIEvent* event,
                                IMailEwsTaskCallback * callback) {
	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);

	nsCOMPtr<nsIRunnable> runnable =
			new AddCalendarItemTask(event,
                                    m_pIncomingServer,
                                    callbacks);

	//calIEvent has to run in main thread
	return NS_DispatchToMainThread(runnable);
}

NS_IMETHODIMP
MailEwsService::ModifyCalendarItem(calIEvent * oldEvent,
                                   calIEvent * newEvent,
                                   IMailEwsTaskCallback * callback) {
	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);

	nsCOMPtr<nsIRunnable> runnable =
			new ModifyCalendarItemTask(oldEvent,
			                           newEvent,
			                           m_pIncomingServer,
			                           callbacks);

	//calIEvent has to run in main thread
	return NS_DispatchToMainThread(runnable);
}

NS_IMETHODIMP
MailEwsService::DeleteCalendarItem(nsACString const& itemId,
                                   nsACString const& changeKey,
                                   IMailEwsTaskCallback * callback) {
	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);

	nsCOMPtr<nsIRunnable> runnable =
			new DeleteCalendarItemTask(nsCString(itemId),
                                       nsCString(changeKey),
                                       m_pIncomingServer,
                                       callbacks);

	return QueueTask(runnable);
}

// NS_IMETHODIMP
// MailEwsService::ConvertCalendarItemIdToUid(nsACString const& itemId, IMailEwsTaskCallback* callback) {
// 	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
// 	IMailEwsTaskCallback * selfCallback = this;
	
// 	callbacks->InsertElementAt(selfCallback, 0);
// 	if (callback)
// 		callbacks->InsertElementAt(callback, 0);

// 	nsCOMPtr<nsIRunnable> runnable =
// 			new ConvertCalendarItemIdToUidTask(nsCString(itemId),
// 			                                   m_pIncomingServer,
// 			                                   callbacks);

// 	return QueueTask(runnable);
//     return NS_ERROR_NOT_IMPLEMENTED;
// }

NS_IMETHODIMP
MailEwsService::SyncTodo(IMailEwsCalendarItemCallback * calCallback,
                         IMailEwsTaskCallback* callback) {
	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);

	nsCOMPtr<nsIRunnable> runnable =
			new SyncTodoTask(calCallback,
			                 m_pIncomingServer,
			                 callbacks);

	return QueueTask(runnable);
}

NS_IMETHODIMP
MailEwsService::AddTodo(calITodo* todo,
                        IMailEwsTaskCallback * callback) {
	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);

	nsCOMPtr<nsIRunnable> runnable =
			new AddTodoTask(todo,
			                m_pIncomingServer,
			                callbacks);

	//calITodo has to run in main thread
	return NS_DispatchToMainThread(runnable);
}

NS_IMETHODIMP
MailEwsService::ModifyTodo(calITodo * oldTodo,
                           calITodo * newTodo,
                           IMailEwsCalendarItemCallback * calCallback,
                           IMailEwsTaskCallback * callback) {
	nsCOMPtr<nsISupportsArray> callbacks = do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID);
	IMailEwsTaskCallback * selfCallback = this;
	
	callbacks->InsertElementAt(selfCallback, 0);
	if (callback)
		callbacks->InsertElementAt(callback, 0);

	nsCOMPtr<nsIRunnable> runnable =
			new ModifyTodoTask(oldTodo,
			                   newTodo,
                               calCallback,
			                   m_pIncomingServer,
			                   callbacks);

	//calITodo has to run in main thread
	return NS_DispatchToMainThread(runnable);
}

nsresult
MailEwsService::GetAllFolderIds(nsTArray<nsCString> & folderIds) {
    nsresult rv = NS_OK;
    
	nsCOMPtr<nsIMsgFolder> rootFolder;
	rv = m_pIncomingServer->GetRootFolder(getter_AddRefs(rootFolder));
	NS_ENSURE_SUCCESS(rv, rv);

    return GetAllFolderIds(rootFolder,
                           folderIds);
}

nsresult
MailEwsService::GetAllFolderIds(nsIMsgFolder * rootFolder,
                                nsTArray<nsCString> & folderIds) {
    nsresult rv = NS_OK;
	//loop all sub folders
	nsCOMPtr<nsISimpleEnumerator> subFolders;
	rv = rootFolder->GetSubFolders(getter_AddRefs(subFolders));
	NS_ENSURE_SUCCESS(rv, rv);

	bool hasMore;
	while (NS_SUCCEEDED(subFolders->HasMoreElements(&hasMore)) && hasMore) {
        nsCOMPtr<nsISupports> item;

		rv = subFolders->GetNext(getter_AddRefs(item));
		NS_ENSURE_SUCCESS(rv, rv);

		nsCOMPtr<nsIMsgFolder> msgFolder(do_QueryInterface(item));
		if (!msgFolder)
			continue;

		nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(msgFolder));
		if (ewsFolder) {
            nsCString folderId("");
            ewsFolder->GetFolderId(folderId);

            if (!folderId.IsEmpty()) {
                folderIds.AppendElement(folderId);
            }
        }

        GetAllFolderIds(msgFolder, folderIds);
	}

	return NS_OK;
}

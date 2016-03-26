#include "MailEwsIncomingServer.h"
#include "MailEwsMsgFolder.h"

#include "MailEwsLog.h"
#include <vector>
#include <string>

#include "nsThreadUtils.h"
#include "nsIProxyInfo.h"
#include "nsProxyRelease.h"
#include "nsDebug.h"
#include "nsIMsgWindow.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgMailSession.h"
#include "nsThreadUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsXPCOMCIDInternal.h"
#include "nsMsgFolderFlags.h"
#include "nsIAbManager.h"
#include "nsAbBaseCID.h"
#include "nsIAbDirectory.h"
#include "nsNetUtil.h"
#include "nsIMsgIdentity.h"
#include "nsIMsgAccountManager.h"

#include "calICalendarManager.h"
#include "calICalendar.h"
#include "calIOperation.h"

#include "MailEwsService.h"
#include "MailEwsSyncFolderUtils.h"
#include "MailEwsCID.h"
#include "MailEwsErrorInfo.h"

NS_IMPL_ADDREF_INHERITED(MailEwsMsgIncomingServer, MsgIncomingServerBase)
NS_IMPL_RELEASE_INHERITED(MailEwsMsgIncomingServer, MsgIncomingServerBase)
NS_IMPL_QUERY_HEAD(MailEwsMsgIncomingServer)
NS_IMPL_QUERY_BODY(IMailEwsMsgIncomingServer)
NS_IMPL_QUERY_TAIL_INHERITING(MsgIncomingServerBase)

MailEwsMsgIncomingServer::MailEwsMsgIncomingServer()
: m_SyncingFolder(false)
		, m_PromptingPassword(false)
		, m_pSyncFolderCallback(new SyncFolderCallback(this))
		, m_IsShuttingDown(false) {
}

MailEwsMsgIncomingServer::~MailEwsMsgIncomingServer() {
}

NS_IMETHODIMP MailEwsMsgIncomingServer::GetLocalStoreType(nsACString & aLocalStoreType)
{
	aLocalStoreType.AssignLiteral("mailbox");
	return NS_OK;
}

nsresult MailEwsMsgIncomingServer::CreateRootFolderFromUri(const nsCString& serverUri, nsIMsgFolder** rootFolder) {
	MailEwsMsgFolder * newRootFolder = new MailEwsMsgFolder();

	if (!newRootFolder)
		return NS_ERROR_OUT_OF_MEMORY;

	NS_ADDREF(*rootFolder = newRootFolder);

	newRootFolder->Init(serverUri.get());

	CreateContactDirectory();
	CreateCalendar();

	return NS_OK;
}

NS_IMETHODIMP MailEwsMsgIncomingServer::PerformExpand(nsIMsgWindow *aMsgWindow) {
	nsCOMPtr<IMailEwsService> service;

	nsresult rv = GetService(getter_AddRefs(service));
	NS_ENSURE_SUCCESS(rv, rv);
	
	return service->SyncFolder(nullptr);
}

// class DLL_LOCAL GetServiceTask : public nsRunnable
// {
// public:
// 	GetServiceTask(nsIMsgIncomingServer * pIncomingServer,
// 	                   IMailEwsService ** ppService)
// 			: m_pIncomingServer(pIncomingServer)
// 			, m_ppService(ppService) {
		
// 	}

// 	NS_IMETHOD Run() {
// 		MOZ_ASSERT(NS_IsMainThread()); // This method is supposed to run on the

//         mailews_logger << "run get service tasks...................." << std::endl;
//         nsCOMPtr<IMailEwsService> ewsService;
//         ewsService = do_CreateInstance(MAIL_EWS_SERVICE_CONTRACTID);
// 		ewsService->InitWithIncomingServer(m_pIncomingServer);

//         NS_ADDREF(*m_ppService = ewsService);
// 		ewsService->ProcessQueuedTasks();

// 		return NS_OK;
// 	}

// private:
// 	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
//     IMailEwsService ** m_ppService;
// };

/* IMailEwsService getService (); */
NS_IMETHODIMP MailEwsMsgIncomingServer::GetService(IMailEwsService * *_retval)
{
	if (!m_pService) {
		//Do not create new service when shutdown
		if (m_IsShuttingDown) {
			*_retval = nullptr;
			return NS_ERROR_FAILURE;
		}
    
		m_pService = do_CreateInstance(MAIL_EWS_SERVICE_CONTRACTID);
		m_pService->InitWithIncomingServer(this);

		CreateContactDirectory();
		CreateCalendar();
		// nsCOMPtr<nsIRunnable> task =
		//         new GetServiceTask(this,
		//                            getter_AddRefs(m_pService));
		// NS_DispatchToMainThread(task);

		// return NS_ERROR_FAILURE;
	}

	NS_ADDREF(*_retval = m_pService);
	return NS_OK;
}

/* attribute bool isSyncingFolder; */
NS_IMETHODIMP MailEwsMsgIncomingServer::GetIsSyncingFolder(bool *aIsSyncingFolder)
{
	*aIsSyncingFolder = m_SyncingFolder;
	return NS_OK;
}

NS_IMETHODIMP MailEwsMsgIncomingServer::SetIsSyncingFolder(bool aIsSyncingFolder)
{
	m_SyncingFolder = aIsSyncingFolder;
	return NS_OK;
}

/* attribute bool isPromptingPassword; */
NS_IMETHODIMP MailEwsMsgIncomingServer::GetIsPromptingPassword(bool *aIsPromptingPassword)
{
	*aIsPromptingPassword = m_PromptingPassword;
	return NS_OK;
}

NS_IMETHODIMP MailEwsMsgIncomingServer::SetIsPromptingPassword(bool aIsPromptingPassword)
{
	m_PromptingPassword = aIsPromptingPassword;
	return NS_OK;
}

NS_IMETHODIMP MailEwsMsgIncomingServer::GetFolderWithFlags(unsigned int flags,
                                                           nsIMsgFolder** ppFolder) {
	nsCOMPtr<IMailEwsService> service;

	nsresult rv = GetService(getter_AddRefs(service));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIMsgFolder> rootFolder;
	rv = GetRootFolder(getter_AddRefs(rootFolder));
	NS_ENSURE_SUCCESS(rv, rv);

	nsTArray<nsCString> * folderIds = NULL;
	rv = service->GetFolderIdNamesToFolderId(&folderIds);
	NS_ENSURE_SUCCESS(rv, rv);

	nsCString empty("");
	int folder_id_name = -1;

	switch(flags) {
	case nsMsgFolderFlags::SentMail:
		folder_id_name = EWSDistinguishedFolderIdName_sentitems;
		break;
	case nsMsgFolderFlags::Drafts:
		folder_id_name = EWSDistinguishedFolderIdName_drafts;
		break;
	case nsMsgFolderFlags::Trash:
		folder_id_name = EWSDistinguishedFolderIdName_deleteditems;
		break;
	case nsMsgFolderFlags::Queue:
		folder_id_name = EWSDistinguishedFolderIdName_outbox;
		break;
	case nsMsgFolderFlags::Junk:
		folder_id_name = EWSDistinguishedFolderIdName_junkemail;
		break;
	case nsMsgFolderFlags::Inbox:
		folder_id_name = EWSDistinguishedFolderIdName_inbox;
		break;
	default:
		break;
	}

	if (folder_id_name < 0) {
		*ppFolder = NULL;
		return NS_ERROR_FAILURE;
	}

	nsCString folder_id = folderIds->SafeElementAt(folder_id_name, empty);

	if (folder_id.Length() == 0) {
		*ppFolder = NULL;
		return NS_ERROR_FAILURE;
	}

	nsCOMPtr<IMailEwsMsgFolder> ewsRootFolder(do_QueryInterface(rootFolder));
	
	return ewsRootFolder->GetChildWithFolderId(folder_id,
	                                           true,
	                                           ppFolder);
}

NS_IMETHODIMP MailEwsMsgIncomingServer::GetPasswordWithoutUI() {
	return MsgIncomingServerBase::GetPasswordWithoutUI();
}

NS_IMETHODIMP MailEwsMsgIncomingServer::GetSyncFolderCallback(SyncFolderCallback ** ppCallback) {
	*ppCallback = m_pSyncFolderCallback.get();

	return NS_OK;
}

NS_IMETHODIMP
MailEwsMsgIncomingServer::Shutdown() {
	nsCOMPtr<IMailEwsService> service;
	m_IsShuttingDown = true;

	nsresult rv = GetService(getter_AddRefs(service));

	if (NS_SUCCEEDED(rv)) {
		service->Shutdown();
	}

	nsCOMPtr<nsIMsgFolder> rootFolder;
	GetRootFolder(getter_AddRefs(rootFolder));
	if (rootFolder)
		rootFolder->Shutdown(true);

	MsgIncomingServerBase::Shutdown();

	m_IsShuttingDown = false;

	return NS_OK;
}

NS_IMETHODIMP
MailEwsMsgIncomingServer::VerifyLogon(nsIUrlListener *aUrlListener,
                                      nsIMsgWindow *aMsgWindow,
                                      nsIURI **aURL)
{
	return NS_OK;
}

NS_IMETHODIMP
MailEwsMsgIncomingServer::PerformBiff(nsIMsgWindow* aMsgWindow)
{
	nsCOMPtr<nsIMsgFolder> rootFolder;
	nsresult rv = GetRootFolder(getter_AddRefs(rootFolder));
	NS_ENSURE_SUCCESS(rv, rv);

	return GetNewMessagesForFolder(rootFolder, true, aMsgWindow);
}

NS_IMETHODIMP
MailEwsMsgIncomingServer::GetNewMessagesForFolder(nsIMsgFolder * rootFolder,
                                                  bool deep,
                                                  nsIMsgWindow * aMsgWindow) {
	//loop all sub folders
	nsCOMPtr<nsISimpleEnumerator> subFolders;
	nsresult rv = rootFolder->GetSubFolders(getter_AddRefs(subFolders));
	NS_ENSURE_SUCCESS(rv, rv);

	if (m_IsShuttingDown)
		return NS_OK;

	bool hasMore;
	while (NS_SUCCEEDED(subFolders->HasMoreElements(&hasMore)) && hasMore) {
		if (m_IsShuttingDown)
            return NS_OK;
        
        nsCOMPtr<nsISupports> item;

		rv = subFolders->GetNext(getter_AddRefs(item));
		NS_ENSURE_SUCCESS(rv, rv);

		nsCOMPtr<nsIMsgFolder> msgFolder(do_QueryInterface(item));
		if (!msgFolder)
			continue;

		rv = msgFolder->GetNewMessages(aMsgWindow, nullptr);
		NS_ENSURE_SUCCESS(rv, rv);
		
		if (deep) {
			rv = GetNewMessagesForFolder(msgFolder, deep, aMsgWindow);
			NS_ENSURE_SUCCESS(rv, rv);
		}
	}

	return NS_OK;
}

NS_IMETHODIMP MailEwsMsgIncomingServer::CreateContactDirectory() {
	nsresult rv = NS_OK;

    if (m_IsShuttingDown)
        return rv;

	nsCOMPtr<nsIAbManager> abManager(do_GetService(NS_ABMANAGER_CONTRACTID,
	                                               &rv));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCString retVal;
	nsString dirName;
	nsCString uri("contactews-abdirectory://");
	nsCString prefName("");

	nsCString aResult;
	nsCString username;
	rv = GetUsername(username);
	aResult.Append(username);
	aResult.Append('@');
	nsCString hostname;
	rv = GetHostName(hostname);
	aResult.Append(hostname);

	dirName.AppendLiteral(aResult.get());
	dirName.AppendLiteral(" Contacts");
	uri.Append(aResult);

	nsCOMPtr<nsIAbDirectory> dir;
    
	rv = abManager->GetDirectory(uri, getter_AddRefs(dir));

	if (NS_SUCCEEDED(rv) && dir) {
		nsCOMPtr<nsIAbDirectory> rootAddressBook(do_GetService(NS_ABDIRECTORY_CONTRACTID, &rv));
		NS_ENSURE_SUCCESS(rv, rv);

		bool hasDir = false;

		rv = rootAddressBook->HasDirectory(dir, &hasDir);

		if (NS_SUCCEEDED(rv) && hasDir) {
			abManager->DeleteAddressBook(uri);
			rootAddressBook->DeleteDirectory(dir);
		}
	}
    
	rv = abManager->NewAddressBook(dirName, //description
	                               uri,
	                               3,//LDAPDirectory
	                               prefName,
	                               retVal);
	NS_ENSURE_SUCCESS(rv, rv);

	return NS_OK;
}

nsresult
MailEwsMsgIncomingServer::CreateCalendarUriAndName(nsIURI ** _ret,
                                                  nsCString & name) {
	nsresult rv = NS_OK;

	nsCString uri("calendarews://");

	nsCString username;
	rv = GetUsername(username);
	uri.Append(username);

	uri.Append('@');

    nsCString hostname;
	rv = GetHostName(hostname);
	uri.Append(hostname);

	nsCOMPtr<nsIURI> calendarUri;
	rv = NS_NewURI(getter_AddRefs(calendarUri),
	               uri);

    name.Append(username);
    name.Append('@');
    name.Append(hostname);
    name.Append(" Calendar");

    NS_IF_ADDREF(*_ret = calendarUri);
    return rv;
}

NS_IMETHODIMP
MailEwsMsgIncomingServer::GetCalendar(calICalendar ** cal) {
	nsresult rv = NS_OK;

    if (m_IsShuttingDown)
        return rv;
    
	nsCOMPtr<calICalendarManager> cm(do_GetService("@mozilla.org/calendar/manager;1",
	                                               &rv));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIURI> calendarUri;
    nsCString name;

    rv = CreateCalendarUriAndName(getter_AddRefs(calendarUri),
                                  name);
	NS_ENSURE_SUCCESS(rv, rv);

	calICalendar ** pp_calendars = nullptr;
	uint32_t count = 0;
    
    nsCString uri;
    calendarUri->GetSpec(uri);

	cm->GetCalendars(&count, &pp_calendars);

	for(uint32_t i = 0;i < count; i++) {
		nsCOMPtr<nsIURI> u;
		rv = pp_calendars[i]->GetUri(getter_AddRefs(u));

		if (NS_SUCCEEDED(rv) && u) {
			nsCString spec;
			u->GetSpec(spec);

			if (uri.Equals(spec)) {
				//find existing calendar
                NS_IF_ADDREF(*cal = pp_calendars[i]);
                return NS_OK;
			}
		}
	}

    return NS_OK;
}

NS_IMETHODIMP MailEwsMsgIncomingServer::CreateCalendar() {
	nsresult rv = NS_OK;

    if (m_IsShuttingDown)
        return rv;
    
	nsCOMPtr<calICalendarManager> cm(do_GetService("@mozilla.org/calendar/manager;1",
	                                               &rv));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<calICalendar> calendar;
    GetCalendar(getter_AddRefs(calendar));

    if (calendar) {
        return NS_OK;
    }

	nsCOMPtr<nsIURI> calendarUri;
    nsCString name;

    rv = CreateCalendarUriAndName(getter_AddRefs(calendarUri),
                                  name);
	NS_ENSURE_SUCCESS(rv, rv);

	rv = cm->CreateCalendar(nsCString("ews"),
	                        calendarUri,
	                        getter_AddRefs(calendar));
	NS_ENSURE_SUCCESS(rv, rv);
    
	nsCOMPtr<nsIMsgAccountManager> accountMgr = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIMsgAccount> account;
	rv = accountMgr->FindAccountForServer(this,
	                                      getter_AddRefs(account));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIMsgIdentity> identity;
	rv = account->GetDefaultIdentity(getter_AddRefs(identity));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCString key;
	rv = identity->GetKey(key);
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIWritableVariant> v =
			do_CreateInstance("@mozilla.org/variant;1", &rv);
	NS_ENSURE_SUCCESS(rv, rv);
	rv = v->SetAsACString(key);
	NS_ENSURE_SUCCESS(rv, rv);
    
	calendar->SetProperty(nsCString("imip.identity.key"),
	                      v);

	calendar->SetName(name);
	return cm->RegisterCalendar(calendar);
}

extern void ResetPasswordPromptStatus();

NS_IMETHODIMP
MailEwsMsgIncomingServer::CloseCachedConnections() {
	MsgIncomingServerBase::CloseCachedConnections();

	if (m_IsShuttingDown)
		return NS_OK;

    mailews_logger << "Close cached connections, also clean password prompt flag."
                   << std::endl;
    
	ResetPasswordPromptStatus();
	m_PromptingPassword = false;
	nsCOMPtr<IMailEwsService> service;
	if (m_pService)
		m_pService->Shutdown();
    
	m_pService = NULL;

	return NS_OK;
}

NS_IMETHODIMP
MailEwsMsgIncomingServer::OnPasswordChanged() {
    return CloseCachedConnections();
}


class DLL_LOCAL RefreshCalendarTask : public nsRunnable
{
public:
	RefreshCalendarTask(nsIMsgIncomingServer * pIncomingServer)
			: m_pIncomingServer(pIncomingServer) {
    }

	NS_IMETHOD Run() {	
        nsresult rv = NS_OK;
    
        nsCOMPtr<calICalendarManager> cm(do_GetService("@mozilla.org/calendar/manager;1",
                                                       &rv));
        NS_ENSURE_SUCCESS(rv, rv);

		nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer));

        nsCOMPtr<calICalendar> calendar;
        ewsServer->GetCalendar(getter_AddRefs(calendar));

        if (calendar) {
            nsCOMPtr<calIOperation> op;
        
            calendar->Refresh(getter_AddRefs(op));
        }

        return NS_OK;
	}

private:
	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
};

NS_IMETHODIMP
MailEwsMsgIncomingServer::RefreshCalendar() {
    nsCOMPtr<nsIRunnable> resultrunnable =
            new RefreshCalendarTask(this);
    NS_DispatchToMainThread(resultrunnable);

    return NS_OK;
}

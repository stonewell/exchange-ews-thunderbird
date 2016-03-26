#include "MailEwsSubscriptionCallback.h"

#include "nsIMsgFolder.h"
#include "nsMsgBaseCID.h"
#include "nsIMsgIncomingServer.h"
#include "msgCore.h"
#include "nsThreadUtils.h"

#include "libews.h"
#include "IMailEwsMsgIncomingServer.h"

#include "MailEwsSyncItemUtils.h"
#include "MailEwsErrorInfo.h"

#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsIMsgAccountManager.h"
#include "nsIMsgIdentity.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"

#include "MailEwsItemsCallback.h"

#include "MailEwsLog.h"

MailEwsSubscriptionCallback::MailEwsSubscriptionCallback(nsIMsgIncomingServer * pIncomingServer)
    : m_URL(NULL)
    , m_pIncomingServer(pIncomingServer)
    , m_NotifyServer(NULL) {
    Initialize();
}

MailEwsSubscriptionCallback::~MailEwsSubscriptionCallback() {
    if (m_URL) free(m_URL);

    int len = m_Subscriptions.Length();

    for(int i=0;i < len;i++) {
	    ews_subscription * subscription = m_Subscriptions.ElementAt(i);
	    if (subscription) ews_free_subscription(subscription);
    }
}

static int folder_id_names[] = {
    EWSDistinguishedFolderIdName_calendar,
    EWSDistinguishedFolderIdName_tasks,
};

nsresult
MailEwsSubscriptionCallback::GetFolderWithId(const nsCString & folderId,
                                             nsIMsgFolder ** pFolder,
                                             bool & calendarOrTaskFolder) {
	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer));

    calendarOrTaskFolder = false;
    
	nsCOMPtr<IMailEwsService> service;

	nsresult rv = ewsServer-> GetService(getter_AddRefs(service));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIMsgFolder> rootFolder;
	rv = m_pIncomingServer->GetRootFolder(getter_AddRefs(rootFolder));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<IMailEwsMsgFolder> ewsRootFolder(do_QueryInterface(rootFolder));

    nsCOMPtr<nsIMsgFolder> parentFolder;
	rv = ewsRootFolder->GetChildWithFolderId(nsCString(folderId),
                                             true,
                                             getter_AddRefs(parentFolder));

    if (!parentFolder) {
        nsTArray<nsCString> * folderIds = NULL;
        rv = service->GetFolderIdNamesToFolderId(&folderIds);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCString empty("");

        for(size_t i = 0; i < sizeof(folder_id_names) / sizeof(int); i++) {
            nsCString folder_id =
                    folderIds->SafeElementAt(folder_id_names[i],
                                             empty);
            if (folder_id.Equals(folderId)) {
                calendarOrTaskFolder = true;
                return NS_OK;
            }
        }
    }

    NS_IF_ADDREF(*pFolder = parentFolder);
    return NS_OK;
}

void MailEwsSubscriptionCallback:: NewMail(const char * parentFolderId,
                                           const char * itemId){
	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer));

	nsCOMPtr<IMailEwsService> service;

	nsresult rv = ewsServer-> GetService(getter_AddRefs(service));
	NS_ENSURE_SUCCESS_VOID(rv);
    
    nsCOMPtr<nsIMsgFolder> parentFolder;
    bool calendarOrTaskFolder = false;
    
    rv = GetFolderWithId(nsCString(parentFolderId),
                         getter_AddRefs(parentFolder),
                         calendarOrTaskFolder);
    NS_ENSURE_SUCCESS_VOID(rv);

    if (!parentFolder && !calendarOrTaskFolder) {
        mailews_logger << "!!!!!!!New Mail in Unknown folder:"
                       << parentFolderId
                       << std::endl;
        return;
    }

    if (parentFolder) {
        nsString folderName;
        rv = parentFolder->GetName(folderName);
        nsCString utf7LeafName;
        CopyUTF16toUTF8(folderName, utf7LeafName);

        mailews_logger << "New Mail in folder:"
                       << utf7LeafName.get()
                       << std::endl;

        rv = service->SyncItems(parentFolder, nullptr);
        NS_ENSURE_SUCCESS_VOID(rv);
    } else if (calendarOrTaskFolder) {
        mailews_logger << "Refresh Calendar and Task"
                       << std::endl;
        ewsServer->RefreshCalendar();
    }
}

void MailEwsSubscriptionCallback:: MoveItem(const char * oldParentFolderId,
                                            const char * parentFolderId,
                                            const char * oldItemId,
                                            const char * itemId){
    mailews_logger << "Move Item:"
              << oldParentFolderId
              << "," << parentFolderId
              << "," << oldItemId
              << "," << itemId
              << std::endl;

	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer));

	nsCOMPtr<IMailEwsService> service;

	nsresult rv = ewsServer-> GetService(getter_AddRefs(service));
	NS_ENSURE_SUCCESS_VOID(rv);
    
    nsCOMPtr<nsIMsgFolder> parentFolder;
    bool calendarOrTask = false;
	rv = GetFolderWithId(nsCString(parentFolderId),
                         getter_AddRefs(parentFolder),
                         calendarOrTask);
	NS_ENSURE_SUCCESS_VOID(rv);

	nsCOMPtr<nsIMsgFolder> oldParentFolder;
    bool oldCalendarOrTask = false;
	rv = GetFolderWithId(nsCString(oldParentFolderId),
                         getter_AddRefs(oldParentFolder),
                         oldCalendarOrTask);
	NS_ENSURE_SUCCESS_VOID(rv);

    if (parentFolder) {
        rv = service->SyncItems(parentFolder, nullptr);
        NS_ENSURE_SUCCESS_VOID(rv);
    } else if (calendarOrTask) {
        mailews_logger << "Refresh Calendar and Task"
                       << std::endl;
        ewsServer->RefreshCalendar();
    } else {
        mailews_logger << "Parent Folder Not found:"
                  << parentFolderId
                  << std::endl;
    }

    if (oldParentFolder) {
        rv = service->SyncItems(oldParentFolder, nullptr);
        NS_ENSURE_SUCCESS_VOID(rv);
    } else if (oldCalendarOrTask) {
        mailews_logger << "Refresh Calendar and Task"
                       << std::endl;
        ewsServer->RefreshCalendar();
    } else {
        mailews_logger << "Old Parent Folder Not found:"
                  << parentFolderId
                  << std::endl;
    }        
}
	
void MailEwsSubscriptionCallback:: ModifyItem(const char * parentFolderId,
                                              const char * itemId,
                                              int unreadCount){
    mailews_logger << "Modify Item:"
              << "," << parentFolderId
              << "," << itemId
              << "," << unreadCount
              << std::endl;

    NewMail(parentFolderId, itemId);
}

void MailEwsSubscriptionCallback:: CopyItem(const char * oldParentFolderId,
                                            const char * parentFolderId,
                                            const char * oldItemId,
                                            const char * itemId){
    mailews_logger << "Copy Item:"
              << oldParentFolderId
              << "," << parentFolderId
              << "," << oldItemId
              << "," << itemId
              << std::endl;

    MoveItem(oldParentFolderId,
             parentFolderId,
             oldItemId,
             itemId);
}
	
void MailEwsSubscriptionCallback:: DeleteItem(const char * parentFolderId,
                                              const char * itemId){
    mailews_logger << "Delete Item:"
              << "," << parentFolderId
              << "," << itemId
              << std::endl;
    NewMail(parentFolderId, itemId);
}
	
void MailEwsSubscriptionCallback:: CreateItem(const char * parentFolderId,
                                              const char * itemId){
    mailews_logger << "Create Item:"
              << "," << parentFolderId
              << "," << itemId
              << std::endl;
    NewMail(parentFolderId, itemId);
}

void MailEwsSubscriptionCallback:: MoveFolder(const char * oldParentFolderId,
                                              const char * parentFolderId,
                                              const char * oldFolderId,
                                              const char * folderId){
}
    
void MailEwsSubscriptionCallback:: ModifyFolder(const char * parentFolderId,
                                                const char * folderId,
                                                int unreadCount){
}
	
void MailEwsSubscriptionCallback:: CopyFolder(const char * oldParentFolderId,
                                              const char * parentFolderId,
                                              const char * oldFolderId,
                                              const char * folderId){
}
	
void MailEwsSubscriptionCallback:: DeleteFolder(const char * parentFolderId,
                                                const char * folderId){
}
	
void MailEwsSubscriptionCallback:: CreateFolder(const char * parentFolderId,
                                                const char * folderId){
}

#define PTR(x) ((MailEwsSubscriptionCallback*)x)

static void move(const char * old_parent_folder_id,
                 const char * parent_folder_id,
                 const char * old_id,
                 const char * id,
                 bool is_item,
                 void * user_data) {
    if (is_item)
        PTR(user_data)->MoveItem(old_parent_folder_id,
                                 parent_folder_id,
                                 old_id,
                                 id);
    else
        PTR(user_data)->MoveFolder(old_parent_folder_id,
                                   parent_folder_id,
                                   old_id,
                                   id);
}

static void copy(const char * old_parent_folder_id,
                 const char * parent_folder_id,
                 const char * old_id,
                 const char * id,
                 bool is_item,
                 void * user_data) {
    if (is_item)
        PTR(user_data)->CopyItem(old_parent_folder_id,
                                 parent_folder_id,
                                 old_id,
                                 id);
    else
        PTR(user_data)->CopyFolder(old_parent_folder_id,
                                   parent_folder_id,
                                   old_id,
                                   id);
}

static void new_mail(const char * parent_folder_id,
                     const char * id,
                     bool is_item,
                     void * user_data) {
    PTR(user_data)->NewMail(parent_folder_id,
                            id);
}

static void modify(const char * parent_folder_id,
                   const char * id,
                   int unreadCount,
                   bool is_item,
                   void * user_data) {

    if (is_item)
        PTR(user_data)->ModifyItem(parent_folder_id,
                                   id,
                                   unreadCount);
    else
        PTR(user_data)->ModifyFolder(parent_folder_id,
                                     id,
                                     unreadCount);
}

static void remove(const char * parent_folder_id,
                   const char * id,
                   bool is_item,
                   void * user_data) {
    if (is_item)
        PTR(user_data)->DeleteItem(parent_folder_id,
                                   id);
    else
        PTR(user_data)->DeleteFolder(parent_folder_id,
                                     id);
}

static void create(const char * parent_folder_id,
                   const char * id,
                   bool is_item,
                   void * user_data) {
    if (is_item)
        PTR(user_data)->CreateItem(parent_folder_id,
                                   id);
    else
        PTR(user_data)->CreateFolder(parent_folder_id,
                                     id);
}

void MailEwsSubscriptionCallback::Initialize() {
    memset(&m_Callback, 0, sizeof(ews_event_notification_callback));

    m_Callback.new_mail = new_mail;
    m_Callback.modify = modify;
    m_Callback.remove = remove;
    m_Callback.create = create;
    m_Callback.move = move;
    m_Callback.copy = copy;
    m_Callback.user_data = this;
}


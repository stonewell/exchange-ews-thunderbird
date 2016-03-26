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
#include "nsArrayUtils.h"

#include "MailEwsItemsCallback.h"
#include "IMailEwsItemOp.h"

#include "MailEwsLog.h"

#define USER_PTR(x) ((SyncItemCallback*)x)

static const char * get_sync_state(void * user_data);
static void set_sync_state(const char * sync_state, void * user_data);
static void new_item(const ews_item * item, void * user_data);
static void update_item(const ews_item * item, void * user_data);
static void delete_item(const ews_item * item, void * user_data);
static void read_item(const ews_item * item, int read, void * user_data);
static int get_max_return_item_count(void * user_data);
static char ** get_ignored_item_ids(int * p_count, void * user_data);

SyncItemCallback::SyncItemCallback(nsIMsgFolder * pFolder)
	: m_SyncState("")
	, m_pFolder(pFolder)
	, m_pCallback(new ews_sync_item_callback)
	, m_Initialized(false)
	, m_Running(false) {

	
}

SyncItemCallback::~SyncItemCallback() {
}

NS_IMETHODIMP SyncItemCallback::Initialize() {
	if (m_Initialized)
		return NS_OK;
	
	//setup sync item callback
	m_pCallback->get_sync_state = get_sync_state;
	m_pCallback->set_sync_state = set_sync_state;
	m_pCallback->new_item = new_item;
	m_pCallback->update_item = update_item;
	m_pCallback->delete_item = delete_item;
	m_pCallback->read_item = read_item;
	m_pCallback->get_max_return_item_count = get_max_return_item_count;
	m_pCallback->get_ignored_item_ids = get_ignored_item_ids;
	m_pCallback->user_data = this;

	m_Initialized = true;
	
	return NS_OK;
}

void SyncItemCallback::Reset() {
	m_NewItemInfos.clear();
	m_UpdateItemInfos.clear();
	m_DelItemInfos.clear();
	m_ReadItemInfos.clear();
	m_DelRemoteItemInfos.clear();
	m_ReadRemoteItemInfos.clear();
}

const std::string & SyncItemCallback::GetSyncState() const {
	return m_SyncState;
}

void SyncItemCallback::SetSyncState(const std::string & syncState) {
	m_SyncState = syncState;
}

void SyncItemCallback::NewItem(const ews_item * pItem) {
    if (pItem->item_type == EWS_Item_Message) {
        ItemInfo fInfo(pItem);
        m_NewItemInfos.push_back(fInfo);
    }
}

void SyncItemCallback::UpdateItem(const ews_item * pItem) {
    if (pItem->item_type == EWS_Item_Message) {
        ItemInfo fInfo(pItem);
        m_UpdateItemInfos.push_back(fInfo);
    }
}

void SyncItemCallback::DeleteItem(const ews_item * pItem) {
	ItemInfo fInfo(pItem);

	m_DelItemInfos.push_back(fInfo);
}

void SyncItemCallback::ReadItem(const ews_item * pItem, bool read) {
	ItemInfo fInfo(pItem, read);

	m_ReadItemInfos.push_back(fInfo);
}

void SyncItemCallback::TaskBegin() {
}

void SyncItemCallback::TaskDone(int result) {
	nsCString syncState(m_SyncState.c_str());

	nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(m_pFolder));
	ewsFolder->SetSyncState(syncState);

    if (HasItem()) {
		//Save item ids needs to process in folder
		//after success process, item id will be removed
		//otherwise will be reprocess when next time sync items
		SaveItemOps();
    }

	SetRunning(false);

	//if has item, start next round
	//if failed, will queued in delayed task queue
	if (HasItem() && result == EWS_SUCCESS) {
        mailews_logger << "start next round sync items"
                       << std::endl;

        nsCOMPtr<nsIMsgIncomingServer> server;
        nsresult rv = m_pFolder->GetServer(getter_AddRefs(server));
        NS_ENSURE_SUCCESS_VOID(rv);

        nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(server, &rv));
        NS_ENSURE_SUCCESS_VOID(rv);

        nsCOMPtr<IMailEwsService> ewsService;
        rv = ewsServer->GetService(getter_AddRefs(ewsService));
        NS_ENSURE_SUCCESS_VOID(rv);

        ewsService->SyncItems(m_pFolder, nullptr);
	}
}

bool SyncItemCallback::HasItem() const {
	return !(m_NewItemInfos.empty() &&
	         m_UpdateItemInfos.empty() &&
	         m_DelItemInfos.empty() &&
	         m_ReadItemInfos.empty());
}

static const char * get_sync_state(void * user_data) {
	return USER_PTR(user_data)->GetSyncState().c_str();
}

static void set_sync_state(const char * sync_state, void * user_data) {
	USER_PTR(user_data)->SetSyncState(sync_state);
}

static void new_item(const ews_item * item, void * user_data){
	USER_PTR(user_data)->NewItem(item);
}

static void update_item(const ews_item * item, void * user_data) {
	USER_PTR(user_data)->UpdateItem(item);
}

static void delete_item(const ews_item * item, void * user_data) {
	USER_PTR(user_data)->DeleteItem(item);
}

static void read_item(const ews_item * item, int read, void * user_data) {
	USER_PTR(user_data)->ReadItem(item, read ? true : false);
}

static int get_max_return_item_count(void * user_data) {
	return 500;
}

static char ** get_ignored_item_ids(int * p_count, void * user_data) {
	*p_count = 0;
	(void)user_data;
	return NULL;
}

NS_IMETHODIMP SyncItemCallback::ProcessItems() {
	if (!HasItem()) return NS_OK;

	mailews_logger << "New Item Count:" << m_NewItemInfos.size()
	               << ", Update Item Count:" << m_UpdateItemInfos.size()
	               << ", Del Remote Count:" << m_DelRemoteItemInfos.size()
	               << ", Del Remote Trash Count:" << m_DelRemoteTrashItemInfos.size()
	               << ", Mark Read Remote Count:" << m_ReadRemoteItemInfos.size()
	               << ", Del Local Count:" << m_DelItemInfos.size()
	               << ", Mark Read Local Count:" << m_ReadItemInfos.size()
	               << std::endl;

	nsCOMPtr<nsIMsgIncomingServer> server;

	nsresult rv = m_pFolder->GetServer(getter_AddRefs(server));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(server));

	nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(m_pFolder));
    
	nsCOMPtr<IMailEwsService> ewsService;
	ewsServer->GetService(getter_AddRefs(ewsService));

	rv = LoadRemoteItems(&m_NewItemInfos, ewsService);
	NS_ENSURE_SUCCESS(rv, rv);
    
	rv = LoadRemoteItems(&m_UpdateItemInfos, ewsService);
	NS_ENSURE_SUCCESS(rv, rv);

	rv = DelRemoteItems(&m_DelRemoteItemInfos, false, ewsService);
	NS_ENSURE_SUCCESS(rv, rv);
	
	rv = DelRemoteItems(&m_DelRemoteTrashItemInfos, true, ewsService);
	NS_ENSURE_SUCCESS(rv, rv);

	rv = MarkReadRemoteItems(&m_ReadRemoteItemInfos, ewsService);
	NS_ENSURE_SUCCESS(rv, rv);

	if (!m_DelItemInfos.empty()) {
		nsCOMPtr<IMailEwsItemsCallback> callback;
		callback = new MailEwsDeleteItemsCallback(true);

		std::vector<ItemInfo>::iterator it;
		nsTArray<nsCString> itemIds;
         
		for(it = m_DelItemInfos.begin(); it != m_DelItemInfos.end(); it++) {
			ItemInfo * pItemInfo = &(*it);

			if (!pItemInfo->itemId.empty()) {
				itemIds.AppendElement(nsCString(pItemInfo->itemId.c_str()));
			}
		}

		callback->SetItemIds(&itemIds);
		callback->SetFolder(m_pFolder);

		rv = ewsService->ProcessItems(m_pFolder,
		                              callback,
		                              nullptr);
		NS_ENSURE_SUCCESS(rv, rv);
	}
    
	if (!m_ReadItemInfos.empty()) {
		nsCOMPtr<IMailEwsItemsCallback> callback;
		MailEwsReadItemsCallback * readCallback;
		callback = readCallback = new MailEwsReadItemsCallback(true);

		std::vector<ItemInfo>::iterator it;
		nsTArray<nsCString> itemIds;

		std::map<std::string, bool> * pItemReadMap = readCallback->GetItemReadMap();

		for(it = m_ReadItemInfos.begin(); it != m_ReadItemInfos.end(); it++) {
			ItemInfo * pItemInfo = &(*it);

			if (pItemInfo && !pItemInfo->itemId.empty()) {
				itemIds.AppendElement(nsCString(pItemInfo->itemId.c_str()));
				pItemReadMap->insert(std::pair<std::string, bool>(pItemInfo->itemId, pItemInfo->read));
			}
		}

		callback->SetItemIds(&itemIds);
		callback->SetFolder(m_pFolder);

		rv = ewsService->ProcessItems(m_pFolder,
		                              callback,
		                              nullptr);
		NS_ENSURE_SUCCESS(rv, rv);
	}

	return NS_OK;
}

NS_IMETHODIMP SyncItemCallback::LoadRemoteItems(std::vector<ItemInfo> * itemInfos,
                                                IMailEwsService * ewsService) {
	std::vector<ItemInfo>::iterator it = itemInfos->begin();
	nsresult rv = NS_OK;
        
	while(it != itemInfos->end()) {
		nsCOMPtr<IMailEwsItemsCallback> callback;
		callback = new MailEwsLoadItemsCallback();

		nsTArray<nsCString> itemIds;
         
		for(int i=0;i < 10 && it != itemInfos->end(); i++, it++) {
			ItemInfo * pItemInfo = &(*it);

			if (!pItemInfo->itemId.empty()) {
				itemIds.AppendElement(nsCString(pItemInfo->itemId.c_str()));
			}
		}

		callback->SetItemIds(&itemIds);
		callback->SetFolder(m_pFolder);

		rv = ewsService->ProcessItems(m_pFolder,
		                              callback,
		                              nullptr);
		NS_ENSURE_SUCCESS(rv, rv);
	}

	return NS_OK;
}

NS_IMETHODIMP SyncItemCallback::SaveItemOps() {
	SaveItemOps(&m_NewItemInfos, IMailEwsItemOp::Saved_Item_New);
	SaveItemOps(&m_UpdateItemInfos, IMailEwsItemOp::Saved_Item_Update);
	SaveItemOps(&m_DelItemInfos, IMailEwsItemOp::Saved_Item_Delete_Local);
	SaveItemOps(&m_ReadItemInfos, IMailEwsItemOp::Saved_Item_Read_Local);

	return NS_OK;
}

NS_IMETHODIMP SyncItemCallback::SaveItemOps(std::vector<ItemInfo> * itemInfos,
                                            int type) {
	nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(m_pFolder));

	std::vector<ItemInfo>::iterator it = itemInfos->begin();
	nsresult rv = NS_OK;
        
	while(it != itemInfos->end()) {
		ItemInfo * pItemInfo = &(*it);
		
		if (!pItemInfo->itemId.empty()) {
			nsCString itemId(pItemInfo->itemId.c_str());
			nsCString changeKey(pItemInfo->changeKey.c_str());

			nsCOMPtr<IMailEwsItemOp> itemOp;

			rv = ewsFolder->CreateItemOp(getter_AddRefs(itemOp));
			if (NS_SUCCEEDED(rv)) {
				itemOp->SetItemId(itemId);
				itemOp->SetChangeKey(changeKey);
				itemOp->SetRead(pItemInfo->read);
				itemOp->SetOpType(type);
			}
		}

		it++;
	}

	return NS_OK;
}

NS_IMETHODIMP SyncItemCallback::LoadItemOps() {
	nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(m_pFolder));
	nsTArray<nsCString> itemIds;
	nsresult rv = NS_OK;
	uint32_t count = 0;

	nsCOMPtr<nsIArray> itemOps;
	rv = ewsFolder->GetItemOps(getter_AddRefs(itemOps));

	rv = itemOps->GetLength(&count);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<IMailEwsItemOp> itemOp;
    for (uint32_t i = 0; i < count; ++i)
	{
		itemOp = do_QueryElementAt(itemOps, i, &rv);
		
		NS_FAILED_WARN(rv);
		if (NS_FAILED(rv))
			continue;

		ItemInfo itemInfo;
		nsCString itemId("");
		nsCString changeKey("");
		bool read = false;
		int opType = -1;
		
		itemOp->GetItemId(itemId);
		itemOp->GetChangeKey(changeKey);
		itemOp->GetRead(&read);
		itemOp->GetOpType(&opType);

		itemInfo.itemId = itemId.get();
		itemInfo.changeKey = changeKey.get();
		itemInfo.read = read;

		switch(opType) {
			/* Update and New are same op
		case IMailEwsItemOp::Saved_Item_Update:
			m_UpdateItemInfos.push_back(itemInfo);
			break;
			*/
		case IMailEwsItemOp::Saved_Item_New:
			m_NewItemInfos.push_back(itemInfo);
			break;
		case IMailEwsItemOp::Saved_Item_Delete_Local:
			m_DelItemInfos.push_back(itemInfo);
			break;
		case IMailEwsItemOp::Saved_Item_Read_Local:
			m_ReadItemInfos.push_back(itemInfo);
			break;
		case IMailEwsItemOp::Saved_Item_Delete_Remote:
			m_DelRemoteItemInfos.push_back(itemInfo);
			break;
		case IMailEwsItemOp::Saved_Item_Delete_Remote_Trash:
			m_DelRemoteTrashItemInfos.push_back(itemInfo);
			break;
		case IMailEwsItemOp::Saved_Item_Read_Remote:
			m_ReadRemoteItemInfos.push_back(itemInfo);
			break;
		default:
			break;
		}
	}

    nsString folderName;
    m_pFolder->GetName(folderName);
    nsCString cFolderName;
    CopyUTF16toUTF8(folderName, cFolderName);
    
    mailews_logger << "Load Item Ops:"
                   << HasItem()
                   << ",Folder:"
                   << cFolderName.get()
                   << std::endl;
	return NS_OK;
}

NS_IMETHODIMP SyncItemCallback::ProcessSavedItemOps() {
	LoadItemOps();
	return ProcessItems();
}

NS_IMETHODIMP SyncItemCallback::DelRemoteItems(std::vector<ItemInfo> * itemInfos,
                                               bool isTrashFolder,
                                               IMailEwsService * ewsService) {
	std::vector<ItemInfo>::iterator it;
	nsTArray<nsCString> itemIdsAndChangeKeys;

	if (itemInfos->size() == 0)
		return NS_OK;

	for(it = itemInfos->begin(); it != itemInfos->end(); it++) {
		ItemInfo * pItemInfo = &(*it);

		if (pItemInfo && !pItemInfo->itemId.empty()) {
			itemIdsAndChangeKeys.AppendElement(nsCString(pItemInfo->itemId.c_str()));
			itemIdsAndChangeKeys.AppendElement(nsCString(pItemInfo->changeKey.c_str()));
		}
	}

	if (itemIdsAndChangeKeys.Length() == 0)
		return NS_OK;
	
	return ewsService->DeleteItems(m_pFolder,
	                               &itemIdsAndChangeKeys,
                                   isTrashFolder,
                                   nullptr);
}

NS_IMETHODIMP SyncItemCallback::MarkReadRemoteItems(std::vector<ItemInfo> * itemInfos,
                                                    IMailEwsService * ewsService) {
	std::vector<ItemInfo>::iterator it;
	nsTArray<nsCString> itemIdsAndChangeKeysRead;
	nsTArray<nsCString> itemIdsAndChangeKeysUnRead;

	if (itemInfos->size() == 0)
		return NS_OK;

	for(it = itemInfos->begin(); it != itemInfos->end(); it++) {
		ItemInfo * pItemInfo = &(*it);

		if (pItemInfo && !pItemInfo->itemId.empty()) {
			if (pItemInfo->read) {
				itemIdsAndChangeKeysRead.AppendElement(nsCString(pItemInfo->itemId.c_str()));
				itemIdsAndChangeKeysRead.AppendElement(nsCString(pItemInfo->changeKey.c_str()));
			} else {
				itemIdsAndChangeKeysUnRead.AppendElement(nsCString(pItemInfo->itemId.c_str()));
				itemIdsAndChangeKeysUnRead.AppendElement(nsCString(pItemInfo->changeKey.c_str()));
			}
		}
	}

	if (itemIdsAndChangeKeysRead.Length() != 0) {
		ewsService->MarkItemsRead(m_pFolder,
		                          &itemIdsAndChangeKeysRead,
		                          true,
		                          nullptr);
	}

	if (itemIdsAndChangeKeysUnRead.Length() != 0) {
		ewsService->MarkItemsRead(m_pFolder,
		                          &itemIdsAndChangeKeysUnRead,
		                          false,
		                          nullptr);
	}
	
	return NS_OK;
}

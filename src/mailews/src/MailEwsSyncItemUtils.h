#pragma once
#ifndef __MAIL_EWS_SYNC_ITEM_UTILS_H__
#define __MAIL_EWS_SYNC_ITEM_UTILS_H__

#include "libews.h"
#include "MailEwsCommon.h"

class nsIRDFService;

class IMailEwsMsgIncomingServer;
class IMailEwsService;
class SyncItemCallback {
private:
	struct ItemInfo {
		ItemInfo(const ews_item * pItem, bool _read = false)
			: itemId(pItem->item_id ? pItem->item_id : "")
			, changeKey(pItem->change_key ? pItem->change_key : "")
            , body(pItem->body ? pItem->body : "")
            , display_cc(pItem->display_cc ? pItem->display_cc : "")
            , display_to(pItem->display_to ? pItem->display_to : "")
            , subject(pItem->subject ? pItem->subject : "")
            , read(_read) {
		}

		ItemInfo() {
		}

		std::string itemId;
		std::string changeKey;
        std::string body;
        std::string display_cc;
        std::string display_to;
        std::string subject;
        bool read;
	};
	
	std::string m_SyncState;
	nsCOMPtr<nsIMsgFolder> m_pFolder;
	std::vector<ItemInfo> m_NewItemInfos;
	std::vector<ItemInfo> m_UpdateItemInfos;
	std::vector<ItemInfo> m_DelItemInfos;
	std::vector<ItemInfo> m_ReadItemInfos;
	std::vector<ItemInfo> m_DelRemoteItemInfos;
	std::vector<ItemInfo> m_DelRemoteTrashItemInfos;
	std::vector<ItemInfo> m_ReadRemoteItemInfos;
	std::auto_ptr<ews_sync_item_callback> m_pCallback;
	bool m_Initialized;
	bool m_Running;
public:
	SyncItemCallback(nsIMsgFolder * pFolder);
	virtual ~SyncItemCallback();

	ews_sync_item_callback * GetEwsSyncItemCallback() const { return m_pCallback.get(); }
	
	virtual const std::string & GetSyncState() const;

	virtual void SetSyncState(const std::string & syncState);

	virtual void NewItem(const ews_item * pItem);
	virtual void UpdateItem(const ews_item * pItem);
	virtual void DeleteItem(const ews_item * pItem);
	virtual void ReadItem(const ews_item * pItem, bool read);

	virtual void TaskDone(int result);
	virtual void TaskBegin();

	bool HasItem() const;
	NS_IMETHOD ProcessItems();
	void Reset();

	NS_IMETHOD Initialize();
	bool IsRunning() const { return m_Running; }
	void SetRunning(bool running) { m_Running = running; }
	NS_IMETHOD ProcessSavedItemOps();
private:
    NS_IMETHOD LoadRemoteItems(std::vector<ItemInfo> * itemInfos,
                    IMailEwsService * service);
    NS_IMETHOD DelRemoteItems(std::vector<ItemInfo> * itemInfos,
                              bool isTrashFolder,
                              IMailEwsService * service);
    NS_IMETHOD MarkReadRemoteItems(std::vector<ItemInfo> * itemInfos,
                    IMailEwsService * service);
	
	NS_IMETHOD LoadItemOps();
	NS_IMETHOD SaveItemOps();
	NS_IMETHOD SaveItemOps(std::vector<ItemInfo> * itemInfos,
	                       int type);
};

#endif // __MAIL_EWS_SYNC_ITEM_UTILS_H__


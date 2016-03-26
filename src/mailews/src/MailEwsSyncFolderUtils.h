#pragma once
#ifndef __MAIL_EWS_SYNC_FOLDER_UTILS_H__
#define __MAIL_EWS_SYNC_FOLDER_UTILS_H__

#include "MailEwsSyncFolderTask.h"
#include "libews.h"

class nsIRDFService;

class IMailEwsMsgIncomingServer;
class SyncFolderCallback
		: public MailEwsTaskCallback {
private:
	struct FolderInfo {
		FolderInfo(ews_folder * pFolder)
			: folderId(pFolder->id ? pFolder->id : "")
			, parentFolderId(pFolder->parent_folder_id ? pFolder->parent_folder_id : "")
			, displayName(pFolder->display_name ? pFolder->display_name : "")
			, changeKey(pFolder->change_key ? pFolder->change_key : "") {
		}

		std::string folderId;
		std::string parentFolderId;
		std::string displayName;
		std::string changeKey;
	};
	
	std::string m_SyncState;
	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
	nsCOMPtr<nsIMsgFolder> m_RootFolder;
	std::vector<FolderInfo> m_NewFolderInfos;
	std::vector<FolderInfo> m_UpdateFolderInfos;
	std::vector<FolderInfo> m_DelFolderInfos;
	std::auto_ptr<ews_sync_folder_callback> m_pCallback;
	bool m_Initialized;
	bool m_FullSync;
public:
	SyncFolderCallback(nsIMsgIncomingServer * pIncomingServer);
	virtual ~SyncFolderCallback();

	ews_sync_folder_callback * GetEwsSyncFolderCallback() const { return m_pCallback.get(); }
	
	virtual const std::string & GetSyncState() const;

	virtual void SetSyncState(const std::string & syncState);
	virtual int GetFolderType() const;

	virtual void NewFolder(ews_folder * pFolder);

	virtual void UpdateFolder(ews_folder * pFolder);
	virtual void DeleteFolder(ews_folder * pFolder);

	virtual void TaskDone();
	virtual void TaskBegin();

	void Reset();

	NS_IMETHOD Initialize();
private:
	bool AddSubFolder(FolderInfo * pFolderInfo);
	bool RemoveSubFolder(FolderInfo * pFolderInfo);
	bool UpdateSubFolder(FolderInfo * pFolderInfo);

	NS_IMETHOD UpdateRootFolderId();
	NS_IMETHOD UpdateFolder(nsIMsgFolder * pFolder,
	                        FolderInfo * pFolderInfo);
	NS_IMETHOD UpdateFolderName(nsIMsgFolder * pFolder,
	                        const char * name);
	NS_IMETHOD MoveFolder(nsIMsgFolder * pFolder,
	                        const char * newParentFolderId);
	void VerifyFolders();
	void VerifyLocalFolder(nsIMsgFolder * pMsgFolder,
	                       std::map<std::string, std::string> * pFolderMap);
	NS_IMETHOD UpdateIdentity();
	bool CheckSpecialFolder(nsIRDFService *rdf,
	                        nsCString &folderUri,
	                        uint32_t folderFlag,
	                        nsCString &existingUri);
	void UpdateSpecialFolder();
};

#endif // __MAIL_EWS_SYNC_FOLDER_UTILS_H__


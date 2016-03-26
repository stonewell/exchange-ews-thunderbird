#ifndef __MAIL_EWS_MSG_FOLDER_H__
#define __MAIL_EWS_MSG_FOLDER_H__

#include "MsgDBFolderBase.h"
#include "IMailEwsMsgFolder.h"
#include "MailEwsCommon.h"

class SyncItemCallback;

class MailEwsMsgFolder
		: public MsgDBFolderBase
		, public IMailEwsMsgFolder {
public:
	NS_DECL_IMAILEWSMSGFOLDER
	NS_DECL_ISUPPORTS_INHERITED
	
	MailEwsMsgFolder();

private:
	virtual ~MailEwsMsgFolder();

public:
	NS_IMETHOD GetNewMessages(nsIMsgWindow *, nsIUrlListener * /* aListener */) override;
	NS_IMETHOD GetDeletable (bool *deletable) override;
	NS_IMETHOD RenameSubFolders(nsIMsgWindow *msgWindow, nsIMsgFolder *oldFolder) override;
	NS_IMETHOD Delete() override;
	NS_IMETHOD DeleteSubFolders(nsIArray* folders, nsIMsgWindow *msgWindow) override;
	NS_IMETHOD CreateSubfolder(const nsAString& folderName, nsIMsgWindow *msgWindow) override;
	NS_IMETHOD Rename(const nsAString& aNewName, nsIMsgWindow *msgWindow) override;
    NS_IMETHOD GetMessages(nsISimpleEnumerator **result) override;
    NS_IMETHOD UpdateFolder(nsIMsgWindow *) override;
    NS_IMETHOD GetFolderURL(nsACString& url) override;
    virtual nsresult CreateBaseMessageURI(const nsACString& aURI) override;
    NS_IMETHOD CopyFileMessage(nsIFile* aFile,
                               nsIMsgDBHdr* messageToReplace,
                               bool isDraftOrTemplate,
                               uint32_t aNewMsgFlags,
                               const nsACString &aNewMsgKeywords,
                               nsIMsgWindow *window,
                               nsIMsgCopyServiceListener* listener) override;
    NS_IMETHOD CopyFolder(nsIMsgFolder* srcFolder,
                          bool isMoveFolder,
                          nsIMsgWindow *window,
                          nsIMsgCopyServiceListener* listener) override;
    NS_IMETHOD CopyMessages(nsIMsgFolder* srcFolder,
                            nsIArray *messages,
                            bool isMove,
                            nsIMsgWindow *window,
                            nsIMsgCopyServiceListener* listener,
                            bool isFolder,
                            bool allowUndo) override;
    NS_IMETHOD DeleteMessages(nsIArray *messages,
                              nsIMsgWindow *msgWindow,
                              bool deleteStorage,
                              bool isMove,
                              nsIMsgCopyServiceListener *listener,
                              bool allowUndo) override;
    NS_IMETHOD OnReadChanged(nsIDBChangeListener * aInstigator) override;
    NS_IMETHOD MarkMessagesRead(nsIArray *messages, bool markRead) override;
    NS_IMETHOD MarkThreadRead(nsIMsgThread *thread) override;
    NS_IMETHOD MarkAllMessagesRead(nsIMsgWindow *aMsgWindow) override;
    NS_IMETHOD CompactAll(nsIUrlListener *aListener,
                          nsIMsgWindow *aMsgWindow,
                          bool aCompactOfflineAlso) override;
    NS_IMETHOD Compact(nsIUrlListener *aListener,
                       nsIMsgWindow *aMsgWindow) override;
    NS_IMETHOD NotifyCompactCompleted() override;
	NS_IMETHOD GetSizeOnDisk(int64_t* aSize) override;
protected:
	virtual void GetIncomingServerType(nsCString& serverType) override;
	virtual nsresult CreateChildFromURI(const nsCString &uri, nsIMsgFolder **folder) override;
	virtual nsresult GetDatabase() override;
	NS_IMETHOD GetSubFolders(nsISimpleEnumerator **aResult) override;
	NS_IMETHOD GetDBFolderInfoAndDB(nsIDBFolderInfo **folderInfo,
	                                nsIMsgDatabase **database) override;
	NS_IMETHOD CreateSubFolders(nsIFile *path);
	NS_IMETHOD AddSubfolderWithPath(nsAString& name,
	                                nsIFile *dbPath,
	                                nsIMsgFolder **child,
	                                bool brandNew = false);
	bool TrashOrDescendentOfTrash(nsIMsgFolder* folder);
    NS_IMETHOD GetTrashFolder(nsIMsgFolder** result);
    NS_IMETHOD RefreshSizeOnDisk();
	NS_IMETHOD DeleteMessagesFromServer(nsIArray * messages);
	NS_IMETHOD MarkMessagesReadOnServer(nsIArray * msgHdrs, bool markRead);
    
    bool m_initialized;
	std::auto_ptr<SyncItemCallback> m_pSyncItemCallback;
    nsCString m_FolderId;
    nsCString m_ChangeKey;
    nsCString m_SyncState;
    bool m_FolderIdLoaded;
    bool m_ChangeKeyLoaded;
    bool m_SyncStateLoaded;
};

#endif //__MAIL_EWS_MSG_FOLDER_H__

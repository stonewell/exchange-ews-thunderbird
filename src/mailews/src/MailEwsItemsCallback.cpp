#include "xpcom-config.h"
#include "libews.h"

#include "nsIMsgIncomingServer.h"
#include "nsIThread.h"

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
#include "nsIOutputStream.h"
#include "nsNetUtil.h"
#include "nsIMsgHdr.h"
#include "nsArrayUtils.h"
#include "nsMsgUtils.h"
#include "plbase64.h"

#include "MailEwsErrorInfo.h"
#include "MailEwsItemsCallback.h"
#include "IMailEwsMsgIncomingServer.h"
#include "IMailEwsService.h"
#include "MailEwsMsgUtils.h"

#include "MailEwsLog.h"
#include "MailEwsCommon.h"

NS_IMPL_ISUPPORTS(MailEwsItemsCallback, IMailEwsItemsCallback);

MailEwsItemsCallback::MailEwsItemsCallback()
	: m_RemoteMsg(NULL)
	, m_RemoteResult(EWS_SUCCESS) {
  /* member initializers and constructor code */
}

MailEwsItemsCallback::~MailEwsItemsCallback()
{
  /* destructor code */
    if (m_RemoteMsg)
        free(m_RemoteMsg);
}

/* [noscript] readonly attribute nsNativeStringArrayPtr itemIds; */
NS_IMETHODIMP MailEwsItemsCallback::GetItemIds(nsTArray<nsCString> **aItemIds)
{
    if (!aItemIds) return NS_OK;
    *aItemIds = &m_ItemIds;
    
    return NS_OK;
}

/* [noscript] readonly attribute nsNativeStringArrayPtr itemIds; */
NS_IMETHODIMP MailEwsItemsCallback::SetItemIds(nsTArray<nsCString> *aItemIds)
{
    m_ItemIds.Clear();

    if (!aItemIds) return NS_OK;
    
	for (PRUint32 i = 0; i < aItemIds->Length(); i++) {
		m_ItemIds.AppendElement((*aItemIds)[i]);
	}

    return NS_OK;
}

NS_IMETHODIMP MailEwsItemsCallback::GetFolder(nsIMsgFolder ** folder) {
	NS_IF_ADDREF(*folder = m_Folder);

	return NS_OK;
}

NS_IMETHODIMP MailEwsItemsCallback::SetFolder(nsIMsgFolder * folder) {
	m_Folder = folder;

	return NS_OK;
}

NS_IMPL_GETSET(MailEwsItemsCallback, RemoteResult, int, m_RemoteResult);
NS_IMPL_GETSET(MailEwsItemsCallback, RemoteMessage, char *, m_RemoteMsg);

/* [noscript] void localOperation (in nsNativeVoidPtr severResponse); */
NS_IMETHODIMP MailEwsItemsCallback::LocalOperation(void *)
{
    return NS_OK;
}

NS_IMETHODIMP MailEwsItemsCallback::FreeRemoteResponse(void *)
{
	return NS_OK;
}

NS_IMETHODIMP MailEwsItemsCallback::RemoteOperation(void **)
{
    return NS_OK;
}


NS_IMPL_ISUPPORTS_INHERITED0(MailEwsLoadItemsCallback, MailEwsItemsCallback)

MailEwsLoadItemsCallback::MailEwsLoadItemsCallback() {
}

MailEwsLoadItemsCallback::~MailEwsLoadItemsCallback() {
}

typedef struct __load_item_response {
    ews_item ** item;
    int item_count;
} load_item_response;

NS_IMETHODIMP MailEwsLoadItemsCallback::LocalOperation(void *response)
{
    load_item_response * r = (load_item_response *)response;
    if (!r) return NS_OK;

    ews_item ** item = r->item;
    nsresult rv;

    //Find existing items for the going to load items
    nsTArray<nsCString> itemIds;
    for(int i=0;i < r->item_count;i++) {
        if (item[i]->item_type == EWS_Item_Message) {
            ews_msg_item * msg_item = (ews_msg_item*)item[i];
            itemIds.AppendElement(nsCString(msg_item->item.item_id));
        }
    }
    nsTArray<nsCString> msgIds;
    for(int i=0;i < r->item_count;i++) {
        if (item[i]->item_type == EWS_Item_Message) {
            ews_msg_item * msg_item = (ews_msg_item*)item[i];
            if (msg_item->internet_message_id)
                msgIds.AppendElement(nsCString(msg_item->internet_message_id));
        }
    }

    nsCOMPtr<nsIArray> msgHdrs;
    nsCOMPtr<nsIArray> msgHdrs2;
    nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(m_Folder));
    
    rv = ewsFolder->GetMsgHdrsForItemIds(&itemIds,
                                        getter_AddRefs(msgHdrs));
    NS_FAILED_WARN(rv);
    
    rv = ewsFolder->GetMsgHdrsForMessageIds(&msgIds,
                                        getter_AddRefs(msgHdrs2));
    NS_FAILED_WARN(rv);

    uint32_t messageCount = 0;
    if (msgHdrs) {
        msgHdrs->GetLength(&messageCount);

        mailews_logger << "find headers with item ids:"
                  << messageCount
                  << ","
                  << itemIds.Length()
                  << std::endl;
    }
    uint32_t messageCount2 = 0;
    if (msgHdrs2) {
        msgHdrs2->GetLength(&messageCount2);

        mailews_logger << "find headers with msg ids:"
                  << messageCount2
                  << ","
                  << msgIds.Length()
                  << std::endl;
    }

    item = r->item;
    
    for(int i=0;i < r->item_count;i++) {
        if (item[i]->item_type == EWS_Item_Message) {
            ews_msg_item * msg_item = (ews_msg_item*)item[i];

            rv = LocalProcessMsgItem(msg_item);
            NS_FAILED_WARN(rv);
        }
    }

    if (msgHdrs && messageCount > 0)
        ewsFolder->ClientDeleteMessages(msgHdrs);
    if (msgHdrs2 && messageCount2 > 0)
        ewsFolder->ClientDeleteMessages(msgHdrs2);
    
    return NS_OK;
}

static
nsresult DeleteFile(nsIFile *pFile) {
	bool      result;
	nsresult  rv = NS_OK;

	result = false;
	pFile->Exists(&result);
	if (result) {
		result = false;
		pFile->IsFile(&result);
		if (result) {
			rv = pFile->Remove(false);
		}
	}
	return rv;
}

class DLL_LOCAL MsgItemTask : public nsRunnable
{
public:
    nsCOMPtr<nsIMsgFolder> mFolder;
    ews_msg_item * mMsgItem;
    nsCOMPtr<nsIFile> msgFile;
	MsgItemTask(nsIMsgFolder * folder,
                ews_msg_item * msg_item,
                nsIFile * pMsgFile)
            : mFolder(folder)
            , mMsgItem(ews_msg_item_clone(msg_item))
            , msgFile(pMsgFile) {
	}

protected:
    virtual ~MsgItemTask() {
        ews_free_item(&mMsgItem->item);
    }

	NS_IMETHOD Run() {	
		nsresult rv;

        rv = AddNewMessageToFolder(mFolder,
                                   mMsgItem,
                                   msgFile);
        DeleteFile(msgFile);
        NS_ENSURE_SUCCESS(rv, rv);

        //Delete Item Op from Folder
        nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(mFolder));
        nsCString itemId(mMsgItem->item.item_id);

        //new and update are same op
        ewsFolder->RemoveItemOp(itemId, IMailEwsItemOp::Saved_Item_New);

		return rv;
	}
    
};

class MailEwsLoadItemListener : public MailEwsSendListener {
public:
    nsCOMPtr<nsIMsgFolder> mFolder;
    ews_msg_item * msg_item;
    
    MailEwsLoadItemListener(nsIMsgFolder * folder,
                            ews_msg_item * msg_item) : mFolder(folder) {
        this->msg_item = ews_msg_item_clone(msg_item);
    }

    virtual ~MailEwsLoadItemListener() {
        ews_free_item(&this->msg_item->item);
    }
    
    NS_IMETHOD OnStopSending(const char *aMsgID,
                             nsresult aStatus,
                             const char16_t *aMsg,
                             nsIFile *returnFile) {
        nsString path;
        returnFile->GetPath(path);
        NS_LossyConvertUTF16toASCII_external convertString(path);

        mailews_logger << "msg file:"
                  << convertString.get()
                  << std::endl;
        
		nsCOMPtr<nsIRunnable> resultrunnable =
				new MsgItemTask(mFolder,
                                msg_item,
                                returnFile);
		NS_DispatchToMainThread(resultrunnable);
        return NS_OK;
    }
};
    
NS_IMETHODIMP MailEwsLoadItemsCallback::LocalProcessMsgItem(ews_msg_item * msg_item) {
    if (msg_item->item.mime_content) {
        mailews_logger << "build message using mime content" << std::endl;
        
        nsCOMPtr <nsIFile> tmpFile;
        nsresult rv = MailEwsMsgCreateTempFile("ews_in_mail.tmp",
                                               getter_AddRefs(tmpFile));

        if (NS_FAILED(rv)) {
            NS_FAILED_WARN(rv);
            mailews_logger << "create temp file failed, fallback" << std::endl;
            goto Fallback;
        }

        char *decodedContent = PL_Base64Decode(msg_item->item.mime_content,
                                                 strlen(msg_item->item.mime_content),
                                                 nullptr);
        nsCString data(decodedContent);
        PR_Free(decodedContent);
        
        nsCOMPtr<nsIOutputStream> outputStream;
        rv = NS_NewLocalFileOutputStream(getter_AddRefs(outputStream), tmpFile,  PR_WRONLY | PR_CREATE_FILE, 00600);

        if (NS_SUCCEEDED(rv)) {
            uint32_t bytesWritten;
            (void) outputStream->Write(data.get(),
                                       data.Length(),
                                       &bytesWritten);

            outputStream->Close();

            if (tmpFile) {
                nsCOMPtr <nsIFile> tmpFile1;
                tmpFile->Clone(getter_AddRefs(tmpFile1));
                tmpFile = do_QueryInterface(tmpFile1);
            }
            
            rv = AddNewMessageToFolder(m_Folder,
                                   msg_item,
                                   tmpFile);
            DeleteFile(tmpFile);

            if (NS_SUCCEEDED(rv)) {
	            //Delete Item Op from Folder
	            nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(m_Folder));
	            nsCString itemId(msg_item->item.item_id);

	            //new and update are same op
	            ewsFolder->RemoveItemOp(itemId, IMailEwsItemOp::Saved_Item_New);
            }
        } else {
            NS_FAILED_WARN(rv);
            mailews_logger << "write temp file failed, fallback" << std::endl;
            goto Fallback;
        }

        return NS_OK;
    } 

Fallback:
    MailEwsMsgCompose compose;

    nsCOMPtr<nsIMsgSendListener> l =
            new MailEwsLoadItemListener(m_Folder,
                                        msg_item);

    mailews_logger << "create RFC822 message file"
              << std::endl;
    nsresult rv = compose.SendTheMessage(msg_item,
                                         l);

    if (NS_FAILED(rv)){
        mailews_logger << "create RFC822 message file fail"
                  << std::endl;
    }
    
    return rv;
}

NS_IMETHODIMP MailEwsLoadItemsCallback::FreeRemoteResponse(void * response)
{
    if (response) {
        load_item_response * r = (load_item_response *)response;
        ews_free_items(r->item, r->item_count);
        free(r);
    }
	return NS_OK;
}

NS_IMETHODIMP MailEwsLoadItemsCallback::RemoteOperation(void ** response)
{
	nsCOMPtr<nsIMsgIncomingServer> server;

    if (m_RemoteMsg) {
        free(m_RemoteMsg);
        m_RemoteMsg = NULL;
    }
    
    nsresult rv = m_Folder->GetServer(getter_AddRefs(server));
    if (NS_FAILED(rv)) {
        NS_FAILED_WARN(rv);
        m_RemoteResult = EWS_FAIL;
        m_RemoteMsg = NULL;

        return rv;
    }

	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(server));

    nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(m_Folder));
    
	nsCOMPtr<IMailEwsService> ewsService;
	ewsServer->GetService(getter_AddRefs(ewsService));

    ews_session * session = NULL;
    rv = ewsService->GetNewSession(&session);

    if (NS_SUCCEEDED(rv) && session) {
        const char ** item_ids =
                (const char **)malloc(sizeof(const char *) * m_ItemIds.Length());
    
        for (PRUint32 i = 0; i < m_ItemIds.Length(); i++) {
            item_ids[i] = m_ItemIds[i].get();
        }

        ews_item ** items = NULL;
        int item_count = m_ItemIds.Length();

        mailews_logger << "load items for:"
                  << item_count
                  << std::endl;
        m_RemoteResult = ews_get_items(session,
                                       item_ids,
                                       item_count,
                                       EWS_GetItems_Flags_MimeContent
                                       | EWS_GetItems_Flags_MessageItem,
                                       &items,
                                       &m_RemoteMsg);

        *response = NULL;
        if (m_RemoteResult == EWS_SUCCESS && items) {
            load_item_response * r;

            *response = r = (load_item_response*)malloc(sizeof(load_item_response));
            r->item = items;
            r->item_count = item_count;
            mailews_logger << "load items done:"
                      << item_count
                      << std::endl;
        }

        free(item_ids);
        
    } else {
        m_RemoteResult = session ? EWS_FAIL : 401;
        m_RemoteMsg = NULL;

        return rv;
    }

    ewsService->ReleaseSession(session);

    return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED0(MailEwsDeleteItemsCallback, MailEwsItemsCallback);
MailEwsDeleteItemsCallback::MailEwsDeleteItemsCallback(bool localOnly)
	: m_LocalOnly(localOnly) {
}

MailEwsDeleteItemsCallback::~MailEwsDeleteItemsCallback() {
}

NS_IMETHODIMP MailEwsDeleteItemsCallback::LocalOperation(void *severResponse)
{
    nsresult rv;

    nsCOMPtr<nsIArray> msgHdrs;
    nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(m_Folder));
    
    rv = ewsFolder->GetMsgHdrsForItemIds(&m_ItemIds,
                                        getter_AddRefs(msgHdrs));
    NS_FAILED_WARN(rv);

    uint32_t messageCount = 0;

    if (msgHdrs) {
        msgHdrs->GetLength(&messageCount);

        mailews_logger << "find headers:"
                  << messageCount
                  << ","
                  << m_ItemIds.Length()
                  << std::endl;
        
        if (messageCount > 0)
	        ewsFolder->ClientDeleteMessages(msgHdrs);
        m_Folder->UpdateSummaryTotals(true);

    }

    //Delete Item Op from Folder
    for (PRUint32 i = 0; i < m_ItemIds.Length(); i++) {
	    ewsFolder->RemoveItemOp(m_ItemIds[i], IMailEwsItemOp::Saved_Item_Delete_Local);
    }
    return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED0(MailEwsReadItemsCallback, MailEwsItemsCallback);
MailEwsReadItemsCallback::MailEwsReadItemsCallback(bool localOnly)
	: m_LocalOnly(localOnly) {
}

MailEwsReadItemsCallback::~MailEwsReadItemsCallback() {
}

NS_IMETHODIMP MailEwsReadItemsCallback::LocalOperation(void *severResponse)
{
    nsresult rv;

    nsCOMPtr<nsIArray> msgHdrs;
    nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(m_Folder));
    
    rv = ewsFolder->GetMsgHdrsForItemIds(&m_ItemIds,
                                        getter_AddRefs(msgHdrs));
    NS_FAILED_WARN(rv);

    uint32_t messageCount = 0;
    std::map<std::string, bool> * pItemReadMap = GetItemReadMap();

    if (msgHdrs) {
        msgHdrs->GetLength(&messageCount);

        mailews_logger << "find headers:"
                  << messageCount
                  << ","
                  << m_ItemIds.Length()
                  << std::endl;
        
        for (PRUint32 i=0; i<messageCount; ++i) {
	        nsCOMPtr<nsIMsgDBHdr> element =
			        do_QueryElementAt(msgHdrs, i, &rv);

	        if (NS_FAILED(rv) || !element)
		        continue;

	        char * item_id = NULL;
	        element->GetStringProperty("item_id", &item_id);

	        std::map<std::string, bool>::iterator it =
			        pItemReadMap->find(item_id);

	        if (it != pItemReadMap->end()) {
		        element->MarkRead(!!it->second);
	        }

	        if (item_id) NS_Free(item_id);
        }

        m_Folder->UpdateSummaryTotals(true);
    }

    //Delete Item Op from Folder
    for (PRUint32 i = 0; i < m_ItemIds.Length(); i++) {
	    ewsFolder->RemoveItemOp(m_ItemIds[i], IMailEwsItemOp::Saved_Item_Read_Local);
    }
    return NS_OK;
}


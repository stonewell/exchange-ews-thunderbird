#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgIncomingServer.h"
#include "nsArrayUtils.h"

#include "libews.h"

#include "MailEwsDeleteItemsTask.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"
#include "IMailEwsMsgFolder.h"

#include "MailEwsLog.h"

class DeleteItemsDoneTask : public MailEwsRunnable
{
public:
	DeleteItemsDoneTask(nsIRunnable * runnable,
	                    nsIMsgFolder * pFolder,
	                    int result,
	                    nsISupportsArray * ewsTaskCallbacks,
	                    nsTArray<nsCString> * itemsToDelete,
	                    bool deleteNoTrash)
		: MailEwsRunnable(ewsTaskCallbacks)
		, m_pFolder(pFolder)
		, m_Runnable(runnable)
		, m_Result(result)
		, m_DeleteNoTrash(deleteNoTrash)
	{
		for (PRUint32 i = 0; i < itemsToDelete->Length(); i+=2) {
			m_ItemIds.AppendElement((*itemsToDelete)[i]);
		}
	}

	NS_IMETHOD Run() {
		NotifyEnd(m_Runnable, m_Result);

		if (m_Result == EWS_SUCCESS && m_pFolder) {
			nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(m_pFolder));
			
			//Delete Item Op from Folder
			for (PRUint32 i = 0; i < m_ItemIds.Length(); i++) {
				ewsFolder->RemoveItemOp(m_ItemIds[i], m_DeleteNoTrash ? IMailEwsItemOp::Saved_Item_Delete_Remote_Trash : IMailEwsItemOp::Saved_Item_Delete_Remote);
			}
		}
		
		return NS_OK;
	}

private:
	nsCOMPtr<nsIMsgFolder> m_pFolder;
	nsCOMPtr<nsIRunnable> m_Runnable;
	int m_Result;
	bool m_DeleteNoTrash;
	nsTArray<nsCString> m_ItemIds;
};


DeleteItemsTask::DeleteItemsTask(nsTArray<nsCString> * itemsToDelete,
                                 bool deleteNoTrash,
                                 nsIMsgFolder * pFolder,
                                 nsIMsgIncomingServer * pIncomingServer,
                                 nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
	, m_pIncomingServer(pIncomingServer)
	, m_pFolder(pFolder)
	, m_DeleteNoTrash(deleteNoTrash) { 
	
	for (PRUint32 i = 0; i < itemsToDelete->Length(); i++) {
		m_ItemsToDelete.AppendElement((*itemsToDelete)[i]);
	}
}

DeleteItemsTask::~DeleteItemsTask() {
}
	
NS_IMETHODIMP DeleteItemsTask::Run() {
	int ret = EWS_FAIL;
	char * err_msg = NULL;
	nsresult rv;
	
	if (EWS_FAIL == NotifyBegin(this)) {
		return NS_OK;
	}
	
	ews_session * session = NULL;
	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer, &rv));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<IMailEwsService> ewsService;
	rv = ewsServer->GetService(getter_AddRefs(ewsService));
	NS_ENSURE_SUCCESS(rv, rv);

	nsresult rv1 = ewsService->GetNewSession(&session);

	if (NS_SUCCEEDED(rv)
	    && NS_SUCCEEDED(rv1)
	    && session) {
		std::vector<ews_item> items;
	
		for (PRUint32 i = 0; i < m_ItemsToDelete.Length(); i+=2) {
			ews_item item;
			item.item_id = const_cast<char *>(m_ItemsToDelete[i].get());
			item.change_key = const_cast<char *>(m_ItemsToDelete[i + 1].get());

			items.push_back(item);
		}

		ret = ews_delete_items(session,
		                       &items[0],
		                       items.size(),
		                       !m_DeleteNoTrash,
		                       &err_msg);

		if (ret != EWS_SUCCESS) {
			mailews_logger << "delete items failed, code="
			          << ret
			          << ","
			          << err_msg
			          << std::endl;
			NotifyError(this, ret, err_msg);
		}

		if (err_msg) free(err_msg);
	} else {
		NotifyError(this, session ? EWS_FAIL : 401, err_msg);
	}

	ewsService->ReleaseSession(session);
	
	nsCOMPtr<nsIRunnable> resultrunnable =
			new DeleteItemsDoneTask(this,
			                        m_pFolder,
			                        ret,
			                        m_pEwsTaskCallbacks,
			                        &m_ItemsToDelete,
			                        m_DeleteNoTrash);
		
	NS_DispatchToMainThread(resultrunnable);

	return NS_OK;
}


#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgIncomingServer.h"
#include "nsArrayUtils.h"

#include "libews.h"

#include "MailEwsMarkItemsReadTask.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"

#include "MailEwsLog.h"

class MarkItemsReadDoneTask : public MailEwsRunnable
{
public:
	MarkItemsReadDoneTask(nsIRunnable * runnable,
	                    nsIMsgFolder * pFolder,
	                    int result,
	                    nsISupportsArray * ewsTaskCallbacks,
	                    nsTArray<nsCString> * itemsToRead)
		: MailEwsRunnable(ewsTaskCallbacks)
		, m_pFolder(pFolder)
		, m_Runnable(runnable)
		, m_Result(result)
	{
		for (PRUint32 i = 0; i < itemsToRead->Length(); i+=2) {
			m_ItemIds.AppendElement((*itemsToRead)[i]);
		}
	}

	NS_IMETHOD Run() {
		NotifyEnd(m_Runnable, m_Result);

		if (m_Result == EWS_SUCCESS) {
			nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(m_pFolder));
			
			//Delete Item Op from Folder
			for (PRUint32 i = 0; i < m_ItemIds.Length(); i++) {
				ewsFolder->RemoveItemOp(m_ItemIds[i],
				                        IMailEwsItemOp::Saved_Item_Read_Remote);
			}
		}
		return NS_OK;
	}

private:
	nsCOMPtr<nsIMsgFolder> m_pFolder;
	nsCOMPtr<nsIRunnable> m_Runnable;
	int m_Result;
	nsTArray<nsCString> m_ItemIds;
};


MarkItemsReadTask::MarkItemsReadTask(nsTArray<nsCString> * itemsToRead,
                                   int read,
                                     nsIMsgFolder * pFolder,
                                   nsIMsgIncomingServer * pIncomingServer,
                                   nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
	, m_pIncomingServer(pIncomingServer)
	, m_pFolder(pFolder)
	, m_Read(read) { 
	
	for (PRUint32 i = 0; i < itemsToRead->Length(); i++) {
		m_ItemsToRead.AppendElement((*itemsToRead)[i]);
	}
}

MarkItemsReadTask::~MarkItemsReadTask() {
}
	
NS_IMETHODIMP MarkItemsReadTask::Run() {
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
	
		for (PRUint32 i = 0; i < m_ItemsToRead.Length(); i+=2) {
			ews_item item;
			item.item_id = const_cast<char *>(m_ItemsToRead[i].get());
			item.change_key = const_cast<char *>(m_ItemsToRead[i + 1].get());

			items.push_back(item);
		}

		ret = ews_mark_items_read(session,
		                       &items[0],
		                       items.size(),
		                       m_Read,
		                       &err_msg);

		if (ret != EWS_SUCCESS) {
			mailews_logger << "mark items read failed, code="
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
			new MarkItemsReadDoneTask(this,
			                          m_pFolder,
			                          ret,
			                          m_pEwsTaskCallbacks,
			                          &m_ItemsToRead);
		
	NS_DispatchToMainThread(resultrunnable);

	return NS_OK;
}


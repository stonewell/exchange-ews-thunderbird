#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgIncomingServer.h"
#include "nsArrayUtils.h"
#include "nsMsgFolderFlags.h"
#include "msgCore.h"

#include "libews.h"

#include "MailEwsItemTask.h"
#include "MailEwsErrorInfo.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"

#include "MailEwsLog.h"

class ItemDoneTask : public MailEwsRunnable
{
public:
	ItemDoneTask(int result,
	             IMailEwsItemsCallback * itemsCallback,
	             void * remoteResponse,
	             nsISupportsArray * ewsTaskCallbacks,
	             nsIRunnable * runnable)
		: MailEwsRunnable(ewsTaskCallbacks)
		, m_Result(result)
		, m_Runnable(runnable)
		, m_ItemsCallback(itemsCallback)
		, m_RemoteResponse(remoteResponse)
	{
	}

	NS_IMETHOD Run() {
		MOZ_ASSERT(NS_IsMainThread()); // This method is supposed to run on the main thread!
		nsresult rv = NS_OK;

        mailews_logger << "item done task running"
                       << std::endl;
		if (m_Result == EWS_SUCCESS) {
			rv = m_ItemsCallback->LocalOperation(m_RemoteResponse);

			if (NS_FAILED(rv))
				m_Result = EWS_FAIL;
		}

		m_ItemsCallback->FreeRemoteResponse(m_RemoteResponse);

		NotifyEnd(m_Runnable, m_Result);
		
        mailews_logger << "item done task done."
                       << std::endl;
		return NS_OK;
	}

private:
	nsCOMPtr<nsISupportsArray> m_pEwsTaskCallbacks;
	int m_Result;
	nsCOMPtr<nsIRunnable> m_Runnable;
	nsCOMPtr<IMailEwsItemsCallback> m_ItemsCallback;
	void * m_RemoteResponse;
};


ItemTask::ItemTask(IMailEwsItemsCallback * itemsCallback,
                   nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
	, m_ItemsCallback(itemsCallback) {
}

ItemTask::~ItemTask() {
}

NS_IMETHODIMP ItemTask::Run() {
	int ret = EWS_FAIL;
	char * err_msg = NULL;
	void * remoteResponse = NULL;
	
	if (EWS_FAIL == NotifyBegin(this)) {
		return NS_OK;
	}

	NS_FAILED_WARN(m_ItemsCallback->RemoteOperation(&remoteResponse));
	m_ItemsCallback->GetRemoteResult(&ret);
	m_ItemsCallback->GetRemoteMessage(&err_msg);
		
	NotifyError(this, ret, err_msg);

	nsCOMPtr<nsIRunnable> resultrunnable =
			new ItemDoneTask(ret,
			                 m_ItemsCallback,
			                 remoteResponse,
			                 m_pEwsTaskCallbacks,
			                 this);
		
	NS_DispatchToMainThread(resultrunnable);

	return NS_OK;
}



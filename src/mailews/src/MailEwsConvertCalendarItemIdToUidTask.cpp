#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgIncomingServer.h"
#include "nsIMsgFolder.h"
#include "nsArrayUtils.h"
#include "nsIMutableArray.h"
#include "nsComponentManagerUtils.h"
#include "nsIVariant.h"
#include "nsIMsgIdentity.h"
#include "nsIMsgAccountManager.h"
#include "nsMsgBaseCID.h"
#include "nsID.h"
#include "nsIUUIDGenerator.h"

#include "calIEvent.h"
#include "calIDateTime.h"
#include "calIAttendee.h"

#include "nsMsgUtils.h"

#include "plbase64.h"

#include "libews.h"

#include "MailEwsConvertCalendarItemIdToUidTask.h"
#include "MailEwsErrorInfo.h"
#include "MailEwsItemsCallback.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"
#include "IMailEwsItemOp.h"
#include "IMailEwsMsgFolder.h"

#include "MailEwsLog.h"
#include "MailEwsCommon.h"

class ConvertCalendarItemIdToUidDoneTask : public MailEwsRunnable
{
public:
	ConvertCalendarItemIdToUidDoneTask(int result,
	                                   char * uid,
	                                   nsISupportsArray * ewsTaskCallbacks,
	                                   ConvertCalendarItemIdToUidTask * runnable)
		: MailEwsRunnable(ewsTaskCallbacks)
		, m_Runnable(runnable)
		, m_Result(result)
		, m_Uid(uid)
	{
	}

	NS_IMETHOD Run() {
		NotifyEnd(m_Runnable, m_Result, m_Uid);
		
		return NS_OK;
	}

private:
	nsCOMPtr<nsIRunnable> m_Runnable;
	int m_Result;
	char * m_Uid;
};

ConvertCalendarItemIdToUidTask::ConvertCalendarItemIdToUidTask(const nsCString & itemId,
                                                               nsIMsgIncomingServer * pIncomingServer,
                                                               nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
    , m_ItemId(itemId)
	, m_pIncomingServer(pIncomingServer)
{
}

ConvertCalendarItemIdToUidTask::~ConvertCalendarItemIdToUidTask() {
}

NS_IMETHODIMP ConvertCalendarItemIdToUidTask::Run() {
	char * err_msg = NULL;
	
	if (EWS_FAIL == NotifyBegin(this)) {
		return NS_OK;
	}

	ews_session * session = NULL;
	nsresult rv = NS_OK;

	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer, &rv));
	NS_ENSURE_SUCCESS(rv, rv);
    
	nsCOMPtr<IMailEwsService> ewsService;
	ewsServer->GetService(getter_AddRefs(ewsService));
	NS_ENSURE_SUCCESS(rv, rv);

	nsresult rv1 = ewsService->GetNewSession(&session);
	int ret = EWS_FAIL;
	char * uid = NULL;
    
	if (NS_SUCCEEDED(rv1)
	    && session) {
        // ret = ews_calendar_item_id_to_uid(session,
        //                                   m_ItemId.get(),
        //                                   &uid,
        //                                   &err_msg);
		
		NotifyError(this, ret, err_msg);

		if (err_msg) free(err_msg);

	} else {
		NotifyError(this, session ? EWS_FAIL : 401, err_msg);
	}

    ewsService->ReleaseSession(session);

    nsCOMPtr<nsIRunnable> resultrunnable =
            new ConvertCalendarItemIdToUidDoneTask(ret,
                                                   uid,
                                                   m_pEwsTaskCallbacks,
                                                   this);
		
    NS_DispatchToMainThread(resultrunnable);

	return NS_OK;
}

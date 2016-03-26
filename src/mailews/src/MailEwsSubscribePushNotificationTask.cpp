#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgIncomingServer.h"
#include "nsArrayUtils.h"
#include "nsMsgFolderFlags.h"
#include "msgCore.h"
#include "nsIMsgFolder.h"

#include "libews.h"

#include "MailEwsSubscribePushNotificationTask.h"
#include "MailEwsErrorInfo.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"

#include "MailEwsLog.h"

SubscribePushNotificationTask::SubscribePushNotificationTask(MailEwsSubscriptionCallback * callback,
                                                             nsIMsgIncomingServer * pIncomingServer,
                                                             nsTArray<nsCString> * folderIds,
                                                             nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
    , m_Callback(callback)
	, m_pIncomingServer(pIncomingServer) {
	int len = folderIds->Length();

	for(int i=0;i<len;i++) {
		m_FolderIds.AppendElement(folderIds->ElementAt(i));
	}
}

SubscribePushNotificationTask::~SubscribePushNotificationTask() {
}


static int folder_id_names[] = {
    EWSDistinguishedFolderIdName_calendar,
    EWSDistinguishedFolderIdName_tasks,
};

NS_IMETHODIMP SubscribePushNotificationTask::Run() {
	int ret = EWS_FAIL;
	char * err_msg = NULL;
	
	if (EWS_FAIL == NotifyBegin(this)) {
		return NS_OK;
	}
	
	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer));

	ews_session * session = NULL;

	nsCOMPtr<IMailEwsService> ewsService;
	nsresult rv = ewsServer->GetService(getter_AddRefs(ewsService));

	nsresult rv1 = ewsService->GetNewSession(&session);
    if (NS_SUCCEEDED(rv)
	    && NS_SUCCEEDED(rv1)
	    && session) {
        //Create notification server
	    const char * url = m_Callback->GetUrl();
        const char ** folder_ids = NULL;
        int folder_ids_count = 0;
        ews_subscription * subscription = NULL;

        //Get All Folder Ids
        folder_ids_count = m_FolderIds.Length();
        NS_ENSURE_SUCCESS(folder_ids_count > 0 ? NS_OK : NS_ERROR_FAILURE, rv);

        folder_ids = (const char **)malloc(m_FolderIds.Length() * sizeof(char *));

        for(int i=0;i<folder_ids_count;i++) {
	        folder_ids[i] = m_FolderIds.ElementAt(i).get();
        }
        
        //subscribe push
        ret = ews_subscribe_to_folders_with_push(session,
                                                 (const char **)folder_ids,
                                                 folder_ids_count,
                                                 folder_id_names,
                                                 sizeof(folder_id_names) / sizeof(int),
                                                 EWS_NOTIFY_EVENT_TYPE_COPY
                                                 | EWS_NOTIFY_EVENT_TYPE_CREATE
                                                 | EWS_NOTIFY_EVENT_TYPE_DELETE
                                                 | EWS_NOTIFY_EVENT_TYPE_MODIFY
                                                 | EWS_NOTIFY_EVENT_TYPE_MOVE
                                                 | EWS_NOTIFY_EVENT_TYPE_NEWMAIL,
                                                 30,
                                                 url,
                                                 m_Callback->GetEwsCallback(),
                                                 &subscription,
                                                 &err_msg);

        if (ret == EWS_SUCCESS) {
            m_Callback->AddEwsSubscription(subscription);
        }

        NotifyError(this, ret, err_msg);
			
        if (err_msg) free(err_msg);
        if (folder_ids) free(folder_ids);
    } else {
        NotifyError(this, session ? EWS_FAIL : 401, err_msg);
    }

    ewsService->ReleaseSession(session);

	return NS_OK;
}


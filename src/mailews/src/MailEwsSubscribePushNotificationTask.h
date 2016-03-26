#pragma once
#ifndef __MAIL_EWS_SUBSCRIBE_PUSH_NOTIFICATION_TASK_H__
#define __MAIL_EWS_SUBSCRIBE_PUSH_NOTIFICATION_TASK_H__

#include "libews.h"

#include "nsISupportsArray.h"
#include "nsIMsgFolder.h"
#include "nsIMsgIncomingServer.h"
#include "MailEwsRunnable.h"
#include "MailEwsSubscriptionCallback.h"

class DLL_LOCAL SubscribePushNotificationTask : public MailEwsRunnable
{
public:
	SubscribePushNotificationTask(MailEwsSubscriptionCallback * callback,
                                  nsIMsgIncomingServer * pIncomingServer,
	                              nsTArray<nsCString> * folderIds,
                                  nsISupportsArray * ewsTaskCallbacks);

	virtual ~SubscribePushNotificationTask();

	NS_IMETHOD Run();

private:
	MailEwsSubscriptionCallback * m_Callback;
    nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
    nsTArray<nsCString> m_FolderIds;
};

#endif //__MAIL_EWS_SUBSCRIBE_PUSH_NOTIFICATION_TASK_H__

#pragma once
#ifndef __MAIL_EWS_NOTIFICATION_SERVER_TASK_H__
#define __MAIL_EWS_NOTIFICATION_SERVER_TASK_H__

#include "libews.h"

#include "nsISupportsArray.h"
#include "nsIMsgFolder.h"
#include "nsIMsgIncomingServer.h"
#include "MailEwsRunnable.h"
#include "MailEwsSubscriptionCallback.h"

class IMailEwsService;
class DLL_LOCAL NotificationServerTask : public MailEwsRunnable
{
public:
	NotificationServerTask(MailEwsSubscriptionCallback * callback,
	                       nsIMsgIncomingServer * pIncomingServer,
	                       nsIThread * notificationThread,
	                       nsISupportsArray * ewsTaskCallbacks);

	virtual ~NotificationServerTask();

	NS_IMETHOD Run();

private:
	MailEwsSubscriptionCallback * m_Callback;
	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
	nsCOMPtr<nsIThread> m_NotificationThread;

	NS_IMETHOD CreateNotificationServer(IMailEwsService * ews_service,
                                        ews_session * session,
	                                    char ** pp_url,
	                                    char ** err_msg);
};

#endif //__MAIL_EWS_NOTIFICATION_SERVER_TASK_H__

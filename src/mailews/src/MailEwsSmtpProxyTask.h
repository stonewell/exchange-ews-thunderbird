#pragma once
#ifndef __MAIL_EWS_SMTP_PROXY_TASK_H__
#define __MAIL_EWS_SMTP_PROXY_TASK_H__

#include "libews.h"

#include "nsISupportsArray.h"
#include "nsIMsgFolder.h"
#include "nsIMsgIncomingServer.h"
#include "MailEwsRunnable.h"

class DLL_LOCAL SmtpProxyTask : public MailEwsRunnable
{
public:
	SmtpProxyTask(nsIMsgIncomingServer * pIncomingServer,
	              nsIThread * smtpProxyThread,
	              nsISupportsArray * ewsTaskCallbacks);

	virtual ~SmtpProxyTask();

	NS_IMETHOD Run();

private:
	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
	nsCOMPtr<nsIThread> m_SmtpProxyThread;
};

#endif //__MAIL_EWS_SMTP_PROXY_TASK_H__


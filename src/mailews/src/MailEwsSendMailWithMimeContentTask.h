#pragma once
#ifndef __MAIL_EWS_SEND_MAIL_WITH_MIMECONTENT_TASK_H__
#define __MAIL_EWS_SEND_MAIL_WITH_MIMECONTENT_TASK_H__

#include "libews.h"

#include "nsISupportsArray.h"
#include "nsIMsgIncomingServer.h"
#include "MailEwsRunnable.h"
#include "IMailEwsMsgFolder.h"


class DLL_LOCAL SendMailWithMimeContentTask : public MailEwsRunnable
{
public:
	SendMailWithMimeContentTask(nsIMsgIncomingServer * pIncomingServer,
	                            const char * mime_content,
	                            int32_t len,
	                            nsISupportsArray * ewsTaskCallbacks);

	virtual ~SendMailWithMimeContentTask();

	NS_IMETHOD Run();

private:
	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
	nsCString m_MimeContent;
};

#endif //__MAIL_EWS_SEND_MAIL_WITH_MIMECONTENT_TASK_H__

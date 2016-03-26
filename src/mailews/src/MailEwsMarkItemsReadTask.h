#pragma once
#ifndef __MAIL_EWS_MARK_ITEMS_READ_TASK_H__
#define  __MAIL_EWS_MARK_ITEMS_READ_TASK_H__

#include "libews.h"

#include "nsTArray.h"
#include "nsIMsgIncomingServer.h"
#include "MailEwsRunnable.h"

class DLL_LOCAL MarkItemsReadTask : public MailEwsRunnable
{
public:
	MarkItemsReadTask(nsTArray<nsCString> * itemsToRead,
	                 int read,
	                  nsIMsgFolder * pFolder,
	                 nsIMsgIncomingServer * pIncomingServer,
	                 nsISupportsArray * ewsTaskCallbacks);

	virtual ~MarkItemsReadTask();

	NS_IMETHOD Run();

private:
	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
	nsCOMPtr<nsIMsgFolder> m_pFolder;
	int m_Read;
	nsTArray<nsCString> m_ItemsToRead;
};

#endif // __MAIL_EWS_MARK_ITEMS_READ_TASK_H__

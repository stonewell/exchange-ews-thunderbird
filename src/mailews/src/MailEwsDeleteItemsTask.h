#pragma once
#ifndef __MAIL_EWS_DELETE_ITEMS_TASK_H__
#define __MAIL_EWS_DELETE_ITEMS_TASK_H__

#include "libews.h"

#include "nsTArray.h"
#include "nsIMsgIncomingServer.h"
#include "MailEwsRunnable.h"

class DLL_LOCAL DeleteItemsTask : public MailEwsRunnable
{
public:
	DeleteItemsTask(nsTArray<nsCString> * itemsToDelete,
	                bool deleteNoTrash,
	                nsIMsgFolder * pFolder,
	                nsIMsgIncomingServer * pIncomingServer,
	                nsISupportsArray * ewsTaskCallbacks);

	virtual ~DeleteItemsTask();

	NS_IMETHOD Run();

private:
	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
	nsCOMPtr<nsIMsgFolder> m_pFolder;
	nsTArray<nsCString> m_ItemsToDelete;
	bool m_DeleteNoTrash;
};

#endif //__MAIL_EWS_DELETE_ITEMS_TASK_H__

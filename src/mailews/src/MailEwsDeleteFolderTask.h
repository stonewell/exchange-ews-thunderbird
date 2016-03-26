#pragma once
#ifndef __MAIL_EWS_DELETE_FOLDER_TASK_H__
#define __MAIL_EWS_DELETE_FOLDER_TASK_H__

#include "libews.h"

#include "nsISupportsArray.h"
#include "nsIMsgIncomingServer.h"
#include "MailEwsRunnable.h"

class DLL_LOCAL DeleteFolderTask : public MailEwsRunnable
{
public:
	DeleteFolderTask(nsISupportsArray * foldersToDelete,
	                 bool deleteNoTrash,
	                 nsIMsgIncomingServer * pIncomingServer,
	                 nsISupportsArray * ewsTaskCallbacks);

	virtual ~DeleteFolderTask();

	NS_IMETHOD Run();

private:
	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
	nsCOMPtr<nsISupportsArray> m_FoldersToDelete;
	bool m_DeleteNoTrash;
};

#endif //__MAIL_EWS_DELETE_FOLDER_TASK_H__

#pragma once
#ifndef __MAIL_EWS_SYNC_FOLDER_ID_NAME_WITH_FOLDER_ID_TASK_H__
#define __MAIL_EWS_SYNC_FOLDER_ID_NAME_WITH_FOLDER_ID_TASK_H__

#include "libews.h"

#include "nsISupportsArray.h"
#include "nsIMsgFolder.h"
#include "MailEwsRunnable.h"

class DLL_LOCAL SyncFolderIdNameWithFolderIdTask : public MailEwsRunnable
{
public:
	SyncFolderIdNameWithFolderIdTask(nsIMsgIncomingServer * incomingServer,
	                                 nsISupportsArray * ewsTaskCallbacks);

	virtual ~SyncFolderIdNameWithFolderIdTask();

	NS_IMETHOD Run();
private:
	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
};

#endif //__MAIL_EWS_SYNC_FOLDER_ID_NAME_WITH_FOLDER_ID_TASK_H__

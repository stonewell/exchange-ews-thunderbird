#pragma once
#ifndef __MAIL_EWS_SYNC_ITEM_TASK_H__
#define __MAIL_EWS_SYNC_ITEM_TASK_H__

#include "libews.h"

#include "nsISupportsArray.h"
#include "nsIMsgIncomingServer.h"
#include "MailEwsRunnable.h"

class MailEwsErrorInfo;

class SyncItemCallback;

class DLL_LOCAL SyncItemTask : public MailEwsRunnable
{
public:
	SyncItemTask(nsIMsgFolder * pFolder,
	               nsISupportsArray * ewsTaskCallbacks);

	virtual ~SyncItemTask();

	NS_IMETHOD Run();

private:
	nsCOMPtr<nsIMsgFolder> m_pFolder;
	SyncItemCallback * m_pSyncItemCallback;
    nsCString m_SyncState;
};

#endif //__MAIL_EWS_SYNC_ITEM_TASK_H__

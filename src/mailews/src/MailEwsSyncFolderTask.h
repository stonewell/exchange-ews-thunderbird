#pragma once
#ifndef __MAIL_EWS_SYNC_FOLDER_TASK_H__
#define __MAIL_EWS_SYNC_FOLDER_TASK_H__

#include "libews.h"

#include "nsISupportsArray.h"
#include "nsIMsgIncomingServer.h"
#include "MailEwsRunnable.h"

class MailEwsErrorInfo;

class MailEwsTaskCallback {
public:
	MailEwsTaskCallback();
	virtual ~MailEwsTaskCallback() {}
public:
	virtual void TaskDone() = 0;
	virtual void TaskBegin() = 0;
	virtual MailEwsErrorInfo * GetErrorInfo() const { return m_pErrorInfo.get(); }
private:
	std::auto_ptr<MailEwsErrorInfo> m_pErrorInfo;
};

class SyncFolderCallback;

class DLL_LOCAL SyncFolderTask : public MailEwsRunnable
{
public:
	SyncFolderTask(nsIMsgIncomingServer * pIncomingServer,
	               nsISupportsArray * ewsTaskCallbacks);

	virtual ~SyncFolderTask();

	NS_IMETHOD Run();

private:
	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
	SyncFolderCallback * m_pSyncFolderCallback;
};

#endif //__MAIL_EWS_SYNC_FOLDER_TASK_H__

#pragma once
#ifndef __MAIL_EWS_GET_FOLDER_TASK_H__
#define __MAIL_EWS_GET_FOLDER_TASK_H__

#include "libews.h"

#include "nsISupportsArray.h"
#include "nsIMsgIncomingServer.h"
#include "MailEwsRunnable.h"
#include "IMailEwsMsgFolder.h"

class DLL_LOCAL GetFolderTask : public MailEwsRunnable
{
public:
	GetFolderTask(nsIMsgIncomingServer * pIncomingServer,
	              const nsACString & folder_id,
                  int32_t folder_id_name,
	              IMailEwsMsgFolder * folder,
	              nsISupportsArray * ewsTaskCallbacks);

	virtual ~GetFolderTask();

	NS_IMETHOD Run();

private:
	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
	nsCString m_FolderId;
	nsCOMPtr<IMailEwsMsgFolder> m_Folder;
    int32_t m_FolderIdName;
};

#endif //__MAIL_EWS_GET_FOLDER_TASK_H__

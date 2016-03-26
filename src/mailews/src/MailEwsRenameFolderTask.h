#pragma once
#ifndef __MAIL_EWS_RENAME_FOLDER_TASK_H__
#define __MAIL_EWS_RENAME_FOLDER_TASK_H__

#include "libews.h"

#include "nsISupportsArray.h"
#include "nsIMsgIncomingServer.h"
#include "MailEwsRunnable.h"

class DLL_LOCAL RenameFolderTask : public MailEwsRunnable
{
public:
	RenameFolderTask(nsIMsgFolder * folder,
	                 const nsAString & aNewName,
	                 nsIMsgWindow * msgWindow,
	                 nsIMsgIncomingServer * pIncomingServer,
	                 nsISupportsArray * ewsTaskCallbacks);

	virtual ~RenameFolderTask();

	NS_IMETHOD Run();

private:
	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
	nsCOMPtr<nsIMsgFolder> m_Folder;
	nsString m_NewName;
	nsCOMPtr<nsIMsgWindow> m_MsgWindow;
};

#endif //__MAIL_EWS_RENAME_FOLDER_TASK_H__

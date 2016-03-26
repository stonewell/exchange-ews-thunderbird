#pragma once
#ifndef __MAIL_EWS_CREATE_SUB_FOLDER_TASK_H__
#define __MAIL_EWS_CREATE_SUB_FOLDER_TASK_H__

#include "libews.h"

#include "nsISupportsArray.h"
#include "nsIMsgFolder.h"
#include "MailEwsRunnable.h"

class DLL_LOCAL CreateSubFolderTask : public MailEwsRunnable
{
public:
	CreateSubFolderTask(nsIMsgFolder * parentFolder,
	                    const nsAString & folderName,
	                    nsISupportsArray * ewsTaskCallbacks);

	virtual ~CreateSubFolderTask();

	NS_IMETHOD Run();

private:
	nsCOMPtr<nsIMsgFolder> m_pParentFolder;
	nsString m_FolderName;
};

#endif //__MAIL_EWS_CREATE_SUB_FOLDER_TASK_H__

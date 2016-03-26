#pragma once
#ifndef __MAIL_EWS_ITEM_TASK_H__
#define __MAIL_EWS_ITEM_TASK_H__

#include "libews.h"

#include "nsISupportsArray.h"
#include "nsIMsgFolder.h"
#include "MailEwsRunnable.h"
#include "IMailEwsItemsCallback.h"

class DLL_LOCAL ItemTask : public MailEwsRunnable
{
public:
	ItemTask(IMailEwsItemsCallback * itemsCallback,
	         nsISupportsArray * ewsTaskCallbacks);

	virtual ~ItemTask();

	NS_IMETHOD Run();

private:
	nsCOMPtr<IMailEwsItemsCallback> m_ItemsCallback;
};

#endif //__MAIL_EWS_ITEM_TASK_H__

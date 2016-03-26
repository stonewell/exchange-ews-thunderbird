#pragma once
#ifndef __MAIL_EWS_ADD_TODO_ITEM_TASK_H__
#define __MAIL_EWS_ADD_TODO_ITEM_TASK_H__

#include "libews.h"

#include "nsISupportsArray.h"
#include "nsIMsgFolder.h"
#include "MailEwsRunnable.h"
#include "IMailEwsCalendarItemCallback.h"
#include "nsCOMPtr.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsTArray.h"
#include "calITodo.h"

class DLL_LOCAL AddTodoTask : public MailEwsRunnable
{
public:
	AddTodoTask(calITodo * todo,
	            nsIMsgIncomingServer * pIncomingServer,
	            nsISupportsArray * ewsTaskCallbacks);

	virtual ~AddTodoTask();

	NS_IMETHOD Run();

private:
	nsCOMPtr<calITodo> m_Todo;
    nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
    nsCString m_MimeContent;
};

#endif //__MAIL_EWS_ADD_TODO_ITEM_TASK_H__

#pragma once
#ifndef __MAIL_EWS_MODIFY_TODO_TASK_H__
#define __MAIL_EWS_MODIFY_TODO_TASK_H__

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

class DLL_LOCAL ModifyTodoTask : public MailEwsRunnable
{
public:
	ModifyTodoTask(calITodo * oldTodo,
                   calITodo * newTodo,
                   IMailEwsCalendarItemCallback * callback,
                   nsIMsgIncomingServer * pIncomingServer,
                   nsISupportsArray * ewsTaskCallbacks);

	virtual ~ModifyTodoTask();

	NS_IMETHOD Run();

private:
	nsCOMPtr<calITodo> m_OldTodo;
	nsCOMPtr<calITodo> m_NewTodo;
    nsCOMPtr<IMailEwsCalendarItemCallback> m_CalendarCallback;
	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
	
	NS_IMETHOD CheckRecurrenceInfo(ews_session * session,
	                               int & ret,
	                               char ** err_msg);
	NS_IMETHOD UpdateTodo(ews_session * session,
	                      nsCString & changeKey,
                          nsCString & itemId,
	                      int & ret,
	                      char ** err_msg,
                          bool & do_sync);

	friend class ModifyTodoDoneTask;
};

#endif //__MAIL_EWS_MODIFY_TODO_TASK_H__

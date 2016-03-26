#pragma once
#ifndef __MAIL_EWS_MODIFY_CALENDAR_ITEM_TASK_H__
#define __MAIL_EWS_MODIFY_CALENDAR_ITEM_TASK_H__

#include "libews.h"

#include "nsISupportsArray.h"
#include "nsIMsgFolder.h"
#include "MailEwsRunnable.h"
#include "IMailEwsCalendarItemCallback.h"
#include "nsCOMPtr.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsTArray.h"
#include "calIEvent.h"

class DLL_LOCAL ModifyCalendarItemTask : public MailEwsRunnable
{
public:
	ModifyCalendarItemTask(calIEvent * oldEvent,
	                       calIEvent * newEvent,
	                       nsIMsgIncomingServer * pIncomingServer,
	                       nsISupportsArray * ewsTaskCallbacks);

	virtual ~ModifyCalendarItemTask();

	NS_IMETHOD Run();

private:
	nsCOMPtr<calIEvent> m_OldEvent;
	nsCOMPtr<calIEvent> m_NewEvent;
	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
	
	NS_IMETHOD ResponseCalendarEvent(ews_session * session,
	                                 int & ret,
	                                 char ** err_msg,
                                     nsCString & itemId,
                                     nsCString & changeKey,
                                     bool occurrence,
                                     int instanceIndex,
                                     bool & declined);
	NS_IMETHOD CheckRecurrenceInfo(ews_session * session,
                                   int & ret,
                                   char ** err_msg,
                                   nsCString & itemId,
                                   nsCString & changeKey,
                                   bool occurrence,
                                   int instanceIndex);
	NS_IMETHOD UpdateCalendarEvent(ews_session * session,
                                   int & ret,
                                   char ** err_msg,
                                   nsCString & itemId,
                                   nsCString & changeKey,
                                   bool occurrence,
                                   int instanceIndex);
    void GetItemIdAndChangeKey(nsCString & itemId,
                              nsCString & changeKey,
                              bool & occurrence,
                              int & instanceIndex);

	friend class ModifyCalendarItemDoneTask;
};

#endif //__MAIL_EWS_MODIFY_CALENDAR_ITEM_TASK_H__

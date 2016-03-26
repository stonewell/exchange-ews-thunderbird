#pragma once
#ifndef __MAIL_EWS_ADD_CALENDAR_ITEM_TASK_H__
#define __MAIL_EWS_ADD_CALENDAR_ITEM_TASK_H__

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

class DLL_LOCAL AddCalendarItemTask : public MailEwsRunnable
{
public:
	AddCalendarItemTask(calIEvent * event,
                        nsIMsgIncomingServer * pIncomingServer,
                        nsISupportsArray * ewsTaskCallbacks);

	virtual ~AddCalendarItemTask();

	NS_IMETHOD Run();

private:
	nsCOMPtr<calIEvent> m_Event;
    nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
    nsCString m_MimeContent;
};

#endif //__MAIL_EWS_ADD_CALENDAR_ITEM_TASK_H__

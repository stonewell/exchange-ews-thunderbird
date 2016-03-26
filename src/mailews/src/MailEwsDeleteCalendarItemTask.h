#pragma once
#ifndef __MAIL_EWS_DELETE_CALENDAR_ITEM_TASK_H__
#define __MAIL_EWS_DELETE_CALENDAR_ITEM_TASK_H__

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

class DLL_LOCAL DeleteCalendarItemTask : public MailEwsRunnable
{
public:
	DeleteCalendarItemTask(const nsCString & itemId,
                           const nsCString & changeKey,
                           nsIMsgIncomingServer * pIncomingServer,
                           nsISupportsArray * ewsTaskCallbacks);

	virtual ~DeleteCalendarItemTask();

	NS_IMETHOD Run();

private:
    nsCString m_ItemId;
    nsCString m_ChangeKey;
    nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
};

#endif //__MAIL_EWS_DELETE_CALENDAR_ITEM_TASK_H__

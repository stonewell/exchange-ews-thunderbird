#pragma once
#ifndef __MAIL_EWS_CONVERT_CALENDAR_ITEM_ID_TO_UID_TASK_H__
#define __MAIL_EWS_CONVERT_CALENDAR_ITEM_ID_TO_UID_TASK_H__

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

class DLL_LOCAL ConvertCalendarItemIdToUidTask : public MailEwsRunnable
{
public:
	ConvertCalendarItemIdToUidTask(const nsCString & itemId,
	                               nsIMsgIncomingServer * pIncomingServer,
	                               nsISupportsArray * ewsTaskCallbacks);

	virtual ~ConvertCalendarItemIdToUidTask();

	NS_IMETHOD Run();

private:
    nsCString m_ItemId;
    nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
};

#endif //__MAIL_EWS_CONVERT_CALENDAR_ITEM_ID_TO_UID_TASK_H__

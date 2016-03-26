#pragma once
#ifndef __MAIL_EWS_SYNC_CALENDAR_TASK_H__
#define __MAIL_EWS_SYNC_CALENDAR_TASK_H__

#include "libews.h"

#include "nsISupportsArray.h"
#include "nsIMsgFolder.h"
#include "MailEwsRunnable.h"
#include "IMailEwsCalendarItemCallback.h"
#include "nsCOMPtr.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsTArray.h"

class DLL_LOCAL SyncCalendarTask : public MailEwsRunnable
{
public:
	SyncCalendarTask(IMailEwsCalendarItemCallback * calCallback,
	                 nsIMsgIncomingServer * pIncomingServer,
                     nsISupportsArray * ewsTaskCallbacks);

	virtual ~SyncCalendarTask();

	NS_IMETHOD Run();

	const nsCString & GetSyncState() const { return m_SyncState; }
	void SetSyncState(const nsCString & syncState) { m_SyncState = syncState; }

	void NewItem(const char * item_id);
	void UpdateItem(const char * item_id);
	void DeleteItem(const char * item_id);

	void CleanupTaskRun();
private:
	nsCOMPtr<IMailEwsCalendarItemCallback> m_pCalCallback;
    nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
    nsCString m_SyncState;
    nsTArray<nsCString> m_ItemIds;
    nsTArray<nsCString> m_DeletedItemIds;
    int m_Result;

    NS_IMETHOD ProcessItems();
    bool HasItems();
    void DoSyncCalendar();
};

#endif //__MAIL_EWS_SYNC_CALENDAR_TASK_H__

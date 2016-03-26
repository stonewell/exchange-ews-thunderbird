#pragma once
#ifndef __CAL_EWS_CALENDAR_H__
#define __CAL_EWS_CALENDAR_H__

#include "calICalendar.h"
#include "calIItipTransport.h"
#include "calIChangeLog.h"
#include "calISchedulingSupport.h"

#include "nsTHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsHashKeys.h"
#include "nsIURI.h"
#include "nsIVariant.h"
#include "nsIMsgIdentity.h"
#include "nsIMsgAccount.h"
#include "nsISupportsArray.h"
#include "nsIMutableArray.h"

#include "IMailEwsCalendarItemCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"

class CalEwsCalendar
        : public calICalendar
        , public calIChangeLog
        , public IMailEwsCalendarItemCallback
        , public calIOperationListener
        , public calISchedulingSupport {
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_CALICALENDAR
    NS_DECL_CALICHANGELOG
    NS_DECL_IMAILEWSCALENDARITEMCALLBACK
    NS_DECL_CALIOPERATIONLISTENER
    NS_DECL_CALISCHEDULINGSUPPORT

    CalEwsCalendar();

private:
    virtual ~CalEwsCalendar();

    nsCString m_Id;
    nsCOMPtr<nsIURI> m_Uri;
    bool m_TransientPropertiesMode;
    nsRefPtrHashtable<nsCStringHashKey, nsIVariant> m_Properties;
	nsCOMPtr<calICalendar> m_SuperCalendar;
	nsCOMPtr<nsISupportsArray> m_Observers;
	int m_BatchCount;
    nsCOMPtr<calISyncWriteCalendar> m_OfflineCal;
	bool m_SyncRunning;
	bool m_TaskSyncRunning;
	nsCOMPtr<calIGenericOperationListener> m_CalendarListener;
    nsCOMPtr<nsIMutableArray> m_ListenerResults;
    nsCOMPtr<nsIMutableArray> m_ExceptionItems;
    nsCOMPtr<calICalendarACLEntry> m_CalendarAclEntry;
	
    static nsTHashtable<nsCStringHashKey> m_TransientProperties;

	static PLDHashOperator EnumPropertiesFunction(const nsACString & aKey,
	                                              nsIVariant * aData,
	                                              void* aUserArg);
	nsresult GetImipTransport(calIItipTransport ** aTransport);
	nsresult GetEmailIdentityAndAccount(nsIMsgIdentity ** aIdentity,
	                                    nsIMsgAccount ** aAccount);
	NS_IMETHOD NotifyObservers(const nsACString & atom = nsCString(""),
	                           const nsACString & aName = nsCString(""),
	                           nsIVariant *aValue = nullptr,
	                           nsIVariant *aOldValue = nullptr,
	                           calIItemBase * aNewItem = nullptr,
	                           calIItemBase * aOldItem = nullptr,
	                           nsresult errorno = NS_OK,
	                           const nsACString & msg = nsCString(""));
	NS_IMETHOD GetService(IMailEwsService ** aService);
	NS_IMETHOD GetIncomingServer(nsIMsgIncomingServer ** aServer);
    NS_IMETHOD CommitItem(calIItemBase * item);
    bool ProcessExceptionItem(calIItemBase * item);
    NS_IMETHOD ProcessSavedExceptionItems();

    friend class CalendarLocalDeleteOperationListener;
	friend class CalUpdateMasterOpListener;
    friend class CalendarItemTaskCallback;
	friend class CalendarLocalDeleteRunnable;
};

#endif //__CAL_EWS_CALENDAR_H__

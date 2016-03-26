#include "nsIVariant.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsIMsgAccountManager.h"
#include "nsIArray.h"
#include "nsMsgBaseCID.h"
#include "nsArrayUtils.h"
#include "nsStringGlue.h"
#include "nsThreadUtils.h"

#include "calIOperation.h"
#include "calICalendarManager.h"
#include "calICalendarACLManager.h"
#include "calIDateTime.h"
#include "calIRecurrenceInfo.h"
#include "calIEvent.h"
#include "calIAttendee.h"
#include "calITodo.h"

#include "CalEwsCalendar.h"
#include "CalEwsCalendarACLEntry.h"

#include "IMailEwsTaskCallback.h"

#include "nsMsgUtils.h"

#define NS_FAILED_WARN(res)	  \
	{ \
		nsresult __rv = res; \
		if (NS_FAILED(__rv)) { \
			NS_ENSURE_SUCCESS_BODY_VOID(res); \
		} \
	}

static
nsresult
GetPropertyString(calIItemBase * event,
                  const nsAString & name,
                  nsCString & sValue) {
	nsCOMPtr<nsIVariant> v;
    
	nsresult rv = event->GetProperty(name,  getter_AddRefs(v));
	NS_ENSURE_SUCCESS(rv, rv);
	rv = v->GetAsACString(sValue);
	NS_ENSURE_SUCCESS(rv, rv);

	return NS_OK;
}

static
nsresult
SetPropertyString(calIItemBase * event,
                  const nsAString & name,
                  const nsCString & sValue) {
    //Save change key
    nsresult rv = NS_OK;
			
    nsCOMPtr<nsIWritableVariant> v =
            do_CreateInstance("@mozilla.org/variant;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = v->SetAsACString(sValue);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = event->SetProperty(name,  v);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

class CalUpdateMasterOpListener : public calIOperationListener
{
public:
	NS_DECL_THREADSAFE_ISUPPORTS
	NS_DECL_CALIOPERATIONLISTENER

	CalUpdateMasterOpListener(CalEwsCalendar * cal,
	                               calIItemBase * item,
	                               nsCString itemId);

private:
	virtual ~CalUpdateMasterOpListener();

protected:
	nsCOMPtr<calICalendar> m_Calendar;
	CalEwsCalendar * m_EwsCalendar;
	nsCOMPtr<calIItemBase> m_ExpItem;
	nsCString m_ItemId;
};

class CalendarItemTaskCallback : public IMailEwsTaskCallback {
public:
	NS_DECL_THREADSAFE_ISUPPORTS
	NS_DECL_IMAILEWSTASKCALLBACK
	
	CalendarItemTaskCallback(CalEwsCalendar * calendar,
	                         calIItemBase *aItem,
	                         calIOperationListener *aListener,
	                         uint32_t aOperationType,
	                         calIItemBase * aOldItem = nullptr)
		: m_Item(aItem)
		, m_Listener(aListener)
		, m_Calendar(calendar)
		, m_OperationType(aOperationType)
		, m_OldItem(aOldItem)
		, m_EwsCalendar(calendar) {
	}

private:
	virtual ~CalendarItemTaskCallback() {
	}

	nsCOMPtr<calIItemBase> m_Item;
	nsCOMPtr<calIOperationListener> m_Listener;
	nsCOMPtr<calICalendar> m_Calendar;
	uint32_t m_OperationType;
	nsCOMPtr<calIItemBase> m_OldItem;
	CalEwsCalendar * m_EwsCalendar;
};

NS_IMPL_ISUPPORTS(CalendarItemTaskCallback, IMailEwsTaskCallback)

/* void onTaskBegin (in nsIRunnable runnable); */
NS_IMETHODIMP CalendarItemTaskCallback::OnTaskBegin(nsIRunnable *runnable)
{
	return NS_OK;
}

/* void onTaskDone (in nsIRunnable runnable, in long result); */
NS_IMETHODIMP CalendarItemTaskCallback::OnTaskDone(nsIRunnable *runnable, int32_t result, void * taskData)
{
    if (!result && taskData) {
        m_Item = do_QueryInterface((nsISupports*)taskData);
    }

    //Update parent item when success
	if (!result && (m_OperationType == calIOperationListener::MODIFY)) {
        nsCOMPtr<calIDateTime> recurrenceId;
        m_Item->GetRecurrenceId(getter_AddRefs(recurrenceId));
        
        nsCOMPtr<calIItemBase> parentItem;
        m_Item->GetParentItem(getter_AddRefs(parentItem));

        if (parentItem && recurrenceId) {
            nsCOMPtr<calIItemBase> newParentItem;
            nsCString changeKey;
            
            parentItem->Clone(getter_AddRefs(newParentItem));

            if (newParentItem
                && NS_SUCCEEDED(GetPropertyString(m_Item,
                                                  NS_LITERAL_STRING("X-CHANGE-KEY"),
                                                  changeKey))
                && NS_SUCCEEDED(SetPropertyString(newParentItem,
                                                  NS_LITERAL_STRING("X-CHANGE-KEY"),
                                                  changeKey))) {
                printf("update parent item change key:%s\n",
                       changeKey.get());
                m_EwsCalendar->CommitItem(newParentItem);
                m_Item->SetParentItem(newParentItem);
            }
                
        }
	}
    
	if (m_Listener) {
		if (!result) {
			nsresult rv = NS_OK;
			nsCOMPtr<nsIWritableVariant> v =
					do_CreateInstance("@mozilla.org/variant;1", &rv);
			NS_ENSURE_SUCCESS(rv, rv);

			nsCOMPtr<nsISupports> s(do_QueryInterface(m_Item));
		
			v->SetAsISupports(s);

			nsCString uid;
			m_Item->GetId(uid);
		
			m_Listener->OnOperationComplete(m_Calendar,
			                                NS_OK,
			                                m_OperationType,
			                                uid.get(),
			                                v);
		} else {
			m_Listener->OnOperationComplete(m_Calendar,
			                                NS_ERROR_FAILURE,
			                                m_OperationType,
			                                nullptr,
			                                nullptr);
		}
	}

	nsCString atom;

	switch(m_OperationType) {
	case calIOperationListener::MODIFY: {
		atom = "onModifyItem";
	}
		break;
	case calIOperationListener::DELETE: {
		atom = "onDeleteItem";
	}
		break;
	case calIOperationListener::ADD:
		atom = "onAddItem";
		break;
	default:
		break;
	}

	if (!atom.IsEmpty() && !result) {
		m_EwsCalendar->NotifyObservers(atom,
		                               nsCString(""),
		                               nullptr,
		                               nullptr,
		                               m_Item,
		                               m_OldItem);

        if (m_OperationType == calIOperationListener::DELETE) {
            nsCOMPtr<calIOperation> op;
            NS_FAILED_WARN(m_EwsCalendar->m_OfflineCal->DeleteItem(m_Item,
                                                                   nullptr,
                                                                   getter_AddRefs(op)));
        } else if (m_OperationType == calIOperationListener::MODIFY) {
            nsCOMPtr<calIOperation> op;
            NS_FAILED_WARN(m_EwsCalendar->m_OfflineCal->ModifyItem(m_Item,
                                                                   m_OldItem,
                                                                   nullptr,
                                                                   getter_AddRefs(op)));
        }

	}
    
	return NS_OK;
}

/* void onTaskError (in nsIRunnable runnable, in long err_code, in ACString err_msg); */
NS_IMETHODIMP CalendarItemTaskCallback::OnTaskError(nsIRunnable *runnable, int32_t err_code, const nsACString & err_msg)
{
	return NS_OK;
}

class CalendarLocalDeleteRunnable : public nsRunnable {
public:
	CalendarLocalDeleteRunnable(CalEwsCalendar * cal)
		: m_DeleteItems(do_CreateInstance(NS_ARRAY_CONTRACTID))
		, m_Calendar((calICalendar*)cal)
		, m_EwsCalendar(cal) {
	}
	
	virtual ~CalendarLocalDeleteRunnable() {
	}

	uint32_t GetDeleteItemCount() {
		uint32_t length = 0;
		m_DeleteItems->GetLength(&length);

		return length;
	}
	
	nsresult AddDeleteItem(calIItemBase * item) { return m_DeleteItems->AppendElement(item, false); }

	NS_IMETHOD Run() {
		uint32_t length = 0;
		m_DeleteItems->GetLength(&length);

		nsresult rv = NS_OK;
		for (uint32_t i = 0; i < length; i++) {
			nsCOMPtr<calIItemBase> item =
					do_QueryElementAt(m_DeleteItems, i, &rv);
			if (!item)
				continue;
			
			nsCOMPtr<calIOperation> op;
			rv = m_EwsCalendar->m_OfflineCal->DeleteItem(item,
			                                             nullptr,
			                                             getter_AddRefs(op));
			NS_FAILED_WARN(rv);
		}

		return NS_OK;
	}

private:
    nsCOMPtr<nsIMutableArray> m_DeleteItems;
	nsCOMPtr<calICalendar> m_Calendar;
	CalEwsCalendar * m_EwsCalendar;
};


class CalendarLocalDeleteOperationListener : public calIOperationListener
{
public:
	NS_DECL_THREADSAFE_ISUPPORTS
	NS_DECL_CALIOPERATIONLISTENER

	CalendarLocalDeleteOperationListener(CalEwsCalendar * cal,
	                                     nsTArray<nsCString> *itemIds);

private:
	virtual ~CalendarLocalDeleteOperationListener();

protected:
	nsCOMPtr<calICalendar> m_Calendar;
	CalEwsCalendar * m_EwsCalendar;
	nsTArray<nsCString> m_ItemIds;
};

/* Implementation file */
NS_IMPL_ISUPPORTS(CalendarLocalDeleteOperationListener, calIOperationListener)

CalendarLocalDeleteOperationListener::CalendarLocalDeleteOperationListener(CalEwsCalendar * cal,
                                                                           nsTArray<nsCString> * itemIds)
: m_Calendar((calICalendar*)cal)
		, m_EwsCalendar(cal)
{
	for(PRUint32 i = 0; i < itemIds->Length(); i++) {
		m_ItemIds.AppendElement((*itemIds)[i]);
	}
}

CalendarLocalDeleteOperationListener::~CalendarLocalDeleteOperationListener()
{
}

/* void onOperationComplete (in calICalendar aCalendar, in nsresult aStatus, in unsigned long aOperationType, in string aId, in nsIVariant aDetail); */
NS_IMETHODIMP CalendarLocalDeleteOperationListener::OnOperationComplete(calICalendar *aCalendar,
	  nsresult aStatus,
	  uint32_t aOperationType,
	  const char * aId,
	  nsIVariant *aDetail) {
	return NS_OK;
}

/* void onGetResult (in calICalendar aCalendar, in nsresult aStatus, in nsIIDRef aItemType, in nsIVariant aDetail, in uint32_t aCount, [array, size_is (aCount), iid_is (aItemType)] in nsQIResult aItems); */
NS_IMETHODIMP CalendarLocalDeleteOperationListener::OnGetResult(calICalendar *aCalendar,
                                                                nsresult aStatus,
                                                                const nsIID & aItemType,
                                                                nsIVariant *aDetail,
                                                                uint32_t aCount,
                                                                void **aItems) {
	nsresult rv = NS_OK;
	CalendarLocalDeleteRunnable * l;

	nsCOMPtr<nsIRunnable> r = l =
			new CalendarLocalDeleteRunnable(m_EwsCalendar);
    
	for(uint32_t i = 0; i < aCount; i++) {
		nsCOMPtr<calIItemBase> item(do_QueryInterface((nsISupports*)aItems[i]));

		if (item) {
            nsCString itemId;
            nsCOMPtr<nsIVariant> v;
            rv = item->GetProperty(NS_LITERAL_STRING("X-ITEM-ID"),
                                   getter_AddRefs(v));

            if (v)
                rv = v->GetAsACString(itemId);

            if (m_ItemIds.Contains(itemId)) {
                l->AddDeleteItem(item);
            }
		}
	}

	if (l->GetDeleteItemCount() > 0) {
		NS_DispatchToMainThread(r);
	}

	return NS_OK;
}

/* Implementation file */
NS_IMPL_ISUPPORTS(CalEwsCalendar,
                  calICalendar,
                  calIChangeLog,
                  IMailEwsCalendarItemCallback,
                  calISchedulingSupport)

CalEwsCalendar::CalEwsCalendar()
: m_Id("")
		, m_TransientPropertiesMode(false)
		, m_Observers(do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID))
		, m_BatchCount(0)
		, m_SyncRunning(false)
		, m_TaskSyncRunning(false)
		, m_ListenerResults(do_CreateInstance(NS_ARRAY_CONTRACTID))
		, m_ExceptionItems(do_CreateInstance(NS_ARRAY_CONTRACTID))
		, m_CalendarAclEntry(new CalEwsCalendarACLEntry(this)) {
	/* member initializers and constructor code */
	if (m_TransientProperties.Count() == 0) {
		m_TransientProperties.PutEntry(nsCString("cache.uncachedCalendar"));
		m_TransientProperties.PutEntry(nsCString("currentStatus"));
		m_TransientProperties.PutEntry(nsCString("itip.transport"));
		m_TransientProperties.PutEntry(nsCString("imip.identity"));
		m_TransientProperties.PutEntry(nsCString("imip.account"));
		m_TransientProperties.PutEntry(nsCString("imip.identity.disabled"));
		m_TransientProperties.PutEntry(nsCString("organizerId"));
		m_TransientProperties.PutEntry(nsCString("organizerCN"));
	}
}

CalEwsCalendar::~CalEwsCalendar()
{
	/* destructor code */
}

/* attribute AUTF8String id; */
NS_IMETHODIMP CalEwsCalendar::GetId(nsACString & aId)
{
	aId = m_Id;
	return NS_OK;
}
NS_IMETHODIMP CalEwsCalendar::SetId(const nsACString & aId)
{
	m_Id = aId;

	m_Properties.EnumerateRead(CalEwsCalendar::EnumPropertiesFunction,
	                           this);
    
	return NS_OK;
}

/* attribute AUTF8String name; */
NS_IMETHODIMP CalEwsCalendar::GetName(nsACString & aName)
{
	nsCOMPtr<nsIVariant> v;

	nsresult rv = GetProperty(nsCString("name"), getter_AddRefs(v));
	NS_ENSURE_SUCCESS(rv, rv);
    
	return v->GetAsACString(aName);
}
NS_IMETHODIMP CalEwsCalendar::SetName(const nsACString & aName)
{
	nsresult rv = NS_OK;
	nsCOMPtr<nsIWritableVariant> v =
			do_CreateInstance("@mozilla.org/variant;1", &rv);
	NS_ENSURE_SUCCESS(rv, rv);
    
	rv = v->SetAsACString(aName);
	NS_ENSURE_SUCCESS(rv, rv);

	if (m_OfflineCal)
		m_OfflineCal->SetName(aName);
    
	return SetProperty(nsCString("name"), v);
}

/* readonly attribute AUTF8String type; */
NS_IMETHODIMP CalEwsCalendar::GetType(nsACString & aType)
{
	aType.AssignLiteral("ews");
	return NS_OK;
}

/* readonly attribute AString providerID; */
NS_IMETHODIMP CalEwsCalendar::GetProviderID(nsAString & aProviderID)
{
	aProviderID.AssignLiteral("{0676d621-83de-4c9f-b2d5-6941b52a4ea9}");
	return NS_OK;
}

/* readonly attribute calICalendarACLManager aclManager; */
NS_IMETHODIMP CalEwsCalendar::GetAclManager(calICalendarACLManager * *aAclManager)
{
	const nsCString defaultACLProviderClass("@mozilla.org/calendar/acl-manager;1?type=default");
	nsCOMPtr<nsIVariant> v;
	nsresult rv = GetProperty(nsCString("aclManagerClass"), getter_AddRefs(v));
	nsCString providerClass = defaultACLProviderClass;
    
	if (NS_SUCCEEDED(rv) && v) {
		rv = v->GetAsACString(providerClass);
		if (NS_FAILED(rv))
			providerClass = defaultACLProviderClass;
	}

	nsCOMPtr<calICalendarACLManager> aclm(do_GetService(providerClass.get(),
	                                                    &rv));
	NS_ENSURE_SUCCESS(rv, rv);

	NS_IF_ADDREF(*aAclManager = aclm);
	return rv;
}

/* readonly attribute calICalendarACLEntry aclEntry; */
NS_IMETHODIMP CalEwsCalendar::GetAclEntry(calICalendarACLEntry * *aAclEntry)
{
	NS_IF_ADDREF(*aAclEntry = m_CalendarAclEntry);
	return NS_OK;
}

/* attribute calICalendar superCalendar; */
NS_IMETHODIMP CalEwsCalendar::GetSuperCalendar(calICalendar * *aSuperCalendar)
{
	if (m_SuperCalendar)
		return m_SuperCalendar->GetSuperCalendar(aSuperCalendar);

	NS_IF_ADDREF(*aSuperCalendar = this);
	return NS_OK;
}
NS_IMETHODIMP CalEwsCalendar::SetSuperCalendar(calICalendar *aSuperCalendar)
{
	m_SuperCalendar = aSuperCalendar;
	return NS_OK;
}

/* attribute nsIURI uri; */
NS_IMETHODIMP CalEwsCalendar::GetUri(nsIURI * *aUri)
{
	NS_IF_ADDREF(*aUri = m_Uri);
    
	return NS_OK;
}
NS_IMETHODIMP CalEwsCalendar::SetUri(nsIURI *aUri)
{
	m_Uri = aUri;
	return NS_OK;
}

/* attribute boolean readOnly; */
NS_IMETHODIMP CalEwsCalendar::GetReadOnly(bool *aReadOnly)
{
	nsCOMPtr<nsIVariant> v;

	nsresult rv = GetProperty(nsCString("readOnly"), getter_AddRefs(v));
	NS_ENSURE_SUCCESS(rv, rv);
    
	return v->GetAsBool(aReadOnly);
}
NS_IMETHODIMP CalEwsCalendar::SetReadOnly(bool aReadOnly)
{
	nsresult rv = NS_OK;
	nsCOMPtr<nsIWritableVariant> v =
			do_CreateInstance("@mozilla.org/variant;1", &rv);
	NS_ENSURE_SUCCESS(rv, rv);
    
	rv = v->SetAsBool(aReadOnly);
	NS_ENSURE_SUCCESS(rv, rv);

	return SetProperty(nsCString("readOnly"), v);
}

/* readonly attribute boolean canRefresh; */
NS_IMETHODIMP CalEwsCalendar::GetCanRefresh(bool *aCanRefresh)
{
	*aCanRefresh = true;
	return NS_OK;
}

/* attribute boolean transientProperties; */
NS_IMETHODIMP CalEwsCalendar::GetTransientProperties(bool *aTransientProperties)
{
	*aTransientProperties = m_TransientPropertiesMode;
	return NS_OK;
}
NS_IMETHODIMP CalEwsCalendar::SetTransientProperties(bool aTransientProperties)
{
	m_TransientPropertiesMode = aTransientProperties;
	return NS_OK;
}

/* nsIVariant getProperty (in AUTF8String aName); */
NS_IMETHODIMP CalEwsCalendar::GetProperty(const nsACString & aName, nsIVariant * *_retval)
{
	nsresult rv = NS_OK;
	nsCOMPtr<nsIWritableVariant> v =
			do_CreateInstance("@mozilla.org/variant;1", &rv);
	NS_ENSURE_SUCCESS(rv, rv);

	rv = v->SetAsEmpty();
	NS_ENSURE_SUCCESS(rv, rv);
	
	if (aName.Equals("itip.transport")) {
		nsCOMPtr<calIItipTransport> transport;
		rv = GetImipTransport(getter_AddRefs(transport));
		NS_ENSURE_SUCCESS(rv, rv);
    
		rv = v->SetAsISupports(transport);
		NS_ENSURE_SUCCESS(rv, rv);
	} else if (aName.Equals("itip.notify-replies")) {
		rv = v->SetAsBool(false);
		NS_ENSURE_SUCCESS(rv, rv);
	} else if (aName.Equals("cache.uncachedCalendar")) {
		rv = v->SetAsISupports((calICalendar*)m_OfflineCal);
		NS_ENSURE_SUCCESS(rv, rv);
	} else if (aName.Equals("isDefaultCalendar")) {
		rv = v->SetAsBool(true);
		NS_ENSURE_SUCCESS(rv, rv);
	} else if (aName.Equals( "cache.enabled") 		// Capabilities
	           || aName.Equals( "cache.always")) {
		rv = v->SetAsBool(true);
		NS_ENSURE_SUCCESS(rv, rv);
	} else if (aName.Equals( "capabilities.timezones.floating.supported")
	           || aName.Equals( "capabilities.attachments.supported")
	           || aName.Equals( "capabilities.priority.supported")) {
		rv = v->SetAsBool(false);
		NS_ENSURE_SUCCESS(rv, rv);
	} else if (aName.Equals( "capabilities.privacy.values")) {
		const char * privacyValues[] = {"DEFAULT",
		                                "PUBLIC",
		                                "PRIVATE"};
		rv = v->SetAsArray(nsIDataType::VTYPE_CHAR_STR,
		                   nullptr,
		                   3,
		                   (void*)privacyValues);
		NS_ENSURE_SUCCESS(rv, rv);
	} else if (aName.Equals( "capabilities.alarms.maxCount")) {
		rv = v->SetAsInt32(5);
		NS_ENSURE_SUCCESS(rv, rv);
	} else if (aName.Equals( "capabilities.alarms.actionValues")) {
		const char * actionValues[] = {"DISPLAY",
		                               "EMAIL",
		                               "SMS"};
		rv = v->SetAsArray(nsIDataType::VTYPE_CHAR_STR,
		                   nullptr,
		                   3,
		                   (void*)actionValues);
		NS_ENSURE_SUCCESS(rv, rv);
	} else if (aName.Equals( "capabilities.tasks.supported")) {
		rv = v->SetAsBool(true);
		NS_ENSURE_SUCCESS(rv, rv);
	} else if (aName.Equals( "capabilities.events.supported")) {
		rv = v->SetAsBool(true);
		NS_ENSURE_SUCCESS(rv, rv);
	} else if (aName.Equals( "readOnly")) {
		rv = v->SetAsBool(false);
		NS_ENSURE_SUCCESS(rv, rv);
	} else if (aName.Equals( "imip.identity.disabled")) {
		rv = v->SetAsBool(false);
		NS_ENSURE_SUCCESS(rv, rv);
	}

	uint16_t dataType = nsIDataType::VTYPE_EMPTY;
	rv = v->GetDataType(&dataType);

	if (dataType == nsIDataType::VTYPE_EMPTY) {
		nsCOMPtr<nsIVariant> tmpV;
		if (!m_Properties.Get(aName, getter_AddRefs(tmpV))) {
			if (aName.Equals("imip.identity")
			    || aName.Equals("imip.account")
			    || aName.Equals("organizerId")
			    || aName.Equals("organizerCN")) {

				nsCOMPtr<nsIMsgIdentity> identity;
				nsCOMPtr<nsIMsgAccount> account;
				rv = GetEmailIdentityAndAccount(getter_AddRefs(identity),
				                                getter_AddRefs(account));
				NS_ENSURE_SUCCESS(rv, rv);

				if (aName.Equals("imip.identity") && identity) {
					rv = v->SetAsISupports(identity);
					NS_ENSURE_SUCCESS(rv, rv);
				} else if (aName.Equals("imip.account") && account) {
					rv = v->SetAsISupports(account);
					NS_ENSURE_SUCCESS(rv, rv);
				} else if (aName.Equals("organizerId") && identity) {
					nsCString email;
					rv = identity->GetEmail(email);
					NS_ENSURE_SUCCESS(rv, rv);

					nsCString orgnaizerId("mailto:");
					orgnaizerId.Append(email);

					rv = v->SetAsACString(orgnaizerId);
					NS_ENSURE_SUCCESS(rv, rv);
				} else if (aName.Equals("organizerCN") && identity) {
					nsString fullname;
					rv = identity->GetFullName(fullname);
					NS_ENSURE_SUCCESS(rv, rv);

					rv = v->SetAsAString(fullname);
					NS_ENSURE_SUCCESS(rv, rv);
				}
			}
			
			dataType = nsIDataType::VTYPE_EMPTY;
			rv = v->GetDataType(&dataType);
			if (dataType == nsIDataType::VTYPE_EMPTY) {
				if (!m_TransientPropertiesMode &&
				    !m_TransientProperties.Contains(aName)) {
					if (!m_Id.IsEmpty()) {
						nsCOMPtr<calICalendarManager> cm(do_GetService("@mozilla.org/calendar/manager;1",
						                                               &rv));
						nsCOMPtr<nsIVariant> vv;
						rv = cm->GetCalendarPref_(this, aName, getter_AddRefs(vv));

						if (NS_SUCCEEDED(rv) && vv) {
							v->SetFromVariant(vv);
						}
					}// the preference only set after Id get set
				}

				if (aName.Equals("suppressAlarms")) {
					nsCOMPtr<nsIVariant> vv;
					rv = GetProperty(nsCString("capabilities.alarms.popup.supported"),
					                 getter_AddRefs(vv));
					bool suppressAlarm = false;
					if (NS_SUCCEEDED(rv) && vv) {
						rv = vv->GetAsBool(&suppressAlarm);

						if (NS_SUCCEEDED(rv)) {
							//if support popup, then don't supress
							suppressAlarm = !suppressAlarm;
						} else {
							//by default don't supress
							suppressAlarm = false;
						}							
					}
					rv = v->SetAsBool(suppressAlarm);
					NS_ENSURE_SUCCESS(rv, rv);
				}//suppressAlarms
			}//check transientProperties
			
			//Cache the property if get value
			dataType = nsIDataType::VTYPE_EMPTY;
			rv = v->GetDataType(&dataType);
			if (dataType != nsIDataType::VTYPE_EMPTY) {
				m_Properties.Put(aName, v);
			}				
		} //property not set
		else if (tmpV) {
			v->SetFromVariant(tmpV);
		}//Property set
	} //check if get value already

	dataType = nsIDataType::VTYPE_EMPTY;
	rv = v->GetDataType(&dataType);

	if (dataType != nsIDataType::VTYPE_EMPTY) {
		NS_IF_ADDREF(*_retval = v);
	} else {
		*_retval = nullptr;
	}
    
	return NS_OK;
}

#define MIN_REFRESH_INTERVAL (30)

/* void setProperty (in AUTF8String aName, in nsIVariant aValue); */
NS_IMETHODIMP CalEwsCalendar::SetProperty(const nsACString & aName, nsIVariant *aValue)
{
	nsresult rv = NS_OK;
    
	if (aName.Equals("refreshInterval")) {
		uint32_t newInterval = 0;
		aValue->GetAsUint32(&newInterval);
		if (newInterval < MIN_REFRESH_INTERVAL && newInterval != 0) {
			nsCOMPtr<calICalendar> superCalendar;

			rv = GetSuperCalendar(getter_AddRefs(superCalendar));
			NS_ENSURE_SUCCESS(rv, rv);

			nsCOMPtr<nsIWritableVariant> v =
					do_CreateInstance("@mozilla.org/variant;1", &rv);
			NS_ENSURE_SUCCESS(rv, rv);
			rv = v->SetAsUint32(2 * MIN_REFRESH_INTERVAL);
			NS_ENSURE_SUCCESS(rv, rv);
			superCalendar->SetProperty(aName,
			                           v);
			return NS_OK;
		}
	}
    
	//TODO:check if oldValue if equals newValue
	nsCOMPtr<nsIVariant> oldValue;
	rv = GetProperty(aName, getter_AddRefs(oldValue));
	NS_ENSURE_SUCCESS(rv, rv);
	
	m_Properties.Put(aName, aValue);

	if (aName.Equals("imip.identity.key")) {
		const char * pps[] = {
			"imip.identity",
			"imip.account",
			"organizerId",
			"organizerCN",
		};

		for (size_t i=0;i < sizeof(pps) / sizeof(const char *); i++) {
			if (m_Properties.Contains(nsCString(pps[i]))) {
				m_Properties.Remove(nsCString(pps[i]));
			}
		}
	}
	
	if (!m_TransientPropertiesMode &&
	    !m_TransientProperties.Contains(aName)) {
		if (!m_Id.IsEmpty()) {
			nsCOMPtr<calICalendarManager> cm(do_GetService("@mozilla.org/calendar/manager;1",
			                                               &rv));
			nsCOMPtr<nsIWritableVariant> v =
					do_CreateInstance("@mozilla.org/variant;1", &rv);
			NS_ENSURE_SUCCESS(rv, rv);
			rv = v->SetAsACString(aName);
			NS_ENSURE_SUCCESS(rv, rv);
			rv = cm->SetCalendarPref_(this, v, aValue);
			NS_ENSURE_SUCCESS(rv, rv);
		}// the preference only set after Id get set
	}

	NotifyObservers(nsCString("onPropertyChanged"),
	                nsCString(aName),
	                aValue,
	                oldValue);
	
	return NS_OK;
}

/* void deleteProperty (in AUTF8String aName); */
NS_IMETHODIMP CalEwsCalendar::DeleteProperty(const nsACString & aName)
{
	nsresult rv = NS_OK;
	NotifyObservers(nsCString("onPropertyDeleting"),
	                nsCString(aName),
	                nullptr,
	                nullptr);
	
	if (m_Properties.Contains(aName)) {
		m_Properties.Remove(aName);
	}
	
	if (!m_TransientPropertiesMode &&
	    !m_TransientProperties.Contains(aName)) {
		if (!m_Id.IsEmpty()) {
			nsCOMPtr<calICalendarManager> cm(do_GetService("@mozilla.org/calendar/manager;1",
			                                               &rv));
			rv = cm->DeleteCalendarPref_(this, aName);
			NS_ENSURE_SUCCESS(rv, rv);
		}// the preference only set after Id get set
	}

	return NS_OK;
}

/* void addObserver (in calIObserver observer); */
NS_IMETHODIMP CalEwsCalendar::AddObserver(calIObserver *observer)
{
	m_Observers->InsertElementAt(observer, 0);

	if (m_BatchCount > 0) {
		for(int i = 0; i < m_BatchCount; i++) {
			observer->OnStartBatch();
		}
	}
	return NS_OK;
}

/* void removeObserver (in calIObserver observer); */
NS_IMETHODIMP CalEwsCalendar::RemoveObserver(calIObserver *observer)
{
	m_Observers->RemoveElement(observer);
	return NS_OK;
}

/* calIOperation addItem (in calIItemBase aItem, in calIOperationListener aListener); */
NS_IMETHODIMP CalEwsCalendar::AddItem(calIItemBase *aItem, calIOperationListener *aListener, calIOperation * *_retval)
{
	return AdoptItem(aItem, aListener, _retval);
}

/* calIOperation adoptItem (in calIItemBase aItem, in calIOperationListener aListener); */
NS_IMETHODIMP CalEwsCalendar::AdoptItem(calIItemBase *aItem, calIOperationListener *aListener, calIOperation * *_retval)
{
	bool hasItemId = false;
	nsresult rv = aItem->HasProperty(NS_LITERAL_STRING("X-ITEM-ID"), &hasItemId);
	NS_ENSURE_SUCCESS(rv, rv);

	if (hasItemId) {
		return ModifyItem(nullptr, aItem, aListener, _retval);
	}

	nsCOMPtr<IMailEwsService> ewsService;
	nsresult retCode = GetService(getter_AddRefs(ewsService));
    
	NS_ENSURE_SUCCESS(retCode, retCode);

	if (!ewsService) {
		return NS_ERROR_FAILURE;
	}

	nsCOMPtr<IMailEwsTaskCallback> callback(new CalendarItemTaskCallback(this,
	                                                                     aItem,
	                                                                     aListener,
	                                                                     calIOperationListener::ADD));
	nsCOMPtr<calIEvent> event(do_QueryInterface(aItem, &rv));
	nsCOMPtr<calITodo> todo(do_QueryInterface(aItem, &rv));
    
	if (event) {
        printf("create event item\n");
		return ewsService->AddCalendarItem(event, callback);
    }
	else if (todo) {
        printf("create todo item\n");
		return ewsService->AddTodo(todo, callback);
    }
	else {
		printf("item is not event or todo\n");
		return NS_ERROR_FAILURE;
	}
}

/* calIOperation modifyItem (in calIItemBase aNewItem, in calIItemBase aOldItem, in calIOperationListener aListener); */
NS_IMETHODIMP CalEwsCalendar::ModifyItem(calIItemBase *aNewItem, calIItemBase *aOldItem, calIOperationListener *aListener, calIOperation * *_retval)
{
	bool hasItemId = false;
	bool hasChangeKey = false;
	nsresult rv = NS_OK;

	if (aOldItem) {
		rv = aOldItem->HasProperty(NS_LITERAL_STRING("X-ITEM-ID"), &hasItemId);
		rv = aOldItem->HasProperty(NS_LITERAL_STRING("X-CHANGE-KEY"), &hasChangeKey);
    }

	if (!hasItemId || !hasChangeKey) {
		rv = aNewItem->HasProperty(NS_LITERAL_STRING("X-ITEM-ID"), &hasItemId);
		rv = aNewItem->HasProperty(NS_LITERAL_STRING("X-CHANGE-KEY"), &hasChangeKey);
	}

	if (!hasItemId || !hasChangeKey) {
        //check if the parent item has
        nsCOMPtr<calIItemBase> parentItem;
        aNewItem->GetParentItem(getter_AddRefs(parentItem));

        if (parentItem) {
            parentItem->HasProperty(NS_LITERAL_STRING("X-ITEM-ID"), &hasItemId);
            parentItem->HasProperty(NS_LITERAL_STRING("X-CHANGE-KEY"), &hasChangeKey);
        }

        if (!hasItemId || !hasChangeKey) {
            nsCString mm;
            aNewItem->GetIcalString(mm);
            nsCString mmmm;
            aOldItem->GetIcalString(mmmm);
            
            printf("----- modifing item is not a remote item:\n%s\n%s\n",
                   mm.get(),
                   mmmm.get());
            
            //not a remote item
            nsresult rv = NS_OK;
            nsCOMPtr<nsIWritableVariant> v =
                    do_CreateInstance("@mozilla.org/variant;1", &rv);
            NS_ENSURE_SUCCESS(rv, rv);

            nsCOMPtr<nsISupports> s(do_QueryInterface(aNewItem));
		
            v->SetAsISupports(s);
        
            nsCString uid;
        
            aNewItem->GetId(uid);

            aListener->OnOperationComplete(this,
                                           NS_OK,
                                           calIOperationListener::MODIFY,
                                           uid.get(),
                                           v);
            NotifyObservers(nsCString("onModifyItem"),
                            nsCString(""),
                            nullptr,
                            nullptr,
                            aOldItem,
                            aNewItem);
            
            nsCOMPtr<calIOperation> op;
            m_OfflineCal->ModifyItem(aNewItem,
                                     aOldItem,
                                     nullptr,
                                     getter_AddRefs(op));
            return NS_OK;
        }
	}

	nsCOMPtr<IMailEwsService> ewsService;
	rv = GetService(getter_AddRefs(ewsService));
	NS_ENSURE_SUCCESS(rv, rv);

	if (!ewsService) {
		return NS_ERROR_FAILURE;
	}

	nsCOMPtr<calIEvent> newEvent(do_QueryInterface(aNewItem, &rv));
	nsCOMPtr<calITodo> newTodo(do_QueryInterface(aNewItem, &rv));
    
	nsCOMPtr<calIEvent> oldEvent;
	nsCOMPtr<calITodo> oldTodo;

	if (aOldItem) {
		oldEvent = do_QueryInterface(aOldItem, &rv);
		oldTodo = do_QueryInterface(aOldItem, &rv);
	}

	nsCOMPtr<IMailEwsTaskCallback> callback(
	    new CalendarItemTaskCallback(this,
	                                 aNewItem,
	                                 aListener,
	                                 calIOperationListener::MODIFY,
	                                 aOldItem));

	if (newEvent)
		return ewsService->ModifyCalendarItem(oldEvent,
		                                      newEvent,
		                                      callback);
	else if (newTodo) 
		return ewsService->ModifyTodo(oldTodo,
		                              newTodo,
                                      this,
		                              callback);
	else {
		printf("not an event or todo\n");
		return NS_ERROR_FAILURE;
	}
}

/* calIOperation deleteItem (in calIItemBase aItem, in calIOperationListener aListener); */
NS_IMETHODIMP CalEwsCalendar::DeleteItem(calIItemBase *aItem, calIOperationListener *aListener, calIOperation * *_retval)
{
	bool hasItemId = false;
	bool hasChangeKey = false;
	nsCString itemId;
	nsCString changeKey;
	nsresult rv = NS_OK;

	rv = aItem->HasProperty(NS_LITERAL_STRING("X-ITEM-ID"), &hasItemId);
	rv = aItem->HasProperty(NS_LITERAL_STRING("X-CHANGE-KEY"), &hasChangeKey);

	if (hasItemId) {
		nsCOMPtr<nsIVariant> v;
		rv = aItem->GetProperty(NS_LITERAL_STRING("X-ITEM-ID"),
		                        getter_AddRefs(v));
		NS_ENSURE_SUCCESS(rv, rv);
		rv = v->GetAsACString(itemId);
		NS_ENSURE_SUCCESS(rv, rv);
	}

	if (hasChangeKey) {
		nsCOMPtr<nsIVariant> v;
		rv = aItem->GetProperty(NS_LITERAL_STRING("X-CHANGE-KEY"),
		                        getter_AddRefs(v));
		NS_ENSURE_SUCCESS(rv, rv);
		rv = v->GetAsACString(changeKey);
		NS_ENSURE_SUCCESS(rv, rv);
	}

	if (!hasItemId || !hasChangeKey) {
        nsCString mmm;
        aItem->GetIcalString(mmm);
        printf("delete item:\n%s\n", mmm.get());
		//not a remote item
		nsresult rv = NS_OK;
		nsCOMPtr<nsIWritableVariant> v =
				do_CreateInstance("@mozilla.org/variant;1", &rv);
		NS_ENSURE_SUCCESS(rv, rv);

		nsCOMPtr<nsISupports> s(do_QueryInterface(aItem));
		
        v->SetAsISupports(s);
        
		nsCString uid;
        
		aItem->GetId(uid);
        
		aListener->OnOperationComplete(this,
		                               NS_OK,
		                               calIOperationListener::DELETE,
		                               uid.get(),
		                               v);
		NotifyObservers(nsCString("onDeleteItem"),
		                nsCString(""),
		                nullptr,
		                nullptr,
		                nullptr,
		                aItem);
        
        nsCOMPtr<calIOperation> op;
        m_OfflineCal->DeleteItem(aItem,
                                 nullptr,
                                 getter_AddRefs(op));
		return NS_OK;
	}

	nsCOMPtr<IMailEwsService> ewsService;
	nsresult retCode = GetService(getter_AddRefs(ewsService));
    
	NS_ENSURE_SUCCESS(retCode, retCode);

	if (!ewsService) {
		return NS_ERROR_FAILURE;
	}

	nsCOMPtr<IMailEwsTaskCallback> callback(
	    new CalendarItemTaskCallback(this,
	                                 aItem,
	                                 aListener,
	                                 calIOperationListener::DELETE,
	                                 aItem));
    
	return ewsService->DeleteCalendarItem(itemId, changeKey, callback);
}

/* calIOperation getItem (in string aId, in calIOperationListener aListener); */
NS_IMETHODIMP CalEwsCalendar::GetItem(const char * aId, calIOperationListener *aListener, calIOperation * *_retval)
{
	return m_OfflineCal->GetItem(aId,
	                             aListener,
	                             _retval);
}

/* calIOperation getItems (in unsigned long aItemFilter, in unsigned long aCount, in calIDateTime aRangeStart, in calIDateTime aRangeEndEx, in calIOperationListener aListener); */
NS_IMETHODIMP CalEwsCalendar::GetItems(uint32_t aItemFilter, uint32_t aCount, calIDateTime *aRangeStart, calIDateTime *aRangeEndEx, calIOperationListener *aListener, calIOperation * *_retval)
{
	return m_OfflineCal->GetItems(aItemFilter,
	                              aCount,
	                              aRangeStart,
	                              aRangeEndEx,
	                              aListener,
	                              _retval);
}

/* calIOperation refresh (); */
NS_IMETHODIMP CalEwsCalendar::Refresh(calIOperation * *_retval)
{
	nsCOMPtr<IMailEwsService> ewsService;
	nsresult retCode = GetService(getter_AddRefs(ewsService));
    
	NS_ENSURE_SUCCESS(retCode, retCode);

	if (!ewsService) {
		return NS_ERROR_FAILURE;
	}

	m_CalendarListener = nullptr;
	ewsService->SyncCalendar(this, nullptr);
    ewsService->SyncTodo(this, nullptr);

	return NS_OK;
}

/* void startBatch (); */
NS_IMETHODIMP CalEwsCalendar::StartBatch()
{
	if (m_BatchCount++ == 0) {
		NotifyObservers(nsCString("onStartBatch"),
		                nsCString(""),
		                nullptr,
		                nullptr);
	}
	
	return NS_OK;
}

/* void endBatch (); */
NS_IMETHODIMP CalEwsCalendar::EndBatch()
{
	if (m_BatchCount > 0) {
		if (--m_BatchCount == 0) {
			NotifyObservers(nsCString("onEndBatch"),
			                nsCString(""),
			                nullptr,
			                nullptr);
		}
	}
	return NS_OK;
}

nsTHashtable<nsCStringHashKey> CalEwsCalendar::m_TransientProperties;

PLDHashOperator CalEwsCalendar::EnumPropertiesFunction(const nsACString & aKey,
                                                       nsIVariant * aData,
                                                       void* aUserArg) {
	nsresult rv = NS_OK;
	nsCOMPtr<calICalendarManager> cm(do_GetService("@mozilla.org/calendar/manager;1",
	                                               &rv));

	calICalendar * cal = static_cast<calICalendar *>(aUserArg);
	if (NS_FAILED(rv)) {
		return PLDHashOperator::PL_DHASH_STOP;
	}

	if (!m_TransientProperties.Contains(aKey)) {
		nsCOMPtr<nsIWritableVariant> v =
				do_CreateInstance("@mozilla.org/variant;1", &rv);
		NS_ENSURE_SUCCESS(rv, PLDHashOperator::PL_DHASH_STOP);
		rv = v->SetAsACString(aKey);
		NS_ENSURE_SUCCESS(rv, PLDHashOperator::PL_DHASH_STOP);
		rv = cm->SetCalendarPref_(cal, v, aData);
		NS_ENSURE_SUCCESS(rv, PLDHashOperator::PL_DHASH_STOP);
	}

	return PLDHashOperator::PL_DHASH_NEXT;
}

nsresult CalEwsCalendar::GetImipTransport(calIItipTransport ** aTransport) {
	nsCOMPtr<nsIVariant> v;
	nsresult rv = GetProperty(nsCString("imip.identity"),
	                          getter_AddRefs(v));

	*aTransport = nullptr;
	
	if (NS_SUCCEEDED(rv) && v) {
		nsCOMPtr<calIItipTransport> transport(do_GetService("@mozilla.org/calendar/itip-transport;1?type=email",
		                                                    &rv));
		NS_ENSURE_SUCCESS(rv, rv);
		
		NS_IF_ADDREF(*aTransport = transport);
	}
	return NS_OK;
}

nsresult CalEwsCalendar::GetEmailIdentityAndAccount(nsIMsgIdentity ** aIdentity,
                                                    nsIMsgAccount ** aAccount) {
	nsCOMPtr<nsIVariant> v;
	nsresult rv = NS_OK;

	*aIdentity = nullptr;
	*aAccount = nullptr;

	nsCOMPtr<nsIMsgAccountManager> accountMgr = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
	NS_ENSURE_SUCCESS(rv, rv);

	rv = GetProperty(nsCString("imip.identity.key"),
	                 getter_AddRefs(v));
	
	if (NS_SUCCEEDED(rv) && v) {
		nsCString key;
		rv = v->GetAsACString(key);
		if (NS_SUCCEEDED(rv)) {
			rv = accountMgr->GetIdentity(key, aIdentity);
			NS_ENSURE_SUCCESS(rv, rv);

			nsCOMPtr<nsIArray> servers;
			rv = accountMgr->GetServersForIdentity(*aIdentity,
			                                       getter_AddRefs(servers));
			if (NS_SUCCEEDED(rv) && servers) {
				PRUint32 length = 0;
				servers->GetLength(&length);

				for (PRUint32 i = 0; i < length; i++) {
					nsCOMPtr<nsIMsgIncomingServer> server =
							do_QueryElementAt(servers, i, &rv);
					if (NS_SUCCEEDED(rv) && server) {
						rv = accountMgr->FindAccountForServer(server,
						                                      aAccount);

						if (NS_SUCCEEDED(rv) && *aAccount) {
							break;
						}
					}
				}// find account for the servers
			}//get all servers
		}//get identity key
	} else { //use default account
		rv = accountMgr->GetDefaultAccount(aAccount);

		if (NS_FAILED(rv) || (nullptr == *aAccount)) {
			//no default account, get first account
			nsCOMPtr<nsIArray> accounts;
			rv = accountMgr->GetAccounts(getter_AddRefs(accounts));

			if (NS_SUCCEEDED(rv) && accounts) {
				PRUint32 length = 0;
				accounts->GetLength(&length);

				nsCOMPtr<nsIMsgAccount> account =
						do_QueryElementAt(accounts, 0, &rv);

				if (NS_SUCCEEDED(rv) && account) {
					account.swap(*aAccount);
				}
			}
		}

		if (nullptr != *aAccount) {
			//find default identity
			rv = (*aAccount)->GetDefaultIdentity(aIdentity);

			if (NS_FAILED(rv) || (nullptr == *aIdentity)) {
				//find first identity
				nsCOMPtr<nsIArray> identities;
				rv = (*aAccount)->GetIdentities(getter_AddRefs(identities));

				if (NS_SUCCEEDED(rv) && identities) {
					PRUint32 length = 0;
					identities->GetLength(&length);

					nsCOMPtr<nsIMsgIdentity> identity =
							do_QueryElementAt(identities, 0, &rv);

					if (NS_SUCCEEDED(rv) && identity) {
						identity.swap(*aIdentity);
					}
				}
			}
		}//find an account
	}
	
	return NS_OK;
}

NS_IMETHODIMP CalEwsCalendar::NotifyObservers(const nsACString & atom,
                                              const nsACString & aName,
                                              nsIVariant *aValue,
                                              nsIVariant *aOldValue,
                                              calIItemBase * aNewItem,
                                              calIItemBase * aOldItem,
                                              nsresult errorno,
                                              const nsACString & msg) {
	PRUint32 length;
	m_Observers->Count(&length);
	nsCOMPtr<calICalendar> superCalendar;

	GetSuperCalendar(getter_AddRefs(superCalendar));
	
	for (PRUint32 i=0; i<length; ++i) {
		nsCOMPtr<nsISupports> _supports;
		m_Observers->GetElementAt(i, getter_AddRefs(_supports));
		nsCOMPtr<calIObserver> element =
				do_QueryInterface(_supports);

        if (!element)
            continue;
        
		if (atom.Equals("onStartBatch")) {
			element->OnStartBatch();
		} else if (atom.Equals("onEndBatch")) {
			element->OnEndBatch();
		} else if (atom.Equals("onLoad")) {
			element->OnLoad(superCalendar);
		} else if (atom.Equals("onAddItem")) {
			element->OnAddItem(aNewItem);
		} else if (atom.Equals("onModifyItem")) {
			element->OnModifyItem(aNewItem, aOldItem);
		} else if (atom.Equals("onDeleteItem")) {
			element->OnDeleteItem(aOldItem);
		} else if (atom.Equals("onError")) {
			element->OnError(superCalendar, errorno, msg);
		} else if (atom.Equals("onPropertyChanged")) {
			element->OnPropertyChanged(superCalendar,
			                           aName,
			                           aValue,
			                           aOldValue);
		} else if (atom.Equals("onPropertyDeleting")) {
			element->OnPropertyDeleting(superCalendar,
			                            aName);
		}
	}
	return NS_OK;
}

/* Implementation of calIChangeLog */
NS_IMETHODIMP CalEwsCalendar::GetOfflineStorage(calISyncWriteCalendar * *aOfflineStorage)
{
	NS_IF_ADDREF(*aOfflineStorage = m_OfflineCal);
	return NS_OK;
}
NS_IMETHODIMP CalEwsCalendar::SetOfflineStorage(calISyncWriteCalendar *aOfflineStorage)
{
	m_OfflineCal = aOfflineStorage;

	nsCString name;
	GetName(name);
	m_OfflineCal->SetName(name);
	return NS_OK;
}

/* void resetLog (); */
NS_IMETHODIMP CalEwsCalendar::ResetLog()
{
	return NS_ERROR_NOT_IMPLEMENTED;
}

/* calIOperation replayChangesOn (in calIGenericOperationListener aListener); */
NS_IMETHODIMP CalEwsCalendar::ReplayChangesOn(calIGenericOperationListener *aListener,
                                              calIOperation * *_retval)
{
	nsCOMPtr<IMailEwsService> ewsService;
	nsresult retCode = GetService(getter_AddRefs(ewsService));
    
	NS_ENSURE_SUCCESS(retCode, retCode);

	if (!ewsService) {
		return NS_ERROR_FAILURE;
	}

	m_CalendarListener = aListener;
	ewsService->SyncCalendar(this, nullptr);
    ewsService->SyncTodo(this, nullptr);

	return NS_OK;
}

static bool eSync = true;
static bool tSync = true;

/* attribute ACString syncState; */
NS_IMETHODIMP CalEwsCalendar::GetSyncState(nsACString & aSyncState,
                                           bool isEvent)
{
	nsresult rv = NS_OK;

	aSyncState.AssignLiteral("");

    if (isEvent && !eSync)
        return NS_OK;

    if (!isEvent && !tSync)
        return NS_OK;
    
	nsCOMPtr<nsIVariant> vv;

	rv = GetProperty(isEvent ? NS_LITERAL_CSTRING("sync_state") :
                     NS_LITERAL_CSTRING("task_sync_state"),
                     getter_AddRefs(vv));

	if (NS_SUCCEEDED(rv) && vv) {
		rv = vv->GetAsACString(aSyncState);
		NS_ENSURE_SUCCESS(rv, NS_OK);
	}

	return NS_OK;
}
NS_IMETHODIMP CalEwsCalendar::SetSyncState(const nsACString & aSyncState,
                                           bool isEvent)
{
	nsresult rv = NS_OK;

    if (isEvent)
        eSync = true;
    else
        tSync = true;
    
	nsCOMPtr<nsIWritableVariant> vv =
			do_CreateInstance("@mozilla.org/variant;1", &rv);
	NS_ENSURE_SUCCESS(rv, rv);
	rv = vv->SetAsACString(aSyncState);
	NS_ENSURE_SUCCESS(rv, rv);

	return SetProperty(isEvent ? NS_LITERAL_CSTRING("sync_state") :
                     NS_LITERAL_CSTRING("task_sync_state"),
                       vv);
}

/* attribute calICalendar calendar; */
NS_IMETHODIMP CalEwsCalendar::GetCalendar(calICalendar * *aCalendar)
{
	*aCalendar = this;
	return NS_OK;
}
NS_IMETHODIMP CalEwsCalendar::SetCalendar(calICalendar *aCalendar)
{
	(void)(aCalendar);
	return NS_OK;
}

/* attribute bool isRunning; */
NS_IMETHODIMP CalEwsCalendar::GetIsRunning(bool *aIsRunning, bool isEvent)
{
    if (isEvent)
        *aIsRunning = m_SyncRunning;
    else
        *aIsRunning = m_TaskSyncRunning;
	return NS_OK;
}
NS_IMETHODIMP CalEwsCalendar::SetIsRunning(bool aIsRunning, bool isEvent)
{
    if (isEvent) 
        m_SyncRunning = aIsRunning;
    else
        m_TaskSyncRunning = aIsRunning;
        
	return NS_OK;
}

class CalGuard
{
public:
	nsCOMPtr<calISyncWriteCalendar> m_OfflineCal;
	CalGuard(calISyncWriteCalendar * offlineCal)
		: m_OfflineCal(offlineCal) {
		m_OfflineCal->StartBatch();
	}

	~CalGuard() {
		m_OfflineCal->EndBatch();
	}

};

/* void onResult (in nsIArray calItems); */
NS_IMETHODIMP CalEwsCalendar::OnResult(nsIArray *calItems)
{
	uint32_t num;
	nsresult rv = calItems->GetLength(&num);
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<calISyncWriteCalendar> offlineCal(do_QueryInterface(m_OfflineCal));
	
	CalGuard cg(offlineCal);
  
	for (uint32_t i = 0; i < num; i++) {
		nsCOMPtr <calIItemBase> item = do_QueryElementAt(calItems, i);

		if (item) {
			nsCOMPtr<calIDateTime> recurrenceId;
			rv = item->GetRecurrenceId(getter_AddRefs(recurrenceId));
			NS_FAILED_WARN(rv);

			if (recurrenceId) {
				m_ExceptionItems->AppendElement(item, false);
				continue;
			}

			rv = CommitItem(item);
			NS_FAILED_WARN(rv);
		}
	}

	//Process Saved ExceptionItem
	rv = ProcessSavedExceptionItems();
	NS_FAILED_WARN(rv);
  
	return NS_OK;
}

/* void onDelete (in nsNativeStringArrayPtr itemIds); */
NS_IMETHODIMP CalEwsCalendar::OnDelete(nsTArray<nsCString> *itemIds)
{
	nsCOMPtr<calIOperationListener> listener(
	    new CalendarLocalDeleteOperationListener(this, itemIds));

	nsCOMPtr<calIOperation> op;
	GetItems(calICalendar::ITEM_FILTER_ALL_ITEMS,
	         0,
	         nullptr,
	         nullptr,
	         listener,
	         getter_AddRefs(op)
	         );

	return NS_OK;
}

NS_IMETHODIMP
CalEwsCalendar::GetService(IMailEwsService ** aService) {
	nsCOMPtr<nsIMsgIncomingServer> incomingServer;
	nsresult retCode = GetIncomingServer(getter_AddRefs(incomingServer));
	NS_ENSURE_SUCCESS(retCode, retCode);

	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(incomingServer, &retCode));
	NS_ENSURE_SUCCESS(retCode, retCode);

	nsCOMPtr<IMailEwsService> ewsService;

	nsresult rv3 = ewsServer->GetService(getter_AddRefs(ewsService));
	NS_ENSURE_SUCCESS(rv3, rv3);

	ewsService.swap(*aService);
	return NS_OK;
}

NS_IMETHODIMP
CalEwsCalendar::GetIncomingServer(nsIMsgIncomingServer ** aServer) {
	//Find username and host name
	nsCString userName;
	nsCString spec;
    
	m_Uri->GetSpec(spec);
    
	userName = Substring(spec,
	                     strlen("calendarews://"));
    
	int32_t searchCharLocation = userName.RFindChar('@');
	nsCString host;

	host = Substring(userName, searchCharLocation + 1);
	userName.SetLength(searchCharLocation);

	nsresult retCode = NS_OK;
    
	nsCOMPtr<nsIMsgAccountManager> accountMgr = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &retCode);
	NS_ENSURE_SUCCESS(retCode, retCode);

	nsCOMPtr<nsIMsgIncomingServer> incomingServer;
	retCode = accountMgr->FindServer(userName, host, nsCString("ews"),
	                                 getter_AddRefs(incomingServer));
	NS_ENSURE_SUCCESS(retCode, retCode);

	incomingServer.swap(*aServer);
	return NS_OK;
}

/* void onOperationComplete (in calICalendar aCalendar, in nsresult aStatus, in unsigned long aOperationType, in string aId, in nsIVariant aDetail); */
NS_IMETHODIMP
CalEwsCalendar::OnOperationComplete(calICalendar *aCalendar, nsresult aStatus, uint32_t aOperationType, const char * aId, nsIVariant *aDetail)
{
	return NS_OK;
}

/* void onGetResult (in calICalendar aCalendar, in nsresult aStatus, in nsIIDRef aItemType, in nsIVariant aDetail, in uint32_t aCount, [array, size_is (aCount), iid_is (aItemType)] in nsQIResult aItems); */
NS_IMETHODIMP
CalEwsCalendar::OnGetResult(calICalendar *aCalendar, nsresult aStatus, const nsIID & aItemType, nsIVariant *aDetail, uint32_t aCount, void **aItems)
{
	for(uint32_t i = 0; i < aCount; i++) {
		nsCOMPtr<calIItemBase> item(do_QueryInterface((nsISupports*)aItems[i]));

		if (item) {
			m_ListenerResults->AppendElement(item, false);
		}
	}
	return NS_OK;
}

bool
CalEwsCalendar::ProcessExceptionItem(calIItemBase * item) {
	nsresult rv = NS_OK;
	
	bool hasMasterId = false;

	rv = item->HasProperty(NS_LITERAL_STRING("X-MASTER_UID"), &hasMasterId);
	NS_ENSURE_SUCCESS(rv, false);
    
	if (!hasMasterId) {
		return false;
	}
	nsCOMPtr<nsIVariant> v;
	rv = item->GetProperty(NS_LITERAL_STRING("X-MASTER_UID"),
	                       getter_AddRefs(v));
	NS_ENSURE_SUCCESS(rv, false);

	if (!v) {
		return false;
	}
	nsCString masterId;
	rv = v->GetAsACString(masterId);
	NS_ENSURE_SUCCESS(rv, false);

	if (masterId.IsEmpty())
		return false;

	nsCOMPtr<calIOperation> op;
	nsCOMPtr<calIOperationListener> listener(
	    new CalUpdateMasterOpListener(this,
	                                         item,
	                                         masterId));
    
	rv = GetItems(calICalendar::ITEM_FILTER_ALL_ITEMS,
	                       0,
	                       nullptr,
	                       nullptr,
	                       listener,
	                       getter_AddRefs(op)
	                       );
	NS_ENSURE_SUCCESS(rv, false);

	return true;
}

NS_IMETHODIMP
CalEwsCalendar::CommitItem(calIItemBase * item) {
	nsCString status;
	nsresult rv = item->GetStatus(status);

	item->SetCalendar(this);
    
	if (status.EqualsLiteral("CANCELLED")) {
		nsCOMPtr<calIOperation> op;
		rv = m_OfflineCal->DeleteItem(item,
		                              nullptr,
		                              getter_AddRefs(op));
		NS_ENSURE_SUCCESS(rv, rv);
	} else {
		nsCOMPtr<calIOperation> op;

		rv = m_OfflineCal->ModifyItem(item,
		                              nullptr,
		                              nullptr,
		                              getter_AddRefs(op));
		NS_ENSURE_SUCCESS(rv, rv);
	}
    
	return NS_OK;
}

NS_IMETHODIMP
CalEwsCalendar::ProcessSavedExceptionItems() {
	uint32_t length = 0;
	nsresult rv = m_ExceptionItems->GetLength(&length);
	NS_ENSURE_SUCCESS(rv, rv);

	for(uint32_t i = 0; i < length; ) {
		nsCOMPtr <calIItemBase> item =
				do_QueryElementAt(m_ExceptionItems, i);

		if (item) {
			if (ProcessExceptionItem(item)) {
				m_ExceptionItems->RemoveElementAt(i);
				length--;
				continue;
			}
		}
        
		i++;
	}

	return NS_OK;
}


/* Implementation file */
NS_IMPL_ISUPPORTS(CalUpdateMasterOpListener, calIOperationListener)

CalUpdateMasterOpListener::CalUpdateMasterOpListener(CalEwsCalendar * cal,
                                                               calIItemBase * item,
                                                               nsCString itemId)
: m_Calendar(do_QueryInterface((calICalendar*)cal))
		, m_EwsCalendar(cal)
		, m_ExpItem(item)
		, m_ItemId(itemId)
{
}

CalUpdateMasterOpListener::~CalUpdateMasterOpListener()
{
}

/* void onOperationComplete (in calICalendar aCalendar, in nsresult aStatus, in unsigned long aOperationType, in string aId, in nsIVariant aDetail); */
NS_IMETHODIMP CalUpdateMasterOpListener::OnOperationComplete(calICalendar *aCalendar, nsresult aStatus, uint32_t aOperationType, const char * aId, nsIVariant *aDetail)
{
	return NS_OK;
}

/* void onGetResult (in calICalendar aCalendar, in nsresult aStatus, in nsIIDRef aItemType, in nsIVariant aDetail, in uint32_t aCount, [array, size_is (aCount), iid_is (aItemType)] in nsQIResult aItems); */
NS_IMETHODIMP CalUpdateMasterOpListener::OnGetResult(calICalendar *aCalendar, nsresult aStatus, const nsIID & aItemType, nsIVariant *aDetail, uint32_t aCount, void **aItems)
{
	nsresult rv = NS_OK;
	for(uint32_t i = 0; i < aCount; i++) {
		nsCOMPtr<calIItemBase> masterItem(do_QueryInterface((nsISupports *)aItems[i]));

		if (masterItem) {
			nsCString uid;
			masterItem->GetId(uid);

			if (m_ItemId.Equals(uid)) {
				nsCOMPtr<calIRecurrenceInfo> recurrenceInfo;
				rv = masterItem->GetRecurrenceInfo(getter_AddRefs(recurrenceInfo));
				NS_FAILED_WARN(rv);

				if (!recurrenceInfo)
					continue;

				m_ExpItem->SetParentItem(masterItem);
				m_ExpItem->SetCalendar(m_EwsCalendar);
				
				nsCString status;
				rv = m_ExpItem->GetStatus(status);
				NS_FAILED_WARN(rv)

						
				if (status.EqualsLiteral("CANCELLED")) {
					nsCOMPtr<calIDateTime> recurrenceId;
					rv = m_ExpItem->GetRecurrenceId(getter_AddRefs(recurrenceId));
					NS_FAILED_WARN(rv);

					if (recurrenceId) {
						rv = recurrenceInfo->RemoveOccurrenceAt(recurrenceId);
						NS_FAILED_WARN(rv);
					}
				} else {
					rv = recurrenceInfo->ModifyException(m_ExpItem, true);
					NS_FAILED_WARN(rv);
				}

				//Save master Item
				rv = m_EwsCalendar->CommitItem(masterItem);
				NS_FAILED_WARN(rv);
			}
		}
	}

	return NS_OK;
}

/* boolean isInvitation (in calIItemBase aItem); */
NS_IMETHODIMP CalEwsCalendar::IsInvitation(calIItemBase *aItem, bool *_retval)
{
	nsCOMPtr<nsIVariant> v;

	*_retval = false;
	
	nsresult rv = GetProperty(nsCString("organizerId"),
	                          getter_AddRefs(v));
	NS_ENSURE_SUCCESS(rv, NS_OK);

	if (!v) return NS_OK;

	nsCString organizerId;
	rv = v->GetAsACString(organizerId);
	NS_ENSURE_SUCCESS(rv, NS_OK);

	nsCOMPtr<calIAttendee> org;
	rv = aItem->GetOrganizer(getter_AddRefs(org));
	NS_ENSURE_SUCCESS(rv, NS_OK);

	if (!org) return NS_OK;
	
	nsCString id;
	rv = org->GetId(id);
	NS_ENSURE_SUCCESS(rv, NS_OK);

	if (organizerId.Equals(id, CaseInsensitiveCompare))
		return NS_OK;

	nsCOMPtr<calIAttendee> attendee;
	rv = aItem->GetAttendeeById(organizerId,
	                            getter_AddRefs(attendee));
	NS_ENSURE_SUCCESS(rv, NS_OK);

	*_retval = (attendee != nullptr);
	
	return NS_OK;
}

/* calIAttendee getInvitedAttendee (in calIItemBase aItem); */
NS_IMETHODIMP CalEwsCalendar::GetInvitedAttendee(calIItemBase *aItem, calIAttendee * *_retval)
{
	nsCOMPtr<nsIVariant> v;

	*_retval = nullptr;
	nsresult rv = GetProperty(nsCString("organizerId"),
	                          getter_AddRefs(v));
	NS_ENSURE_SUCCESS(rv, rv);

	if (!v) return NS_ERROR_FAILURE;

	nsCString organizerId;
	rv = v->GetAsACString(organizerId);
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<calIAttendee> attendee;
	rv = aItem->GetAttendeeById(organizerId,
	                            getter_AddRefs(attendee));
	NS_ENSURE_SUCCESS(rv, rv);

	NS_IF_ADDREF(*_retval = attendee);

	return NS_OK;

}

/* boolean canNotify (in AUTF8String aMethod, in calIItemBase aItem); */
NS_IMETHODIMP CalEwsCalendar::CanNotify(const nsACString & aMethod, calIItemBase *aItem, bool *_retval)
{
	*_retval = false;
	return NS_OK;
}


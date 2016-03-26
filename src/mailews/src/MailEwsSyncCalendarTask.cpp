#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgIncomingServer.h"
#include "nsIMsgFolder.h"
#include "nsArrayUtils.h"
#include "nsIMutableArray.h"
#include "nsComponentManagerUtils.h"
#include "nsIVariant.h"

#include "calBaseCID.h"
#include "calIEvent.h"
#include "calIAttendee.h"
#include "calIRecurrenceInfo.h"
#include "calIDateTime.h"
#include "calIRecurrenceDate.h"
#include "calITimezoneProvider.h"
#include "calITimezone.h"

#include "nsMsgUtils.h"

#include "plbase64.h"

#include "libews.h"

#include "MailEwsSyncCalendarTask.h"
#include "MailEwsErrorInfo.h"
#include "MailEwsItemsCallback.h"
#include "MailEwsCalendarUtils.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"
#include "IMailEwsItemOp.h"
#include "IMailEwsMsgFolder.h"

#include "MailEwsLog.h"

static const char * get_sync_state(void * user_data);
static void set_sync_state(const char * sync_state, void * user_data);
static void new_item(const ews_item * item, void * user_data);
static void update_item(const ews_item * item, void * user_data);
static void delete_item(const ews_item * item, void * user_data);
static void read_item(const ews_item * item, int read, void * user_data);
static int get_max_return_item_count(void * user_data);
static char ** get_ignored_item_ids(int * p_count, void * user_data);

#define USER_PTR(x) ((SyncCalendarTask*)x)

extern
nsresult
GenerateUid(nsCString & uid);

extern
nsresult
SetPropertyString(calIItemBase * event,
                  const nsAString & name,
                  const nsCString & sValue);

extern
nsresult
GetOwnerStatus(calIEvent * event, const nsCString & ownerEmail, nsCString & status);

extern
nsresult
GetOwnerEmail(nsIMsgIncomingServer * server, nsCString & ownerEmail);

class MailEwsLoadCalendarItemsCallback : public MailEwsItemsCallback {
public:
	NS_DECL_ISUPPORTS_INHERITED
    MailEwsLoadCalendarItemsCallback(IMailEwsCalendarItemCallback * calCallback,
                                     nsIMsgIncomingServer * pIncomingServer);

	NS_IMETHOD LocalOperation(void *severResponse) override;
	NS_IMETHOD RemoteOperation(void **severResponse) override;
	NS_IMETHOD FreeRemoteResponse(void *severResponse) override;
protected:
    virtual ~MailEwsLoadCalendarItemsCallback();

	nsCOMPtr<IMailEwsCalendarItemCallback> m_pCalCallback;
    nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
};

class SyncCalendarDoneTask : public MailEwsRunnable
{
public:
	SyncCalendarDoneTask(int result,
	                     nsISupportsArray * ewsTaskCallbacks,
	                     SyncCalendarTask * runnable)
		: MailEwsRunnable(ewsTaskCallbacks)
		, m_Runnable(runnable)
		, m_SyncCalendarTask(runnable)
		, m_Result(result)
	{
	}

	NS_IMETHOD Run() {
        mailews_logger << "sync calendar done task running."
                       << std::endl;
		m_SyncCalendarTask->CleanupTaskRun();
		
		NotifyEnd(m_Runnable, m_Result);
		
        mailews_logger << "sync calendar done task done."
                       << std::endl;
		return NS_OK;
	}

private:
	nsCOMPtr<nsIRunnable> m_Runnable;
	SyncCalendarTask * m_SyncCalendarTask;
	int m_Result;
};

SyncCalendarTask::SyncCalendarTask(IMailEwsCalendarItemCallback * calCallback,
	                 nsIMsgIncomingServer * pIncomingServer,
                     nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
	, m_pCalCallback(calCallback)
	, m_pIncomingServer(pIncomingServer)
	, m_Result(EWS_FAIL) {
	calCallback->GetSyncState(m_SyncState, true);

    mailews_logger << "calendar sync state:"
                   << (m_SyncState.get())
                   << std::endl;
}

SyncCalendarTask::~SyncCalendarTask() {
}
	
NS_IMETHODIMP SyncCalendarTask::Run() {
	char * err_msg = NULL;
	
	if (EWS_FAIL == NotifyBegin(this)) {
		return NS_OK;
	}

	ews_session * session = NULL;
    nsresult rv = NS_OK;

	bool running = false;
	rv = m_pCalCallback->GetIsRunning(&running, true/*event*/);
	NS_ENSURE_SUCCESS(rv, rv);

	if (running) {
		return NS_OK;
	}

    mailews_logger << "sync calendar task running..."
                   << std::endl;
    
	m_pCalCallback->SetIsRunning(true, true/*event*/);
	
	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer, &rv));
	NS_ENSURE_SUCCESS(rv, rv);
    
	nsCOMPtr<IMailEwsService> ewsService;
	ewsServer->GetService(getter_AddRefs(ewsService));
	NS_ENSURE_SUCCESS(rv, rv);

	nsresult rv1 = ewsService->GetNewSession(&session);

	if (NS_SUCCEEDED(rv1)
	    && session) {
		ews_sync_item_callback callback;
		
		callback.get_sync_state = get_sync_state;
		callback.set_sync_state = set_sync_state;
		callback.new_item = new_item;
		callback.update_item = update_item;
		callback.delete_item = delete_item;
		callback.read_item = read_item;
		callback.get_max_return_item_count = get_max_return_item_count;
		callback.get_ignored_item_ids = get_ignored_item_ids;
		callback.user_data = this;

		m_Result = ews_sync_calendar(session,
		                         &callback,
                                 &err_msg);
			
        NotifyError(this, m_Result, err_msg);

        if (err_msg) free(err_msg);

        if (m_Result == EWS_SUCCESS) {
	        ProcessItems();
        }
    } else {
        NotifyError(this, session ? EWS_FAIL : 401, err_msg);
    }

    nsCOMPtr<nsIRunnable> resultrunnable =
            new SyncCalendarDoneTask(m_Result,
                                     m_pEwsTaskCallbacks,
                                     this);
		
    NS_DispatchToMainThread(resultrunnable);

    ewsService->ReleaseSession(session);
    
    mailews_logger << "sync calendar task done."
                   << std::endl;
    return NS_OK;
}

static const char * get_sync_state(void * user_data) {
	return USER_PTR(user_data)->GetSyncState().get();
}

static void set_sync_state(const char * sync_state, void * user_data) {
	USER_PTR(user_data)->SetSyncState(nsCString(sync_state));
}

static void new_item(const ews_item * item, void * user_data){
    if (item->item_type == EWS_Item_Calendar)
        USER_PTR(user_data)->NewItem(item->item_id);
}

static void update_item(const ews_item * item, void * user_data) {
    if (item->item_type == EWS_Item_Calendar)
        USER_PTR(user_data)->UpdateItem(item->item_id);
}

static void delete_item(const ews_item * item, void * user_data) {
	USER_PTR(user_data)->DeleteItem(item->item_id);
}

static void read_item(const ews_item * item, int read, void * user_data) {
}

static int get_max_return_item_count(void * user_data) {
	return 500;
}

static char ** get_ignored_item_ids(int * p_count, void * user_data) {
	*p_count = 0;
	(void)user_data;
	return NULL;
}

void SyncCalendarTask::NewItem(const char * item_id) {
	UpdateItem(item_id);
}

void SyncCalendarTask::UpdateItem(const char * item_id) {
	nsCString itemId(item_id);

	if (!m_ItemIds.Contains(itemId))
		m_ItemIds.AppendElement(itemId);
}

void SyncCalendarTask::DeleteItem(const char * item_id) {
	nsCString itemId(item_id);

	if (!m_DeletedItemIds.Contains(itemId))
		m_DeletedItemIds.AppendElement(itemId);
}

NS_IMETHODIMP
SyncCalendarTask::ProcessItems() {
	nsresult rv = NS_OK;

	if (!HasItems()) {
        mailews_logger << "sync calendar found 0 items."
                       << std::endl;
		return NS_OK;
    }
	
	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer, &rv));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<IMailEwsService> ewsService;
	rv = ewsServer->GetService(getter_AddRefs(ewsService));
	NS_ENSURE_SUCCESS(rv, rv);
	
	nsCOMPtr<nsIMsgFolder> folder;
	rv = m_pIncomingServer->GetRootFolder(getter_AddRefs(folder));
	NS_ENSURE_SUCCESS(rv, rv);

	for (PRUint32 i = 0; i < m_ItemIds.Length(); i++) {
		nsCOMPtr<IMailEwsItemsCallback> callback;
		callback = new MailEwsLoadCalendarItemsCallback(m_pCalCallback,
                                                        m_pIncomingServer);

		nsTArray<nsCString> itemIds;

		for(PRUint32 j=0; j < 10 && i < m_ItemIds.Length(); j++, i++) {
			itemIds.AppendElement(m_ItemIds[i]);
		}
		
		callback->SetItemIds(&itemIds);
		callback->SetFolder(folder);

		rv = ewsService->ProcessItems(folder,
		                              callback,
		                              nullptr);
		NS_ENSURE_SUCCESS(rv, rv);
	}

    mailews_logger << "sync calendar process items done."
                   << std::endl;
	return rv;
}

void
SyncCalendarTask::CleanupTaskRun() {
	m_pCalCallback->SetSyncState(m_SyncState, true);
	m_pCalCallback->OnDelete(&m_DeletedItemIds);
	m_pCalCallback->SetIsRunning(false, true /*event*/);
	
	if (HasItems()) {
		if (m_Result == EWS_SUCCESS) {
            mailews_logger << "sync calendar, start next round sync"
                           << std::endl;
			DoSyncCalendar();
		}
	}
}

bool
SyncCalendarTask::HasItems() {
	return m_ItemIds.Length() > 0 ||
			m_DeletedItemIds.Length() > 0;
}

void SyncCalendarTask::DoSyncCalendar() {
	nsresult rv = NS_OK;
	
	mailews_logger << "start next round sync calendar items"
	               << std::endl;

	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer, &rv));
	NS_ENSURE_SUCCESS_VOID(rv);

	nsCOMPtr<IMailEwsService> ewsService;
	rv = ewsServer->GetService(getter_AddRefs(ewsService));
	NS_ENSURE_SUCCESS_VOID(rv);

	ewsService->SyncCalendar(m_pCalCallback, nullptr);
}

NS_IMPL_ISUPPORTS_INHERITED0(MailEwsLoadCalendarItemsCallback, MailEwsItemsCallback)

MailEwsLoadCalendarItemsCallback::MailEwsLoadCalendarItemsCallback(IMailEwsCalendarItemCallback * calCallback,
                                                                   nsIMsgIncomingServer * pIncomingServer)
: m_pCalCallback(calCallback)
                      , m_pIncomingServer(pIncomingServer)
{
}

MailEwsLoadCalendarItemsCallback::~MailEwsLoadCalendarItemsCallback() {
}

typedef struct __load_item_response {
    ews_item ** item;
    int item_count;
} load_item_response;

#include "MailEwsTimeZones.h"

static
void CleanUpIcalString(nsCString & data) {
    //remove all '\r\n '
    PRInt32 pos = -1;
            
    while ((pos = data.Find("\r\n ")) >= 0) {
        data.Cut(pos, 3);
    }

    char src[512] = {0};
    char dst[512] = {0};
    
    //Update TZID,
    //the exchange will use tz name some times update to TZID
    for(size_t i=0;i < TIMEZONES_COUNT; i+=2) {
        sprintf(src, "TZID:%s", TIMEZONES[i]);
        sprintf(dst, "TZID:%s", TIMEZONES[i + 1]);

        while((pos = data.Find(src)) >= 0) {
            data.Replace(pos,
                         strlen(src),
                         dst,
                         strlen(dst));
        }

        sprintf(src, "TZID=\"%s\"", TIMEZONES[i]);
        sprintf(dst, "TZID=\"%s\"", TIMEZONES[i + 1]);

        while((pos = data.Find(src)) >= 0) {
            data.Replace(pos,
                         strlen(src),
                         dst,
                         strlen(dst));
        }
    }
}

static
nsresult
From(nsIMsgIncomingServer * , ews_calendar_item * item, calIEvent * e);

static
nsresult
UpdateAttendee(calIEvent * e,
               ews_calendar_item * item,
               const nsCString & ownerEmail,
               bool required);
static
nsresult
UpdateOccurrence(calIEvent * e,
                 ews_calendar_item * item);

static
nsresult
UpdateStatus(calIEvent * e,
             nsCString ownerEmail);

NS_IMETHODIMP
MailEwsLoadCalendarItemsCallback::LocalOperation(void * response) {
    load_item_response * r = (load_item_response *)response;
    if (!r) return NS_OK;

    ews_item ** item = r->item;
    nsresult rv;

    mailews_logger << "create calIEvent on main threads:"
                   << r->item_count
                   << std::endl;

    nsCOMPtr<nsIMutableArray> calendarItems =
		    do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCString ownerEmail;
	rv = GetOwnerEmail(m_pIncomingServer, ownerEmail);
	NS_ENSURE_SUCCESS(rv, rv);

    for(int i=0;i < r->item_count;i++) {
        if (item[i]->item_type == EWS_Item_Calendar) {
            ews_calendar_item * cal_item = (ews_calendar_item*)item[i];

	        nsCOMPtr<calIEvent> e =
			        do_CreateInstance(CAL_EVENT_CONTRACTID, &rv);
	        NS_ENSURE_SUCCESS(rv, rv);

            if (item[i]->mime_content) {
                char *decodedContent =
                        PL_Base64Decode(item[i]->mime_content,
                                        strlen(item[i]->mime_content),
                                        nullptr);
                nsCString data;
                data.Adopt(decodedContent);

                CleanUpIcalString(data);

                rv = e->SetIcalString(data);
                NS_ENSURE_SUCCESS(rv, rv);

                nsCString uid(((ews_calendar_item*)item[i])->uid);
                
                if (uid.IsEmpty()) {
                    GenerateUid(uid);
                    mailews_logger << "generate uid for event:"
                                   << item[i]->item_id
                                   << ","
                                   << item[i]->change_key
                                   << ","
                                   << uid.get()
                                   << std::endl
                                   << data.get()
                                   << std::endl;
                }
                e->SetId(uid);

                nsCOMPtr<calIDateTime> dtStart;
                e->GetStartDate(getter_AddRefs(dtStart));

                if (!dtStart) {
                    mailews_logger << "skip no start date event:"
                                   << item[i]->item_id
                                   << ","
                                   << item[i]->change_key
                                   << ","
                                   << uid.get()
                                   << std::endl
                                   << data.get()
                                   << std::endl;
                    continue;
                }

                rv = e->RemoveAllAttendees();
                NS_ENSURE_SUCCESS(rv, rv);
	
                rv = UpdateAttendee(e,
                                    (ews_calendar_item*)item[i],
                                    ownerEmail,
                                    true);
                NS_ENSURE_SUCCESS(rv, rv);
                rv = UpdateAttendee(e,
                                    (ews_calendar_item*)item[i],
                                    ownerEmail,
                                    false);
                NS_ENSURE_SUCCESS(rv, rv);
                rv = UpdateOccurrence(e,
                                      (ews_calendar_item*)item[i]);
                NS_ENSURE_SUCCESS(rv, rv);

                rv = UpdateStatus(e,
                                  ownerEmail);
                NS_ENSURE_SUCCESS(rv, rv);
            } else {
                rv = From(m_pIncomingServer,
                          (ews_calendar_item *)item[i], e);
                NS_ENSURE_SUCCESS(rv, rv);
            }

	        //Save item id
            rv = SetPropertyString(e,
                                   NS_LITERAL_STRING("X-ITEM-ID"),
                                   nsCString(item[i]->item_id));
	        NS_ENSURE_SUCCESS(rv, rv);

	        //Save change key
            rv = SetPropertyString(e,
                                   NS_LITERAL_STRING("X-CHANGE-KEY"),
                                   nsCString(item[i]->change_key));
	        NS_ENSURE_SUCCESS(rv, rv);

            //Save Master id
            if (cal_item->recurring_master_id) {
                rv = SetPropertyString(e,
                                       NS_LITERAL_STRING("X-MASTER_ID"),
                                       nsCString(cal_item->recurring_master_id));
                NS_ENSURE_SUCCESS(rv, rv);

                rv = SetPropertyString(e,
                                       NS_LITERAL_STRING("X-MASTER_UID"),
                                       nsCString(cal_item->recurring_master_uid));
                NS_ENSURE_SUCCESS(rv, rv);
            } else if (cal_item->calendar_item_type ==
                       EWS_CalendarItemType_Single) {
                e->SetRecurrenceId(nullptr);
            }

	        calendarItems->AppendElement(e, false);
        }
    }

    nsCOMPtr<nsIArray> resultArray(do_QueryInterface(calendarItems));
    
    mailews_logger << "create calIEvent on main threads done:"
                   << r->item_count
                   << std::endl;
    return m_pCalCallback->OnResult(resultArray);
}

static
void
UpdateRecurringMasterItemId(ews_session * session,
                            ews_item ** items,
                            int item_count) {
    for(int i=0;i < item_count;i++) {
        if (items[i]->item_type == EWS_Item_Calendar) {
            ews_calendar_item * item = (ews_calendar_item*)items[i];

            if (item->calendar_item_type ==
                EWS_CalendarItemType_Exception) {
                item->recurring_master_id = NULL;
                item->recurring_master_uid = NULL;
                char * err_msg = NULL;

                ews_cal_get_recurrence_master_id(session,
                                                 item->item.item_id,
                                                 &item->recurring_master_id,
                                                 &item->recurring_master_uid,
                                                 &err_msg);
                if (err_msg) free(err_msg);
            }
        }
    }
}
        

NS_IMETHODIMP
MailEwsLoadCalendarItemsCallback::RemoteOperation(void ** response) {
	nsCOMPtr<nsIMsgIncomingServer> server;

    if (m_RemoteMsg) {
        free(m_RemoteMsg);
        m_RemoteMsg = NULL;
    }
    
    nsresult rv = m_Folder->GetServer(getter_AddRefs(server));
    if (NS_FAILED(rv)) {
        NS_FAILED_WARN(rv);
        m_RemoteResult = EWS_FAIL;
        m_RemoteMsg = NULL;

        return rv;
    }

	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(server));

    nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(m_Folder));
    
	nsCOMPtr<IMailEwsService> ewsService;
	ewsServer->GetService(getter_AddRefs(ewsService));

    ews_session * session = NULL;
    rv = ewsService->GetNewSession(&session);

    if (NS_SUCCEEDED(rv) && session) {
        const char ** item_ids =
                (const char **)malloc(sizeof(const char *) * m_ItemIds.Length());
    
        for (PRUint32 i = 0; i < m_ItemIds.Length(); i++) {
            item_ids[i] = m_ItemIds[i].get();
        }

        ews_item ** items = NULL;
        int item_count = m_ItemIds.Length();

        mailews_logger << "load calendar items for:"
                  << item_count
                  << std::endl;
        m_RemoteResult = ews_get_items(session,
                                       item_ids,
                                       item_count,
                                       EWS_GetItems_Flags_MimeContent
                                       | EWS_GetItems_Flags_CalendarItem,
                                       &items,
                                       &m_RemoteMsg);

        *response = NULL;
        if (m_RemoteResult == EWS_SUCCESS && items) {
            load_item_response * r;

            UpdateRecurringMasterItemId(session,
                                        items,
                                        item_count);
            
            *response = r = (load_item_response*)malloc(sizeof(load_item_response));
            r->item = items;
            r->item_count = item_count;
            mailews_logger << "load calendar items done:"
                      << item_count
                      << std::endl;
        } else {
            mailews_logger << "load calendar items failed:"
                           << std::endl;
            
            for (PRUint32 i = 0; i < m_ItemIds.Length(); i++) {
                mailews_logger << "item:"
                               << i
                               << ", id:"
                               << item_ids[i]
                               << std::endl;;
            }
        }

        free(item_ids);
        
    } else {
        m_RemoteResult = session ? EWS_FAIL : 401;
        m_RemoteMsg = NULL;
    }

    ewsService->ReleaseSession(session);
    return rv;
}

NS_IMETHODIMP
MailEwsLoadCalendarItemsCallback::FreeRemoteResponse(void *response) {
    if (response) {
        load_item_response * r = (load_item_response *)response;
        ews_free_items(r->item, r->item_count);
        free(r);
    }
	return NS_OK;
}

static
nsresult
From(nsIMsgIncomingServer * pIncomingServer,
     ews_calendar_item * item,
     calIEvent * e) {
    nsresult rv = NS_OK;
    
    nsCString ownerEmail;
	rv = GetOwnerEmail(pIncomingServer, ownerEmail);
	NS_ENSURE_SUCCESS(rv, rv);

    rv = e->RemoveAllAttendees();
    NS_ENSURE_SUCCESS(rv, rv);
	
    rv = UpdateAttendee(e, item, ownerEmail, true);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = UpdateAttendee(e, item, ownerEmail, false);
    NS_ENSURE_SUCCESS(rv, rv);
	return NS_OK;
}

static
const char * participationArray[] = {
	"NEEDS-ACTION",
	"ACCEPTED",
	"TENTATIVE",
	"ACCEPTED",
	"DECLINED",
	"NEEDS-ACTION",
};

static
nsresult
UpdateAttendee(calIEvent * e,
               ews_calendar_item * item,
               const nsCString & ownerEmail,
               bool required) {
	int c = 0;
	ews_attendee ** attendees = NULL;
	nsresult rv = NS_OK;

	if (required) {
		c = item->required_attendees_count;
		attendees = item->required_attendees;
	} else {
		c = item->optional_attendees_count;
		attendees = item->optional_attendees;
	}

	if (c == 0 || !attendees)
		return NS_OK;

	for(int i=0;i < c;i++) {
		nsCOMPtr<calIAttendee> a =
				do_CreateInstance(CAL_ATTENDEE_CONTRACTID, &rv);
		NS_ENSURE_SUCCESS(rv, rv);

		nsCString id;
		
		if (attendees[i]->email_address.routing_type &&
		    !strcmp(attendees[i]->email_address.routing_type, "EX")) {
			id.AppendLiteral("ldap:");
		} else {
			id.AppendLiteral("mailto:");
		}
		id.AppendLiteral(attendees[i]->email_address.email);
		rv = a->SetId(id);
		NS_ENSURE_SUCCESS(rv, rv);

		rv = a->SetRsvp(nsCString("FALSE"));
		NS_ENSURE_SUCCESS(rv, rv);

		rv = a->SetUserType(nsCString("INDIVIDUAL"));
		NS_ENSURE_SUCCESS(rv, rv);

		rv = a->SetRole(nsCString(required ? "REQ-PARTICIPANT" : "OPT-PARTICIPANT"));
		NS_ENSURE_SUCCESS(rv, rv);

		nsCString name;
		if (attendees[i]->email_address.name) {
			name.AssignLiteral(attendees[i]->email_address.name);
		}
		
		rv = a->SetCommonName(name);
		NS_ENSURE_SUCCESS(rv, rv);

        nsCString status;

        if (ownerEmail.Equals(nsCString(attendees[i]->email_address.email),
                              nsCaseInsensitiveCStringComparator())) {
            status.AssignLiteral(participationArray[item->my_response_type]);
        } else {
            status.AssignLiteral(participationArray[attendees[i]->response_type]);
        }
                              
		rv = a->SetParticipationStatus(status);
		NS_ENSURE_SUCCESS(rv, rv);

		rv = e->AddAttendee(a);
		NS_ENSURE_SUCCESS(rv, rv);
	}
	
	return NS_OK;
}

static
nsresult
UpdateOccurrence(calIEvent * e,
                 ews_calendar_item * item) {
    if (item->modified_occurrences_count == 0
        && item->deleted_occurrences_count == 0) {
        return NS_OK;
    }

    nsresult rv = NS_OK;

    nsCOMPtr<calIRecurrenceInfo> recurrenceInfo;
    rv = e->GetRecurrenceInfo(getter_AddRefs(recurrenceInfo));
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<calITimezoneService> tzService(do_GetService(CAL_TIMEZONESERVICE_CONTRACTID));
    nsCOMPtr<calITimezone> timezone;
    
    if (item->meeting_time_zone->time_zone_name && tzService) {
        nsCString tzid(item->meeting_time_zone->time_zone_name);
        
        tzService->GetTimezone(tzid, getter_AddRefs(timezone));
    }

    if (!timezone) {
        //Get time zone from event
        nsCOMPtr<calIDateTime> dt;
        
        e->GetStartDate(getter_AddRefs(dt));

        if (dt) {
            dt->GetTimezone(getter_AddRefs(timezone));
        }
    }
    
    nsCOMPtr<calIItemBase> meItem;
        
    e->Clone(getter_AddRefs(meItem));

    nsCOMPtr<calIEvent> parentClone(do_QueryInterface(meItem));
    parentClone->SetRecurrenceInfo(nullptr);

    for (int i = 0; i < item->modified_occurrences_count; i++) {
        ews_occurrence_info * info =
                item->modified_occurrences[i];

        parentClone->Clone(getter_AddRefs(meItem));

        nsCOMPtr<calIEvent> me(do_QueryInterface(meItem));
        
        rv = me->SetParentItem(e);
        
        nsCOMPtr<calIDateTime> rid =
                do_CreateInstance(CAL_DATETIME_CONTRACTID, &rv);
		NS_ENSURE_SUCCESS(rv, rv);

        if (timezone) {
            rid->SetTimezone(timezone);
        }
        rv = rid->SetNativeTime(info->original_start * UNIX_TIME_TO_PRTIME);
		NS_ENSURE_SUCCESS(rv, rv);

        rv = me->SetRecurrenceId(rid);
		NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<calIDateTime> ds =
                do_CreateInstance(CAL_DATETIME_CONTRACTID, &rv);
		NS_ENSURE_SUCCESS(rv, rv);
                
        if (timezone) {
            ds->SetTimezone(timezone);
        }
        rv = ds->SetNativeTime(info->start * UNIX_TIME_TO_PRTIME);
		NS_ENSURE_SUCCESS(rv, rv);

        rv = me->SetStartDate(ds);
		NS_ENSURE_SUCCESS(rv, rv);

        if (info->end > 0) {
            nsCOMPtr<calIDateTime> de =
                    do_CreateInstance(CAL_DATETIME_CONTRACTID, &rv);
            NS_ENSURE_SUCCESS(rv, rv);
                
            if (timezone) {
                de->SetTimezone(timezone);
            }

            rv = de->SetNativeTime(info->end * UNIX_TIME_TO_PRTIME);
            NS_ENSURE_SUCCESS(rv, rv);

            rv = me->SetEndDate(de);
            NS_ENSURE_SUCCESS(rv, rv);
        }

        rv = recurrenceInfo->ModifyException(me, true);
		NS_ENSURE_SUCCESS(rv, rv);
    }

    for (int i = 0; i < item->deleted_occurrences_count; i++) {
        ews_deleted_occurrence_info * info =
                item->deleted_occurrences[i];

        nsCOMPtr<calIDateTime> dt =
                do_CreateInstance(CAL_DATETIME_CONTRACTID, &rv);
		NS_ENSURE_SUCCESS(rv, rv);
                
        rv = dt->SetNativeTime(info->start * UNIX_TIME_TO_PRTIME);
		NS_ENSURE_SUCCESS(rv, rv);

        rv = recurrenceInfo->RemoveOccurrenceAt(dt);
		NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}

static
nsresult
UpdateStatus(calIEvent * e,
             nsCString ownerEmail) {
    nsCString status;
    nsresult rv = NS_OK;
    
    rv = GetOwnerStatus(e, ownerEmail, status);

    if (NS_FAILED(rv)
        || status.IsEmpty()
        || status.Equals("NEEDS-ACTION")) {
        rv = e->SetStatus(NS_LITERAL_CSTRING("NEEDS-ACTION"));
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}

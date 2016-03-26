#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgIncomingServer.h"
#include "nsIMsgFolder.h"
#include "nsArrayUtils.h"
#include "nsIMutableArray.h"
#include "nsComponentManagerUtils.h"
#include "nsIVariant.h"
#include "nsIMsgIdentity.h"
#include "nsIMsgAccountManager.h"
#include "nsMsgBaseCID.h"
#include "nsID.h"
#include "nsIUUIDGenerator.h"

#include "calITodo.h"
#include "calIDateTime.h"
#include "calIAttendee.h"
#include "calIICSService.h"
#include "calBaseCID.h"
#include "calITimezone.h"
#include "calIRecurrenceInfo.h"
#include "calIRecurrenceRule.h"
#include "calITimezone.h"
#include "calITimezoneProvider.h"

#include "nsMsgUtils.h"

#include "plbase64.h"

#include "libews.h"

#include "MailEwsAddTodoTask.h"
#include "MailEwsErrorInfo.h"
#include "MailEwsItemsCallback.h"
#include "MailEwsCalendarUtils.h"
#include "MailEwsTimeZones.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"
#include "IMailEwsItemOp.h"
#include "IMailEwsMsgFolder.h"

#include "MailEwsLog.h"
#include "MailEwsCommon.h"

#include "rfc3339/rfc3339.h"

void
FromCalTodoToTodo(calITodo * todo, ews_task_item * item);

extern
nsresult
GetPropertyString(calIItemBase * event, const nsAString & name, nsCString & sValue);

extern
nsresult
SetPropertyString(calIItemBase * event,
                  const nsAString & name,
                  const nsCString & sValue);

class AddTodoDoneTask : public MailEwsRunnable
{
public:
	AddTodoDoneTask(int result,
	                calITodo * todo,
	                nsCString item_id,
	                nsCString change_key,
	                nsISupportsArray * ewsTaskCallbacks,
	                AddTodoTask * runnable)
		: MailEwsRunnable(ewsTaskCallbacks)
		, m_Runnable(runnable)
		, m_Result(result)
		, m_Todo(todo)
		, m_ItemId(item_id)
		, m_ChangeKey(change_key)
	{
	}

	NS_IMETHOD Run() {
		nsresult rv = NS_OK;
		if (m_Result == EWS_SUCCESS) {
			//Save item id
			rv = SetPropertyString(m_Todo,
			                       NS_LITERAL_STRING("X-ITEM-ID"),
			                       m_ItemId);
			NS_ENSURE_SUCCESS(rv, rv);

			//Save change key
			rv = SetPropertyString(m_Todo,
			                       NS_LITERAL_STRING("X-CHANGE-KEY"),
			                       m_ChangeKey);
			NS_ENSURE_SUCCESS(rv, rv);

			nsCString oldUid;
			rv = m_Todo->GetId(oldUid);

			if (NS_SUCCEEDED(rv) && oldUid.IsEmpty()) {
				//Updat the uid
				m_Todo->SetId(m_ItemId);
			}
		}
		NotifyEnd(m_Runnable, m_Result);
		
		return NS_OK;
	}

private:
	nsCOMPtr<nsIRunnable> m_Runnable;
	int m_Result;
	nsCOMPtr<calITodo> m_Todo;
	nsCString m_ItemId;
	nsCString m_ChangeKey;
};

AddTodoTask::AddTodoTask(calITodo * todo,
                         nsIMsgIncomingServer * pIncomingServer,
                         nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
	, m_Todo(todo)
	, m_pIncomingServer(pIncomingServer)
	, m_MimeContent("")
{
}

AddTodoTask::~AddTodoTask() {
}

static
nsresult
GenerateUid(nsCString & uid) {
	nsresult rv = NS_OK;
    
	nsCOMPtr<nsIUUIDGenerator> uuidgen =
			do_GetService("@mozilla.org/uuid-generator;1", &rv);
	NS_ENSURE_SUCCESS(rv, rv);

	nsID uuid;
	rv = uuidgen->GenerateUUIDInPlace(&uuid);
	NS_ENSURE_SUCCESS(rv, rv);

	char uuidChars[NSID_LENGTH];
	uuid.ToProvidedString(uuidChars);

	//skip {}
	uuidChars[NSID_LENGTH - 2] = 0;

	uid.Assign(&uuidChars[1]);

	return NS_OK;
}

static
void UpdateMimeContentToEwsFormat(nsCString & mimeContent,
                                  bool & has_uid,
                                  nsCString & uid) {
	nsresult rv = NS_OK;
    
	nsCOMPtr<calIICSService> icsService =
			do_GetService(CAL_ICSSERVICE_CONTRACTID, &rv);
	NS_FAILED_WARN(rv);

	if (!icsService)
		return;

	nsCOMPtr<calIIcalComponent> component;
    
	rv = icsService->ParseICS(mimeContent,
	                          nullptr,
	                          getter_AddRefs(component));
	NS_FAILED_WARN(rv);

	if (!component)
		return;

	//check METHOD
	nsCString method;
	rv = component->GetMethod(method);
	NS_FAILED_WARN(rv);

	if (NS_FAILED(rv) || method.IsEmpty()) {
		component->SetMethod(NS_LITERAL_CSTRING("PUBLISH"));
	}

	//Check UID
	nsCOMPtr<calIIcalComponent> todo;
	rv = component->GetFirstSubcomponent(NS_LITERAL_CSTRING("VTODO"),
	                                     getter_AddRefs(todo));
	NS_FAILED_WARN(rv);

	if (todo) {
		rv = todo->GetUid(uid);
		NS_FAILED_WARN(rv);

		if (NS_FAILED(rv) || uid.IsEmpty()) {
			has_uid = false;
			rv = GenerateUid(uid);
			NS_FAILED_WARN(rv);

			if (NS_SUCCEEDED(rv) && !uid.IsEmpty()) {
				todo->SetUid(uid);
			}
		} else {
			has_uid = true;
		}
	} else {
		has_uid = false;
		uid.AssignLiteral("");
	}

	mailews_logger << "origianl ics:"
	               << mimeContent.get()
	               << std::endl;
    
	component->SerializeToICS(mimeContent);
    
	mailews_logger << "updated ics:"
	               << mimeContent.get()
	               << std::endl;
}

void
UpdateSessionTimezone(calITodo * todo,
                      ews_session * session);

NS_IMETHODIMP AddTodoTask::Run() {
	char * err_msg = NULL;
	
	// This method is supposed to run on the main
	if(!NS_IsMainThread()) {
		NS_DispatchToMainThread(this);
		return NS_OK;
	}
	
	if (EWS_FAIL == NotifyBegin(this)) {
		return NS_OK;
	}

	m_Todo->GetIcalString(m_MimeContent);

	if (m_MimeContent.Length() == 0)
		return NS_OK;

	bool has_uid = false;
	nsCString uid;
    
	UpdateMimeContentToEwsFormat(m_MimeContent, has_uid, uid);
	
	ews_session * session = NULL;
	nsresult rv = NS_OK;
	nsCString itemId;
	nsCString changeKey;
	bool hasItemId = false;
	bool hasChangeKey = false;

	m_Todo->HasProperty(NS_LITERAL_STRING("X-ITEM-ID"), &hasItemId);
	m_Todo->HasProperty(NS_LITERAL_STRING("X-CHANGE-KEY"), &hasChangeKey);

	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer, &rv));
	NS_ENSURE_SUCCESS(rv, rv);
    
	nsCOMPtr<IMailEwsService> ewsService;
	ewsServer->GetService(getter_AddRefs(ewsService));
	NS_ENSURE_SUCCESS(rv, rv);

	nsresult rv1 = ewsService->GetNewSession(&session);
	int ret = EWS_FAIL;
    
	if (NS_SUCCEEDED(rv1)
	    && session) {
		UpdateSessionTimezone(m_Todo,
		                      session);
		
        //create new todo item
        ews_task_item * item = (ews_task_item *)malloc(sizeof(ews_task_item));
        memset(item, 0, sizeof(ews_task_item));
			
        item->item.item_type = EWS_Item_Task;

        FromCalTodoToTodo(m_Todo, item);

        ret = ews_create_item_by_distinguished_id_name(session,
                                                       EWSDistinguishedFolderIdName_tasks,
                                                       &item->item,
                                                       &err_msg);

        if (ret == EWS_SUCCESS) {
            if (item->item.item_id) {
                itemId.AssignLiteral(item->item.item_id);
            }
                
            if (item->item.change_key)
                changeKey.AssignLiteral(item->item.change_key);
                
            mailews_logger << "new created todo item, item id:"
                           << itemId.get()
                           << "uid:" << uid.get()
                           << "change key:" << changeKey.get()
                           << std::endl;
        } else {
            mailews_logger << "Error Mime content:"
                           << m_MimeContent.get()
                           << std::endl;
        }

        ews_free_item(&item->item);
		
		NotifyError(this, ret, err_msg);

		if (err_msg) free(err_msg);
	} else {
		NotifyError(this, session ? EWS_FAIL : 401, err_msg);
	}

	ewsService->ReleaseSession(session);

	if (!(has_uid && hasItemId && hasChangeKey)) {
		nsCOMPtr<nsIRunnable> resultrunnable =
				new AddTodoDoneTask(ret,
				                    m_Todo,
				                    itemId,
				                    changeKey,
				                    m_pEwsTaskCallbacks,
				                    this);
		
		NS_DispatchToMainThread(resultrunnable);
	}
	return NS_OK;
}

static
ews_recurrence *
To(calIRecurrenceInfo * info,
   calIDateTime * dtStart);

void
FromCalTodoToTodo(calITodo * todo, ews_task_item * item) {
	//Item Id
	bool hasP = false;

	if (NS_SUCCEEDED(todo->HasProperty(NS_LITERAL_STRING("X-ITEM-ID"), &hasP))
	    && hasP) {
		nsCString v;
		GetPropertyString(todo, NS_LITERAL_STRING("X-ITEM-ID"), v);
		item->item.item_id = strdup(v.get());
	}

	//Change Key
	hasP = false;
	if (NS_SUCCEEDED(todo->HasProperty(NS_LITERAL_STRING("X-CHANGE-KEY"), &hasP))
	    && hasP) {
		nsCString v;
		GetPropertyString(todo, NS_LITERAL_STRING("X-CHANGE-KEY"), v);
		item->item.change_key = strdup(v.get());
	}

    nsCOMPtr<calIDateTime> dtStart;
    todo->GetEntryDate(getter_AddRefs(dtStart));

	GET_DATE_TIME_3(todo->GetEntryDate, item->start_date, true);
	GET_DATE_TIME_3(todo->GetDueDate, item->due_date, true);
	GET_DATE_TIME_3(todo->GetCompletedDate, item->complete_date, true);

	todo->GetIsCompleted(&item->is_complete);;

	int16_t percent = 0;
	todo->GetPercentComplete(&percent);
	item->percent_complete = (double)percent;

	nsCString status;
	todo->GetStatus(status);

	if (status.Equals("NONE")) {
		item->status = EWS__TaskStatusType__NotStarted;
	} else if (status.Equals("IN-PROCESS")) {
		item->status = EWS__TaskStatusType__InProgress;
	} else if (status.Equals("COMPLETED")) {
		item->status = EWS__TaskStatusType__Completed;
	} else if (status.Equals("NEEDS-ACTION")) {
		item->status = EWS__TaskStatusType__WaitingOnOthers;
	} else if (status.Equals("CANCELLED")) {
		item->status = EWS__TaskStatusType__Deferred;
	} else {
		item->status = EWS__TaskStatusType__NotStarted;
	}
	
	nsCOMPtr<calIRecurrenceInfo> rInfo;

	todo->GetRecurrenceInfo(getter_AddRefs(rInfo));

	if (rInfo) {
		item->recurrence =
				To(rInfo,
                   dtStart);
		
		date::Rfc3339 rfc3339;

		item->recurrence->range.start_date =
				strdup(rfc3339.serialize(item->start_date, true).c_str());
	}

	nsCString desc;
	GetPropertyString(todo,
	                  NS_LITERAL_STRING("DESCRIPTION"),
	                  desc);
	item->item.body = strdup(desc.get());

	nsCString title;
	todo->GetTitle(title);
	item->item.subject = strdup(title.get());
}

static
void
To(int16_t v,
   int & day_of_week_index,
   int & days_of_week) {
	//Outlook only has Last Week
	int16_t vv = v;
	
	if (v < 0) vv *= -1;

	day_of_week_index = vv / 8;
	days_of_week = vv % 8;

	if (day_of_week_index == 1 && v < 0) {
		day_of_week_index = EWS_DayOfWeekIndex_Last;
	} else {
		day_of_week_index --;
	}

	days_of_week --;
}

static
bool
To(calIRecurrenceRule * rule,
   calIDateTime * dtStart,
   ews_recurrence_pattern * p) {
	nsCString t;
	int16_t * v_by_day;
	uint32_t count_by_day;
	int16_t * v_by_month;
	uint32_t count_by_month;
	int16_t * v_by_month_day;
	uint32_t count_by_month_day;
	int32_t interval;
    
	rule->GetType(t);
	rule->GetInterval(&interval);
	rule->GetComponent(NS_LITERAL_CSTRING("BYDAY"),
	                   &count_by_day,
	                   &v_by_day);
	rule->GetComponent(NS_LITERAL_CSTRING("BYMONTH"),
	                   &count_by_month,
	                   &v_by_month);
	rule->GetComponent(NS_LITERAL_CSTRING("BYMONTHDAY"),
	                   &count_by_month_day,
	                   &v_by_month_day);

	if (t.EqualsLiteral("YEARLY")) {
		if (count_by_day == 1 && v_by_day
		    && count_by_month == 1 && v_by_month) {
			p->pattern_type =
					EWS_Recurrence_RelativeYearly;

			To(v_by_day[0],
			   p->day_of_week_index,
			   p->days_of_week);
			
			p->month = v_by_month[0] - 1;
		} else if (count_by_month == 1 && v_by_month
		           && count_by_month_day == 1 && v_by_month_day) {
			p->pattern_type =
					EWS_Recurrence_AbsoluteYearly;
			p->month = v_by_month[0] - 1;
			p->day_of_month = v_by_month_day[0];
		} else if (dtStart) {
			p->pattern_type =
					EWS_Recurrence_AbsoluteYearly;
            int16_t month;
            int16_t day;
            dtStart->GetMonth(&month);
            dtStart->GetDay(&day);

            p->month = month;
            p->day_of_month = day;
        }
	} else if (t.EqualsLiteral("MONTHLY")) {
		if (count_by_day == 1 && v_by_day) {
			p->pattern_type =
					EWS_Recurrence_RelativeMonthly;

			To(v_by_day[0],
			   p->day_of_week_index,
			   p->days_of_week);

			p->interval = interval;
		} else if ( count_by_month_day == 1 && v_by_month_day) {
			p->pattern_type =
					EWS_Recurrence_AbsoluteMonthly;
			p->interval = interval;
			p->day_of_month = v_by_month_day[0];
		} else if (dtStart) {
			p->pattern_type =
					EWS_Recurrence_AbsoluteMonthly;
            int16_t month;
            int16_t day;
            dtStart->GetMonth(&month);
            dtStart->GetDay(&day);

			p->interval = interval;
            p->day_of_month = day;
        }
	} else if (t.EqualsLiteral("WEEKLY")) {
		p->pattern_type =
				EWS_Recurrence_Weekly;
		p->interval = interval;
		p->days_of_week = v_by_day[0];
	} else if (t.EqualsLiteral("DAILY")) {
		p->pattern_type =
				EWS_Recurrence_Daily;
		p->interval = interval;
	}

	return true;
}

static
bool
To(calIRecurrenceRule * l,
   ews_recurrence_range * r) {

	bool is_by_count = false;
	int32_t count = -1;
	nsCOMPtr<calIDateTime> dt;
	
	l->GetIsByCount(&is_by_count);
	l->GetCount(&count);
	l->GetUntilDate(getter_AddRefs(dt));

	if (is_by_count && count != -1) {
		r->range_type = EWSRecurrenceRangeType_Numbered;
		r->number_of_occurrences = count;
	} else if (!is_by_count && dt) {
		r->range_type = EWSRecurrenceRangeType_EndDate;
		PRTime end;
		dt->GetNativeTime(&end);

		end /= 1000000;
		
		date::Rfc3339 rfc3339;

        r->end_date = strdup(rfc3339.serialize((time_t)end, true).c_str());
	} else {
		r->range_type = EWSRecurrenceRangeType_NoEnd;
	}

	return true;
}

static
ews_recurrence *
To(calIRecurrenceInfo * info,
   calIDateTime * dtStart) {
	nsCOMPtr<calIRecurrenceRule> rule;
	uint32_t count;
	calIRecurrenceItem ** items;

	nsresult rv = info->GetRecurrenceItems(&count,
	                         &items);

	if (NS_FAILED(rv) || count == 0) {
		return nullptr;
	}

	for(uint32_t i = 0; i < count; i++) {
		rule = do_QueryInterface(items[i]);

		if (rule)
			break;
	}

	if (!rule)
		return nullptr;

	ews_recurrence * r = (ews_recurrence*)malloc(sizeof(ews_recurrence));
	memset(r, 0, sizeof(ews_recurrence));
	
	To(rule,
       dtStart,
       &r->pattern);
    
	To(rule, &r->range);

	return r;
}

static
nsresult
GetTimezoneOrDefault(calIDateTime * dt,
                     calITimezone ** tz) {
    nsCOMPtr<calITimezone> timezone;
    nsresult rv = NS_OK;

    if (dt)
        rv = dt->GetTimezone(getter_AddRefs(timezone));
    bool isUTC = false;
    bool isFloating = false;

    if (timezone) {
        timezone->GetIsUTC(&isUTC);
        timezone->GetIsFloating(&isFloating);
    }
    
    if (!timezone ||
        isUTC ||
        isFloating) {
        nsCOMPtr<calITimezoneService> tzService(do_GetService(CAL_TIMEZONESERVICE_CONTRACTID));

        rv = tzService->GetDefaultTimezone(getter_AddRefs(timezone));
    }

    NS_ASSERTION(timezone, "could not find timezone");

    if (timezone &&
        NS_SUCCEEDED(timezone->GetIsUTC(&isUTC)) &&
        !isUTC) {
        nsCString content;
        timezone->ToString(content);
        
        mailews_logger << "get time zone:"
                       << content.get()
                       << std::endl;
    }
    
    NS_IF_ADDREF(*tz = timezone);

    return rv;
}

nsresult
GetExchangeTZId(calITimezone * tz,
                nsCString & tzId) {
    nsCOMPtr<calITimezoneService> tzService(do_GetService(CAL_TIMEZONESERVICE_CONTRACTID));

    tzId.AssignLiteral("");
    
    for (size_t i = 0; i < TIMEZONES_COUNT; i+=2) {
        nsCOMPtr<calITimezone> timezone;
        nsCString exId;
        exId.AssignLiteral(TIMEZONES[i + 1]);
        tzService->GetTimezone(exId,
                               getter_AddRefs(timezone));

        if (tz == timezone) {
            tzId.AssignLiteral(TIMEZONES[i + 1]);
            break;
        }
    }

    return NS_OK;
}

void
UpdateSessionTimezone(calITodo * todo,
                      ews_session * session) {
	nsCOMPtr<calITimezone> timezone;
	nsCOMPtr<calIDateTime> dtStart;

    todo->GetCreationDate(getter_AddRefs(dtStart));

    if (!dtStart)
        todo->GetEntryDate(getter_AddRefs(dtStart));

	GetTimezoneOrDefault(dtStart, getter_AddRefs(timezone));

	if (timezone) {
		nsCString tzId("");
		GetExchangeTZId(timezone, tzId);

		if (!tzId.IsEmpty()) {
            mailews_logger << "update session timezone:"
                           << tzId.get()
                           << std::endl;
            
			ews_session_set_timezone_id(session,
			                            tzId.get());
		}
	}
}

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
#include "calITodo.h"
#include "calIAttendee.h"
#include "calIRecurrenceInfo.h"
#include "calIDateTime.h"
#include "calIRecurrenceDate.h"
#include "calITimezoneProvider.h"
#include "calITimezone.h"
#include "calIRecurrenceInfo.h"
#include "calIRecurrenceRule.h"

#include "plbase64.h"

#include "libews.h"

#include "MailEwsSyncTodoTask.h"
#include "MailEwsErrorInfo.h"
#include "MailEwsItemsCallback.h"
#include "MailEwsCalendarUtils.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"
#include "IMailEwsItemOp.h"
#include "IMailEwsMsgFolder.h"

#include "MailEwsLog.h"
#include "rfc3339/rfc3339.h"

static const char * get_sync_state(void * user_data);
static void set_sync_state(const char * sync_state, void * user_data);
static void new_item(const ews_item * item, void * user_data);
static void update_item(const ews_item * item, void * user_data);
static void delete_item(const ews_item * item, void * user_data);
static void read_item(const ews_item * item, int read, void * user_data);
static int get_max_return_item_count(void * user_data);
static char ** get_ignored_item_ids(int * p_count, void * user_data);

#define USER_PTR(x) ((SyncTodoTask*)x)

extern
nsresult
SetPropertyString(calIItemBase * event,
                  const nsAString & name,
                  const nsCString & sValue);
extern
nsresult
GetPropertyString(calIItemBase * event,
                  const nsAString & name,
                  nsCString & sValue);

class MailEwsLoadTodoItemsCallback : public MailEwsItemsCallback {
public:
	NS_DECL_ISUPPORTS_INHERITED
    MailEwsLoadTodoItemsCallback(IMailEwsCalendarItemCallback * calCallback);

	NS_IMETHOD LocalOperation(void *severResponse) override;
	NS_IMETHOD RemoteOperation(void **severResponse) override;
	NS_IMETHOD FreeRemoteResponse(void *severResponse) override;
protected:
    virtual ~MailEwsLoadTodoItemsCallback();

	nsCOMPtr<IMailEwsCalendarItemCallback> m_pCalCallback;
};

class SyncTodoDoneTask : public MailEwsRunnable
{
public:
	SyncTodoDoneTask(int result,
                     nsISupportsArray * ewsTaskCallbacks,
                     SyncTodoTask * runnable)
		: MailEwsRunnable(ewsTaskCallbacks)
		, m_Runnable(runnable)
		, m_SyncTodoTask(runnable)
		, m_Result(result)
	{
	}

	NS_IMETHOD Run() {
        mailews_logger << "sync task done task running."
                       << std::endl;
		m_SyncTodoTask->CleanupTaskRun();
		
		NotifyEnd(m_Runnable, m_Result);
		
        mailews_logger << "sync task done task done."
                       << std::endl;
		return NS_OK;
	}

private:
	nsCOMPtr<nsIRunnable> m_Runnable;
	SyncTodoTask * m_SyncTodoTask;
	int m_Result;
};

SyncTodoTask::SyncTodoTask(IMailEwsCalendarItemCallback * calCallback,
                           nsIMsgIncomingServer * pIncomingServer,
                           nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
	, m_pCalCallback(calCallback)
	, m_pIncomingServer(pIncomingServer)
	, m_Result(EWS_FAIL) {
	calCallback->GetSyncState(m_SyncState, false);

    mailews_logger << "task sync state:"
                   << (m_SyncState.get())
                   << std::endl;
}

SyncTodoTask::~SyncTodoTask() {
}
	
NS_IMETHODIMP SyncTodoTask::Run() {
	char * err_msg = NULL;
	
	if (EWS_FAIL == NotifyBegin(this)) {
		return NS_OK;
	}

	ews_session * session = NULL;
    nsresult rv = NS_OK;

	bool running = false;
	rv = m_pCalCallback->GetIsRunning(&running, false /*todo*/);
	NS_ENSURE_SUCCESS(rv, rv);

	if (running) {
		return NS_OK;
	}

    mailews_logger << "sync task task running..."
                   << std::endl;
    
	m_pCalCallback->SetIsRunning(true, false /* todo */);
	
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

		m_Result = ews_sync_task(session,
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
            new SyncTodoDoneTask(m_Result,
                                 m_pEwsTaskCallbacks,
                                 this);
		
    NS_DispatchToMainThread(resultrunnable);

    ewsService->ReleaseSession(session);
    
    mailews_logger << "sync task task done."
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
    if (item->item_type == EWS_Item_Task)
        USER_PTR(user_data)->NewItem(item->item_id);
}

static void update_item(const ews_item * item, void * user_data) {
    if (item->item_type == EWS_Item_Task)
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

void SyncTodoTask::NewItem(const char * item_id) {
	UpdateItem(item_id);
}

void SyncTodoTask::UpdateItem(const char * item_id) {
	nsCString itemId(item_id);

	if (!m_ItemIds.Contains(itemId))
		m_ItemIds.AppendElement(itemId);
}

void SyncTodoTask::DeleteItem(const char * item_id) {
	nsCString itemId(item_id);

	if (!m_DeletedItemIds.Contains(itemId))
		m_DeletedItemIds.AppendElement(itemId);
}

NS_IMETHODIMP
SyncTodoTask::ProcessItems() {
	nsresult rv = NS_OK;

	if (!HasItems()) {
        mailews_logger << "sync task found 0 items."
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
		callback = new MailEwsLoadTodoItemsCallback(m_pCalCallback);

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

    mailews_logger << "sync task process items done."
                   << std::endl;
	return rv;
}

void
SyncTodoTask::CleanupTaskRun() {
	m_pCalCallback->SetSyncState(m_SyncState, false);
	m_pCalCallback->OnDelete(&m_DeletedItemIds);
	m_pCalCallback->SetIsRunning(false, false /*todo*/);
	
	if (HasItems()) {
		if (m_Result == EWS_SUCCESS) {
            mailews_logger << "sync task, start next round sync"
                           << std::endl;
			DoSyncTodo();
		}
	}
}

bool
SyncTodoTask::HasItems() {
	return m_ItemIds.Length() > 0 ||
			m_DeletedItemIds.Length() > 0;
}

void SyncTodoTask::DoSyncTodo() {
	nsresult rv = NS_OK;
	
	mailews_logger << "start next round sync task items"
	               << std::endl;

	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer, &rv));
	NS_ENSURE_SUCCESS_VOID(rv);

	nsCOMPtr<IMailEwsService> ewsService;
	rv = ewsServer->GetService(getter_AddRefs(ewsService));
	NS_ENSURE_SUCCESS_VOID(rv);

	ewsService->SyncTodo(m_pCalCallback, nullptr);
}

NS_IMPL_ISUPPORTS_INHERITED0(MailEwsLoadTodoItemsCallback, MailEwsItemsCallback)

MailEwsLoadTodoItemsCallback::MailEwsLoadTodoItemsCallback(IMailEwsCalendarItemCallback * calCallback)
:m_pCalCallback(calCallback)
{
}

MailEwsLoadTodoItemsCallback::~MailEwsLoadTodoItemsCallback() {
}

typedef struct __load_item_response {
    ews_item ** item;
    int item_count;
} load_item_response;

static
void
From(calITodo * e,
     ews_task_item * item);

NS_IMETHODIMP
MailEwsLoadTodoItemsCallback::LocalOperation(void * response) {
    load_item_response * r = (load_item_response *)response;
    if (!r) return NS_OK;

    ews_item ** item = r->item;
    nsresult rv;

    nsCOMPtr<nsIMutableArray> taskItems =
		    do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    mailews_logger << "create calITodo on main thread:"
                   << r->item_count
                   << std::endl;
    
    for(int i=0;i < r->item_count;i++) {
        if (item[i]->item_type == EWS_Item_Task) {
	        nsCOMPtr<calITodo> e =
			        do_CreateInstance(CAL_TODO_CONTRACTID, &rv);
	        NS_ENSURE_SUCCESS(rv, rv);

            From(e, (ews_task_item*)item[i]);

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

	        taskItems->AppendElement(e, false);
        }
    }

    nsCOMPtr<nsIArray> resultArray(do_QueryInterface(taskItems));

    uint32_t length;
    resultArray->GetLength(&length);
    
    mailews_logger << "create calITodo done on main thread:"
                   << length
                   << std::endl;
    return m_pCalCallback->OnResult(resultArray);
}

NS_IMETHODIMP
MailEwsLoadTodoItemsCallback::RemoteOperation(void ** response) {
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

        mailews_logger << "load task items for:"
                       << item_count
                       << std::endl;
        m_RemoteResult = ews_get_items(session,
                                       item_ids,
                                       item_count,
                                       EWS_GetItems_Flags_AllProperties
                                       | EWS_GetItems_Flags_TaskItem,
                                       &items,
                                       &m_RemoteMsg);

        *response = NULL;
        if (m_RemoteResult == EWS_SUCCESS && items) {
            load_item_response * r;

            *response = r = (load_item_response*)malloc(sizeof(load_item_response));
            r->item = items;
            r->item_count = item_count;
            mailews_logger << "load task items done:"
                           << item_count
                           << std::endl;
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
MailEwsLoadTodoItemsCallback::FreeRemoteResponse(void *response) {
    if (response) {
        load_item_response * r = (load_item_response *)response;
        ews_free_items(r->item, r->item_count);
        free(r);
    }
	return NS_OK;
}

static
int16_t
From(int day_of_week_index,
     int days_of_week) {
	if (day_of_week_index == EWS_DayOfWeekIndex_Last) {
		return -8 - days_of_week - 1;
	} else {
		return 8 * (day_of_week_index + 1) + days_of_week + 1;
	}
}

static
bool
From(calIRecurrenceRule * rule,
     ews_recurrence_pattern * p) {
	int16_t v = From(p->day_of_week_index,
	                 p->days_of_week);

	switch(p->pattern_type) {
	case EWS_Recurrence_RelativeYearly : {
		rule->SetType(NS_LITERAL_CSTRING("YEARLY"));

		rule->SetComponent(NS_LITERAL_CSTRING("BYDAY"),
		                   1,
		                   &v);

		v = (int16_t)p->month + 1;
		rule->SetComponent(NS_LITERAL_CSTRING("BYMONTH"),
		                   1,
		                   &v);
	}
		                   
		break;
	case EWS_Recurrence_AbsoluteYearly : {
		rule->SetType(NS_LITERAL_CSTRING("YEARLY"));
		v = (int16_t)p->month + 1;
		rule->SetComponent(NS_LITERAL_CSTRING("BYMONTH"),
		                   1,
		                   &v);
		v = (int16_t)p->day_of_month;
		rule->SetComponent(NS_LITERAL_CSTRING("BYMONTHDAY"),
		                   1,
		                   &v);
	}
		break;
	case EWS_Recurrence_RelativeMonthly : {
		rule->SetType(NS_LITERAL_CSTRING("MONTHLY"));

		rule->SetInterval(p->interval);
		
		rule->SetComponent(NS_LITERAL_CSTRING("BYDAY"),
		                   1,
		                   &v);
	}
		break;
	case EWS_Recurrence_AbsoluteMonthly : {
		rule->SetType(NS_LITERAL_CSTRING("MONTHLY"));

		rule->SetInterval(p->interval);
		v = p->day_of_month;
		rule->SetComponent(NS_LITERAL_CSTRING("BYMONTHDAY"),
		                   1,
		                   &v);
	}
		break;
	case EWS_Recurrence_Weekly : {
		rule->SetType(NS_LITERAL_CSTRING("WEEKLY"));
		rule->SetInterval(p->interval);
		v = p->days_of_week;
		rule->SetComponent(NS_LITERAL_CSTRING("BYDAY"),
		                   1,
		                   &v);
	}
		break;
	case EWS_Recurrence_Daily :
		rule->SetType(NS_LITERAL_CSTRING("DAILY"));
		rule->SetInterval(p->interval);
		break;
	default:
		return false;
	}

	return true;
}

static
bool
From(calIRecurrenceRule * l,
     ews_recurrence_range * r) {

	switch(r->range_type) {
	case EWSRecurrenceRangeType_NoEnd:
		break;
	case EWSRecurrenceRangeType_EndDate: {
		date::Rfc3339 rfc3339;
		time_t end = rfc3339.parse(r->end_date);
	
		nsCOMPtr<calIDateTime> dt =
				do_CreateInstance(CAL_DATETIME_CONTRACTID);
		dt->SetNativeTime(end * UNIX_TIME_TO_PRTIME);

		l->SetUntilDate(dt);
	}
		break;
	case EWSRecurrenceRangeType_Numbered:
		l->SetCount(r->number_of_occurrences);
		break;
	default:
		return false;
	}

	return true;
}

static
void
From(calIRecurrenceInfo * info,
     ews_recurrence * r) {
	nsCOMPtr<calIRecurrenceRule> rule(do_CreateInstance(CAL_RECURRENCERULE_CONTRACTID));

	if (!From(rule, &r->pattern)) {
		return;
	}

	if (!From(rule, &r->range)) {
		return;
	}

	nsCOMPtr<calIRecurrenceItem> item(do_QueryInterface(rule));
	
	info->AppendRecurrenceItem(item);
}

static
void
From(calITodo * e,
     ews_task_item * item) {
    nsresult rv = NS_OK;

    e->SetId(nsCString(item->item.item_id));

    e->SetTitle(nsCString(item->item.subject));
    SetPropertyString(e,
                      NS_LITERAL_STRING("DESCRIPTION"),
                      nsCString(item->item.body));

    bool isCompleted = item->is_complete ||
            item->status == EWS__TaskStatusType__Completed ||
            (int32_t)item->percent_complete == 100;
    
    e->SetIsCompleted(isCompleted);
    
    if (item->start_date > 0) {
        nsCOMPtr<calIDateTime> dt =
                do_CreateInstance(CAL_DATETIME_CONTRACTID, &rv);
        rv = dt->SetNativeTime(item->start_date * UNIX_TIME_TO_PRTIME);
        e->SetEntryDate(dt);
    }

    if (item->due_date > 0) {
        nsCOMPtr<calIDateTime> dt =
                do_CreateInstance(CAL_DATETIME_CONTRACTID, &rv);
        rv = dt->SetNativeTime(item->due_date * UNIX_TIME_TO_PRTIME);
        e->SetDueDate(dt);
    }
    
    if (item->complete_date > 0) {
        nsCOMPtr<calIDateTime> dt =
                do_CreateInstance(CAL_DATETIME_CONTRACTID, &rv);
        rv = dt->SetNativeTime(item->complete_date * UNIX_TIME_TO_PRTIME);
        e->SetCompletedDate(dt);
    }

    if (isCompleted) {
        e->SetStatus(NS_LITERAL_CSTRING("COMPLETED"));
    } else {
	    switch(item->status) {
	    default:
	    case EWS__TaskStatusType__NotStarted :
		    e->SetStatus(NS_LITERAL_CSTRING("NONE"));
		    break;
	    case EWS__TaskStatusType__InProgress :
		    e->SetStatus(NS_LITERAL_CSTRING("IN-PROCESS"));
		    break;
	    case EWS__TaskStatusType__Completed :
		    e->SetStatus(NS_LITERAL_CSTRING("COMPLETED"));
		    break;
	    case EWS__TaskStatusType__WaitingOnOthers :
		    e->SetStatus(NS_LITERAL_CSTRING("NEEDS-ACTION"));
		    break;
	    case EWS__TaskStatusType__Deferred :
		    e->SetStatus(NS_LITERAL_CSTRING("CANCELLED"));
		    break;
	    }
    }

    if (isCompleted) {
        e->SetPercentComplete((int16_t)100);
    } else {        
        e->SetPercentComplete((int16_t)(item->percent_complete));
    }

    if (item->recurrence) {
	    nsCOMPtr<calIRecurrenceInfo> rInfo;

	    e->GetRecurrenceInfo(getter_AddRefs(rInfo));

        if (!rInfo) {
            rInfo = do_CreateInstance(CAL_RECURRENCEINFO_CONTRACTID);
            rInfo->SetItem(e);
            e->SetRecurrenceInfo(rInfo);
        }

	    From(rInfo, item->recurrence);
    }
}

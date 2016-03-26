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

#include "calIItemBase.h"
#include "calICalendar.h"
#include "calIOperation.h"
#include "calITodo.h"
#include "calIDateTime.h"
#include "calIAttendee.h"
#include "calIRecurrenceInfo.h"
#include "calIRecurrenceItem.h"
#include "calIRecurrenceDate.h"
#include "calITimezoneProvider.h"
#include "calITimezone.h"

#include "nsMsgUtils.h"

#include "plbase64.h"

#include "libews.h"

#include "MailEwsModifyTodoTask.h"
#include "MailEwsErrorInfo.h"
#include "MailEwsItemsCallback.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"
#include "IMailEwsItemOp.h"
#include "IMailEwsMsgFolder.h"

#include "MailEwsLog.h"
#include "MailEwsCommon.h"

#ifdef Compare
#undef Compare
#endif

extern
void
FromCalTodoToTodo(calITodo * todo, ews_task_item * item);

extern
nsresult
GetOwnerStatus(calITodo * todo, const nsCString & ownerEmail, nsCString & status);

extern
nsresult
GetOwnerEmail(nsIMsgIncomingServer * server, nsCString & ownerEmail);

extern
nsresult
GetPropertyString(calIItemBase * todo, const nsAString & name, nsCString & sValue);

extern
nsresult
SetPropertyString(calIItemBase * todo,
                  const nsAString & name,
                  const nsCString & sValue);

extern
nsresult
GetOccurrenceInstanceIndex(calIRecurrenceInfo * rInfo,
                           calIDateTime * todoStart,
                           calIDateTime * occurrenceId,
                           int & instanceIndex);

extern
bool
IsPropertyChanges(calIItemBase * oldTodo,
                  calIItemBase * newTodo,
                  const nsString & name);
extern
bool
IsAttendeeChanges(calIItemBase * oldTodo,
                  calIItemBase * newTodo,
                  bool required);
extern
bool
IsSubjectChanges(calIItemBase * oldTodo,
                 calIItemBase * newTodo);


ModifyTodoTask::ModifyTodoTask(calITodo * oldTodo,
                               calITodo * newTodo,
                               IMailEwsCalendarItemCallback * callback,
                               nsIMsgIncomingServer * pIncomingServer,
                               nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
	, m_OldTodo(oldTodo)
	, m_NewTodo(newTodo)
    , m_CalendarCallback(callback)
	, m_pIncomingServer(pIncomingServer)
{
}

ModifyTodoTask::~ModifyTodoTask() {
}

extern
void
UpdateSessionTimezone(calITodo * todo,
                      ews_session * session);

NS_IMETHODIMP
ModifyTodoTask::Run() {
	char * err_msg = NULL;
	
	// This method is supposed to run on the main
	if(!NS_IsMainThread()) {
		NS_DispatchToMainThread(this);
		return NS_OK;
	}
	
	if (EWS_FAIL == NotifyBegin(this)) {
		return NS_OK;
	}

	ews_session * session = NULL;
	nsresult rv = NS_OK;
	bool hasItemId = false;
	bool hasChangeKey = false;

	if (m_OldTodo) {
		rv = m_OldTodo->HasProperty(NS_LITERAL_STRING("X-ITEM-ID"), &hasItemId);
		rv = m_OldTodo->HasProperty(NS_LITERAL_STRING("X-CHANGE-KEY"), &hasChangeKey);
	}

	if (!hasItemId || !hasChangeKey) {
		rv = m_NewTodo->HasProperty(NS_LITERAL_STRING("X-ITEM-ID"), &hasItemId);
		rv = m_NewTodo->HasProperty(NS_LITERAL_STRING("X-CHANGE-KEY"), &hasChangeKey);
	}

	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer, &rv));
	NS_ENSURE_SUCCESS(rv, rv);
    
	nsCOMPtr<IMailEwsService> ewsService;
	ewsServer->GetService(getter_AddRefs(ewsService));
	NS_ENSURE_SUCCESS(rv, rv);

	nsresult rv1 = ewsService->GetNewSession(&session);
	int ret = EWS_FAIL;
	nsCString newChangeKey("");
	nsCString newItemId("");
	
	if (NS_SUCCEEDED(rv1)
	    && session) {
		UpdateSessionTimezone(m_NewTodo,
		                      session);

		if (hasItemId && hasChangeKey) {
			if (NS_SUCCEEDED(rv)) {
				rv = CheckRecurrenceInfo(session,
				                         ret,
				                         &err_msg);
				NotifyError(this, ret, err_msg);
			}
		} else {
			rv = NS_OK;
		}

		if (NS_SUCCEEDED(rv)) {
            bool do_sync = false;
            
			rv = UpdateTodo(session,
			                newChangeKey,
                            newItemId,
			                ret,
			                &err_msg,
                            do_sync);
			NotifyError(this, ret, err_msg);

            if (ret == EWS_SUCCESS && do_sync) {
                NS_ASSERTION(m_CalendarCallback,
                             "Calendar callback is not valid");
                
                if (m_CalendarCallback) {
                    ewsService->SyncTodo(m_CalendarCallback,
                                         nullptr);
                } 
            }
		}

		if (err_msg) free(err_msg);

	} else {
		NotifyError(this, session ? EWS_FAIL : 401, err_msg);
	}

	ewsService->ReleaseSession(session);

	nsCOMPtr<calITodo> newTodo;
	if (ret == EWS_SUCCESS) {
		nsCOMPtr<calIItemBase> itemBase;
		rv = m_NewTodo->Clone(getter_AddRefs(itemBase));
		NS_FAILED_WARN(rv);

		if (itemBase) {
			newTodo = do_QueryInterface(itemBase);
		}

		if (newTodo && !newChangeKey.IsEmpty()) {
			NS_FAILED_WARN(SetPropertyString(newTodo,
			                                 NS_LITERAL_STRING("X-CHANGE-KEY"),
			                                 newChangeKey));
		}
		if (newTodo && !newItemId.IsEmpty()) {
			NS_FAILED_WARN(SetPropertyString(newTodo,
			                                 NS_LITERAL_STRING("X-ITEM-ID"),
			                                 newItemId));
		}
	}
	NotifyEnd(this, ret, newTodo);

	return NS_OK;
}

NS_IMETHODIMP
ModifyTodoTask::CheckRecurrenceInfo(ews_session * session,
                                    int & ret,
                                    char ** err_msg) {
	nsresult rv = NS_OK;
	nsCOMPtr<calIRecurrenceInfo> oldRecurrenceInfo;
	nsCOMPtr<calIRecurrenceInfo> newRecurrenceInfo;
	nsCOMPtr<calIDateTime> start;
	nsCString itemId;

	ret = EWS_SUCCESS;
	*err_msg = NULL;

	if (!m_OldTodo || !m_NewTodo) {
		//no recurrence info change
		ret = EWS_SUCCESS;
		return NS_OK;
	}

	rv = m_OldTodo->GetRecurrenceInfo(getter_AddRefs(oldRecurrenceInfo));
	NS_ENSURE_SUCCESS(rv, rv);
	rv = m_NewTodo->GetRecurrenceInfo(getter_AddRefs(newRecurrenceInfo));
	NS_ENSURE_SUCCESS(rv, rv);
	rv = GetPropertyString(m_NewTodo, NS_LITERAL_STRING("X-ITEM-ID"), itemId);
	NS_ENSURE_SUCCESS(rv, rv);
	rv = m_NewTodo->GetEntryDate(getter_AddRefs(start));
	NS_ENSURE_SUCCESS(rv, rv);

	calIRecurrenceItem ** newItems;
	uint32_t new_count = 0;
	calIRecurrenceItem ** oldItems;
	uint32_t old_count = 0;

	if (newRecurrenceInfo) {
		rv = newRecurrenceInfo->GetRecurrenceItems(&new_count,
		                                           &newItems);
		NS_ENSURE_SUCCESS(rv, rv);
	}

	if (oldRecurrenceInfo) {
		rv = oldRecurrenceInfo->GetRecurrenceItems(&old_count,
		                                           &oldItems);
		NS_ENSURE_SUCCESS(rv, rv);
	}

	if (old_count >= new_count) {
		NS_ASSERTION((old_count == new_count),
		             "restore of delete meeting is not supported");

		return NS_OK;
	}

     
	for(uint32_t i = old_count;i < new_count; i++) {
		nsCOMPtr<calIRecurrenceDate> d(do_QueryInterface(newItems[i], &rv));

		if (!d)
			continue;

		nsCOMPtr<calIDateTime> dt;
		rv = d->GetDate(getter_AddRefs(dt));
		NS_ASSERTION(dt, "DateTime not found in recurrence item");

		if (!dt)
			continue;

		int instanceIndex = 0;
		rv = GetOccurrenceInstanceIndex(newRecurrenceInfo,
		                                start,
		                                dt,
		                                instanceIndex);
		NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to get the instance index");
        
		if (NS_FAILED(rv))
			continue;

		mailews_logger << "delete todo:"
		               << itemId.get()
		               << "occurrence idex:"
		               << instanceIndex
		               << std::endl;
		ret = ews_calendar_delete_occurrence(session,
		                                     itemId.get(),
		                                     instanceIndex,
		                                     err_msg);
		if (ret != EWS_SUCCESS) {
			mailews_logger << "delete todo occurrence fail:"
			               << ret
			               << (*err_msg)
			               << std::endl;
		}
	}
    
	return NS_OK;
}

static
bool
IsDateTimeChanges(calITodo * oldEvent,
                  calITodo * newEvent,
                  int start) {
	nsresult rv, rv1;
	nsCOMPtr<calIDateTime> oldV;
	nsCOMPtr<calIDateTime> newV;
	int32_t ret = 0;
    
	if (start == 1) {
		rv = oldEvent->GetEntryDate(getter_AddRefs(oldV));
		rv1 = newEvent->GetEntryDate(getter_AddRefs(newV));
	} else if (start == 0) {
		rv = oldEvent->GetDueDate(getter_AddRefs(oldV));
		rv1 = newEvent->GetDueDate(getter_AddRefs(newV));
	} else if (start == 2) {
		rv = oldEvent->GetCompletedDate(getter_AddRefs(oldV));
		rv1 = newEvent->GetCompletedDate(getter_AddRefs(newV));
	}
    
	if (!oldV && !newV)
		return true;
	if (oldV && !newV)
		return true;
	if (!oldV && newV)
		return true;

	return !((rv == rv1)
	         && oldV
	         && NS_SUCCEEDED(oldV->Compare(newV, &ret))
	         && !ret);
}

static
bool
IsStatusChange(calITodo * oldTodo,
               calITodo * newTodo) {
	nsCString oldStatus;
	nsCString newStatus;

	oldTodo->GetStatus(oldStatus);
	newTodo->GetStatus(newStatus);

	return !oldStatus.Equals(newStatus);
}

static
bool
IsPercentChange(calITodo * oldTodo,
                calITodo * newTodo) {
	int16_t oldPercent;
	int16_t newPercent;

	oldTodo->GetPercentComplete(&oldPercent);
	newTodo->GetPercentComplete(&newPercent);

	return oldPercent != newPercent;
}

static
bool
IsCompleteChange(calITodo * oldTodo,
                 calITodo * newTodo) {
	bool oldIs;
	bool newIs;

	oldTodo->GetIsCompleted(&oldIs);
	newTodo->GetIsCompleted(&newIs);

	return oldIs != newIs;
}

NS_IMETHODIMP
ModifyTodoTask::UpdateTodo(ews_session * session,
                           nsCString & changeKey,
                           nsCString & itemId,
                           int & ret,
                           char ** err_msg,
                           bool & do_sync) {
	if (!m_OldTodo || !m_NewTodo) {
		return NS_OK;
	}

	uint32_t flags = 0;

    do_sync = false;

	//1.Description
	if (IsPropertyChanges(m_OldTodo,
	                      m_NewTodo,
	                      NS_LITERAL_STRING("DESCRIPTION"))) {
		flags |= EWS_UpdateCalendar_Flags_Body;
	}
    
	//2.Entry
	if (IsDateTimeChanges(m_OldTodo, m_NewTodo, 1)) {
		flags |= EWS_UpdateCalendar_Flags_Start;
	}
	//3.Due
	if (IsDateTimeChanges(m_OldTodo, m_NewTodo, 0)) {
		flags |= EWS_UpdateCalendar_Flags_End;
	}
	//3.Complete
	if (IsDateTimeChanges(m_OldTodo, m_NewTodo, 2)) {
		flags |= EWS_UpdateCalendar_Flags_Complete;
	}

	//7.Subject
	if (IsSubjectChanges(m_OldTodo, m_NewTodo)) {
		flags |= EWS_UpdateCalendar_Flags_Subject;
	}

	//status, percent, iscomplete
	if (IsStatusChange(m_OldTodo, m_NewTodo)) {
		flags |= EWS_UpdateCalendar_Flags_Status;
	}
	
	if (IsPercentChange(m_OldTodo, m_NewTodo)) {
		flags |= EWS_UpdateCalendar_Flags_Percent;
	}

	if (IsCompleteChange(m_OldTodo, m_NewTodo)) {
		flags |= EWS_UpdateCalendar_Flags_Is_Complete;
	}
	
	int instanceIndex = 0;
    
	if (!flags) {
		//No supported change found
		mailews_logger << "no change found"
		               << std::endl;
		ret = EWS_SUCCESS;
		return NS_OK;
	}

	bool newExp = false;
	nsCString newExpItemId;
	nsCString newExpChangeKey;
	nsCString newExpTzid;
    
	nsCOMPtr<calIDateTime> recurrenceId;
	m_NewTodo->GetRecurrenceId(getter_AddRefs(recurrenceId));
	nsCOMPtr<calIItemBase> parentItem;
	m_NewTodo->GetParentItem(getter_AddRefs(parentItem));
    nsCOMPtr<calIRecurrenceInfo> recurrenceInfo;
    m_NewTodo->GetRecurrenceInfo(getter_AddRefs(recurrenceInfo));

	if (recurrenceId && parentItem) {
		nsCOMPtr<calITodo> parentTodo(do_QueryInterface(parentItem));
		nsCOMPtr<calIDateTime> startDT;

		parentItem->GetRecurrenceInfo(getter_AddRefs(recurrenceInfo));
		parentTodo->GetEntryDate(getter_AddRefs(startDT));

		//Get ParentItem TZID
		if (startDT) {
			nsCOMPtr<calITimezone> tz;
			startDT->GetTimezone(getter_AddRefs(tz));

			if (tz) {
				tz->GetTzid(newExpTzid);
			}
		}

		if (recurrenceInfo) {
			nsresult rv = GetOccurrenceInstanceIndex(recurrenceInfo,
			                                         startDT,
			                                         recurrenceId,
			                                         instanceIndex);
			NS_FAILED_WARN(rv);

			if (NS_SUCCEEDED(rv)) {
				newExp = true;
				GetPropertyString(parentItem,
				                  NS_LITERAL_STRING("X-ITEM-ID"),
				                  newExpItemId);
				GetPropertyString(parentItem,
				                  NS_LITERAL_STRING("X-CHANGE-KEY"),
				                  newExpChangeKey);
			}
		}
	}
    
	mailews_logger << "changes found:"
	               << flags
	               << ", new exp:"
	               << newExp
	               << ", itemId:"
	               << newExpItemId.get()
	               << ", changeKey:"
	               << newExpChangeKey.get()
	               << ", instanceIndex:"
	               << instanceIndex
	               << ", tzid:"
	               << newExpTzid.get()
	               << std::endl;
	
	ews_task_item * item = (ews_task_item *)malloc(sizeof(ews_task_item));
	memset(item, 0, sizeof(ews_task_item));
			
	item->item.item_type = EWS_Item_Task;

	FromCalTodoToTodo(m_NewTodo,
	                  item);

	if (newExp) {
		if (item->item.item_id) free(item->item.item_id);
		if (item->item.change_key) free(item->item.change_key);

		item->item.item_id = strdup(newExpItemId.get());
		item->item.change_key = strdup(newExpChangeKey.get());
	}

	// if (newExp) {
	// 	ret = ews_calendar_update_task_occurrence(session,
	// 	                                          item,
	// 	                                          instanceIndex,
	// 	                                          flags,
	// 	                                          err_msg);
	// } else {
		ret = ews_calendar_update_task(session,
		                               item,
		                               flags,
		                               err_msg);
	// }
    
	//update change key
	if (ret == EWS_SUCCESS) {
        nsCString uid;
        m_NewTodo->GetId(uid);
        
        if (recurrenceId || recurrenceInfo) {
            /*
              if an UpdateItem request sets the Completed value of a recurring task to true, the UpdateItemResponse will include a new Id and ChangeKey that represent a newly created one-off task. The Id that was included in the request is still valid and the recurring task that is represented by that Id has been updated to represent the next occurrence. The ChangeKey that was included in the request is no longer valid because the recurring task has been updated. */
            do_sync = true;

            mailews_logger << "update a recurrence todo:"
                           << uid.get()
                           << ", do sync task"
                           << std::endl;
        } else {
            changeKey.AssignLiteral(item->item.change_key);
            itemId.AssignLiteral(item->item.item_id);

            NS_ASSERTION(itemId.Equals(uid),
                         "non recurrence task update but item id changes");
            
            mailews_logger <<"update change key="
                           << changeKey.get()
                           << "item id="
                           << itemId.get()
                           << " for todo item uid:"
                           << uid.get()
                           << std::endl;
        }
	}
    
	ews_free_item(&item->item);
	return NS_OK;
}


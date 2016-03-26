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

#include "calICalendar.h"
#include "calIOperation.h"
#include "calIItemBase.h"
#include "calIEvent.h"
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

#include "MailEwsModifyCalendarItemTask.h"
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
FromCalEventToCalendarItem(calIEvent * event, ews_calendar_item * item);

extern
nsresult
GetOwnerStatus(calIEvent * event, const nsCString & ownerEmail, nsCString & status);

extern
nsresult
GetOwnerEmail(nsIMsgIncomingServer * server, nsCString & ownerEmail);

extern
nsresult
GetPropertyString(calIItemBase * event, const nsAString & name, nsCString & sValue);

extern
nsresult
SetPropertyString(calIItemBase * event,
                  const nsAString & name,
                  const nsCString & sValue);

ModifyCalendarItemTask::ModifyCalendarItemTask(calIEvent * oldEvent,
                                               calIEvent * newEvent,
                                               nsIMsgIncomingServer * pIncomingServer,
                                               nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
	, m_OldEvent(oldEvent)
	, m_NewEvent(newEvent)
	, m_pIncomingServer(pIncomingServer)
{
}

ModifyCalendarItemTask::~ModifyCalendarItemTask() {
}

NS_IMETHODIMP
ModifyCalendarItemTask::Run() {
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

    if (m_OldEvent) {
        nsCString mm;
        m_OldEvent->GetIcalString(mm);

        printf("old event:\n%s\n", mm.get());
    }
    
    if (m_NewEvent) {
        nsCString mm;
        m_NewEvent->GetIcalString(mm);

        printf("new event:\n%s\n", mm.get());
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
    bool occurrence = false;
    int instanceIndex = 0;

    GetItemIdAndChangeKey(newItemId,
                          newChangeKey,
                          occurrence,
                          instanceIndex);
	NS_ASSERTION(!newItemId.IsEmpty() && !newChangeKey.IsEmpty(),
                 "Item id and change key not found");

    mailews_logger << "modify calendar item,"
                   << "item id:" << newItemId.get()
                   << ",change key:" << newChangeKey.get()
                   << ",occurrence:" << occurrence
                   << ",instance index:" << instanceIndex
                   << std::endl;

    if (newItemId.IsEmpty() || newChangeKey.IsEmpty()) {
        ret = EWS_SUCCESS;
    }
    
	if (NS_SUCCEEDED(rv1)
	    && session
        && !newItemId.IsEmpty()
        && !newChangeKey.IsEmpty()) {
        bool declined = false;
        
        rv = ResponseCalendarEvent(session,
                                   ret,
                                   &err_msg,
                                   newItemId,
                                   newChangeKey,
                                   occurrence,
                                   instanceIndex,
                                   declined);
        NotifyError(this, ret, err_msg);

        if (NS_SUCCEEDED(rv) && !declined && !occurrence) {
            rv = CheckRecurrenceInfo(session,
                                     ret,
                                     &err_msg,
                                     newItemId,
                                     newChangeKey,
                                     occurrence,
                                     instanceIndex);
            NotifyError(this, ret, err_msg);
        }

		if (NS_SUCCEEDED(rv) && !declined) {
			rv = UpdateCalendarEvent(session,
                                     ret,
                                     &err_msg,
                                     newItemId,
                                     newChangeKey,
                                     occurrence,
                                     instanceIndex);
			NotifyError(this, ret, err_msg);
		}

		if (err_msg) free(err_msg);

	} else {
		NotifyError(this, session ? EWS_FAIL : 401, err_msg);
	}

    ewsService->ReleaseSession(session);

    nsCOMPtr<calIEvent> newEvent;
    if (ret == EWS_SUCCESS) {
	    nsCOMPtr<calIItemBase> itemBase;
	    rv = m_NewEvent->Clone(getter_AddRefs(itemBase));
	    NS_FAILED_WARN(rv);

	    if (itemBase) {
		    newEvent = do_QueryInterface(itemBase);
	    }

	    if (newEvent && !newChangeKey.IsEmpty()) {
		    NS_FAILED_WARN(SetPropertyString(newEvent,
		                                     NS_LITERAL_STRING("X-CHANGE-KEY"),
		                                     newChangeKey));
	    }
	    if (newEvent && !newItemId.IsEmpty()) {
		    NS_FAILED_WARN(SetPropertyString(newEvent,
		                                     NS_LITERAL_STRING("X-ITEM-ID"),
		                                     newItemId));
	    }
    }
    NotifyEnd(this, ret, newEvent);

	return NS_OK;
}

NS_IMETHODIMP
ModifyCalendarItemTask::ResponseCalendarEvent(ews_session * session,
                                              int & ret,
                                              char ** err_msg,
                                              nsCString & itemId,
                                              nsCString & changeKey,
                                              bool occurrence,
                                              int instance_index,
                                              bool & declined) {
	nsCString ownerEmail;
	nsCString oldStatus;
	nsCString newStatus;
    nsCString status;

    declined = false;
    
	nsresult rv = GetOwnerEmail(m_pIncomingServer, ownerEmail);
	NS_ENSURE_SUCCESS(rv, rv);

	if (m_OldEvent) {
		GetOwnerStatus(m_OldEvent, ownerEmail, oldStatus);
	}
	
	if (m_NewEvent) {
		GetOwnerStatus(m_NewEvent, ownerEmail, newStatus);
	}

	if (newStatus.Equals(oldStatus)) {
		ret = EWS_SUCCESS;
		return NS_OK;
	}

	ews_calendar_item * item = (ews_calendar_item *)malloc(sizeof(ews_calendar_item));
	memset(item, 0, sizeof(ews_calendar_item));
			
	item->item.item_type = EWS_Item_Calendar;
	item->item.item_id = strdup(itemId.get());
	item->item.change_key = strdup(changeKey.get());

    bool do_work = true;
    
	if (newStatus.Equals("ACCEPTED", nsCaseInsensitiveCStringComparator())) {
        if (!occurrence) {
            ret = ews_calendar_accept_event(session,
                                            item,
                                            err_msg);
        } else {
            ret = EWS_SUCCESS;
            do_work = false;
        }
	} else if (newStatus.Equals("TENTATIVE", nsCaseInsensitiveCStringComparator())) {
        if (!occurrence) {
            ret = ews_calendar_tentatively_accept_event(session,
                                                        item,
                                                        err_msg);
        }
        else {
            ret = EWS_SUCCESS;
            do_work = false;
        }
    } else if (newStatus.Equals("DECLINED", nsCaseInsensitiveCStringComparator())) {
        if (occurrence) {
            ret = ews_calendar_delete_occurrence(session,
                                                 itemId.get(),
                                                 instance_index,
                                                 err_msg);
            declined = (ret == EWS_SUCCESS);
        } else {
            ret = ews_calendar_decline_event(session,
                                             item,
                                             err_msg);
        }
	} else {
        do_work = false;
        ret = EWS_SUCCESS;
    }

    if (ret == EWS_SUCCESS && do_work) {
        itemId.AssignLiteral(item->item.item_id);
        changeKey.AssignLiteral(item->item.change_key);
    }
    
	ews_free_item(&item->item);

	return NS_OK;
}

extern
nsresult
GetOccurrenceInstanceIndex(calIRecurrenceInfo * rInfo,
                           calIDateTime * eventStart,
                           calIDateTime * occurrenceId,
                           int & instanceIndex);

NS_IMETHODIMP
ModifyCalendarItemTask::CheckRecurrenceInfo(ews_session * session,
                                            int & ret,
                                            char ** err_msg,
                                            nsCString & itemId,
                                            nsCString & changeKey,
                                            bool occurrence,
                                            int instanceIndex) {
    nsresult rv = NS_OK;
    nsCOMPtr<calIRecurrenceInfo> oldRecurrenceInfo;
    nsCOMPtr<calIRecurrenceInfo> newRecurrenceInfo;
    nsCOMPtr<calIDateTime> start;

    ret = EWS_SUCCESS;
    *err_msg = NULL;

	if (!m_OldEvent || !m_NewEvent || occurrence) {
        //no recurrence info change
        ret = EWS_SUCCESS;
        return NS_OK;
	}

    rv = m_OldEvent->GetRecurrenceInfo(getter_AddRefs(oldRecurrenceInfo));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = m_NewEvent->GetRecurrenceInfo(getter_AddRefs(newRecurrenceInfo));
    NS_ENSURE_SUCCESS(rv, rv);

    if (itemId.IsEmpty()) {
        rv = GetPropertyString(m_NewEvent, NS_LITERAL_STRING("X-ITEM-ID"), itemId);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    rv = m_NewEvent->GetStartDate(getter_AddRefs(start));
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

        mailews_logger << "delete event:"
                       << itemId.get()
                       << "occurrence idex:"
                       << instanceIndex
                       << std::endl;
        ret = ews_calendar_delete_occurrence(session,
                                             itemId.get(),
                                             instanceIndex,
                                             err_msg);
        if (ret != EWS_SUCCESS) {
            mailews_logger << "delete fail:"
                           << ret
                           << (*err_msg)
                           << std::endl;
        }
    }
    
    return NS_OK;
}

nsresult
GetOccurrenceInstanceIndex(calIRecurrenceInfo * rInfo,
                           calIDateTime * eventStart,
                           calIDateTime * occurrenceId,
                           int & instanceIndex) {
    //Combine and Sort occurrences with RecurrenceItem and Exception Items
    std::map<PRTime,PRTime> found_dates;
    nsresult rv = NS_OK;
    calIDateTime ** dates;
    uint32_t count = 0;
    int32_t ret = 0;

    instanceIndex = 0;
    // Exception items will included in occurrence dats
    // //Check exception items
    // rv = rInfo->GetExceptionIds(&count, &dates);
    // NS_ENSURE_SUCCESS(rv, rv);

    // for(uint32_t i = 0; i < count; i++) {
    //     rv = occurrenceId->Compare(dates[i], &ret);
    //     NS_ENSURE_SUCCESS(rv, rv);

    //     PRTime nativeTime;
    //     dates[i]->GetNativeTime(&nativeTime);

    //     if (found_dates.find(nativeTime) != found_dates.end()) {
    //         continue;
    //     }

    //     found_dates[nativeTime] = nativeTime;
        
    //     if (ret > 0)
    //         instanceIndex++;
    // }

    //Check RecurrenceItems
    calIRecurrenceItem ** items;
    uint32_t item_count = 0;

    rv = rInfo->GetRecurrenceItems(&item_count,
                                   &items);
    NS_ENSURE_SUCCESS(rv, rv);

    for(uint32_t i = 0; i < item_count; i++) {
        nsCOMPtr<calIRecurrenceDate> d(do_QueryInterface(items[i], &rv));

        if (!d)
            continue;

        nsCOMPtr<calIDateTime> dt;
        rv = d->GetDate(getter_AddRefs(dt));
        NS_ENSURE_SUCCESS(rv, rv);

        if (!dt)
            return NS_ERROR_FAILURE;
        
        PRTime nativeTime;
        dt->GetNativeTime(&nativeTime);

        if (found_dates.find(nativeTime) != found_dates.end()) {
            continue;
        }
        
        found_dates[nativeTime] = nativeTime;

        rv = occurrenceId->Compare(dt, &ret);
        NS_ENSURE_SUCCESS(rv, rv);

        if (ret > 0)
            instanceIndex++;
    }

    nsCOMPtr<calIDateTime> tmpStart(eventStart);
    count = 0;

    do {
        rv = rInfo->GetOccurrenceDates(tmpStart,
                                       occurrenceId,
                                       1000,
                                       &count,
                                       &dates);

        for(uint32_t i = 0; i < count; i++) {
            rv = occurrenceId->Compare(dates[i], &ret);
            NS_ENSURE_SUCCESS(rv, rv);

            PRTime nativeTime;
            dates[i]->GetNativeTime(&nativeTime);

            if (found_dates.find(nativeTime) != found_dates.end()) {
                continue;
            }
        
            found_dates[nativeTime] = nativeTime;

            if (ret > 0)
                instanceIndex++;
            else if (ret == 0) {
                count = 0;
                break;
            }
        }

        if (count > 0) {
            tmpStart = dates[count - 1];
        }
    } while (count == 1000);

    //Instance index start from 1
    instanceIndex++;
    return NS_OK;
}

bool
IsPropertyChanges(calIItemBase * oldEvent,
                  calIItemBase * newEvent,
                  const nsString & name);

static
bool
IsDateTimeChanges(calIEvent * oldEvent,
                  calIEvent * newEvent,
                  bool start);
bool
IsAttendeeChanges(calIItemBase * oldEvent,
                  calIItemBase * newEvent,
                  bool required);
bool
IsSubjectChanges(calIItemBase * oldEvent,
                 calIItemBase * newEvent);

NS_IMETHODIMP
ModifyCalendarItemTask::UpdateCalendarEvent(ews_session * session,
                                            int & ret,
                                            char ** err_msg,
                                            nsCString & itemId,
                                            nsCString & changeKey,
                                            bool occurrence,
                                            int instanceIndex) {
    if (!m_OldEvent || !m_NewEvent) {
        return NS_OK;
    }

    uint32_t flags = 0;

    //1.Description
    if (IsPropertyChanges(m_OldEvent,
                          m_NewEvent,
                          NS_LITERAL_STRING("DESCRIPTION"))) {
        flags |= EWS_UpdateCalendar_Flags_Body;
    }
    
    //2.Start
    if (IsDateTimeChanges(m_OldEvent, m_NewEvent, true)) {
        flags |= EWS_UpdateCalendar_Flags_Start;
    }
    //3.End
    if (IsDateTimeChanges(m_OldEvent, m_NewEvent, false)) {
        flags |= EWS_UpdateCalendar_Flags_End;
    }
    // //4.Required Attendee
    // if (IsAttendeeChanges(m_OldEvent, m_NewEvent, true)) {
    //     flags |= EWS_UpdateCalendar_Flags_Required_Attendee;
    // }
    // //5.Optional Attendee
    // if (IsAttendeeChanges(m_OldEvent, m_NewEvent, false)) {
    //     flags |= EWS_UpdateCalendar_Flags_Optional_Attendee;
    // }
    //6.Location
    if (IsPropertyChanges(m_OldEvent,
                          m_NewEvent,
                          NS_LITERAL_STRING("LOCATION"))) {
        flags |= EWS_UpdateCalendar_Flags_Location;
    }
    //7.Subject
    if (IsSubjectChanges(m_OldEvent, m_NewEvent)) {
        flags |= EWS_UpdateCalendar_Flags_Subject;
    }

    if (!flags) {
        //No supported change found
        mailews_logger << "no change found"
                       << std::endl;
        ret = EWS_SUCCESS;
        return NS_OK;
    }
    
    mailews_logger << "changes found:"
                   << flags
                   << std::endl;
    
    ews_calendar_item * item = (ews_calendar_item *)malloc(sizeof(ews_calendar_item));
    memset(item, 0, sizeof(ews_calendar_item));
			
    item->item.item_type = EWS_Item_Calendar;

    FromCalEventToCalendarItem(m_NewEvent,
                               item);

    if (!itemId.IsEmpty()) {
        if (item->item.item_id) free(item->item.item_id);
        item->item.item_id = strdup(itemId.get());
    }

    if (!changeKey.IsEmpty()) {
        if (item->item.change_key) free(item->item.change_key);
        item->item.change_key = strdup(changeKey.get());
    }

    if (occurrence) {
        ret = ews_calendar_update_event_occurrence(session,
                                                   item,
                                                   instanceIndex,
                                                   flags,
                                                   err_msg);
    } else {
        ret = ews_calendar_update_event(session,
                                        item,
                                        flags,
                                        err_msg);
    }
    
    //update change key
    if (ret == EWS_SUCCESS) {
	    changeKey.AssignLiteral(item->item.change_key);
        itemId.AssignLiteral(item->item.item_id);
        
        nsCString uid;
        m_NewEvent->GetId(uid);

        mailews_logger <<"update change key="
                       << changeKey.get()
                       << " for item uid:"
                       << uid.get()
                       << std::endl;
    }
    
    ews_free_item(&item->item);
    return NS_OK;
}

bool
IsPropertyChanges(calIItemBase * oldEvent,
                  calIItemBase * newEvent,
                  const nsString & name) {
    nsCString oldV("");
    nsCString newV("");
    
    nsresult rv = GetPropertyString(oldEvent,
                                    name,
                                    oldV);
    nsresult rv1 = GetPropertyString(newEvent,
                                     name,
                                     newV);

    return !((rv1 == rv) && (oldV.Equals(newV)));
}

static
bool
IsDateTimeChanges(calIEvent * oldEvent,
                  calIEvent * newEvent,
                  bool start) {
    nsresult rv, rv1;
    nsCOMPtr<calIDateTime> oldV;
    nsCOMPtr<calIDateTime> newV;
    int32_t ret = 0;
    
    if (start) {
        rv = oldEvent->GetStartDate(getter_AddRefs(oldV));
        rv1 = newEvent->GetStartDate(getter_AddRefs(newV));
    } else {
        rv = oldEvent->GetEndDate(getter_AddRefs(oldV));
        rv1 = newEvent->GetEndDate(getter_AddRefs(newV));
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
void
GetAttendeeMap(calIItemBase * event,
               bool required,
               std::map<std::string, std::string> & m) {
	uint32_t count = 0;
	calIAttendee ** attendees = NULL;

	nsresult rv = event->GetAttendees(&count, &attendees);
    
    m.clear();
	
	if (NS_SUCCEEDED(rv) && count > 0) {
		for (uint32_t i = 0; i < count; i++) {
			nsCString role;
			nsCString id;
			nsCString status;
			
			attendees[i]->GetRole(role);
			attendees[i]->GetId(id);
			attendees[i]->GetParticipationStatus(status);
            
			if ((required && role.EqualsLiteral("REQ-PARTICIPANT"))
                || (!required && !role.EqualsLiteral("REQ-PARTICIPANT"))) {
                m[id.get()] = status.get();
            }
        }
    }
}
               
bool
IsAttendeeChanges(calIItemBase * oldEvent,
                  calIItemBase * newEvent,
                  bool required) {
    std::map<std::string, std::string> mOld;
    std::map<std::string, std::string> mNew;

    GetAttendeeMap(oldEvent, required, mOld);
    GetAttendeeMap(newEvent, required, mNew);

    if (mOld.size() != mNew.size())
        return true;

    std::map<std::string, std::string>::iterator itNew =
            mNew.begin();

    while(itNew != mNew.end()) {
        std::map<std::string, std::string>::iterator itOld =
                mOld.find(itNew->first);

        if (itOld == mOld.end())
            return true;

        if (itNew->second != itOld->second)
            return true;

        itNew++;
    }

    return false;
}

bool
IsSubjectChanges(calIItemBase * oldEvent,
                 calIItemBase * newEvent) {
    nsCString oldV("");
    nsCString newV("");
    nsresult rv, rv1;

    rv = oldEvent->GetTitle(oldV);
    rv1 = newEvent->GetTitle(newV);

    return !((rv1 == rv) && (oldV.Equals(newV)));
}

void
ModifyCalendarItemTask::GetItemIdAndChangeKey(nsCString & itemId,
                                              nsCString & changeKey,
                                              bool & occurrence,
                                              int & instanceIndex) {
    itemId.Assign("");
    changeKey.Assign("");
    occurrence = false;
	bool hasItemId = false;
	bool hasChangeKey = false;

    if (m_NewEvent) {
		m_NewEvent->HasProperty(NS_LITERAL_STRING("X-ITEM-ID"), &hasItemId);
		m_NewEvent->HasProperty(NS_LITERAL_STRING("X-CHANGE-KEY"), &hasChangeKey);

        if (hasItemId) {
            GetPropertyString(m_NewEvent,
                              NS_LITERAL_STRING("X-ITEM-ID"),
                              itemId);
        }

        if (hasChangeKey) {
            GetPropertyString(m_NewEvent,
                              NS_LITERAL_STRING("X-CHANGE-KEY"),
                              changeKey);
        }
    }
    
	if (m_OldEvent) {
		m_OldEvent->HasProperty(NS_LITERAL_STRING("X-ITEM-ID"), &hasItemId);
		m_OldEvent->HasProperty(NS_LITERAL_STRING("X-CHANGE-KEY"), &hasChangeKey);

        if (hasItemId) {
            GetPropertyString(m_OldEvent,
                              NS_LITERAL_STRING("X-ITEM-ID"),
                              itemId);
        }

        if (hasChangeKey) {
            GetPropertyString(m_NewEvent,
                              NS_LITERAL_STRING("X-CHANGE-KEY"),
                              changeKey);
        }
    }

    nsCOMPtr<calIDateTime> recurrenceId;
    m_NewEvent->GetRecurrenceId(getter_AddRefs(recurrenceId));
    nsCOMPtr<calIItemBase> parentItem;
    m_NewEvent->GetParentItem(getter_AddRefs(parentItem));

    if (recurrenceId && parentItem) {
        nsCOMPtr<calIRecurrenceInfo> recurrenceInfo;
        nsCOMPtr<calIEvent> parentEvent(do_QueryInterface(parentItem));
        nsCOMPtr<calIDateTime> startDT;

        parentItem->GetRecurrenceInfo(getter_AddRefs(recurrenceInfo));
        parentEvent->GetStartDate(getter_AddRefs(startDT));

        if (recurrenceInfo) {
            nsresult rv = GetOccurrenceInstanceIndex(recurrenceInfo,
                                                     startDT,
                                                     recurrenceId,
                                                     instanceIndex);
            NS_FAILED_WARN(rv);

            if (NS_SUCCEEDED(rv)) {
                occurrence = true;
                GetPropertyString(parentEvent,
                                  NS_LITERAL_STRING("X-ITEM-ID"),
                                  itemId);
                GetPropertyString(parentEvent,
                                  NS_LITERAL_STRING("X-CHANGE-KEY"),
                                  changeKey);
            }
        }
    }
}


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

#include "calIEvent.h"
#include "calIDateTime.h"
#include "calIAttendee.h"
#include "calIICSService.h"
#include "calBaseCID.h"
#include "calITimezone.h"

#include "nsMsgUtils.h"

#include "plbase64.h"

#include "libews.h"

#include "MailEwsAddCalendarItemTask.h"
#include "MailEwsErrorInfo.h"
#include "MailEwsItemsCallback.h"
#include "MailEwsCalendarUtils.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"
#include "IMailEwsItemOp.h"
#include "IMailEwsMsgFolder.h"

#include "MailEwsLog.h"
#include "MailEwsCommon.h"

void
FromCalEventToCalendarItem(calIEvent * event, ews_calendar_item * item);

nsresult
GetOwnerStatus(calIEvent * event, const nsCString & ownerEmail, nsCString & status);

nsresult
GetOwnerEmail(nsIMsgIncomingServer * server, nsCString & ownerEmail);

nsresult
GetPropertyString(calIItemBase * event, const nsAString & name, nsCString & sValue) {
	nsCOMPtr<nsIVariant> v;
    
	nsresult rv = event->GetProperty(name,  getter_AddRefs(v));
	NS_ENSURE_SUCCESS(rv, rv);
	rv = v->GetAsACString(sValue);
	NS_ENSURE_SUCCESS(rv, rv);

	return NS_OK;
}

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

class AddCalendarItemDoneTask : public MailEwsRunnable
{
public:
	AddCalendarItemDoneTask(int result,
	                        calIEvent * event,
	                        nsCString item_id,
	                        nsCString change_key,
                            nsCString uid,
	                        nsISupportsArray * ewsTaskCallbacks,
	                        AddCalendarItemTask * runnable)
		: MailEwsRunnable(ewsTaskCallbacks)
		, m_Runnable(runnable)
		, m_Result(result)
		, m_Event(event)
		, m_ItemId(item_id)
		, m_ChangeKey(change_key)
        , m_Uid(uid)
	{
	}

	NS_IMETHOD Run() {
		nsresult rv = NS_OK;
		if (m_Result == EWS_SUCCESS) {
			//Save item id
            rv = SetPropertyString(m_Event,
                                   NS_LITERAL_STRING("X-ITEM-ID"),
                                   m_ItemId);
			NS_ENSURE_SUCCESS(rv, rv);

			//Save change key
            rv = SetPropertyString(m_Event,
                                   NS_LITERAL_STRING("X-CHANGE-KEY"),
                                   m_ChangeKey);
			NS_ENSURE_SUCCESS(rv, rv);

            nsCString oldUid;
            rv = m_Event->GetId(oldUid);

            if (NS_SUCCEEDED(rv) && oldUid.IsEmpty()) {
                //Updat the uid
                m_Event->SetId(m_Uid);
            }
		}
		NotifyEnd(m_Runnable, m_Result);
		
		return NS_OK;
	}

private:
	nsCOMPtr<nsIRunnable> m_Runnable;
	int m_Result;
	nsCOMPtr<calIEvent> m_Event;
	nsCString m_ItemId;
	nsCString m_ChangeKey;
    nsCString m_Uid;
};

AddCalendarItemTask::AddCalendarItemTask(calIEvent * event,
                                         nsIMsgIncomingServer * pIncomingServer,
                                         nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
	, m_Event(event)
	, m_pIncomingServer(pIncomingServer)
	, m_MimeContent("")
{
}

AddCalendarItemTask::~AddCalendarItemTask() {
}

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
    nsCOMPtr<calIIcalComponent> event;
    rv = component->GetFirstSubcomponent(NS_LITERAL_CSTRING("VEVENT"),
                                         getter_AddRefs(event));
    NS_FAILED_WARN(rv);

    if (event) {
        rv = event->GetUid(uid);
        NS_FAILED_WARN(rv);

        if (NS_FAILED(rv) || uid.IsEmpty()) {
            has_uid = false;
            rv = GenerateUid(uid);
            NS_FAILED_WARN(rv);

            if (NS_SUCCEEDED(rv) && !uid.IsEmpty()) {
                event->SetUid(uid);
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

NS_IMETHODIMP AddCalendarItemTask::Run() {
	char * err_msg = NULL;
	
	// This method is supposed to run on the main
	if(!NS_IsMainThread()) {
		NS_DispatchToMainThread(this);
		return NS_OK;
	}
	
	if (EWS_FAIL == NotifyBegin(this)) {
		return NS_OK;
	}

	m_Event->GetIcalString(m_MimeContent);

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

    m_Event->HasProperty(NS_LITERAL_STRING("X-ITEM-ID"), &hasItemId);
    m_Event->HasProperty(NS_LITERAL_STRING("X-CHANGE-KEY"), &hasChangeKey);

	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer, &rv));
	NS_ENSURE_SUCCESS(rv, rv);
    
	nsCOMPtr<IMailEwsService> ewsService;
	ewsServer->GetService(getter_AddRefs(ewsService));
	NS_ENSURE_SUCCESS(rv, rv);

	nsresult rv1 = ewsService->GetNewSession(&session);
	int ret = EWS_FAIL;
    
	if (NS_SUCCEEDED(rv1)
	    && session) {
		if (has_uid && hasItemId && hasChangeKey) {
			//Has UID, accept/decline/tenetive
			nsCString ownerEmail;
			nsCString status;

			nsresult rv = GetOwnerEmail(m_pIncomingServer, ownerEmail);
			NS_ENSURE_SUCCESS(rv, rv);

			rv = GetOwnerStatus(m_Event, ownerEmail, status);
			NS_ENSURE_SUCCESS(rv, rv);

			rv = GetPropertyString(m_Event, NS_LITERAL_STRING("X-ITEM-ID"), itemId);
			NS_ENSURE_SUCCESS(rv, rv);
			rv = GetPropertyString(m_Event, NS_LITERAL_STRING("X-CHANGE-KEY"), changeKey);
			NS_ENSURE_SUCCESS(rv, rv);

			ews_calendar_item * item = (ews_calendar_item *)malloc(sizeof(ews_calendar_item));
			memset(item, 0, sizeof(ews_calendar_item));
			
			item->item.item_type = EWS_Item_Calendar;
            item->item.item_id = strdup(itemId.get());
            item->item.change_key = strdup(changeKey.get());
            
			if (status.Equals("ACCEPTED", nsCaseInsensitiveCStringComparator())) {
                ret = ews_calendar_accept_event(session,
                                               item,
                                               &err_msg);
			} else if (status.Equals("TENTATIVE", nsCaseInsensitiveCStringComparator())) {
                ret = ews_calendar_tentatively_accept_event(session,
                                               item,
                                               &err_msg);
			} else if (status.Equals("DECLINED", nsCaseInsensitiveCStringComparator())) {
                ret = ews_calendar_decline_event(session,
                                               item,
                                               &err_msg);
			}

            ews_free_item(&item->item);
		} else {
			//No UID, create new calendar item
			ews_calendar_item * item = (ews_calendar_item *)malloc(sizeof(ews_calendar_item));
			memset(item, 0, sizeof(ews_calendar_item));
			
			item->item.item_type = EWS_Item_Calendar;

			char * encoded_content = PL_Base64Encode(m_MimeContent.get(),
			                                         m_MimeContent.Length(),
			                                         nullptr);

			item->item.mime_content = encoded_content;
			FromCalEventToCalendarItem(m_Event, item);

			ret = ews_create_item_by_distinguished_id_name(session,
			                                               EWSDistinguishedFolderIdName_calendar,
			                                               &item->item,
			                                               &err_msg);

			if (ret == EWS_SUCCESS) {
				if (item->item.item_id) {
					itemId.AssignLiteral(item->item.item_id);

                    ews_item ** items = NULL;

                    if (err_msg) free(err_msg);

                    //try to find the uid
                    int item_count = 1;
                    const char * item_id = item->item.item_id;
                    
                    ret = ews_get_items(session,
                                        &item_id,
                                        item_count,
                                        EWS_GetItems_Flags_IdOnly
                                        | EWS_GetItems_Flags_CalendarItem,
                                        &items,
                                        &err_msg);

                    if (ret == EWS_SUCCESS && item_count == 1) {
                        ews_calendar_item * c_item =
                                (ews_calendar_item*)items[0];

                        uid.AssignLiteral(c_item->uid);
                    }

                    ews_free_items(items, item_count);
                }
                
				if (item->item.change_key)
					changeKey.AssignLiteral(item->item.change_key);
                
                mailews_logger << "new created calendar item, item id:"
                               << itemId.get()
                               << "uid:" << uid.get()
                               << "change key:" << changeKey.get()
                               << std::endl;
			} else {
                mailews_logger << "Error Mime content:"
                               << m_MimeContent.get()
                               << std::endl
                               << "encoded content:"
                               << encoded_content
                               << std::endl;
            }

			ews_free_item(&item->item);
		}
		
		NotifyError(this, ret, err_msg);

		if (err_msg) free(err_msg);

	} else {
		NotifyError(this, session ? EWS_FAIL : 401, err_msg);
	}

    ewsService->ReleaseSession(session);

    if (!(has_uid && hasItemId && hasChangeKey)) {
        nsCOMPtr<nsIRunnable> resultrunnable =
                new AddCalendarItemDoneTask(ret,
                                            m_Event,
                                            itemId,
                                            changeKey,
                                            uid,
                                            m_pEwsTaskCallbacks,
                                            this);
		
        NS_DispatchToMainThread(resultrunnable);
    }
	return NS_OK;
}

time_t
FromCalDateTimeToTime(calIDateTime * date_time, bool notime) {
	PRTime t = 0;

	if (!date_time)
		return 0;

    if (notime) {
        nsCOMPtr<calIDateTime> newDt;
        date_time->Clone(getter_AddRefs(newDt));

        newDt->SetIsDate(true);
        newDt->GetNativeTime(&t);
    } else {
        date_time->GetNativeTime(&t);
    }

	return t / UNIX_TIME_TO_PRTIME;
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

void
FromCalEventToCalendarItem(calIEvent * event, ews_calendar_item * item) {
	//Attendees
	uint32_t count = 0;
	calIAttendee ** attendees = NULL;

	nsresult rv = event->GetAttendees(&count, &attendees);

	item->required_attendees_count = 0;
	item->optional_attendees_count = 0;
	
	if (NS_SUCCEEDED(rv) && count > 0) {
		std::vector<ews_attendee*> required;
		std::vector<ews_attendee*> optional;
		
		for (uint32_t i = 0; i < count; i++) {
			nsCString role;
			nsCString id;
			nsCString status;
			nsCString name;
            bool isOrganizer = false;
			
			attendees[i]->GetRole(role);
			attendees[i]->GetId(id);
			attendees[i]->GetParticipationStatus(status);
			attendees[i]->GetCommonName(name);
			attendees[i]->GetIsOrganizer(&isOrganizer);
			
			int32_t pos = id.Find(":");

			if (pos <= 0)
				continue;
			
			ews_attendee * ewsAttendee = (ews_attendee*)malloc(sizeof(ews_attendee));
			memset(ewsAttendee, 0, sizeof(ews_attendee));
			
			if (pos == 4) {
				ewsAttendee->email_address.routing_type = strdup("EX");
			}

			ewsAttendee->email_address.email = strdup(&(id.get()[pos + 1]));
			ewsAttendee->email_address.name = strdup(name.get());
			
			ewsAttendee->response_type = 0;
			for (size_t j = 0; j < sizeof(participationArray) / sizeof(const char *); j++) {
				if (status.EqualsLiteral(participationArray[j])) {
					ewsAttendee->response_type = j;

                    if (j == 1 && !isOrganizer) {
                        //Not organizer, convert to ACCEPTED
                        ewsAttendee->response_type = 3;
                    }
					break;
				}
			}

			if (role.EqualsLiteral("REQ-PARTICIPANT")) {
				required.push_back(ewsAttendee);
			} else {
				optional.push_back(ewsAttendee);
			}
		} //for i

		item->required_attendees_count = required.size();
		item->required_attendees = (ews_attendee**)malloc(sizeof(ews_attendee*) * required.size());
		for(size_t j = 0; j < required.size(); j++) {
			item->required_attendees[j] = required[j];
		}
		
		item->optional_attendees_count = optional.size();
		item->optional_attendees = (ews_attendee**)malloc(sizeof(ews_attendee*) * optional.size());
		for(size_t j = 0; j < optional.size(); j++) {
			item->optional_attendees[j] = optional[j];
		}
	}

	//Start End
	GET_DATE_TIME(event->GetStartDate, item->start);
	GET_DATE_TIME(event->GetEndDate, item->end);

    //Time Zone
    // nsCOMPtr<calIDateTime> dt;
    // event->GetStartDate(getter_AddRefs(dt));
    // if (dt) {
    //     nsCOMPtr<calITimezone> tz;
    //     dt->GetTimezone(getter_AddRefs(tz));

    //     if (tz) {
    //         nsCString tzid;
    //         tz->GetTzid(tzid);

    //         if (!item->meeting_time_zone) {
    //             item->meeting_time_zone =
    //                     (ews_time_zone *)malloc(sizeof(ews_time_zone));
    //             memset(item->meeting_time_zone,
    //                    0,
    //                    sizeof(ews_time_zone));
    //         }

    //         item->meeting_time_zone->time_zone_name =
    //                 strdup(tzid.get());
    //     }
    // }

    //Title
    nsCString title;
    event->GetTitle(title);
    item->item.subject = strdup(title.get());

    //Description
    nsCString desc;
    GetPropertyString(event,
                      NS_LITERAL_STRING("DESCRIPTION"),
                      desc);
    item->item.body = strdup(desc.get());

    //Location
    nsCString loc;
    GetPropertyString(event,
                      NS_LITERAL_STRING("LOCATION"),
                      loc);
    item->location = strdup(loc.get());

    //Item Id
    bool hasP = false;

    if (NS_SUCCEEDED(event->HasProperty(NS_LITERAL_STRING("X-ITEM-ID"), &hasP))
        && hasP) {
        nsCString v;
        GetPropertyString(event, NS_LITERAL_STRING("X-ITEM-ID"), v);
        item->item.item_id = strdup(v.get());
    }

    //Change Key
    hasP = false;
    if (NS_SUCCEEDED(event->HasProperty(NS_LITERAL_STRING("X-CHANGE-KEY"), &hasP))
        && hasP) {
        nsCString v;
        GetPropertyString(event, NS_LITERAL_STRING("X-CHANGE-KEY"), v);
        item->item.change_key = strdup(v.get());
    }
}

nsresult
GetOwnerEmail(nsIMsgIncomingServer * server, nsCString & ownerEmail) {
	nsresult rv = NS_OK;
	
	nsCOMPtr<nsIMsgAccountManager> accountMgr = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIMsgAccount> account;
	rv = accountMgr->FindAccountForServer(server,
	                                      getter_AddRefs(account));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIMsgIdentity> identity;
	rv = account->GetDefaultIdentity(getter_AddRefs(identity));
	NS_ENSURE_SUCCESS(rv, rv);

	rv = identity->GetEmail(ownerEmail);
	
	return rv;
}

nsresult
GetOwnerStatus(calIEvent * event, const nsCString & ownerEmail, nsCString & status) {
	uint32_t count = 0;
	calIAttendee ** attendees = NULL;

	nsresult rv = event->GetAttendees(&count, &attendees);
	NS_ENSURE_SUCCESS(rv, rv);

	for (uint32_t i = 0; i < count; i++) {
		nsCString id;
			
		attendees[i]->GetId(id);
		attendees[i]->GetParticipationStatus(status);
			
		int32_t pos = id.Find(":");

		if (pos <= 0)
			continue;
			
		nsCString attendeeEmail(&(id.get()[pos + 1]));

		if (ownerEmail.Equals(attendeeEmail,
                              nsCaseInsensitiveCStringComparator())) {
			return NS_OK;
		}
	} //for i

	return NS_ERROR_FAILURE;
}	

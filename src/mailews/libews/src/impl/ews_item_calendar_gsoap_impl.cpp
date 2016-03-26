#include "libews_gsoap.h"
#include <memory>
#include <map>

using namespace ews;

CEWSCalendarOperation::CEWSCalendarOperation() {

}

CEWSCalendarOperation::~CEWSCalendarOperation() {

}


CEWSRecurrenceRange::CEWSRecurrenceRange() {
}


CEWSRecurrenceRange::~CEWSRecurrenceRange() {
}


CEWSNoEndRecurrenceRange::CEWSNoEndRecurrenceRange() {
}


CEWSNoEndRecurrenceRange::~CEWSNoEndRecurrenceRange() {
}


CEWSEndDateRecurrenceRange::CEWSEndDateRecurrenceRange() {
}


CEWSEndDateRecurrenceRange::~CEWSEndDateRecurrenceRange() {
}


CEWSNumberedRecurrenceRange::CEWSNumberedRecurrenceRange() {
}


CEWSNumberedRecurrenceRange::~CEWSNumberedRecurrenceRange() {
}


CEWSRecurrencePattern::CEWSRecurrencePattern() {
}


CEWSRecurrencePattern::~CEWSRecurrencePattern() {
}


CEWSRecurrencePatternInterval::CEWSRecurrencePatternInterval() {
}


CEWSRecurrencePatternInterval::~CEWSRecurrencePatternInterval() {
}


CEWSRecurrencePatternRelativeYearly::CEWSRecurrencePatternRelativeYearly() {
}


CEWSRecurrencePatternRelativeYearly::~CEWSRecurrencePatternRelativeYearly() {
}


CEWSRecurrencePatternAbsoluteYearly::CEWSRecurrencePatternAbsoluteYearly() {
}


CEWSRecurrencePatternAbsoluteYearly::~CEWSRecurrencePatternAbsoluteYearly() {
}


CEWSRecurrencePatternRelativeMonthly::CEWSRecurrencePatternRelativeMonthly() {
}


CEWSRecurrencePatternRelativeMonthly::~CEWSRecurrencePatternRelativeMonthly() {
}


CEWSRecurrencePatternAbsoluteMonthly::CEWSRecurrencePatternAbsoluteMonthly() {
}


CEWSRecurrencePatternAbsoluteMonthly::~CEWSRecurrencePatternAbsoluteMonthly() {
}


CEWSRecurrencePatternWeekly::CEWSRecurrencePatternWeekly() {
}


CEWSRecurrencePatternWeekly::~CEWSRecurrencePatternWeekly() {
}


CEWSRecurrencePatternDaily::CEWSRecurrencePatternDaily() {
}


CEWSRecurrencePatternDaily::~CEWSRecurrencePatternDaily() {
}


CEWSRecurrence::CEWSRecurrence() {
}


CEWSRecurrence::~CEWSRecurrence() {
}


CEWSOccurrenceInfo::CEWSOccurrenceInfo() {
}


CEWSOccurrenceInfo::~CEWSOccurrenceInfo() {
}


CEWSDeletedOccurrenceInfo::CEWSDeletedOccurrenceInfo() {
}


CEWSDeletedOccurrenceInfo::~CEWSDeletedOccurrenceInfo() {
}


CEWSAttendee::CEWSAttendee() {
}


CEWSAttendee::~CEWSAttendee() {
}


CEWSTimeChangeSequence::CEWSTimeChangeSequence() {
}


CEWSTimeChangeSequence::~CEWSTimeChangeSequence() {
}


CEWSTimeChangeSequenceRelativeYearly::CEWSTimeChangeSequenceRelativeYearly() {
}


CEWSTimeChangeSequenceRelativeYearly::~CEWSTimeChangeSequenceRelativeYearly() {
}


CEWSTimeChangeSequenceAbsoluteDate::CEWSTimeChangeSequenceAbsoluteDate() {
}


CEWSTimeChangeSequenceAbsoluteDate::~CEWSTimeChangeSequenceAbsoluteDate() {
}


CEWSTimeChange::CEWSTimeChange() {
}


CEWSTimeChange::~CEWSTimeChange() {
}


CEWSTimeZone::CEWSTimeZone() {
}


CEWSTimeZone::~CEWSTimeZone() {
}

CEWSCalendarItem::CEWSCalendarItem() {

}

CEWSCalendarItem::~CEWSCalendarItem() {
}

CEWSCalendarOperationGsoapImpl::CEWSCalendarOperationGsoapImpl(CEWSSessionGsoapImpl * pSession)
	: m_pSession(pSession)
    , m_ItemOp(pSession) {

}

CEWSCalendarOperationGsoapImpl::~CEWSCalendarOperationGsoapImpl() {

}

bool CEWSCalendarOperationGsoapImpl::SyncCalendar(CEWSItemOperationCallback * callback,
                                                  CEWSError * pError) {
	ews2__ItemResponseShapeType * pItemShape =
			soap_new_req_ews2__ItemResponseShapeType(m_pSession->GetProxy(),
                                                     ews2__DefaultShapeNamesType__IdOnly);
	ews2__TargetFolderIdType targetFolderIdType;
	targetFolderIdType.__union_TargetFolderIdType =
            SOAP_UNION__ews2__union_TargetFolderIdType_DistinguishedFolderId;

    ews2__DistinguishedFolderIdType	folderId;
	folderId.Id =
            ConvertDistinguishedFolderIdName(CEWSFolder::calendar);
	_ews2__union_TargetFolderIdType * union_TargetFolderIdType =
			&targetFolderIdType.union_TargetFolderIdType;
	union_TargetFolderIdType->DistinguishedFolderId = &folderId;

    return m_ItemOp.__SyncItems(&targetFolderIdType,
                                pItemShape,
                                callback,
                                pError);
}

static
void
FromCalendarItemType(CEWSCalendarItemGsoapImpl * pItem,
                     ews2__CalendarItemType * item);

void
CEWSCalendarItemGsoapImpl::FromItemType(class ews2__ItemType * pItem) {
    CEWSItemGsoapImpl::FromItemType(pItem);

    ews2__CalendarItemType * pCalendarItem = dynamic_cast<ews2__CalendarItemType *>(pItem);

    if (pCalendarItem) {
        FromCalendarItemType(this, pCalendarItem);
    }
}

static
void
SetArrayProperty(CEWSAttendeeList * aList,
                 ews2__NonEmptyArrayOfAttendeesType * item) {
	for(size_t i = 0; i < item->Attendee.size(); i++) {
		CEWSAttendee * p = new CEWSAttendeeGsoapImpl();

		SET_PROPERTY_2(item->Attendee[i], p, ResponseType, CEWSAttendee::EWSResponseTypeEnum);
		SET_PROPERTY(item->Attendee[i], p, LastResponseTime);

		if (item->Attendee[i]->Mailbox) {
			SET_PROPERTY_STR(item->Attendee[i]->Mailbox,
			                 p,
			                 Name);
			SET_PROPERTY_STR(item->Attendee[i]->Mailbox,
			                 p,
			                 EmailAddress);

			if (item->Attendee[i]->Mailbox->ItemId) {
				p->SetItemId(item->Attendee[i]->Mailbox->ItemId->Id.c_str());
				SET_PROPERTY_STR(item->Attendee[i]->Mailbox->ItemId,
				                 p,
				                 ChangeKey);
			}
			SET_PROPERTY_STR(item->Attendee[i]->Mailbox,
			                 p,
			                 RoutingType);
			SET_PROPERTY_2(item->Attendee[i]->Mailbox,
			               p,
			               MailboxType,
			               CEWSEmailAddress::EWSMailboxTypeEnum);
		}
		aList->push_back(p);
	}
}

static
void
SetArrayProperty(CEWSItemList * aList,
                 ews2__NonEmptyArrayOfAllItemsType * item) {
	for(size_t i = 0; i < item-> __size_NonEmptyArrayOfAllItemsType; i++) {
		CEWSItem * p = NULL;

		__ews2__union_NonEmptyArrayOfAllItemsType * union_NonEmptyArrayOfAllItems =
				item->__union_NonEmptyArrayOfAllItemsType + i;

		CEWSItem::EWSItemType itemType = CEWSItem::Item_Message;
		ews2__ItemType * pItem = NULL;
            
		switch (union_NonEmptyArrayOfAllItems->__union_NonEmptyArrayOfAllItemsType) {
		case SOAP_UNION__ews2__union_NonEmptyArrayOfAllItemsType_Message:
			itemType = CEWSItem::Item_Message;
			pItem = union_NonEmptyArrayOfAllItems->union_NonEmptyArrayOfAllItemsType.Message;
			break;
		case SOAP_UNION__ews2__union_NonEmptyArrayOfAllItemsType_MeetingMessage:
			itemType = CEWSItem::Item_Message;
			pItem = union_NonEmptyArrayOfAllItems->union_NonEmptyArrayOfAllItemsType.MeetingMessage;
			break;
		case SOAP_UNION__ews2__union_NonEmptyArrayOfAllItemsType_MeetingRequest:
			itemType = CEWSItem::Item_Message;
			pItem = union_NonEmptyArrayOfAllItems->union_NonEmptyArrayOfAllItemsType.MeetingRequest;
			break;
		case SOAP_UNION__ews2__union_NonEmptyArrayOfAllItemsType_MeetingResponse:
			itemType = CEWSItem::Item_Message;
			pItem = union_NonEmptyArrayOfAllItems->union_NonEmptyArrayOfAllItemsType.MeetingResponse;
			break;
		case SOAP_UNION__ews2__union_NonEmptyArrayOfAllItemsType_MeetingCancellation:
			itemType = CEWSItem::Item_Message;
			pItem = union_NonEmptyArrayOfAllItems->union_NonEmptyArrayOfAllItemsType.MeetingCancellation;
			break;
		case SOAP_UNION__ews2__union_NonEmptyArrayOfAllItemsType_CalendarItem:
			itemType = CEWSItem::Item_Calendar;
			pItem = union_NonEmptyArrayOfAllItems->union_NonEmptyArrayOfAllItemsType.CalendarItem;
			break;
		default:
			continue;
		}

		p = CEWSItemGsoapImpl::CreateInstance(itemType, pItem);
		aList->push_back(p);
	}
}

static
CEWSRecurrence *
From(ews2__RecurrenceType * item) {
	CEWSRecurrenceGsoapImpl * p = new CEWSRecurrenceGsoapImpl;

	p->SetRecurrenceRange(RangeFrom(item->__union_RecurrenceType_,
	                           &item->union_RecurrenceType_));
	p->SetRecurrencePattern(PatternFrom(item->__union_RecurrenceType,
	                             &item->union_RecurrenceType));
	return p;
}

static
CEWSOccurrenceInfo *
From(ews2__OccurrenceInfoType * item) {
	CEWSOccurrenceInfo * p = new CEWSOccurrenceInfoGsoapImpl;
	
	if (item->ItemId) {
		p->SetItemId(item->ItemId->Id.c_str());
		SET_PROPERTY_STR(item->ItemId,
		                 p,
		                 ChangeKey);
	}

	SET_PROPERTY_3(item,
	               p,
	               Start,
	               time_t);
	SET_PROPERTY_3(item,
	               p,
	               End,
	               time_t);
	SET_PROPERTY_3(item,
	               p,
	               OriginalStart,
	               time_t);
	return p;
}

static
void
SetArrayProperty(CEWSOccurrenceInfoList * pList,
                 ews2__NonEmptyArrayOfOccurrenceInfoType * item) {
	for(size_t i = 0; i < item->Occurrence.size(); i++) {
		pList->push_back(From(item->Occurrence[i]));
	}
}

static
CEWSTimeChangeSequence *
From(__ews2__TimeChangeType_sequence * item) {
	CEWSTimeChangeSequence * p = NULL;

	switch(item->__union_TimeChangeType) {
	case SOAP_UNION__ews2__union_TimeChangeType_RelativeYearlyRecurrence:{
		CEWSTimeChangeSequenceRelativeYearlyGsoapImpl * pp = new CEWSTimeChangeSequenceRelativeYearlyGsoapImpl;
		p = pp;

		From(pp->GetRelativeYearly(),
		     item->union_TimeChangeType.RelativeYearlyRecurrence);
	}
		break;
	case SOAP_UNION__ews2__union_TimeChangeType_AbsoluteDate: {
		CEWSTimeChangeSequenceAbsoluteDateGsoapImpl * pp = new CEWSTimeChangeSequenceAbsoluteDateGsoapImpl;
		p = pp;

		SET_PROPERTY_STR((&item->union_TimeChangeType),
		                 pp,
		                 AbsoluteDate);
	}
		break;
	default:
		break;
	}

	return p;
}

static
void
From(CEWSTimeChangeGsoapImpl * p, ews2__TimeChangeType * item) {
	SET_PROPERTY_3(item,
	               p,
	               Offset,
	               long long);
	p->SetTime(item->Time.c_str());
	SET_PROPERTY_STR(item,
	                 p,
	                 TimeZoneName);

	if (item->__TimeChangeType_sequence) {
		p->SetTimeChangeSequence(From(item->__TimeChangeType_sequence));
	}
}
     
static
CEWSTimeZone *
From(ews2__TimeZoneType * item) {
	CEWSTimeZone * p = new CEWSTimeZoneGsoapImpl;

	SET_PROPERTY_STR(item, p, TimeZoneName);

	if (item->__TimeZoneType_sequence) {
		SET_PROPERTY_3(item->__TimeZoneType_sequence,
		               p,
		               BaseOffset,
		               long long);

		if (item->__TimeZoneType_sequence->__TimeZoneType_sequence_) {
			if (item->__TimeZoneType_sequence->__TimeZoneType_sequence_->Standard) {
                CEWSTimeChangeGsoapImpl * timeChange = new CEWSTimeChangeGsoapImpl;
                
				From(timeChange,
                     item->__TimeZoneType_sequence->__TimeZoneType_sequence_->Standard);
                p->SetStandard(timeChange);
            }
            
			if (item->__TimeZoneType_sequence->__TimeZoneType_sequence_->Daylight) {
                CEWSTimeChangeGsoapImpl * timeChange = new CEWSTimeChangeGsoapImpl;
				From(timeChange,
				     item->__TimeZoneType_sequence->__TimeZoneType_sequence_->Daylight);
                p->SetDaylight(timeChange);
            }
		}
		               
	}
	return p;
}

static
void
SetArrayProperty(CEWSDeletedOccurrenceInfoList * aList,
                 ews2__NonEmptyArrayOfDeletedOccurrencesType * item) {
	for(size_t i = 0; i < item->DeletedOccurrence.size(); i++) {
		CEWSDeletedOccurrenceInfo * p = new CEWSDeletedOccurrenceInfoGsoapImpl;
		SET_PROPERTY_3(item->DeletedOccurrence[i],
		               p,
		               Start,
		               time_t);
		aList->push_back(p);
	}
}

static
void
FromCalendarItemType(CEWSCalendarItemGsoapImpl * pItem,
                     ews2__CalendarItemType * item) {
    SET_PROPERTY_STR(item, pItem, UID);
    SET_PROPERTY(item, pItem, RecurrenceId);
    SET_PROPERTY(item, pItem, DateTimeStamp);
    SET_PROPERTY(item, pItem, Start);
    SET_PROPERTY(item, pItem, End);
    SET_PROPERTY(item, pItem, OriginalStart);
    SET_PROPERTY_B(item, pItem, AllDayEvent);
    SET_PROPERTY_2(item, pItem, LegacyFreeBusyStatus, enum CEWSCalendarItem::EWSLegacyFreeBusyTypeEnum);
    SET_PROPERTY_STR(item, pItem, Location);
    SET_PROPERTY_STR(item, pItem, When);
    SET_PROPERTY_B(item, pItem, Meeting);
    SET_PROPERTY_B(item, pItem, Cancelled);
    SET_PROPERTY_B(item, pItem, Recurring);
    SET_PROPERTY(item, pItem, MeetingRequestWasSent);
    SET_PROPERTY_B(item, pItem, ResponseRequested);
    SET_PROPERTY_2(item, pItem, CalendarItemType, enum CEWSCalendarItem::EWSCalendarItemTypeEnum);
    SET_PROPERTY_2(item, pItem, MyResponseType, enum CEWSAttendee::EWSResponseTypeEnum);

    if (item->Organizer) {
        pItem->SetOrganizer(CEWSRecipientGsoapImpl::CreateInstance(item->Organizer));
    }

    SET_PROPERTY_ARRAY(item, pItem, RequiredAttendees);
    SET_PROPERTY_ARRAY(item, pItem, OptionalAttendees);
    SET_PROPERTY_ARRAY(item, pItem, Resources);
    SET_PROPERTY(item, pItem, ConflictingMeetingCount);
    SET_PROPERTY(item, pItem, AdjacentMeetingCount);
    SET_PROPERTY_ARRAY(item, pItem, ConflictingMeetings);
    SET_PROPERTY_ARRAY(item, pItem, AdjacentMeetings);
    SET_PROPERTY_STR(item, pItem, Duration);
    SET_PROPERTY_STR(item, pItem, TimeZone);
    SET_PROPERTY(item, pItem, AppointmentReplyTime);
    SET_PROPERTY(item, pItem, AppointmentSequenceNumber);
    SET_PROPERTY(item, pItem, AppointmentState);

    if (item->Recurrence) {
	    pItem->SetRecurrence(From(item->Recurrence));
    }

    if (item->FirstOccurrence)
	    pItem->SetFirstOccurrence(From(item->FirstOccurrence));
    if (item->LastOccurrence)
	    pItem->SetLastOccurrence(From(item->LastOccurrence));
    
    SET_PROPERTY_ARRAY(item, pItem, ModifiedOccurrences);
    SET_PROPERTY_ARRAY(item, pItem, DeletedOccurrences);

    if (item->MeetingTimeZone)
	    pItem->SetMeetingTimeZone(From(item->MeetingTimeZone));

    SET_PROPERTY(item, pItem, ConferenceType);
    SET_PROPERTY(item, pItem, AllowNewTimeProposal);
    SET_PROPERTY_B(item, pItem, OnlineMeeting);
    SET_PROPERTY_STR(item, pItem, MeetingWorkspaceUrl);
    SET_PROPERTY_STR(item, pItem, NetShowUrl);
}

bool
CEWSCalendarOperationGsoapImpl::GetRecurrenceMasterItemId(const CEWSString & itemId,
                                                        CEWSString & masterItemId,
                                                        CEWSString & masterUId,
                                                        CEWSError * pError) {
    std::auto_ptr<CEWSItem> pItem(
        m_ItemOp.GetItem(itemId,
                         ews::CEWSItemOperation::IdOnly
                         | ews::CEWSItemOperation::CalendarItem
                         | ews::CEWSItemOperation::RecurringMasterItem,
                         pError));

    if (!pItem.get()) return false;

    masterItemId = pItem->GetItemId();
    masterUId = dynamic_cast<CEWSCalendarItem*>(pItem.get())->GetUID();

    return true;
}

CalendarItemTypeBuilder::CalendarItemTypeBuilder(soap * soap,
                                                 const CEWSCalendarItem * pEWSItem,
                                                 CEWSObjectPool * pObjPool)
    : ItemTypeBuilder(soap, pEWSItem, pObjPool) {
}

CalendarItemTypeBuilder::~CalendarItemTypeBuilder() {
}

ews2__ItemType * CalendarItemTypeBuilder::CreateItemType() {
	return new ews2__CalendarItemType();
}


extern void ToRecipient(const CEWSRecipient * pRecipient,
                        ews2__SingleRecipientType * pItem,
                        CEWSObjectPool * pObjPool);

static void FromArrayProperty(ews2__NonEmptyArrayOfAttendeesType * attendees,
                              CEWSAttendeeList * attendeeList,
                              CEWSObjectPool * pObjPool) {
    CEWSAttendeeList::iterator it = attendeeList->begin();

    while(it != attendeeList->end()) {
        CEWSAttendee * pAttendee = *it;

        ews2__AttendeeType * attendee =
                pObjPool->Create<ews2__AttendeeType>();
        attendee->Mailbox =
                pObjPool->Create<ews2__EmailAddressType>();

        FROM_PROPERTY_STR_4(pAttendee,
                            attendee->Mailbox,
                            pObjPool,
                            Name);
        FROM_PROPERTY_STR_4(pAttendee,
                            attendee->Mailbox,
                            pObjPool,
                            EmailAddress);
        FROM_PROPERTY_ITEM_ID(pAttendee,
                              attendee->Mailbox,
                              pObjPool);
        FROM_PROPERTY_STR_4(pAttendee,
                            attendee->Mailbox,
                            pObjPool,
                            RoutingType);
        FROM_PROPERTY_5(pAttendee,
                        attendee->Mailbox,
                        pObjPool,
                        MailboxType,
                        enum ews2__MailboxTypeType);
        FROM_PROPERTY_5(pAttendee,
                        attendee,
                        pObjPool,
                        ResponseType,
                        enum ews2__ResponseTypeType);
        FROM_PROPERTY_6(pAttendee,
                        attendee,
                        pObjPool,
                        LastResponseTime,
                        time_t,
                        0);

        attendees->Attendee.push_back(attendee);
        it++;
    }
}

static void FromArrayProperty(ews2__NonEmptyArrayOfAllItemsType * items,
                              CEWSItemList * itemList,
                              CEWSObjectPool * pObjPool) {
    //TODO: We don't need this when update exchange
    items->__size_NonEmptyArrayOfAllItemsType = 0;
}

static void FromArrayProperty(ews2__NonEmptyArrayOfDeletedOccurrencesType * occurrences,
                              CEWSDeletedOccurrenceInfoList * occurrenceList,
                              CEWSObjectPool * pObjPool) {
    CEWSDeletedOccurrenceInfoList::iterator it = occurrenceList->begin();

    while(it != occurrenceList->end()) {
        CEWSDeletedOccurrenceInfo * s =
                *it;
        ews2__DeletedOccurrenceInfoType * d =
                pObjPool->Create<ews2__DeletedOccurrenceInfoType>();

        d->Start = s->GetStart();

        occurrences->DeletedOccurrence.push_back(d);

        it++;
    }
}

static void FromOccurrenceInfo(ews2__OccurrenceInfoType * occurrence,
                               CEWSOccurrenceInfo * ewsOccurrence,
                               CEWSObjectPool * pObjPool) {
    FROM_PROPERTY_ITEM_ID(ewsOccurrence,
                          occurrence,
                          pObjPool);
        
    occurrence->Start = ewsOccurrence->GetStart();
    occurrence->End = ewsOccurrence->GetEnd();
    occurrence->OriginalStart = ewsOccurrence->GetOriginalStart();
}

static void FromArrayProperty(ews2__NonEmptyArrayOfOccurrenceInfoType * occurrences,
                              CEWSOccurrenceInfoList * occurrenceList,
                              CEWSObjectPool * pObjPool) {
    CEWSOccurrenceInfoList::iterator it = occurrenceList->begin();

    while(it != occurrenceList->end()) {
        CEWSOccurrenceInfo * s =
                *it;
        ews2__OccurrenceInfoType * d =
                pObjPool->Create<ews2__OccurrenceInfoType>();

        FromOccurrenceInfo(d, s, pObjPool);

        occurrences->Occurrence.push_back(d);

        it++;
    }
}

static void FromTimeChangeSequence(CEWSTimeChangeSequence * p,
                                   __ews2__TimeChangeType_sequence *& t,
                                   CEWSObjectPool * pObjPool) {
    if (!p) return;
    
    t = pObjPool->Create<__ews2__TimeChangeType_sequence>();

    switch(p->GetTimeChangeSequenceType()) {
    case CEWSTimeChangeSequence::RelativeYearly: {
        t->__union_TimeChangeType =
                SOAP_UNION__ews2__union_TimeChangeType_RelativeYearlyRecurrence;
        t->union_TimeChangeType.RelativeYearlyRecurrence =
                pObjPool->Create<ews2__RelativeYearlyRecurrencePatternType>();

        CEWSRecurrencePatternRelativeYearly * pp =
                dynamic_cast<CEWSTimeChangeSequenceRelativeYearly*>(p)->GetRelativeYearly();
        
        t->union_TimeChangeType.RelativeYearlyRecurrence->DaysOfWeek =
                (enum ews2__DayOfWeekType)pp->GetDaysOfWeek();
        t->union_TimeChangeType.RelativeYearlyRecurrence->DayOfWeekIndex =
                (enum ews2__DayOfWeekIndexType)pp->GetDayOfWeekIndex();
        t->union_TimeChangeType.RelativeYearlyRecurrence->Month =
                (enum ews2__MonthNamesType)pp->GetMonth();
    }
        break;
    case CEWSTimeChangeSequence::AbsoluteDate:
        t->__union_TimeChangeType =
                SOAP_UNION__ews2__union_TimeChangeType_AbsoluteDate;
        FROM_PROPERTY_STR_4(dynamic_cast<CEWSTimeChangeSequenceAbsoluteDate*>(p),
                            (&(t->union_TimeChangeType)),
                            pObjPool,
                            AbsoluteDate);
        break;
    default:
        break;
    }
}

static void FromTimeChange(CEWSTimeChange * p,
                           ews2__TimeChangeType *& t,
                           CEWSObjectPool * pObjPool) {
    if (!p) return;

    t = pObjPool->Create<ews2__TimeChangeType>();

    t->Offset = (LONG64)p->GetOffset();
    t->Time = p->GetTime().GetData();
    FROM_PROPERTY_STR_4(p,
                        t,
                        pObjPool,
                        TimeZoneName);

    FromTimeChangeSequence(p->GetTimeChangeSequence(),
                           t->__TimeChangeType_sequence,
                           pObjPool);
}

static void FromTimeZone(ews2__TimeZoneType * timezone,
                         CEWSTimeZone * ewsTimeZone,
                         CEWSObjectPool * pObjPool) {
    FROM_PROPERTY_STR_4(ewsTimeZone,
                        timezone,
                        pObjPool,
                        TimeZoneName);

    if (!ewsTimeZone->GetStandard() && !ewsTimeZone->GetDaylight()) {
        return;
    }

    timezone->__TimeZoneType_sequence =
            pObjPool->Create<__ews2__TimeZoneType_sequence>();

    timezone->__TimeZoneType_sequence->BaseOffset =
            ewsTimeZone->GetBaseOffset();
    
    timezone->__TimeZoneType_sequence->__TimeZoneType_sequence_ =
            pObjPool->Create<__ews2__TimeZoneType_sequence_>();

    FromTimeChange(ewsTimeZone->GetStandard(),
                   timezone->__TimeZoneType_sequence->__TimeZoneType_sequence_->Standard,
                   pObjPool);
    FromTimeChange(ewsTimeZone->GetDaylight(),
                   timezone->__TimeZoneType_sequence->__TimeZoneType_sequence_->Daylight,
                   pObjPool);
}

static
ews2__ExtendedPropertyType *
AttendeesToExtendedPropertyType(CEWSAttendeeList * pList,
                                const CEWSString & propertyName,
                                CEWSObjectPool * pPool) {
	ews2__ExtendedPropertyType * p = pPool->Create<ews2__ExtendedPropertyType>();

	p->ExtendedFieldURI = pPool->Create<ews2__PathToExtendedFieldType>();

	p->ExtendedFieldURI->DistinguishedPropertySetId =
			pPool->Create<enum ews2__DistinguishedPropertySetType>(ews2__DistinguishedPropertySetType__InternetHeaders);
	p->ExtendedFieldURI->PropertyName = pPool->Create<std::string>(propertyName.GetData());
	p->ExtendedFieldURI->PropertyType = ews2__MapiPropertyTypeType__String;

	p->__union_ExtendedPropertyType =
			SOAP_UNION__ews2__union_ExtendedPropertyType_Value;

	CEWSAttendeeList::iterator it = pList->begin();

	CEWSString v("");
	
	while (it != pList->end()) {
		CEWSAttendee * pAttendee = *it;

		if (v.GetLength() > 0)
			v.Append(", ");

		if (pAttendee->GetName().IsEmpty()) {
			v.Append(pAttendee->GetEmailAddress());
		} else {
			v.Append(pAttendee->GetName());
			v.Append(" <");
			v.Append(pAttendee->GetEmailAddress());
			v.Append(">");
		}
		
		it++;
	}
	
	p->union_ExtendedPropertyType.Value = pPool->Create<std::string>(v.GetData());
	
	return p;
}

static
void FromRecurrence(ews2__RecurrenceType * recurrence,
                    CEWSRecurrence * ewsRecurrence,
                    CEWSObjectPool * pObjPool) {
    FromRecurrenceRange(ewsRecurrence->GetRecurrenceRange(),
                        recurrence->__union_RecurrenceType_,
                        recurrence->union_RecurrenceType_,
                        pObjPool);

    FromRecurrencePattern(ewsRecurrence->GetRecurrencePattern(),
                          recurrence->__union_RecurrenceType,
                          recurrence->union_RecurrenceType,
                          pObjPool);
}

ews2__ItemType * CalendarItemTypeBuilder::Build() {
    //will setup the mime content
	ews2__ItemType * pItem = ItemTypeBuilder::Build();

	if (!pItem)
		return NULL;

    ews2__CalendarItemType * pCalendar =
            dynamic_cast<ews2__CalendarItemType*>(pItem);
    const CEWSCalendarItem * pEwsCalendar =
            dynamic_cast<const CEWSCalendarItem*>(m_pEWSItem);

    if (!pCalendar || !pEwsCalendar) {
        delete pItem;
        return NULL;
    }
    
    FROM_PROPERTY_STR(UID);
    //ReadOnly property
    //FROM_PROPERTY_3(RecurrenceId, time_t, 0);
    FROM_PROPERTY_3(Start, time_t, 0);
    FROM_PROPERTY_3(End, time_t, 0);
    FROM_PROPERTY_3(DateTimeStamp, time_t, 0);
    FROM_PROPERTY_3(OriginalStart, time_t, 0);
    FROM_PROPERTY_BOOL(IsAllDayEvent);
    FROM_PROPERTY_2(LegacyFreeBusyStatus, enum ews2__LegacyFreeBusyType);
    // FROM_PROPERTY_BOOL(IsMeeting);
    // FROM_PROPERTY_BOOL(IsCancelled);
    // FROM_PROPERTY_BOOL(IsRecurring);
    // FROM_PROPERTY_2(MeetingRequestWasSent, bool);
    // FROM_PROPERTY_BOOL(IsResponseRequested);
    //FROM_PROPERTY_2(CalendarItemType, enum ews2__CalendarItemTypeType);
    //FROM_PROPERTY_2(MyResponseType, enum ews2__ResponseTypeType);
    //FROM_PROPERTY_RECIPIENT(Organizer);

    //If no mime content, save attendees as normal
    //otherwise save attendees to extend property
    if (m_pEWSItem->GetMimeContent().IsEmpty()) {
        FROM_PROPERTY_STR(Location);
        FROM_PROPERTY_STR(When);
        
	    FROM_ARRAY_PROPERTY(RequiredAttendees, ews2__NonEmptyArrayOfAttendeesType);
	    FROM_ARRAY_PROPERTY(OptionalAttendees, ews2__NonEmptyArrayOfAttendeesType);
    } else {
	    if (pEwsCalendar->GetRequiredAttendees() != NULL &&
	        pEwsCalendar->GetRequiredAttendees()->size() > 0) {
		    pItem->ExtendedProperty.push_back(
		        AttendeesToExtendedPropertyType(pEwsCalendar->GetRequiredAttendees(),
		                                        "to",
		                                        m_pObjPool));
	    }
	    if (pEwsCalendar->GetOptionalAttendees() != NULL &&
	        pEwsCalendar->GetOptionalAttendees()->size() > 0) {
		    pItem->ExtendedProperty.push_back(
		        AttendeesToExtendedPropertyType(pEwsCalendar->GetOptionalAttendees(),
		                                        "cc",
		                                        m_pObjPool));
	    }
    }
    
    FROM_ARRAY_PROPERTY(Resources, ews2__NonEmptyArrayOfAttendeesType);
    // FROM_PROPERTY_3(ConflictingMeetingCount, int, 0);
    // FROM_PROPERTY_3(AdjacentMeetingCount, int, 0);
    // FROM_ARRAY_PROPERTY(ConflictingMeetings, ews2__NonEmptyArrayOfAllItemsType);
    // FROM_ARRAY_PROPERTY(AdjacentMeetings, ews2__NonEmptyArrayOfAllItemsType);
    FROM_PROPERTY_STR(Duration);
    FROM_PROPERTY_STR(TimeZone);
    //FROM_PROPERTY_3(AppointmentReplyTime, time_t, 0);
    //FROM_PROPERTY_2(AppointmentSequenceNumber, int);
    //FROM_PROPERTY_2(AppointmentState, int);
    FROM_RECURRENCE(Recurrence);
    FROM_OCCURRENCE(FirstOccurrence);
    FROM_OCCURRENCE(LastOccurrence);
    FROM_ARRAY_PROPERTY(ModifiedOccurrences, ews2__NonEmptyArrayOfOccurrenceInfoType);
    FROM_ARRAY_PROPERTY(DeletedOccurrences, ews2__NonEmptyArrayOfDeletedOccurrencesType);
    FROM_TIMEZONE(MeetingTimeZone);
    //FROM_PROPERTY_2(ConferenceType, int);
    //FROM_PROPERTY_BOOL_2(AllowNewTimeProposal);
    //FROM_PROPERTY_BOOL(IsOnlineMeeting);
    FROM_PROPERTY_STR(MeetingWorkspaceUrl);
    FROM_PROPERTY_STR(NetShowUrl);
    return pItem;
}

DEF_CREATE_INSTANCE(CEWSPhysicalAddress)
DEF_CREATE_INSTANCE(CEWSAttendee)
DEF_CREATE_INSTANCE(CEWSOccurrenceInfo);
DEF_CREATE_INSTANCE(CEWSDeletedOccurrenceInfo);
DEF_CREATE_INSTANCE(CEWSRecurrence);
DEF_CREATE_INSTANCE(CEWSTimeChange);
DEF_CREATE_INSTANCE(CEWSTimeZone);

CEWSRecurrenceRange * CEWSRecurrenceRange::CreateInstance(EWSRecurrenceRangeTypeEnum rangeType) {
    switch(rangeType){
    case CEWSRecurrenceRange::NoEnd:
        return new CEWSNoEndRecurrenceRangeGsoapImpl();
    case CEWSRecurrenceRange::EndDate:
        return new CEWSEndDateRecurrenceRangeGsoapImpl();
    case CEWSRecurrenceRange::Numbered:
        return new CEWSNumberedRecurrenceRangeGsoapImpl();
    default:
        break;
    }

    return NULL;
}

CEWSRecurrencePattern * CEWSRecurrencePattern::CreateInstance(EWSRecurrenceTypeEnum patternType) {
    switch(patternType) {
    case CEWSRecurrencePattern::RelativeYearly:
        return new CEWSRecurrencePatternRelativeYearlyGsoapImpl;
    case CEWSRecurrencePattern::AbsoluteYearly:
        return new CEWSRecurrencePatternAbsoluteYearlyGsoapImpl;
    case CEWSRecurrencePattern::RelativeMonthly:
        return new CEWSRecurrencePatternRelativeMonthlyGsoapImpl;
    case CEWSRecurrencePattern::AbsoluteMonthly:
        return new CEWSRecurrencePatternAbsoluteMonthlyGsoapImpl;
    case CEWSRecurrencePattern::Weekly:
        return new CEWSRecurrencePatternWeeklyGsoapImpl;
    case CEWSRecurrencePattern::Daily:
        return new CEWSRecurrencePatternDailyGsoapImpl;
    default:
        break;
    }

    return NULL;
}

CEWSTimeChangeSequence * CEWSTimeChangeSequence::CreateInstance(EWSTimeChangeSequenceTypeEnum sequenceType) {
    switch(sequenceType) {
    case CEWSTimeChangeSequence::RelativeYearly:
        return new CEWSTimeChangeSequenceRelativeYearlyGsoapImpl;
    case CEWSTimeChangeSequence::AbsoluteDate:
        return new CEWSTimeChangeSequenceAbsoluteDateGsoapImpl;
    default:
        break;
    }

    return NULL;
}


bool
CEWSCalendarOperationGsoapImpl::__ResponseToEvent(int responseType,
                                                  ews2__WellKnownResponseObjectType * itemType,
                                                  CEWSCalendarItem * pEWSItem,
                                                  CEWSError * pError) {
	CEWSSessionRequestGuard guard(m_pSession);

	__ews1__CreateItemResponse response;

	ews2__NonEmptyArrayOfAllItemsType arrayOfAllItemsType;
	arrayOfAllItemsType.__size_NonEmptyArrayOfAllItemsType = 1;

	__ews2__union_NonEmptyArrayOfAllItemsType union_NonEmptyArrayOfAllItemsType;

	ews1__CreateItemType createItemType;

    ews2__MessageDispositionType messageDispositionType =
            ews2__MessageDispositionType__SaveOnly;

	createItemType.MessageDisposition = &messageDispositionType;

	ews2__TargetFolderIdType targetFolderType;
	targetFolderType.__union_TargetFolderIdType =
            SOAP_UNION__ews2__union_TargetFolderIdType_DistinguishedFolderId;

	ews2__DistinguishedFolderIdType DistinguishedFolderIdType;
	DistinguishedFolderIdType.Id =
            ews2__DistinguishedFolderIdNameType__calendar;
    
	_ews2__union_TargetFolderIdType * union_TargetFolderIdType =
			&targetFolderType.union_TargetFolderIdType;
	union_TargetFolderIdType->DistinguishedFolderId =
			&DistinguishedFolderIdType;

	createItemType.SavedItemFolderId = &targetFolderType;

	CEWSObjectPool objPool;

	enum ews2__CalendarItemCreateOrDeleteOperationType
			CalendarItemCreateOrDeleteOperationType =
            ews2__CalendarItemCreateOrDeleteOperationType__SendToNone;
    
    union_NonEmptyArrayOfAllItemsType.__union_NonEmptyArrayOfAllItemsType =
            responseType;
    
    createItemType.SendMeetingInvitations =
				&CalendarItemCreateOrDeleteOperationType;

    switch(responseType) {
    case SOAP_UNION__ews2__union_NonEmptyArrayOfAllItemsType_AcceptItem:
        union_NonEmptyArrayOfAllItemsType.union_NonEmptyArrayOfAllItemsType.AcceptItem =
                dynamic_cast<ews2__AcceptItemType*>(itemType);
        break;
    case SOAP_UNION__ews2__union_NonEmptyArrayOfAllItemsType_DeclineItem:
        union_NonEmptyArrayOfAllItemsType.union_NonEmptyArrayOfAllItemsType.DeclineItem =
                dynamic_cast<ews2__DeclineItemType*>(itemType);
        break;
    case SOAP_UNION__ews2__union_NonEmptyArrayOfAllItemsType_TentativelyAcceptItem:
        union_NonEmptyArrayOfAllItemsType.union_NonEmptyArrayOfAllItemsType.TentativelyAcceptItem =
                dynamic_cast<ews2__TentativelyAcceptItemType*>(itemType);
        break;
    default:
	    SET_INVALID_RESPONSE;
        return false;
    }
        
	arrayOfAllItemsType.__union_NonEmptyArrayOfAllItemsType =
			&union_NonEmptyArrayOfAllItemsType;

	createItemType.Items = &arrayOfAllItemsType;

	int ret = m_pSession->GetProxy()->CreateItem(&createItemType, response);

	if (ret != SOAP_OK) {
		if (pError) {
			pError->SetErrorMessage(m_pSession->GetErrorMsg());
			pError->SetErrorCode(ret);
		}

		return false;
	}

	ews1__ArrayOfResponseMessagesType * messages =
			response.ews1__CreateItemResponse->ResponseMessages;
	for (int i = 0; i < messages->__size_ArrayOfResponseMessagesType; i++) {
		__ews1__union_ArrayOfResponseMessagesType * message =
				messages->__union_ArrayOfResponseMessagesType + i;

		if (message->__union_ArrayOfResponseMessagesType
				!=
				SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_CreateItemResponseMessage) {
			continue;
		}

		ews1__ItemInfoResponseMessageType * responseMessage =
				message->union_ArrayOfResponseMessagesType.CreateItemResponseMessage;

		if (responseMessage->ResponseClass
				!= ews2__ResponseClassType__Success) {
            SAVE_ERROR(pError, responseMessage);
			return false;
		}

		if (!responseMessage->Items)
			continue;

        bool got_calendar_item = false;
        std::string meeting_response_item_id("");
        std::string meeting_response_change_key("");
        
		for (int j = 0; j < responseMessage->Items->__size_ArrayOfRealItemsType;
				j++) {
			__ews2__union_ArrayOfRealItemsType * unionFolderChange =
					responseMessage->Items->__union_ArrayOfRealItemsType + j;

			ews2__ItemType * pItem =
					unionFolderChange->union_ArrayOfRealItemsType.Item;

            if (unionFolderChange->__union_ArrayOfRealItemsType ==
                SOAP_UNION__ews2__union_ArrayOfRealItemsType_CalendarItem) {
                //Calendar item response
                if (pItem->ItemId) {
                    pEWSItem->SetItemId(pItem->ItemId->Id.c_str());

                    if (pItem->ItemId->ChangeKey)
                        pEWSItem->SetChangeKey(pItem->ItemId->ChangeKey->c_str());

                    got_calendar_item =  true;
                }
            } else if (unionFolderChange->__union_ArrayOfRealItemsType ==
                SOAP_UNION__ews2__union_ArrayOfRealItemsType_MeetingResponse) {
                if (pItem->ItemId) {
                    meeting_response_item_id = pItem->ItemId->Id;

                    if (pItem->ItemId->ChangeKey)
                        meeting_response_change_key =
                                *pItem->ItemId->ChangeKey;
                }
            }
		} //for all items change

        if (!meeting_response_item_id.empty() &&
            !meeting_response_change_key.empty()) {
            //delete the draft meeting response
            std::auto_ptr<CEWSItem> pItem(CEWSItem::CreateInstance(CEWSItem::Item_Message));
            pItem->SetItemId(meeting_response_item_id.c_str());
            pItem->SetChangeKey(meeting_response_change_key.c_str());
            m_ItemOp.DeleteItem(pItem.get(),
                                false,
                                NULL);
        }
        
        if (got_calendar_item)
            return true;
	}

    if (pError) {
        pError->SetErrorMessage("Invalid Response Message.");
        pError->SetErrorCode(EWS_FAIL);
    }
    
    return false;
}

bool
CEWSCalendarOperationGsoapImpl::AcceptEvent(CEWSCalendarItem * pEwsItem,
                                            CEWSError * pError) {
    ews2__AcceptItemType AcceptItem;

    ews2__ItemIdType ItemId;
    ItemId.Id = pEwsItem->GetItemId().GetData();

    std::string cKey = pEwsItem->GetChangeKey().GetData();
    
    ItemId.ChangeKey = &cKey;
    
    AcceptItem.ReferenceItemId = &ItemId;

    return __ResponseToEvent(SOAP_UNION__ews2__union_NonEmptyArrayOfAllItemsType_AcceptItem,
                             &AcceptItem,
                             pEwsItem,
                             pError);
}

bool
CEWSCalendarOperationGsoapImpl::TentativelyAcceptEvent(CEWSCalendarItem * pEwsItem,
                                                       CEWSError * pError) {
    ews2__TentativelyAcceptItemType TentativelyAcceptItem;

    ews2__ItemIdType ItemId;
    ItemId.Id = pEwsItem->GetItemId().GetData();

    std::string cKey = pEwsItem->GetChangeKey().GetData();
    
    ItemId.ChangeKey = &cKey;
    
    TentativelyAcceptItem.ReferenceItemId = &ItemId;

    return __ResponseToEvent(SOAP_UNION__ews2__union_NonEmptyArrayOfAllItemsType_TentativelyAcceptItem,
                             &TentativelyAcceptItem,
                             pEwsItem,
                             pError);
}

bool
CEWSCalendarOperationGsoapImpl::DeclineEvent(CEWSCalendarItem * pEwsItem,
                                             CEWSError * pError) {
    ews2__DeclineItemType DeclineItem;

    ews2__ItemIdType ItemId;
    ItemId.Id = pEwsItem->GetItemId().GetData();

    std::string cKey = pEwsItem->GetChangeKey().GetData();
    
    ItemId.ChangeKey = &cKey;
    
    DeclineItem.ReferenceItemId = &ItemId;

    return __ResponseToEvent(SOAP_UNION__ews2__union_NonEmptyArrayOfAllItemsType_DeclineItem,
                             &DeclineItem,
                             pEwsItem,
                             pError);
}

// bool
// CEWSCalendarOperationGsoapImpl::ItemIdToUID(const CEWSString & itemId,
//                                             CEWSString & uid,
//                                             CEWSError * pError) {
// 	CEWSSessionRequestGuard guard(m_pSession);

//     ews1__ConvertIdType ConvertIdType;
//     __ews1__ConvertIdResponse response;

//     ConvertIdType.DestinationFormat =
//             ews2__IdFormatType__HexEntryId;

//     ews2__NonEmptyArrayOfAlternateIdsType NonEmptyArrayOfAlternateIdsType;
//     ConvertIdType.SourceIds = &NonEmptyArrayOfAlternateIdsType;

//     NonEmptyArrayOfAlternateIdsType.__size_NonEmptyArrayOfAlternateIdsType = 1;

//     __ews2__union_NonEmptyArrayOfAlternateIdsType union_NonEmptyArrayOfAlternateIdsType;
    
//     NonEmptyArrayOfAlternateIdsType
//             .__union_NonEmptyArrayOfAlternateIdsType =
//             &union_NonEmptyArrayOfAlternateIdsType;

//     union_NonEmptyArrayOfAlternateIdsType
//             .__union_NonEmptyArrayOfAlternateIdsType =
//             SOAP_UNION__ews2__union_NonEmptyArrayOfAlternateIdsType_AlternateId;

//     ews2__AlternateIdType AlternateIdRequest;
//     union_NonEmptyArrayOfAlternateIdsType
//             .union_NonEmptyArrayOfAlternateIdsType
//             .AlternateId = &AlternateIdRequest;

//     AlternateIdRequest.Id = itemId.GetData();
//     AlternateIdRequest.Format = ews2__IdFormatType__EwsId;
//     AlternateIdRequest.Mailbox =
//             m_pSession->GetAccountInfo()->EmailAddress.GetData();
    
// 	int ret = m_pSession->GetProxy()->ConvertId(&ConvertIdType, response);

// 	if (ret != SOAP_OK) {
// 		if (pError) {
// 			pError->SetErrorMessage(m_pSession->GetErrorMsg());
// 			pError->SetErrorCode(ret);
// 		}

// 		return false;
// 	}

// 	ews1__ArrayOfResponseMessagesType * messages =
// 			response.ews1__ConvertIdResponse->ResponseMessages;
// 	for (int i = 0; i < messages->__size_ArrayOfResponseMessagesType; i++) {
// 		__ews1__union_ArrayOfResponseMessagesType * message =
// 				messages->__union_ArrayOfResponseMessagesType + i;

// 		if (message->__union_ArrayOfResponseMessagesType
// 				!=
// 				SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_ConvertIdResponseMessage) {
// 			continue;
// 		}

// 		ews1__ConvertIdResponseMessageType * responseMessage =
// 				message->union_ArrayOfResponseMessagesType.ConvertIdResponseMessage;

// 		if (responseMessage->ResponseClass
// 				!= ews2__ResponseClassType__Success) {
//             SAVE_ERROR(pError, responseMessage);

// 			return false;
// 		}

// 		if (!responseMessage->AlternateId)
// 			continue;

//         if (responseMessage->AlternateId->Format !=
//             ews2__IdFormatType__HexEntryId)
//             continue;

//         ews2__AlternateIdType * AlternateId =
//                 (ews2__AlternateIdType*)responseMessage->AlternateId;

//         uid.Clear();
//         uid.Append(AlternateId->Id.c_str());
        
//         return true;
// 	}

//     if (pError) {
//         pError->SetErrorMessage("Invalid Response Message.");
//         pError->SetErrorCode(EWS_FAIL);
//     }
//     return false;
// }

bool
CEWSCalendarOperationGsoapImpl::DeleteOccurrence(const CEWSString & masterItemId,
                                                 int occurrenceInstance,
                                                 CEWSError * pError) {
	CEWSSessionRequestGuard guard(m_pSession);

	ews1__DeleteItemType deleteItemType;
	__ews1__DeleteItemResponse response;
	CEWSObjectPool objPool;

    enum ews2__AffectedTaskOccurrencesType AffectedTaskOccurrences =
            ews2__AffectedTaskOccurrencesType__SpecifiedOccurrenceOnly;

    deleteItemType.AffectedTaskOccurrences =
            &AffectedTaskOccurrences;
    
    enum ews2__CalendarItemCreateOrDeleteOperationType
			CalendarItemCreateOrDeleteOperationType =
            ews2__CalendarItemCreateOrDeleteOperationType__SendToNone;
    
	deleteItemType.DeleteType =
            ews2__DisposalType__MoveToDeletedItems;
    deleteItemType.SendMeetingCancellations =
            &CalendarItemCreateOrDeleteOperationType;

	ews2__NonEmptyArrayOfBaseItemIdsType itemIdsType;
	itemIdsType.__size_NonEmptyArrayOfBaseItemIdsType = 1;

	__ews2__union_NonEmptyArrayOfBaseItemIdsType union_NonEmptyArrayOfBaseItemIdsType;
    
	__ews2__union_NonEmptyArrayOfBaseItemIdsType * tmp =
			&union_NonEmptyArrayOfBaseItemIdsType;
	
    tmp->__union_NonEmptyArrayOfBaseItemIdsType =
            SOAP_UNION__ews2__union_NonEmptyArrayOfBaseItemIdsType_OccurrenceItemId;

    ews2__OccurrenceItemIdType itemIdType;
    itemIdType.RecurringMasterId = masterItemId;
    itemIdType.InstanceIndex = occurrenceInstance;

    tmp->union_NonEmptyArrayOfBaseItemIdsType.OccurrenceItemId =
			&itemIdType;
	
	itemIdsType.__union_NonEmptyArrayOfBaseItemIdsType =
			tmp;

	deleteItemType.ItemIds = &itemIdsType;

	int ret = m_pSession->GetProxy()->DeleteItem(&deleteItemType, response);

	if (ret != SOAP_OK) {
		if (pError) {
			pError->SetErrorMessage(m_pSession->GetErrorMsg());
			pError->SetErrorCode(ret);
		}

		return false;
	}

	ews1__ArrayOfResponseMessagesType * messages =
			response.ews1__DeleteItemResponse->ResponseMessages;
	for (int i = 0; i < messages->__size_ArrayOfResponseMessagesType; i++) {
		__ews1__union_ArrayOfResponseMessagesType * message =
				messages->__union_ArrayOfResponseMessagesType + i;

		if (message->__union_ArrayOfResponseMessagesType
				!=
				SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_DeleteItemResponseMessage) {
			continue;
		}

		ews1__ResponseMessageType * responseMessage =
				message->union_ArrayOfResponseMessagesType.DeleteItemResponseMessage;

		if (responseMessage->ResponseClass
				!= ews2__ResponseClassType__Success) {
            SAVE_ERROR(pError, responseMessage);

            if (*(responseMessage->__ResponseMessageType_sequence->ResponseCode)
                == ews1__ResponseCodeType__ErrorCalendarOccurrenceIsDeletedFromRecurrence) {
                //Item already deleted return delete ok
                return true;
            }
			return false;
		}

		return true;
	}

	SET_INVALID_RESPONSE;
	return false;
}

static
void
CreateItemChangeDescBody(CEWSCalendarItem * pItem,
                         std::vector<__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType> & ItemChangeDescs,
                         CEWSObjectPool * pObjPool) {
	ItemChangeDescs.resize(ItemChangeDescs.size() + 1);
	
	__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType * ItemChangeDesc =
			&ItemChangeDescs[ItemChangeDescs.size() - 1];
	
	ItemChangeDesc->__union_NonEmptyArrayOfItemChangeDescriptionsType =
			SOAP_UNION__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType_SetItemField;

	ews2__SetItemFieldType * SetItemField =
			pObjPool->Create<ews2__SetItemFieldType>();
	ItemChangeDesc->union_NonEmptyArrayOfItemChangeDescriptionsType.SetItemField =
			SetItemField;

	SetItemField->__unionPath = SOAP_UNION__ews2__union_ChangeDescriptionType_FieldURI;

	ews2__PathToUnindexedFieldType * FieldURI =
			pObjPool->Create<ews2__PathToUnindexedFieldType>();
	SetItemField->__union_ChangeDescriptionType.FieldURI = FieldURI;
	FieldURI->FieldURI = ews2__UnindexedFieldURIType__item_x003aBody;

	SetItemField->__union_SetItemFieldType_ = SOAP_UNION__ews2__union_SetItemFieldType__CalendarItem;

	ews2__CalendarItemType * itemType =
			pObjPool->Create<ews2__CalendarItemType>();
	SetItemField->union_SetItemFieldType_.CalendarItem = itemType;

	ews2__BodyType * Body =
			pObjPool->Create<ews2__BodyType>();

	itemType->Body = Body;
	
	Body->BodyType = ews2__BodyTypeType__HTML;
	Body->__item = pItem->GetBody().GetData();
}

static
void
CreateItemChangeDescDateTime(CEWSCalendarItem * pItem,
                             bool start,
                             std::vector<__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType> & ItemChangeDescs,
                             CEWSObjectPool * pObjPool) {
	ItemChangeDescs.resize(ItemChangeDescs.size() + 1);
	
	__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType * ItemChangeDesc =
			&ItemChangeDescs[ItemChangeDescs.size() - 1];
	
	ItemChangeDesc->__union_NonEmptyArrayOfItemChangeDescriptionsType =
			SOAP_UNION__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType_SetItemField;

	ews2__SetItemFieldType * SetItemField =
			pObjPool->Create<ews2__SetItemFieldType>();
	ItemChangeDesc->union_NonEmptyArrayOfItemChangeDescriptionsType.SetItemField =
			SetItemField;

	SetItemField->__unionPath = SOAP_UNION__ews2__union_ChangeDescriptionType_FieldURI;

	ews2__PathToUnindexedFieldType * FieldURI =
			pObjPool->Create<ews2__PathToUnindexedFieldType>();
	SetItemField->__union_ChangeDescriptionType.FieldURI = FieldURI;
	FieldURI->FieldURI =
			start ? ews2__UnindexedFieldURIType__calendar_x003aStart :
			ews2__UnindexedFieldURIType__calendar_x003aEnd;

	SetItemField->__union_SetItemFieldType_ = SOAP_UNION__ews2__union_SetItemFieldType__CalendarItem;

	ews2__CalendarItemType * itemType =
			pObjPool->Create<ews2__CalendarItemType>();
	SetItemField->union_SetItemFieldType_.CalendarItem = itemType;

	time_t * dt =
			pObjPool->Create<time_t>();

	if (start) {
		itemType->Start = dt;
		*dt = pItem->GetStart();
	} else {
		itemType->End = dt;
		*dt = pItem->GetEnd();
	}
}

static
void
CreateItemChangeDescAttendee(CEWSCalendarItem * pEwsCalendar,
                             bool required,
                             std::vector<__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType> & ItemChangeDescs,
                             CEWSObjectPool * m_pObjPool) {
	ItemChangeDescs.resize(ItemChangeDescs.size() + 1);
	
	__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType * ItemChangeDesc =
			&ItemChangeDescs[ItemChangeDescs.size() - 1];
	
	ItemChangeDesc->__union_NonEmptyArrayOfItemChangeDescriptionsType =
			SOAP_UNION__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType_SetItemField;

	ews2__SetItemFieldType * SetItemField =
			m_pObjPool->Create<ews2__SetItemFieldType>();
	ItemChangeDesc->union_NonEmptyArrayOfItemChangeDescriptionsType.SetItemField =
			SetItemField;

	SetItemField->__unionPath = SOAP_UNION__ews2__union_ChangeDescriptionType_FieldURI;

	ews2__PathToUnindexedFieldType * FieldURI =
			m_pObjPool->Create<ews2__PathToUnindexedFieldType>();
	SetItemField->__union_ChangeDescriptionType.FieldURI = FieldURI;
	FieldURI->FieldURI =
			required ? ews2__UnindexedFieldURIType__calendar_x003aRequiredAttendees :
			ews2__UnindexedFieldURIType__calendar_x003aOptionalAttendees;

	SetItemField->__union_SetItemFieldType_ = SOAP_UNION__ews2__union_SetItemFieldType__CalendarItem;

	ews2__CalendarItemType * pCalendar =
			m_pObjPool->Create<ews2__CalendarItemType>();
	SetItemField->union_SetItemFieldType_.CalendarItem = pCalendar;

	if (required) {
		FROM_ARRAY_PROPERTY(RequiredAttendees, ews2__NonEmptyArrayOfAttendeesType);
	} else {
		FROM_ARRAY_PROPERTY(OptionalAttendees, ews2__NonEmptyArrayOfAttendeesType);
	}
}
 
static
void
CreateItemChangeDescLocation(CEWSCalendarItem * pItem,
                             std::vector<__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType> & ItemChangeDescs,
                             CEWSObjectPool * pObjPool) {
	ItemChangeDescs.resize(ItemChangeDescs.size() + 1);
	
	__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType * ItemChangeDesc =
			&ItemChangeDescs[ItemChangeDescs.size() - 1];
	
	ItemChangeDesc->__union_NonEmptyArrayOfItemChangeDescriptionsType =
			SOAP_UNION__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType_SetItemField;

	ews2__SetItemFieldType * SetItemField =
			pObjPool->Create<ews2__SetItemFieldType>();
	ItemChangeDesc->union_NonEmptyArrayOfItemChangeDescriptionsType.SetItemField =
			SetItemField;

	SetItemField->__unionPath = SOAP_UNION__ews2__union_ChangeDescriptionType_FieldURI;

	ews2__PathToUnindexedFieldType * FieldURI =
			pObjPool->Create<ews2__PathToUnindexedFieldType>();
	SetItemField->__union_ChangeDescriptionType.FieldURI = FieldURI;
	FieldURI->FieldURI = ews2__UnindexedFieldURIType__calendar_x003aLocation;

	SetItemField->__union_SetItemFieldType_ = SOAP_UNION__ews2__union_SetItemFieldType__CalendarItem;

	ews2__CalendarItemType * itemType =
			pObjPool->Create<ews2__CalendarItemType>();
	SetItemField->union_SetItemFieldType_.CalendarItem = itemType;

	std::string * v =
			pObjPool->Create<std::string>(pItem->GetLocation().GetData());

	itemType->Location = v;
}

static
void
CreateItemChangeDescSubject(CEWSCalendarItem * pItem,
                            std::vector<__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType> & ItemChangeDescs,
                            CEWSObjectPool * pObjPool) {
	ItemChangeDescs.resize(ItemChangeDescs.size() + 1);
	
	__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType * ItemChangeDesc =
			&ItemChangeDescs[ItemChangeDescs.size() - 1];
	
	ItemChangeDesc->__union_NonEmptyArrayOfItemChangeDescriptionsType =
			SOAP_UNION__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType_SetItemField;

	ews2__SetItemFieldType * SetItemField =
			pObjPool->Create<ews2__SetItemFieldType>();
	ItemChangeDesc->union_NonEmptyArrayOfItemChangeDescriptionsType.SetItemField =
			SetItemField;

	SetItemField->__unionPath = SOAP_UNION__ews2__union_ChangeDescriptionType_FieldURI;

	ews2__PathToUnindexedFieldType * FieldURI =
			pObjPool->Create<ews2__PathToUnindexedFieldType>();
	SetItemField->__union_ChangeDescriptionType.FieldURI = FieldURI;
	FieldURI->FieldURI = ews2__UnindexedFieldURIType__item_x003aSubject;

	SetItemField->__union_SetItemFieldType_ = SOAP_UNION__ews2__union_SetItemFieldType__CalendarItem;

	ews2__CalendarItemType * itemType =
			pObjPool->Create<ews2__CalendarItemType>();
	SetItemField->union_SetItemFieldType_.CalendarItem = itemType;

	std::string * v =
			pObjPool->Create<std::string>(pItem->GetSubject().GetData());

	itemType->Subject = v;
}

static
void
CreateItemChangeDescMeetingTimeZone(CEWSCalendarItem * pItem,
                            std::vector<__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType> & ItemChangeDescs,
                            CEWSObjectPool * pObjPool) {
	ItemChangeDescs.resize(ItemChangeDescs.size() + 1);
	
	__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType * ItemChangeDesc =
			&ItemChangeDescs[ItemChangeDescs.size() - 1];
	
	ItemChangeDesc->__union_NonEmptyArrayOfItemChangeDescriptionsType =
			SOAP_UNION__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType_SetItemField;

	ews2__SetItemFieldType * SetItemField =
			pObjPool->Create<ews2__SetItemFieldType>();
	ItemChangeDesc->union_NonEmptyArrayOfItemChangeDescriptionsType.SetItemField =
			SetItemField;

	SetItemField->__unionPath = SOAP_UNION__ews2__union_ChangeDescriptionType_FieldURI;

	ews2__PathToUnindexedFieldType * FieldURI =
			pObjPool->Create<ews2__PathToUnindexedFieldType>();
	SetItemField->__union_ChangeDescriptionType.FieldURI = FieldURI;
	FieldURI->FieldURI = ews2__UnindexedFieldURIType__calendar_x003aMeetingTimeZone;

	SetItemField->__union_SetItemFieldType_ = SOAP_UNION__ews2__union_SetItemFieldType__CalendarItem;

	ews2__CalendarItemType * itemType =
			pObjPool->Create<ews2__CalendarItemType>();
	SetItemField->union_SetItemFieldType_.CalendarItem = itemType;

    itemType->MeetingTimeZone = pObjPool->Create<ews2__TimeZoneType>();
    FromTimeZone(itemType->MeetingTimeZone,
                 pItem->GetMeetingTimeZone(),
                 pObjPool);                 
}

static
void
CreateItemChangeDescs(CEWSCalendarItem * pItem,
                      int flags,
                      std::vector<__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType> & ItemChangeDescs,
                      CEWSObjectPool * pObjPool) {
	if ((flags & CEWSCalendarOperation::Body) == CEWSCalendarOperation::Body) {
		CreateItemChangeDescBody(pItem,
		                     ItemChangeDescs,
		                     pObjPool);
	}
	if ((flags & CEWSCalendarOperation::Start) == CEWSCalendarOperation::Start) {
		CreateItemChangeDescDateTime(pItem,
		                             true,
		                             ItemChangeDescs,
		                             pObjPool);
	}
	if ((flags & CEWSCalendarOperation::End) == CEWSCalendarOperation::End) {
		CreateItemChangeDescDateTime(pItem,
		                             false,
		                             ItemChangeDescs,
		                             pObjPool);
	}
	if ((flags & CEWSCalendarOperation::RequiredAttendee) == CEWSCalendarOperation::RequiredAttendee) {
		CreateItemChangeDescAttendee(pItem,
		                             true,
		                             ItemChangeDescs,
		                             pObjPool);
	}
	if ((flags & CEWSCalendarOperation::OptionalAttendee) == CEWSCalendarOperation::OptionalAttendee) {
		CreateItemChangeDescAttendee(pItem,
		                             false,
		                             ItemChangeDescs,
		                             pObjPool);
	}
	if ((flags & CEWSCalendarOperation::Location) == CEWSCalendarOperation::Location) {
		CreateItemChangeDescLocation(pItem,
		                             ItemChangeDescs,
		                             pObjPool);
	}
	if ((flags & CEWSCalendarOperation::Subject) == CEWSCalendarOperation::Subject) {
		CreateItemChangeDescSubject(pItem,
                                    ItemChangeDescs,
                                    pObjPool);
	}

    if (pItem->GetMeetingTimeZone()) {
		CreateItemChangeDescMeetingTimeZone(pItem,
                                            ItemChangeDescs,
                                            pObjPool);
    }
}

bool
CEWSCalendarOperationGsoapImpl::__UpdateEvent(CEWSCalendarItem * pItem,
                                             int flags,
                                             ews2__ItemChangeType * ItemChange,
                                             CEWSError * pError) {
	CEWSSessionRequestGuard guard(m_pSession);

	ews1__UpdateItemType updateItemType;
	__ews1__UpdateItemResponse response;
	CEWSObjectPool objPool;

	updateItemType.ConflictResolution = ews2__ConflictResolutionType__AutoResolve;
	enum ews2__MessageDispositionType MessageDisposition = ews2__MessageDispositionType__SaveOnly;
	updateItemType.MessageDisposition = &MessageDisposition;

	ews2__NonEmptyArrayOfItemChangesType ItemChanges;

	updateItemType.ItemChanges = &ItemChanges;
	enum ews2__CalendarItemUpdateOperationType SendMeetingInvitationsOrCancellations =
			ews2__CalendarItemUpdateOperationType__SendToNone;
	updateItemType.SendMeetingInvitationsOrCancellations =
			&SendMeetingInvitationsOrCancellations;

	ews2__NonEmptyArrayOfItemChangeDescriptionsType * Updates =
			objPool.Create<ews2__NonEmptyArrayOfItemChangeDescriptionsType>();
	ItemChange->Updates = Updates;

	std::vector<__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType> ItemChangeDescs;

    CreateItemChangeDescs(pItem,
                          flags,
                          ItemChangeDescs,
                          &objPool);
    
	Updates->__size_NonEmptyArrayOfItemChangeDescriptionsType = ItemChangeDescs.size();


	Updates->__union_NonEmptyArrayOfItemChangeDescriptionsType = &ItemChangeDescs[0];

	ItemChanges.ItemChange.push_back(ItemChange);

	int ret = m_pSession->GetProxy()->UpdateItem(&updateItemType, response);

	if (ret != SOAP_OK) {
		if (pError) {
			pError->SetErrorMessage(m_pSession->GetErrorMsg());
			pError->SetErrorCode(ret);
		}

		return false;
	}

	ews1__ArrayOfResponseMessagesType * messages =
			response.ews1__UpdateItemResponse->ResponseMessages;
	for (int i = 0; i < messages->__size_ArrayOfResponseMessagesType; i++) {
		__ews1__union_ArrayOfResponseMessagesType * message =
				messages->__union_ArrayOfResponseMessagesType + i;

		if (message->__union_ArrayOfResponseMessagesType
				!=
				SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_UpdateItemResponseMessage) {
			continue;
		}

		ews1__UpdateItemResponseMessageType * responseMessage =
				message->union_ArrayOfResponseMessagesType.UpdateItemResponseMessage;

		if (responseMessage->ResponseClass
				!= ews2__ResponseClassType__Success) {
            SAVE_ERROR(pError, responseMessage);
			return false;
		}

		if (!responseMessage->Items)
			continue;

		for (int j = 0; j < responseMessage->Items->__size_ArrayOfRealItemsType;
		     j++) {
			__ews2__union_ArrayOfRealItemsType * unionFolderChange =
					responseMessage->Items->__union_ArrayOfRealItemsType + j;

			ews2__ItemType * pItemType =
					unionFolderChange->union_ArrayOfRealItemsType.Item;

			if (pItemType->ItemId) {
				pItem->SetItemId(pItemType->ItemId->Id.c_str());

				if (pItemType->ItemId->ChangeKey)
					pItem->SetChangeKey(pItemType->ItemId->ChangeKey->c_str());

				return true;
			}
		} //for all items change
	}

	SET_INVALID_RESPONSE;
	return false;
}

bool
CEWSCalendarOperationGsoapImpl::UpdateEvent(CEWSCalendarItem * pEwsItem,
                                            int flags,
                                            CEWSError * pError) {
	ews2__ItemChangeType ItemChange;

	ItemChange.__union_ItemChangeType = SOAP_UNION__ews2__union_ItemChangeType_ItemId;

	ews2__ItemIdType ItemId;

	ItemId.Id = pEwsItem->GetItemId();

	std::string change_key =
			pEwsItem->GetChangeKey().GetData();
	ItemId.ChangeKey = &change_key;

	ItemChange.union_ItemChangeType.ItemId = &ItemId;

	return __UpdateEvent(pEwsItem,
	                     flags,
	                     &ItemChange,
	                     pError);
}

bool
CEWSCalendarOperationGsoapImpl::UpdateEventOccurrence(CEWSCalendarItem * pEwsItem,
                                                      int instanceIndex,
                                                      int flags,
                                                      CEWSError * pError) {
	ews2__ItemChangeType ItemChange;

	ItemChange.__union_ItemChangeType = SOAP_UNION__ews2__union_ItemChangeType_OccurrenceItemId;

	ews2__OccurrenceItemIdType ItemId;

	ItemId.RecurringMasterId = pEwsItem->GetItemId();
    ItemId.InstanceIndex = instanceIndex;
	std::string change_key =
			pEwsItem->GetChangeKey().GetData();
	ItemId.ChangeKey = &change_key;

	ItemChange.union_ItemChangeType.OccurrenceItemId = &ItemId;

	return __UpdateEvent(pEwsItem,
	                     flags,
	                     &ItemChange,
	                     pError);
}

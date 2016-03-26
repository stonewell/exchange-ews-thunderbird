#include "libews_gsoap.h"
#include <memory>
#include <map>

using namespace ews;

CEWSTaskOperation::CEWSTaskOperation() {

}

CEWSTaskOperation::~CEWSTaskOperation() {

}

CEWSTaskItem::CEWSTaskItem() {

}

CEWSTaskItem::~CEWSTaskItem() {
}

CEWSTaskOperationGsoapImpl::CEWSTaskOperationGsoapImpl(CEWSSessionGsoapImpl * pSession)
	: m_pSession(pSession)
	, m_ItemOp(pSession) {

}

CEWSTaskOperationGsoapImpl::~CEWSTaskOperationGsoapImpl() {

}

bool CEWSTaskOperationGsoapImpl::SyncTask(CEWSItemOperationCallback * callback,
                                          CEWSError * pError) {
	ews2__ItemResponseShapeType * pItemShape =
			soap_new_req_ews2__ItemResponseShapeType(m_pSession->GetProxy(),
			                                         ews2__DefaultShapeNamesType__IdOnly);
	ews2__TargetFolderIdType targetFolderIdType;
	targetFolderIdType.__union_TargetFolderIdType =
			SOAP_UNION__ews2__union_TargetFolderIdType_DistinguishedFolderId;

	ews2__DistinguishedFolderIdType	folderId;
	folderId.Id =
			ConvertDistinguishedFolderIdName(CEWSFolder::tasks);
	_ews2__union_TargetFolderIdType * union_TargetFolderIdType =
			&targetFolderIdType.union_TargetFolderIdType;
	union_TargetFolderIdType->DistinguishedFolderId = &folderId;

	return m_ItemOp.__SyncItems(&targetFolderIdType,
	                            pItemShape,
	                            callback,
	                            pError);
}

static
CEWSRecurrence *
From(ews2__TaskRecurrenceType * item) {
	CEWSRecurrenceGsoapImpl * p = new CEWSRecurrenceGsoapImpl;

	p->SetRecurrenceRange(RangeFrom(item->__union_TaskRecurrenceType_,
	                                &item->union_TaskRecurrenceType_));
	p->SetRecurrencePattern(PatternFrom(item->__union_TaskRecurrenceType,
	                                    &item->union_TaskRecurrenceType));
	return p;
}

void
CEWSTaskItemGsoapImpl::FromItemType(class ews2__ItemType * pItem) {
	CEWSItemGsoapImpl::FromItemType(pItem);

	ews2__TaskType * taskItem = dynamic_cast<ews2__TaskType*>(pItem);
    
	SET_PROPERTY(taskItem, this, ActualWork);
	SET_PROPERTY(taskItem, this, AssignedTime);
	SET_PROPERTY_STR(taskItem, this, BillingInformation);
	SET_PROPERTY(taskItem, this, ChangeCount);
	ARRAY_OF_STRING_TYPE_TO_STRING_LIST(taskItem, this, Companies);
	SET_PROPERTY(taskItem, this, CompleteDate);
	ARRAY_OF_STRING_TYPE_TO_STRING_LIST(taskItem, this, Contacts);
	SET_PROPERTY_2(taskItem, this, DelegationState, enum DelegateStateTypeEnum);
	SET_PROPERTY_STR(taskItem, this, Delegator);
	SET_PROPERTY(taskItem, this, DueDate);
	SET_PROPERTY(taskItem, this, IsAssignmentEditable);
	SET_PROPERTY_B(taskItem, this, Complete);
	SET_PROPERTY_B(taskItem, this, Recurring);
	SET_PROPERTY_B(taskItem, this, TeamTask);
	SET_PROPERTY_STR(taskItem, this, Mileage);
	SET_PROPERTY_STR(taskItem, this, Owner);
	SET_PROPERTY(taskItem, this, PercentComplete);

	if (taskItem->Recurrence) {
		SetRecurrence(From(taskItem->Recurrence));
	}

	SET_PROPERTY(taskItem, this, StartDate);
	SET_PROPERTY_2(taskItem, this, Status, enum StatusTypeEnum);
	SET_PROPERTY_STR(taskItem, this, StatusDescription);
	SET_PROPERTY(taskItem, this, TotalWork);
}

static
void FromRecurrence(ews2__TaskRecurrenceType * recurrence,
                    CEWSRecurrence * ewsRecurrence,
                    CEWSObjectPool * pObjPool) {
	FromRecurrenceRange(ewsRecurrence->GetRecurrenceRange(),
	                    recurrence->__union_TaskRecurrenceType_,
	                    recurrence->union_TaskRecurrenceType_,
	                    pObjPool);

	FromRecurrencePattern(ewsRecurrence->GetRecurrencePattern(),
	                      recurrence->__union_TaskRecurrenceType,
	                      recurrence->union_TaskRecurrenceType,
	                      pObjPool);
}

TaskItemTypeBuilder::TaskItemTypeBuilder(soap * soap,
                                         const CEWSTaskItem * pEWSItem,
                                         CEWSObjectPool * pObjPool)
	: ItemTypeBuilder(soap, pEWSItem, pObjPool) {
}

TaskItemTypeBuilder::~TaskItemTypeBuilder() {
}

ews2__ItemType * TaskItemTypeBuilder::CreateItemType() {
	return new ews2__TaskType();
}

ews2__ItemType * TaskItemTypeBuilder::Build() {
	//will setup the mime content
	ews2__ItemType * pItem = ItemTypeBuilder::Build();

	if (!pItem)
		return NULL;

	ews2__TaskType * pTask =
			dynamic_cast<ews2__TaskType*>(pItem);
	const CEWSTaskItem * pEwsTask =
			dynamic_cast<const CEWSTaskItem*>(m_pEWSItem);

	if (!pTask || !pEwsTask) {
		delete pItem;
		return NULL;
	}

#define pEwsCalendar pEwsTask
#define pCalendar pTask
    
	FROM_PROPERTY_3(ActualWork, int, 0);
	FROM_PROPERTY_3(AssignedTime, time_t, 0);
	FROM_PROPERTY_STR(BillingInformation);
	FROM_PROPERTY_3(ChangeCount, int, 0);
	STRING_LIST_TO_ARRAY_OF_STRING_TYPE(pTask, pEwsTask, Companies);
	FROM_PROPERTY_3(CompleteDate, time_t, 0);
	STRING_LIST_TO_ARRAY_OF_STRING_TYPE(pTask, pEwsTask, Contacts);
	// FROM_PROPERTY_2(DelegationState, enum ews2__TaskDelegateStateType);
	FROM_PROPERTY_STR(Delegator);
	FROM_PROPERTY_3(DueDate, time_t, 0);
	FROM_PROPERTY_3(IsAssignmentEditable, int, 0);
	// FROM_PROPERTY_BOOL(IsComplete);
	// FROM_PROPERTY_BOOL(IsRecurring);
	// FROM_PROPERTY_BOOL(IsTeamTask);
	FROM_PROPERTY_STR(Mileage);
	FROM_PROPERTY_STR(Owner);
	FROM_PROPERTY_3(PercentComplete, double, 0);
	FROM_RECURRENCE_2(Recurrence, ews2__TaskRecurrenceType);
	FROM_PROPERTY_3(StartDate, time_t, 0);
	FROM_PROPERTY_2(Status, enum ews2__TaskStatusType);
	FROM_PROPERTY_STR(StatusDescription);
	FROM_PROPERTY_3(TotalWork, int, 0);
    
	return pItem;
}

bool
CEWSTaskOperationGsoapImpl::UpdateTask(CEWSTaskItem * pEwsItem,
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

	return __UpdateTask(pEwsItem,
	                    flags,
	                    &ItemChange,
	                    pError);
}

bool
CEWSTaskOperationGsoapImpl::UpdateTaskOccurrence(CEWSTaskItem * pEwsItem,
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

	return __UpdateTask(pEwsItem,
	                    flags,
	                    &ItemChange,
	                    pError);
}

static
void
CreateItemChangeDescBody(CEWSTaskItem * pItem,
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

	SetItemField->__union_SetItemFieldType_ = SOAP_UNION__ews2__union_SetItemFieldType__Task;

	ews2__TaskType * itemType =
			pObjPool->Create<ews2__TaskType>();
	SetItemField->union_SetItemFieldType_.Task = itemType;

	ews2__BodyType * Body =
			pObjPool->Create<ews2__BodyType>();

	itemType->Body = Body;
	
	Body->BodyType = ews2__BodyTypeType__HTML;
	Body->__item = pItem->GetBody().GetData();
}

static
void
CreateItemChangeDescDateTime(CEWSTaskItem * pItem,
                             int start,
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

	switch (start) {
	case 1:
		FieldURI->FieldURI =
				ews2__UnindexedFieldURIType__task_x003aStartDate;
		break;
	case 0:
		FieldURI->FieldURI =
				ews2__UnindexedFieldURIType__task_x003aDueDate;
		break;
	default:
		FieldURI->FieldURI =
				ews2__UnindexedFieldURIType__task_x003aCompleteDate;
		break;
	}

	SetItemField->__union_SetItemFieldType_ = SOAP_UNION__ews2__union_SetItemFieldType__Task;

	ews2__TaskType * itemType =
			pObjPool->Create<ews2__TaskType>();
	SetItemField->union_SetItemFieldType_.Task = itemType;

	time_t * dt =
			pObjPool->Create<time_t>();

	switch (start) {
	case 1:
		itemType->StartDate = dt;
		*dt = pItem->GetStartDate();
		break;
	case 0:
		itemType->DueDate = dt;
		*dt = pItem->GetDueDate();
		break;
	default:
		itemType->CompleteDate = dt;
		*dt = pItem->GetCompleteDate();
		break;
	}
}

static
void
CreateItemChangeDescSubject(CEWSTaskItem * pItem,
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

	SetItemField->__union_SetItemFieldType_ = SOAP_UNION__ews2__union_SetItemFieldType__Task;

	ews2__TaskType * itemType =
			pObjPool->Create<ews2__TaskType>();
	SetItemField->union_SetItemFieldType_.Task = itemType;

	std::string * v =
			pObjPool->Create<std::string>(pItem->GetSubject().GetData());

	itemType->Subject = v;
}

static
void
CreateItemChangeDescStatus(CEWSTaskItem * pItem,
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
	FieldURI->FieldURI = ews2__UnindexedFieldURIType__task_x003aStatus;

	SetItemField->__union_SetItemFieldType_ = SOAP_UNION__ews2__union_SetItemFieldType__Task;

	ews2__TaskType * itemType =
			pObjPool->Create<ews2__TaskType>();
	SetItemField->union_SetItemFieldType_.Task = itemType;

	enum ews2__TaskStatusType * v =
			pObjPool->Create<enum ews2__TaskStatusType>();

	*v = (enum ews2__TaskStatusType)pItem->GetStatus();
	itemType->Status = v;
}

static
void
CreateItemChangeDescPercent(CEWSTaskItem * pItem,
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
	FieldURI->FieldURI = ews2__UnindexedFieldURIType__task_x003aPercentComplete;

	SetItemField->__union_SetItemFieldType_ = SOAP_UNION__ews2__union_SetItemFieldType__Task;

	ews2__TaskType * itemType =
			pObjPool->Create<ews2__TaskType>();
	SetItemField->union_SetItemFieldType_.Task = itemType;

	double * v =
			pObjPool->Create<double>();

	*v = pItem->GetPercentComplete();
	itemType->PercentComplete = v;
}

static
void
CreateItemChangeDescs(CEWSTaskItem * pItem,
                      int flags,
                      std::vector<__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType> & ItemChangeDescs,
                      CEWSObjectPool * pObjPool) {
	if ((flags & CEWSTaskOperation::Body) == CEWSTaskOperation::Body) {
		CreateItemChangeDescBody(pItem,
		                     ItemChangeDescs,
		                     pObjPool);
	}
	if ((flags & CEWSTaskOperation::Start) == CEWSTaskOperation::Start) {
		CreateItemChangeDescDateTime(pItem,
		                             1,
		                             ItemChangeDescs,
		                             pObjPool);
	}
	if ((flags & CEWSTaskOperation::End) == CEWSTaskOperation::End) {
		CreateItemChangeDescDateTime(pItem,
		                             0,
		                             ItemChangeDescs,
		                             pObjPool);
    }
	if ((flags & CEWSTaskOperation::End) == CEWSTaskOperation::Complete) {
		CreateItemChangeDescDateTime(pItem,
		                             2,
		                             ItemChangeDescs,
		                             pObjPool);
	}
	if ((flags & CEWSTaskOperation::Subject) == CEWSTaskOperation::Subject) {
		CreateItemChangeDescSubject(pItem,
                                    ItemChangeDescs,
                                    pObjPool);
	}
	if ((flags & CEWSTaskOperation::Status) == CEWSTaskOperation::Status) {
		CreateItemChangeDescStatus(pItem,
                                    ItemChangeDescs,
                                    pObjPool);
	}
	if ((flags & CEWSTaskOperation::Percent) == CEWSTaskOperation::Percent) {
		CreateItemChangeDescPercent(pItem,
                                    ItemChangeDescs,
                                    pObjPool);
	}
}

bool
CEWSTaskOperationGsoapImpl::__UpdateTask(CEWSTaskItem * pItem,
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

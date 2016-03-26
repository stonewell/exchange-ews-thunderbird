/*
 * ews_item_gsoap_impl.cpp
 *
 *  Created on: Jun 16, 2014
 *      Author: stone
 */

#include "libews_gsoap.h"
#include <memory>
#include <map>
#include <vector>

using namespace ews;

void CEWSItemGsoapImpl::UpdateAttachments(
    ews2__NonEmptyArrayOfAttachmentsType * pAttachments) {
	if (!pAttachments->__size_NonEmptyArrayOfAttachmentsType)
		return;

	__ews2__union_NonEmptyArrayOfAttachmentsType * arrayOfAttachmentsType =
			pAttachments->__union_NonEmptyArrayOfAttachmentsType;

	for (int i = 0; i < pAttachments->__size_NonEmptyArrayOfAttachmentsType;
	     i++) {
		switch (arrayOfAttachmentsType->__union_NonEmptyArrayOfAttachmentsType) {
		case SOAP_UNION__ews2__union_NonEmptyArrayOfAttachmentsType_ItemAttachment:
			AddAttachment(
			    CEWSAttachmentGsoapImpl::CreateInstance(
			        CEWSAttachment::Attachment_Item,
			        arrayOfAttachmentsType->union_NonEmptyArrayOfAttachmentsType.ItemAttachment));
			break;
		case SOAP_UNION__ews2__union_NonEmptyArrayOfAttachmentsType_FileAttachment:
			AddAttachment(
			    CEWSAttachmentGsoapImpl::CreateInstance(
			        CEWSAttachment::Attachment_File,
			        arrayOfAttachmentsType->union_NonEmptyArrayOfAttachmentsType.FileAttachment));
			break;
		default:
			break;
		}
		arrayOfAttachmentsType++;
	}
}

#define SET_VALUE(t, p)	  \
	if (pItem->p) \
		Set##p((t)(*pItem->p));

#define SET_VALUE_B(t, p)	  \
	if (pItem->Is##p) \
		Set##p((t)(*pItem->Is##p));

#define SET_VALUE_STR(p)	  \
	if (pItem->p) \
		Set##p(pItem->p->c_str());

void CEWSItemGsoapImpl::FromItemType(ews2__ItemType * pItem) {
	if (pItem->MimeContent) {
		SetMimeContent(pItem->MimeContent->__item.c_str());
	}
    
	if (pItem->ItemId) {
		SetItemId(pItem->ItemId->Id.c_str());
      
		if (pItem->ItemId->ChangeKey)
			SetChangeKey(pItem->ItemId->ChangeKey->c_str());
	}

	if (pItem->ParentFolderId) {
		SetParentFolderId(pItem->ParentFolderId->Id.c_str());

		if (pItem->ParentFolderId->ChangeKey)
			SetParentFolderChangeKey(pItem->ParentFolderId->ChangeKey->c_str());
	}

	if (pItem->HasAttachments && *pItem->HasAttachments) {
		SetHasAttachments(*(pItem->HasAttachments));

		if (pItem->Attachments) {
			UpdateAttachments(pItem->Attachments);
		}
	} else {
		SetHasAttachments(false);
	}

	if (pItem->Body) {
		SetBodyType(
		    pItem->Body->BodyType == ews2__BodyTypeType__HTML ?
		    CEWSItem::BODY_HTML : CEWSItem::BODY_TEXT);
		SetBody(pItem->Body->__item.c_str());
	}

	if (pItem->DateTimeCreated) {
		SetCreateTime(*pItem->DateTimeCreated);
	}

	if (pItem->DateTimeReceived) {
		SetReceivedTime(*pItem->DateTimeReceived);
	}

	if (pItem->DateTimeSent) {
		SetSentTime(*pItem->DateTimeSent);
	}

	if (pItem->Subject) {
		SetSubject(pItem->Subject->c_str());
	}

	SET_VALUE_STR(ItemClass);
	SET_VALUE(enum EWSSensitivityTypeEnum, Sensitivity);

	SET_VALUE(int, Size);
	SET_VALUE(enum EWSImportanceTypeEnum, Importance);
    
	SET_VALUE_STR(InReplyTo);

	SET_VALUE_B(bool, Submitted);
	SET_VALUE_B(bool, Draft);
	SET_VALUE_B(bool, FromMe);
	SET_VALUE_B(bool, Resend);
	SET_VALUE_B(bool, Unmodified);

	SET_VALUE(time_t, ReminderDueBy);
	SET_VALUE(bool, ReminderIsSet);
	SET_VALUE_STR(ReminderMinutesBeforeStart);
    
	SET_VALUE_STR(DisplayCc);
	SET_VALUE_STR(DisplayTo);

	SET_VALUE_STR(LastModifiedName);
	SET_VALUE(time_t, LastModifiedTime);

	if (pItem->Categories) {
		std::vector<std::string>::iterator it =
				pItem->Categories->String.begin();

		std::auto_ptr<CEWSStringList> pList(new CEWSStringList);

		while(it != pItem->Categories->String.end()) {
			pList->push_back((*it).c_str());

			it++;
		}

		SetCategories(pList.release());
	}

	if (pItem->InternetMessageHeaders) {
		std::vector<class ews2__InternetHeaderType * >::iterator it =
				pItem->InternetMessageHeaders->InternetMessageHeader.begin();

		std::auto_ptr<CEWSInternetMessageHeaderList> pList(new CEWSInternetMessageHeaderList);
		while(it != pItem->InternetMessageHeaders->InternetMessageHeader.end()) {
			ews2__InternetHeaderType * t = (*it);

			CEWSInternetMessageHeader * p =
					new CEWSInternetMessageHeaderGsoapImpl;

			p->SetValue(t->__item.c_str());
			p->SetHeaderName(t->HeaderName.c_str());

			pList->push_back(p);
			it++;
		}

		SetInternetMessageHeaders(pList.release());
	}
}

CEWSItemGsoapImpl * CEWSItemGsoapImpl::CreateInstance(EWSItemType itemType,
                                                      ews2__ItemType * pItem) {
	std::auto_ptr<CEWSItemGsoapImpl> pEWSItem(NULL);

	if (itemType == CEWSItem::Item_Message) {
		pEWSItem.reset(new CEWSMessageItemGsoapImpl());
	} else if (itemType == CEWSItem::Item_Contact) {
		pEWSItem.reset(new CEWSContactItemGsoapImpl());
	} else if (itemType == CEWSItem::Item_Calendar) {
		pEWSItem.reset(new CEWSCalendarItemGsoapImpl());
	} else if (itemType == CEWSItem::Item_Task) {
		pEWSItem.reset(new CEWSTaskItemGsoapImpl());
	}

	if (pEWSItem.get() && pItem) {
		pEWSItem->FromItemType(pItem);
	}

	if (pEWSItem.get()) {
		pEWSItem->SetItemType(itemType);
	}
	return pEWSItem.release();
}

CEWSItem * CEWSItem::CreateInstance(EWSItemType itemType) {
	return CEWSItemGsoapImpl::CreateInstance(itemType, NULL);
}

CEWSItemOperationGsoapImpl::CEWSItemOperationGsoapImpl(
    CEWSSessionGsoapImpl * pSession) :
	m_pSession(pSession) {

}

CEWSItemOperationGsoapImpl::~CEWSItemOperationGsoapImpl() {

}

bool CEWSItemOperationGsoapImpl::SyncItems(const CEWSFolder * pFolder,
                                           CEWSItemOperationCallback * callback, CEWSError * pError) {
	return SyncItems(pFolder->GetFolderId(), callback, pError);
}

bool
CEWSItemOperationGsoapImpl::SyncItems(CEWSFolder::EWSDistinguishedFolderIdNameEnum distinguishedFolderId,
                                      CEWSItemOperationCallback * callback,
                                      CEWSError * pError) {
	ews2__TargetFolderIdType targetFolderIdType;
	targetFolderIdType.__union_TargetFolderIdType =
			SOAP_UNION__ews2__union_TargetFolderIdType_DistinguishedFolderId;

	ews2__DistinguishedFolderIdType	folderId;
	folderId.Id =
			ConvertDistinguishedFolderIdName(distinguishedFolderId);
	_ews2__union_TargetFolderIdType * union_TargetFolderIdType =
			&targetFolderIdType.union_TargetFolderIdType;
	union_TargetFolderIdType->DistinguishedFolderId = &folderId;

	ews2__ItemResponseShapeType * pItemShape =
			soap_new_req_ews2__ItemResponseShapeType(m_pSession->GetProxy(),
			                                         ews2__DefaultShapeNamesType__IdOnly);

	return __SyncItems(&targetFolderIdType,
	                   pItemShape,
	                   callback,
	                   pError);
}

bool CEWSItemOperationGsoapImpl::SyncItems(const CEWSString & strFolderId,
                                           CEWSItemOperationCallback * callback, CEWSError * pError) {
	ews2__TargetFolderIdType targetFolderIdType;
	targetFolderIdType.__union_TargetFolderIdType =
			SOAP_UNION__ews2__union_TargetFolderIdType_FolderId;

	ews2__FolderIdType folderId;
	folderId.Id = strFolderId;
	_ews2__union_TargetFolderIdType * union_TargetFolderIdType =
			&targetFolderIdType.union_TargetFolderIdType;
	union_TargetFolderIdType->FolderId = &folderId;

	ews2__ItemResponseShapeType * pItemShape =
			soap_new_req_ews2__ItemResponseShapeType(m_pSession->GetProxy(),
			                                         ews2__DefaultShapeNamesType__IdOnly);

	return __SyncItems(&targetFolderIdType,
	                   pItemShape,
	                   callback,
	                   pError);
}

bool
CEWSItemOperationGsoapImpl::__SyncItems(ews2__TargetFolderIdType * targetFolderIdType,
                                        ews2__ItemResponseShapeType * pItemShape,
                                        CEWSItemOperationCallback * callback,
                                        CEWSError * pError) {
	CEWSSessionRequestGuard guard(m_pSession);

	__ews1__SyncFolderItemsResponse response;

	ews1__SyncFolderItemsType * pFolderItemsType =
			soap_new_req_ews1__SyncFolderItemsType(m_pSession->GetProxy(),
			                                       pItemShape, targetFolderIdType,
			                                       callback->GetSyncMaxReturnItemCount());

	std::string syncState = callback->GetSyncState().GetData();
	if (!callback->GetSyncState().IsEmpty())
		pFolderItemsType->SyncState = &syncState;

	const CEWSStringList * pIgnoreList = &callback->GetIgnoreItemIdList();
	__ews2__union_ArrayOfBaseItemIdsType * itemIdsType = NULL;

	if (pIgnoreList->size() > 0) {
		ews2__ArrayOfBaseItemIdsType * ignoreIds =
				soap_new_ews2__ArrayOfBaseItemIdsType(m_pSession->GetProxy());
		ignoreIds->__size_ArrayOfBaseItemIdsType = pIgnoreList->size();
		__ews2__union_ArrayOfBaseItemIdsType * tmp =
				itemIdsType =
				ignoreIds->__union_ArrayOfBaseItemIdsType =
				new __ews2__union_ArrayOfBaseItemIdsType [pIgnoreList->size()];

		CEWSStringList::const_iterator cit = pIgnoreList->begin();
		while (cit != pIgnoreList->end()) {
			tmp->__union_ArrayOfBaseItemIdsType =
					SOAP_UNION__ews2__union_ArrayOfBaseItemIdsType_ItemId;
			tmp->union_ArrayOfBaseItemIdsType.ItemId =
					soap_new_ews2__ItemIdType(m_pSession->GetProxy());
			tmp->union_ArrayOfBaseItemIdsType.ItemId->Id = *cit;

			cit++;
			tmp++;
		}

		pFolderItemsType->Ignore = ignoreIds;
	}

	int ret = m_pSession->GetProxy()->SyncFolderItems(pFolderItemsType,
	                                                  response);

	if (itemIdsType)
		delete[] itemIdsType;

	if (ret != SOAP_OK) {
		if (pError) {
			pError->SetErrorMessage(m_pSession->GetErrorMsg());
			pError->SetErrorCode(ret);
		}

		return false;
	}

	ews1__ArrayOfResponseMessagesType * messages =
			response.ews1__SyncFolderItemsResponse->ResponseMessages;
	for (int i = 0; i < messages->__size_ArrayOfResponseMessagesType; i++) {
		__ews1__union_ArrayOfResponseMessagesType * message =
				messages->__union_ArrayOfResponseMessagesType + i;

		if (message->__union_ArrayOfResponseMessagesType
		    !=
		    SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_SyncFolderItemsResponseMessage) {
			continue;
		}

		ews1__SyncFolderItemsResponseMessageType * syncItemsResponseMessage =
				message->union_ArrayOfResponseMessagesType.SyncFolderItemsResponseMessage;

		if (syncItemsResponseMessage->ResponseClass
		    != ews2__ResponseClassType__Success) {
			SAVE_ERROR(pError,
			           syncItemsResponseMessage);

			return false;
		}

		if (syncItemsResponseMessage->SyncState)
			callback->SetSyncState(syncItemsResponseMessage->SyncState->c_str());

		if (!syncItemsResponseMessage->Changes)
			continue;

		for (int j = 0;
		     j
				     < syncItemsResponseMessage->Changes->__size_SyncFolderItemsChangesType;
		     j++) {
			__ews2__union_SyncFolderItemsChangesType * unionFolderChange =
					syncItemsResponseMessage->Changes->__union_SyncFolderItemsChangesType
					+ j;
			ews2__SyncFolderItemsCreateOrUpdateType * createOrUpdateFolder =
					NULL;

			switch (unionFolderChange->__union_SyncFolderItemsChangesType) {
			case SOAP_UNION__ews2__union_SyncFolderItemsChangesType_Create:
				createOrUpdateFolder =
						unionFolderChange->union_SyncFolderItemsChangesType.Create;
				break;
			case SOAP_UNION__ews2__union_SyncFolderItemsChangesType_Update:
				createOrUpdateFolder =
						unionFolderChange->union_SyncFolderItemsChangesType.Update;
				break;
			case SOAP_UNION__ews2__union_SyncFolderItemsChangesType_Delete: {
				ews2__SyncFolderItemsDeleteType * deleteType =
						unionFolderChange->union_SyncFolderItemsChangesType.Delete;

				std::auto_ptr<CEWSItem> pItem(
				    CEWSItem::CreateInstance(CEWSItem::Item_Message));
				pItem->SetItemId(deleteType->ItemId->Id.c_str());
				if (deleteType->ItemId->ChangeKey) {
					pItem->SetChangeKey(deleteType->ItemId->ChangeKey->c_str());
				}
				callback->DeleteItem(pItem.get());
			}
				continue;
			case SOAP_UNION__ews2__union_SyncFolderItemsChangesType_ReadFlagChange: {
				ews2__SyncFolderItemsReadFlagType * readflagType =
						unionFolderChange->union_SyncFolderItemsChangesType.ReadFlagChange;

				std::auto_ptr<CEWSItem> pItem(
				    CEWSItem::CreateInstance(CEWSItem::Item_Message));
				pItem->SetItemId(readflagType->ItemId->Id.c_str());
				if (readflagType->ItemId->ChangeKey) {
					pItem->SetChangeKey(readflagType->ItemId->ChangeKey->c_str());
				}
				callback->ReadItem(pItem.get(), readflagType->IsRead);
			}
				continue;
			default:
				continue;
			}

			CEWSItem::EWSItemType itemType = CEWSItem::Item_Message;
			ews2__ItemType * pItem = NULL;
			_ews2__union_SyncFolderItemsCreateOrUpdateType * createOrUpdate =
					&createOrUpdateFolder->union_SyncFolderItemsCreateOrUpdateType;

			switch (createOrUpdateFolder->__union_SyncFolderItemsCreateOrUpdateType) {
			case SOAP_UNION__ews2__union_SyncFolderItemsCreateOrUpdateType_Message:
				itemType = CEWSItem::Item_Message;
				pItem = createOrUpdate->Message;
				break;
			case SOAP_UNION__ews2__union_SyncFolderItemsCreateOrUpdateType_MeetingMessage:
				itemType = CEWSItem::Item_Message;
				pItem = createOrUpdate->MeetingMessage;
				break;
			case SOAP_UNION__ews2__union_SyncFolderItemsCreateOrUpdateType_MeetingRequest:
				itemType = CEWSItem::Item_Message;
				pItem = createOrUpdate->MeetingRequest;
				break;
			case SOAP_UNION__ews2__union_SyncFolderItemsCreateOrUpdateType_MeetingResponse:
				itemType = CEWSItem::Item_Message;
				pItem = createOrUpdate->MeetingResponse;
				break;
			case SOAP_UNION__ews2__union_SyncFolderItemsCreateOrUpdateType_MeetingCancellation:
				itemType = CEWSItem::Item_Message;
				pItem = createOrUpdate->MeetingCancellation;
				break;
			case SOAP_UNION__ews2__union_SyncFolderItemsCreateOrUpdateType_CalendarItem:
				itemType = CEWSItem::Item_Calendar;
				pItem = createOrUpdate->CalendarItem;
				break;
			case SOAP_UNION__ews2__union_SyncFolderItemsCreateOrUpdateType_Task:
				itemType = CEWSItem::Item_Task;
				pItem = createOrUpdate->Task;
				break;
			default:
				continue;
			}

			std::auto_ptr<CEWSItemGsoapImpl> pEWSItem(
			    CEWSItemGsoapImpl::CreateInstance(itemType, pItem));

			if (unionFolderChange->__union_SyncFolderItemsChangesType
			    == SOAP_UNION__ews2__union_SyncFolderItemsChangesType_Create) {
				callback->NewItem(pEWSItem.get());
			} else if (unionFolderChange->__union_SyncFolderItemsChangesType
			           == SOAP_UNION__ews2__union_SyncFolderItemsChangesType_Update) {
				callback->UpdateItem(pEWSItem.get());
			}
		} //for all items change
	} //for all response message

	return true;
}

void CEWSItemGsoapImpl::AddAttachment(CEWSAttachment * pAttachment) {
	GetAttachments()->push_back(pAttachment);
}

void CEWSItemGsoapImpl::RemoveAttachment(CEWSAttachment * pAttachment) {
	CEWSAttachmentList::iterator it;

	for (it = GetAttachments()->begin(); it != GetAttachments()->end(); it++) {
		if (*it == pAttachment || pAttachment->IsSame(*it)) {
			GetAttachments()->erase(it);
			break;
		}
	}
}

void CEWSItemGsoapImpl::ClearAttachments() {
	GetAttachments()->Clear();
}

CEWSItem * CEWSItemOperationGsoapImpl::GetItem(const CEWSString & itemId,
                                               int flags,
                                               CEWSError * pError) {
	CEWSItemList itemList;
	CEWSStringList itemIdList;
	itemIdList.push_back(itemId);

	if (GetItems(itemIdList, &itemList, flags, pError)
	    && itemList.size() > 0) {
		CEWSItem * pItem = *itemList.begin();
		itemList.erase(itemList.begin());

		return pItem;
	}

	return NULL;
}

template<class T>
class ArrayWrapper {
private:
	T * m_ptr;
public:
	ArrayWrapper(int size) {
		m_ptr = new T[size];
	}

	~ArrayWrapper() {
		delete[] m_ptr;
	}

	T * get() {
		return m_ptr;
	}
};

typedef std::vector<enum ews2__UnindexedFieldURIType> FieldVector;
typedef std::vector<ews2__PathToUnindexedFieldType> PathToFieldVector;
typedef std::vector<__ews2__union_NonEmptyArrayOfPathsToElementType> PathToElementVector;

static
void ProcessGetItemsFlags(int flags,
                          FieldVector & fieldVector,
                          bool includeMimeContent) {
	const enum ews2__UnindexedFieldURIType mimeFieldUriTypes[] = {
		ews2__UnindexedFieldURIType__item_x003aMimeContent,
	};

	const size_t size_mime_fields = sizeof(mimeFieldUriTypes) / sizeof(enum ews2__UnindexedFieldURIType);
	includeMimeContent = (flags & EWS_GetItems_Flags_MimeContent) ==
			EWS_GetItems_Flags_MimeContent;

	if (includeMimeContent) {
		for (size_t i = 0; i < size_mime_fields; i++) {
			fieldVector.push_back(mimeFieldUriTypes[i]);
		}
	}
	
	const enum ews2__UnindexedFieldURIType msgFieldUriTypes[] = {
		ews2__UnindexedFieldURIType__item_x003aSubject,
		ews2__UnindexedFieldURIType__item_x003aDateTimeReceived,
		ews2__UnindexedFieldURIType__item_x003aHasAttachments,
		ews2__UnindexedFieldURIType__message_x003aIsRead,
		ews2__UnindexedFieldURIType__message_x003aInternetMessageId,
	};
	const size_t size_msg_fields = sizeof(msgFieldUriTypes) / sizeof(enum ews2__UnindexedFieldURIType);
	bool includeMsgField = (flags & EWS_GetItems_Flags_MessageItem) ==
			EWS_GetItems_Flags_MessageItem;
    
	if (includeMsgField) {
		for (size_t i = 0; i < size_msg_fields; i++) {
			fieldVector.push_back(msgFieldUriTypes[i]);
		}
	}

	const enum ews2__UnindexedFieldURIType calendarFieldUriTypes[] = {
		ews2__UnindexedFieldURIType__calendar_x003aCalendarItemType,
		ews2__UnindexedFieldURIType__calendar_x003aRecurrenceId,
		ews2__UnindexedFieldURIType__calendar_x003aRequiredAttendees,
		ews2__UnindexedFieldURIType__calendar_x003aOptionalAttendees,
		ews2__UnindexedFieldURIType__calendar_x003aResources,
		ews2__UnindexedFieldURIType__calendar_x003aMyResponseType,
		ews2__UnindexedFieldURIType__calendar_x003aUID,
		ews2__UnindexedFieldURIType__calendar_x003aModifiedOccurrences,
		ews2__UnindexedFieldURIType__calendar_x003aDeletedOccurrences,
		ews2__UnindexedFieldURIType__calendar_x003aMeetingTimeZone,
		ews2__UnindexedFieldURIType__calendar_x003aTimeZone,
	};

	const size_t size_calendar_fields = sizeof(calendarFieldUriTypes) / sizeof(enum ews2__UnindexedFieldURIType);
	bool includeCalendarField = (flags & EWS_GetItems_Flags_CalendarItem) ==
			EWS_GetItems_Flags_CalendarItem;
	if (includeCalendarField) {
		for (size_t i = 0; i < size_calendar_fields; i++) {
			fieldVector.push_back(calendarFieldUriTypes[i]);
		}
	}
}

bool CEWSItemOperationGsoapImpl::GetItems(const CEWSStringList & itemIds,
                                          CEWSItemList * pItemList,
                                          int flags,
                                          CEWSError * pError) {
	CEWSSessionRequestGuard guard(m_pSession);

	FieldVector fieldVector;
	bool includeMimeContent = false;

	ProcessGetItemsFlags(flags, fieldVector, includeMimeContent);
    

	ews2__NonEmptyArrayOfPathsToElementType AdditionalProperties;
	AdditionalProperties.__size_NonEmptyArrayOfPathsToElementType =
			fieldVector.size();

	PathToFieldVector PathToUnindexedFieldType(fieldVector.size());

	for(int i =0;i < fieldVector.size(); i++) {
		PathToUnindexedFieldType[i].FieldURI = fieldVector[i];
	}

	PathToElementVector pathsToElementType(fieldVector.size());
    
	for (int i=0; i< fieldVector.size(); i++) {
		pathsToElementType[i].__union_NonEmptyArrayOfPathsToElementType = SOAP_UNION__ews2__union_NonEmptyArrayOfPathsToElementType_FieldURI;
		pathsToElementType[i].union_NonEmptyArrayOfPathsToElementType.FieldURI = &PathToUnindexedFieldType[i];
	}

	AdditionalProperties.__union_NonEmptyArrayOfPathsToElementType =
			&pathsToElementType[0];

	__ews1__GetItemResponse response;

	ews2__ItemResponseShapeType itemShape;
	if ((flags & EWS_GetItems_Flags_IdOnly) == EWS_GetItems_Flags_IdOnly) {
		itemShape.BaseShape = ews2__DefaultShapeNamesType__IdOnly;
	}
	else if ((flags & EWS_GetItems_Flags_AllProperties) == EWS_GetItems_Flags_AllProperties) {
		itemShape.BaseShape = ews2__DefaultShapeNamesType__AllProperties;
	} else {
		itemShape.BaseShape = ews2__DefaultShapeNamesType__Default;
	}
	ews2__BodyTypeResponseType bodyType = ews2__BodyTypeResponseType__Best;
	itemShape.BodyType = &bodyType;
	itemShape.IncludeMimeContent = &includeMimeContent;

	if (pathsToElementType.size() > 0) {
		itemShape.AdditionalProperties = &AdditionalProperties;
	}

	ews1__GetItemType itemType;
	itemType.ItemShape = &itemShape;
	std::vector<__ews2__union_NonEmptyArrayOfBaseItemIdsType> itemIdsType(itemIds.size());
	ews2__NonEmptyArrayOfBaseItemIdsType baseItemIdsType;
    
	if (itemIds.size() > 0) {
		itemType.ItemIds = &baseItemIdsType;
		itemType.ItemIds->__size_NonEmptyArrayOfBaseItemIdsType =
				itemIds.size();
		itemType.ItemIds->__union_NonEmptyArrayOfBaseItemIdsType =
				&itemIdsType[0];

		CEWSStringList::const_iterator cit = itemIds.begin();
		std::vector<__ews2__union_NonEmptyArrayOfBaseItemIdsType>::iterator tmp
				= itemIdsType.begin();
		while (cit != itemIds.end()) {
			if ((flags & EWS_GetItems_Flags_RecurringMasterItem) ==
			    EWS_GetItems_Flags_RecurringMasterItem) {
				tmp->__union_NonEmptyArrayOfBaseItemIdsType =
						SOAP_UNION__ews2__union_NonEmptyArrayOfBaseItemIdsType_RecurringMasterItemId;
				tmp->union_NonEmptyArrayOfBaseItemIdsType.RecurringMasterItemId =
						soap_new_ews2__RecurringMasterItemIdType(m_pSession->GetProxy());
				tmp->union_NonEmptyArrayOfBaseItemIdsType.RecurringMasterItemId->OccurrenceId
						= *cit;
			} else if ((flags & EWS_GetItems_Flags_OccurrenceItem) ==
			           EWS_GetItems_Flags_OccurrenceItem) {
				tmp->__union_NonEmptyArrayOfBaseItemIdsType =
						SOAP_UNION__ews2__union_NonEmptyArrayOfBaseItemIdsType_OccurrenceItemId;
				tmp->union_NonEmptyArrayOfBaseItemIdsType.OccurrenceItemId =
						soap_new_ews2__OccurrenceItemIdType(m_pSession->GetProxy());
				tmp->union_NonEmptyArrayOfBaseItemIdsType.OccurrenceItemId->RecurringMasterId
						= *cit;
				tmp->union_NonEmptyArrayOfBaseItemIdsType.OccurrenceItemId->InstanceIndex
						= 0;
			} else {
				tmp->__union_NonEmptyArrayOfBaseItemIdsType =
						SOAP_UNION__ews2__union_NonEmptyArrayOfBaseItemIdsType_ItemId;
				tmp->union_NonEmptyArrayOfBaseItemIdsType.ItemId =
						soap_new_ews2__ItemIdType(m_pSession->GetProxy());
				tmp->union_NonEmptyArrayOfBaseItemIdsType.ItemId->Id = *cit;
			}
			cit++;
			tmp++;
		}

	}

	int ret = m_pSession->GetProxy()->GetItem(&itemType, response);

	if (ret != SOAP_OK) {
		if (pError) {
			pError->SetErrorMessage(m_pSession->GetErrorMsg());
			pError->SetErrorCode(ret);
		}

		return false;
	}

	ews1__ArrayOfResponseMessagesType * messages =
			response.ews1__GetItemResponse->ResponseMessages;
	for (int i = 0; i < messages->__size_ArrayOfResponseMessagesType; i++) {
		__ews1__union_ArrayOfResponseMessagesType * message =
				messages->__union_ArrayOfResponseMessagesType + i;

		if (message->__union_ArrayOfResponseMessagesType
		    !=
		    SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_GetItemResponseMessage) {
			continue;
		}

		ews1__ItemInfoResponseMessageType * syncItemsResponseMessage =
				message->union_ArrayOfResponseMessagesType.GetItemResponseMessage;

		if (syncItemsResponseMessage->ResponseClass
		    != ews2__ResponseClassType__Success) {
			SAVE_ERROR(pError,syncItemsResponseMessage);

			return false;
		}

		if (!syncItemsResponseMessage->Items)
			continue;

		for (int j = 0;
		     j < syncItemsResponseMessage->Items->__size_ArrayOfRealItemsType;
		     j++) {
			__ews2__union_ArrayOfRealItemsType * unionFolderChange =
					syncItemsResponseMessage->Items->__union_ArrayOfRealItemsType
					+ j;

			CEWSItem::EWSItemType itemType = CEWSItem::Item_Message;
			ews2__ItemType * pItem = NULL;

			switch (unionFolderChange->__union_ArrayOfRealItemsType) {
			case SOAP_UNION__ews2__union_ArrayOfRealItemsType_Message:
				itemType = CEWSItem::Item_Message;
				pItem = unionFolderChange->union_ArrayOfRealItemsType.Message;
				break;
			case SOAP_UNION__ews2__union_ArrayOfRealItemsType_MeetingMessage:
				itemType = CEWSItem::Item_Message;
				pItem = unionFolderChange->union_ArrayOfRealItemsType.MeetingMessage;
				break;
			case SOAP_UNION__ews2__union_ArrayOfRealItemsType_MeetingRequest:
				itemType = CEWSItem::Item_Message;
				pItem = unionFolderChange->union_ArrayOfRealItemsType.MeetingRequest;
				break;
			case SOAP_UNION__ews2__union_ArrayOfRealItemsType_MeetingResponse:
				itemType = CEWSItem::Item_Message;
				pItem = unionFolderChange->union_ArrayOfRealItemsType.MeetingResponse;
				break;
			case SOAP_UNION__ews2__union_ArrayOfRealItemsType_MeetingCancellation:
				itemType = CEWSItem::Item_Message;
				pItem = unionFolderChange->union_ArrayOfRealItemsType.MeetingCancellation;
				break;
			case SOAP_UNION__ews2__union_ArrayOfRealItemsType_CalendarItem:
				itemType = CEWSItem::Item_Calendar;
				pItem = unionFolderChange->union_ArrayOfRealItemsType.CalendarItem;
				break;
			case SOAP_UNION__ews2__union_ArrayOfRealItemsType_Task:
				itemType = CEWSItem::Item_Task;
				pItem = unionFolderChange->union_ArrayOfRealItemsType.Task;
				break;
			default:
				continue;
			}

			std::auto_ptr<CEWSItemGsoapImpl> pEWSItem(
			    CEWSItemGsoapImpl::CreateInstance(itemType, pItem));

			pItemList->push_back(pEWSItem.release());
		} //for all items change
	} //for all response message

	return true;
}

bool CEWSItemOperationGsoapImpl::CreateItem(CEWSItem * pItem,
                                            const CEWSFolder * pSaveFolder,
                                            CEWSError * pError) {
	return CreateItem(pItem, pSaveFolder, ews2__MessageDispositionType__SaveOnly, pError);
}

bool CEWSItemOperationGsoapImpl::CreateItem(CEWSItem * pItem,
                                            const CEWSFolder * pSaveFolder,
                                            ews2__MessageDispositionType messageDispositionType,
                                            CEWSError * pError) {
	ews1__CreateItemType createItemType;
	createItemType.MessageDisposition = &messageDispositionType;
	ews2__TargetFolderIdType targetFolderType;
	targetFolderType.__union_TargetFolderIdType =
			SOAP_UNION__ews2__union_TargetFolderIdType_FolderId;

	ews2__FolderIdType folderIdType;
	folderIdType.Id = pSaveFolder->GetFolderId();

	_ews2__union_TargetFolderIdType * union_TargetFolderIdType =
			&targetFolderType.union_TargetFolderIdType;
	union_TargetFolderIdType->FolderId = &folderIdType;

	createItemType.SavedItemFolderId = &targetFolderType;

	return CreateItem(pItem, &createItemType, pError);
}

bool CEWSItemOperationGsoapImpl::CreateItem(CEWSItem * pItem,
                                            CEWSFolder::EWSDistinguishedFolderIdNameEnum distinguishedFolderId,
                                            CEWSError * pError) {
	return CreateItem(pItem, 
	                  distinguishedFolderId, 
	                  ews2__MessageDispositionType__SaveOnly, 
	                  pError);
}

bool CEWSItemOperationGsoapImpl::CreateItem(CEWSItem * pItem,
                                            CEWSFolder::EWSDistinguishedFolderIdNameEnum distinguishedFolderId,
                                            ews2__MessageDispositionType messageDispositionType,
                                            CEWSError * pError) {
	ews1__CreateItemType createItemType;

	createItemType.MessageDisposition = &messageDispositionType;

	ews2__TargetFolderIdType targetFolderType;
	targetFolderType.__union_TargetFolderIdType =
			SOAP_UNION__ews2__union_TargetFolderIdType_DistinguishedFolderId;

	ews2__DistinguishedFolderIdType DistinguishedFolderIdType;
	DistinguishedFolderIdType.Id = ConvertDistinguishedFolderIdName(
	    distinguishedFolderId);

	_ews2__union_TargetFolderIdType * union_TargetFolderIdType =
			&targetFolderType.union_TargetFolderIdType;
	union_TargetFolderIdType->DistinguishedFolderId =
			&DistinguishedFolderIdType;

	createItemType.SavedItemFolderId = &targetFolderType;

	return CreateItem(pItem, &createItemType, pError);
}

bool CEWSItemOperationGsoapImpl::CreateItem(CEWSItem * pEWSItem,
                                            ews1__CreateItemType * pCreateItemType,
                                            CEWSError * pError) {
	CEWSSessionRequestGuard guard(m_pSession);

	__ews1__CreateItemResponse response;

	ews2__NonEmptyArrayOfAllItemsType arrayOfAllItemsType;
	arrayOfAllItemsType.__size_NonEmptyArrayOfAllItemsType = 1;

	__ews2__union_NonEmptyArrayOfAllItemsType union_NonEmptyArrayOfAllItemsType;
	std::auto_ptr<ews2__ItemType> pItemType(NULL);

	CEWSObjectPool objPool;

	enum ews2__CalendarItemCreateOrDeleteOperationType
			CalendarItemCreateOrDeleteOperationType =
			ews2__CalendarItemCreateOrDeleteOperationType__SendToNone;

	switch (pEWSItem->GetItemType()) {
	case CEWSItem::Item_Message: {
		union_NonEmptyArrayOfAllItemsType.__union_NonEmptyArrayOfAllItemsType =
				SOAP_UNION__ews2__union_NonEmptyArrayOfAllItemsType_Message;
		MessageItemTypeBuilder builder(m_pSession->GetProxy(),
		                               dynamic_cast<CEWSMessageItem*>(pEWSItem),
		                               &objPool);
		pItemType.reset(builder.Build());
	}
		break;
	case CEWSItem::Item_Calendar: {
		union_NonEmptyArrayOfAllItemsType.__union_NonEmptyArrayOfAllItemsType =
				SOAP_UNION__ews2__union_NonEmptyArrayOfAllItemsType_CalendarItem;
		CalendarItemTypeBuilder builder(m_pSession->GetProxy(),
		                                dynamic_cast<CEWSCalendarItem*>(pEWSItem),
		                                &objPool);
		pItemType.reset(builder.Build());
		pCreateItemType->SendMeetingInvitations =
				&CalendarItemCreateOrDeleteOperationType;
	}
		break;
	case CEWSItem::Item_Task: {
		union_NonEmptyArrayOfAllItemsType.__union_NonEmptyArrayOfAllItemsType =
				SOAP_UNION__ews2__union_NonEmptyArrayOfAllItemsType_Task;
		TaskItemTypeBuilder builder(m_pSession->GetProxy(),
                                    dynamic_cast<CEWSTaskItem*>(pEWSItem),
                                    &objPool);
		pItemType.reset(builder.Build());
	}
		break;
	default:
		if (pError) {
			pError->SetErrorMessage("Invalid Item type");
			pError->SetErrorCode(EWS_FAIL);
		}
		return false;
	}

	if (!pItemType.get()) {
		if (pError) {
			pError->SetErrorMessage("Invalid Item type or Item Instance");
			pError->SetErrorCode(EWS_FAIL);
		}
		return false;
	}
	
	union_NonEmptyArrayOfAllItemsType.union_NonEmptyArrayOfAllItemsType.Item =
			pItemType.get();

	arrayOfAllItemsType.__union_NonEmptyArrayOfAllItemsType =
			&union_NonEmptyArrayOfAllItemsType;

	pCreateItemType->Items = &arrayOfAllItemsType;

	int ret = m_pSession->GetProxy()->CreateItem(pCreateItemType, response);

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

		if (!responseMessage->Items
            || responseMessage->Items->__size_ArrayOfRealItemsType == 0) {
            if (pCreateItemType->MessageDisposition
                && ((*pCreateItemType->MessageDisposition) ==
                    ews2__MessageDispositionType__SendAndSaveCopy)) {
                return true;
            }
			continue;
        }

		for (int j = 0; j < responseMessage->Items->__size_ArrayOfRealItemsType;
		     j++) {
			__ews2__union_ArrayOfRealItemsType * unionFolderChange =
					responseMessage->Items->__union_ArrayOfRealItemsType + j;

			ews2__ItemType * pItem =
					unionFolderChange->union_ArrayOfRealItemsType.Item;

			if (pItem->ItemId) {
				pEWSItem->SetItemId(pItem->ItemId->Id.c_str());

				if (pItem->ItemId->ChangeKey)
					pEWSItem->SetChangeKey(pItem->ItemId->ChangeKey->c_str());

				if (pEWSItem->GetAttachments()
				    && pEWSItem->GetAttachments()->size() > 0) {
					std::auto_ptr<CEWSAttachmentOperation> attachmentOperation(
					    m_pSession->CreateAttachmentOperation());

					return attachmentOperation->CreateAttachments(
					    pEWSItem->GetAttachments(), pEWSItem, pError);
				}

				return true;
			}
		} //for all items change
	}

	SET_INVALID_RESPONSE;
	return false;
}

bool CEWSItemOperationGsoapImpl::SendItem(CEWSMessageItem * pMessageItem,
                                          CEWSError * pError) {
	if (pMessageItem->GetItemId().IsEmpty()) {
		if (!pMessageItem->GetMimeContent().IsEmpty() ||
		    (pMessageItem->GetAttachments() && pMessageItem->GetAttachments()->size() == 0) ||
		    (!pMessageItem->GetAttachments())) {
			return CreateItem(pMessageItem, 
			                  CEWSFolder::sentitems, 
			                  ews2__MessageDispositionType__SendAndSaveCopy, 
			                  pError);
		} else if (!CreateItem(pMessageItem, 
		                       CEWSFolder::drafts, 
		                       ews2__MessageDispositionType__SaveOnly, 
		                       pError)) {
			return false;
		}
	}

	CEWSSessionRequestGuard guard(m_pSession);

	ews1__SendItemType sendItemType;
	__ews1__SendItemResponse response;

	ews2__NonEmptyArrayOfBaseItemIdsType itemIdsType;
	itemIdsType.__size_NonEmptyArrayOfBaseItemIdsType = 1;

	__ews2__union_NonEmptyArrayOfBaseItemIdsType union_NonEmptyArrayOfBaseItemIdsType;
	union_NonEmptyArrayOfBaseItemIdsType.__union_NonEmptyArrayOfBaseItemIdsType =
			SOAP_UNION__ews2__union_NonEmptyArrayOfBaseItemIdsType_ItemId;

	ews2__ItemIdType itemIdType;
	itemIdType.Id = pMessageItem->GetItemId();

	std::string changeKey = pMessageItem->GetChangeKey().GetData();
	itemIdType.ChangeKey = &changeKey;

	union_NonEmptyArrayOfBaseItemIdsType.union_NonEmptyArrayOfBaseItemIdsType.ItemId =
			&itemIdType;

	itemIdsType.__union_NonEmptyArrayOfBaseItemIdsType =
			&union_NonEmptyArrayOfBaseItemIdsType;

	sendItemType.ItemIds = &itemIdsType;

	int ret = m_pSession->GetProxy()->SendItem(&sendItemType, response);

	if (ret != SOAP_OK) {
		if (pError) {
			pError->SetErrorMessage(m_pSession->GetErrorMsg());
			pError->SetErrorCode(ret);
		}

		return false;
	}

	ews1__ArrayOfResponseMessagesType * messages =
			response.ews1__SendItemResponse->ResponseMessages;
	for (int i = 0; i < messages->__size_ArrayOfResponseMessagesType; i++) {
		__ews1__union_ArrayOfResponseMessagesType * message =
				messages->__union_ArrayOfResponseMessagesType + i;

		if (message->__union_ArrayOfResponseMessagesType
		    !=
		    SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_SendItemResponseMessage) {
			continue;
		}

		ews1__ResponseMessageType * responseMessage =
				message->union_ArrayOfResponseMessagesType.SendItemResponseMessage;

		if (responseMessage->ResponseClass
		    != ews2__ResponseClassType__Success) {
			SAVE_ERROR(pError, responseMessage);

			return false;
		}

		return true;
	}

	SET_INVALID_RESPONSE;
	return false;
}

bool CEWSItemOperationGsoapImpl::DeleteItem(CEWSItem * pItem,
                                            bool moveToDeleted, CEWSError * pError) {
	CEWSItemList itemLists;
	itemLists.push_back(pItem);
	
	bool ret = DeleteItems(&itemLists,
	                       moveToDeleted,
	                       pError);

	itemLists.erase(itemLists.begin());

	return ret;
}

bool CEWSItemOperationGsoapImpl::DeleteItems(CEWSItemList * pItemList,
                                             bool moveToDeleted,
                                             CEWSError * pError) {
	CEWSSessionRequestGuard guard(m_pSession);

	ews1__DeleteItemType deleteItemType;
	__ews1__DeleteItemResponse response;
	CEWSObjectPool objPool;

    enum ews2__AffectedTaskOccurrencesType AffectedTaskOccurrences =
            ews2__AffectedTaskOccurrencesType__AllOccurrences;

    deleteItemType.AffectedTaskOccurrences =
            &AffectedTaskOccurrences;

	enum ews2__CalendarItemCreateOrDeleteOperationType
			CalendarItemCreateOrDeleteOperationType =
			ews2__CalendarItemCreateOrDeleteOperationType__SendToNone;

	deleteItemType.DeleteType =
			moveToDeleted ?
			ews2__DisposalType__MoveToDeletedItems :
			ews2__DisposalType__HardDelete;
	deleteItemType.SendMeetingCancellations =
			&CalendarItemCreateOrDeleteOperationType;

	ews2__NonEmptyArrayOfBaseItemIdsType itemIdsType;
	itemIdsType.__size_NonEmptyArrayOfBaseItemIdsType = pItemList->size();

	__ews2__union_NonEmptyArrayOfBaseItemIdsType * union_NonEmptyArrayOfBaseItemIdsType =
			objPool.CreateArray<__ews2__union_NonEmptyArrayOfBaseItemIdsType>(pItemList->size());

	CEWSItemList::iterator it = pItemList->begin();
	__ews2__union_NonEmptyArrayOfBaseItemIdsType * tmp =
			union_NonEmptyArrayOfBaseItemIdsType;
	
	while(it != pItemList->end()) {
		CEWSItem * pItem = *it;
		tmp->__union_NonEmptyArrayOfBaseItemIdsType =
				SOAP_UNION__ews2__union_NonEmptyArrayOfBaseItemIdsType_ItemId;

		ews2__ItemIdType * itemIdType = objPool.Create<ews2__ItemIdType>();
		itemIdType->Id = pItem->GetItemId();

		std::string * change_key =
				objPool.Create<std::string>(pItem->GetChangeKey().GetData());
		itemIdType->ChangeKey = change_key;
		tmp->union_NonEmptyArrayOfBaseItemIdsType.ItemId =
				itemIdType;

		tmp++;
		it++;
	}
	
	itemIdsType.__union_NonEmptyArrayOfBaseItemIdsType =
			union_NonEmptyArrayOfBaseItemIdsType;

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
			    == ews1__ResponseCodeType__ErrorItemNotFound) {
				//Item NOT found means delete ok
				return true;
			}
			return false;
		}

		return true;
	}

	SET_INVALID_RESPONSE;
	return false;
}

#define GET_VALUE(t, p)	  \
	{ \
		pItem->p = m_pObjPool->Create<t>(); \
		*pItem->p = (t)m_pEWSItem->Get##p(); \
	}

#define GET_VALUE_3(t, p, default)	  \
	{ \
		if ((t)m_pEWSItem->Get##p() != default) { \
			pItem->p = m_pObjPool->Create<t>(); \
			*pItem->p = (t)m_pEWSItem->Get##p(); \
		} \
	}

#define GET_VALUE_B(t, p)	  \
	{ \
		pItem->Is##p = m_pObjPool->Create<t>(); \
		*pItem->Is##p = (t)m_pEWSItem->Is##p(); \
	}

#define GET_VALUE_STR(p)	  \
	if (!m_pEWSItem->Get##p().IsEmpty()) \
	{ \
		pItem->p = m_pObjPool->Create<std::string>(m_pEWSItem->Get##p().GetData()); \
	}

ItemTypeBuilder::ItemTypeBuilder(soap * soap,
                                 const CEWSItem * pEWSItem,
                                 CEWSObjectPool * pObjPool)
	: m_pSoap(soap)
	, m_pEWSItem(pEWSItem)
	, m_pObjPool(pObjPool) {
}

ItemTypeBuilder::~ItemTypeBuilder() {
}	
	
ews2__ItemType * ItemTypeBuilder::CreateItemType() {
	return new ews2__ItemType();
}

ews2__ItemType * ItemTypeBuilder::Build() {
	if (!m_pEWSItem)
		return NULL;
	
	ews2__ItemType * pItem = CreateItemType();
	
	if (!m_pEWSItem->GetItemId().IsEmpty()) {
		pItem->ItemId = m_pObjPool->Create<ews2__ItemIdType>();
		pItem->ItemId->Id = m_pEWSItem->GetItemId();
	}

	if (!m_pEWSItem->GetChangeKey().IsEmpty() && pItem->ItemId) {
		pItem->ItemId->ChangeKey = m_pObjPool->Create<std::string>(
		    m_pEWSItem->GetChangeKey().GetData());
	}
	
	if (!m_pEWSItem->GetParentFolderId().IsEmpty()) {
		pItem->ParentFolderId = m_pObjPool->Create<ews2__FolderIdType>();
		pItem->ParentFolderId->Id = m_pEWSItem->GetParentFolderId();
	}

	if (!m_pEWSItem->GetParentFolderChangeKey().IsEmpty() && pItem->ParentFolderId) {
		pItem->ParentFolderId->ChangeKey = m_pObjPool->Create<std::string>(
		    m_pEWSItem->GetParentFolderChangeKey().GetData());
	}

	if (!m_pEWSItem->GetBody().IsEmpty() && m_pEWSItem->GetMimeContent().IsEmpty()) {
		pItem->Body = m_pObjPool->Create<ews2__BodyType>();
		pItem->Body->BodyType =
				m_pEWSItem->GetBodyType() == CEWSItem::BODY_HTML ?
				ews2__BodyTypeType__HTML : ews2__BodyTypeType__Text;
		pItem->Body->__item = m_pEWSItem->GetBody();
	}

	if (!m_pEWSItem->GetMimeContent().IsEmpty()) {
		pItem->MimeContent = m_pObjPool->Create<ews2__MimeContentType>();
		pItem->MimeContent->__item = m_pEWSItem->GetMimeContent();
		pItem->MimeContent->CharacterSet = m_pObjPool->Create<std::string>("UTF-8");
	}

	if (m_pEWSItem->GetCreateTime() > 0)
		pItem->DateTimeCreated = m_pObjPool->Create<time_t>(
		    m_pEWSItem->GetCreateTime());

	if (m_pEWSItem->GetReceivedTime())
		pItem->DateTimeReceived = m_pObjPool->Create<time_t>(
		    m_pEWSItem->GetReceivedTime());

	if (m_pEWSItem->GetSentTime() > 0)
		pItem->DateTimeSent = m_pObjPool->Create<time_t>(m_pEWSItem->GetSentTime());

	if (m_pEWSItem->GetMimeContent().IsEmpty()) {
        if (!m_pEWSItem->GetSubject().IsEmpty())
            pItem->Subject = m_pObjPool->Create<std::string>(m_pEWSItem->GetSubject().GetData());
    }

	GET_VALUE_STR(ItemClass);
	
	if (m_pEWSItem->GetMimeContent().IsEmpty()) {
		GET_VALUE(enum ews2__SensitivityChoicesType, Sensitivity);
		GET_VALUE(enum ews2__ImportanceChoicesType, Importance);
	}
	
	GET_VALUE_STR(InReplyTo);

	//GET_VALUE_B(bool, Submitted);
	//GET_VALUE_B(bool, Draft);
	//GET_VALUE_B(bool, FromMe);
	//GET_VALUE_B(bool, Resend);
	//GET_VALUE_B(bool, Unmodified);

	if (m_pEWSItem->GetMimeContent().IsEmpty()) {
		//Will in the mime content
		GET_VALUE_3(time_t, ReminderDueBy, 0);
		GET_VALUE(bool, ReminderIsSet);
		GET_VALUE_STR(ReminderMinutesBeforeStart);
	}
    
	GET_VALUE_STR(DisplayCc);
	GET_VALUE_STR(DisplayTo);

	GET_VALUE_STR(LastModifiedName);
	//GET_VALUE(time_t, LastModifiedTime);

	if (m_pEWSItem->GetCategories() && m_pEWSItem->GetCategories()->size() > 0) {
		pItem->Categories =
				m_pObjPool->Create<ews2__ArrayOfStringsType>();

		CEWSStringList::iterator it =
				m_pEWSItem->GetCategories()->begin();

		while(it != m_pEWSItem->GetCategories()->end()) {
			pItem->Categories->String.push_back((*it).GetData());

			it++;
		}
	}

	if (m_pEWSItem->GetInternetMessageHeaders()
	    && m_pEWSItem->GetInternetMessageHeaders()->size() > 0) {
		CEWSInternetMessageHeaderList::iterator it =
				m_pEWSItem->GetInternetMessageHeaders()->begin();

		pItem->InternetMessageHeaders =
				m_pObjPool->Create<ews2__NonEmptyArrayOfInternetHeadersType>();
		
		while(it != m_pEWSItem->GetInternetMessageHeaders()->end()) {
			CEWSInternetMessageHeader * p = *it;

			ews2__InternetHeaderType * t =
					m_pObjPool->Create<ews2__InternetHeaderType>();

			t->__item = p->GetValue().GetData();
			t->HeaderName = p->GetHeaderName().GetData();

			pItem->InternetMessageHeaders->InternetMessageHeader.push_back(t);
			
			it++;
		}
	}

	return pItem;
}

bool CEWSItemOperationGsoapImpl::GetUnreadItems(const CEWSString & strFolderId,
                                                CEWSItemOperationCallback * callback, CEWSError * pError) {
	CEWSSessionRequestGuard guard(m_pSession);

	__ews1__FindItemResponse response;

	ews2__ItemResponseShapeType * pItemShape =
			soap_new_req_ews2__ItemResponseShapeType(m_pSession->GetProxy(),
			                                         ews2__DefaultShapeNamesType__IdOnly);

	ews1__FindItemType request;

	request.ItemShape = pItemShape;
	request.Traversal = ews2__ItemQueryTraversalType__Shallow;

	ews2__NonEmptyArrayOfBaseFolderIdsType ParentFolderIds;

	request.ParentFolderIds = &ParentFolderIds;

	ParentFolderIds.__size_NonEmptyArrayOfBaseFolderIdsType = 1;

	__ews2__union_NonEmptyArrayOfBaseFolderIdsType targetFolder;
	targetFolder.__union_NonEmptyArrayOfBaseFolderIdsType =
			SOAP_UNION__ews2__union_NonEmptyArrayOfBaseFolderIdsType_FolderId;

	ews2__FolderIdType FolderId;
	FolderId.Id = strFolderId;
	targetFolder.union_NonEmptyArrayOfBaseFolderIdsType.FolderId=
			&FolderId;
	ParentFolderIds.__union_NonEmptyArrayOfBaseFolderIdsType =
			&targetFolder;

	ews2__RestrictionType restriction;
	request.Restriction = &restriction;
	restriction.__unionSearchExpression = SOAP_UNION__ews2__union_RestrictionType_IsEqualTo;
	_ews2__union_RestrictionType * union_RestrictionType = &restriction.__union_RestrictionType;

	ews2__IsEqualToType isEqualTo;
	union_RestrictionType->IsEqualTo = &isEqualTo;

	ews2__PathToUnindexedFieldType FieldURI;
	isEqualTo.__unionPath = SOAP_UNION__ews2__union_TwoOperandExpressionType_FieldURI;

	_ews2__union_TwoOperandExpressionType * union_TwoOperandExpressionType =
			&isEqualTo.__union_TwoOperandExpressionType;

	union_TwoOperandExpressionType->FieldURI = &FieldURI;
	FieldURI.FieldURI = ews2__UnindexedFieldURIType__message_x003aIsRead;

	ews2__FieldURIOrConstantType FieldURIOrConstantType;
	isEqualTo.FieldURIOrConstant = &FieldURIOrConstantType;
	FieldURIOrConstantType.__union_FieldURIOrConstantType =
			SOAP_UNION__ews2__union_FieldURIOrConstantType_Constant;

	ews2__ConstantValueType ConstantValueType;
	FieldURIOrConstantType.union_FieldURIOrConstantType.Constant = &ConstantValueType;
	ConstantValueType.Value = "false";

	int ret = m_pSession->GetProxy()->FindItem(&request,
	                                           response);

	if (ret != SOAP_OK) {
		if (pError) {
			pError->SetErrorMessage(m_pSession->GetErrorMsg());
			pError->SetErrorCode(ret);
		}

		return false;
	}

	ews1__ArrayOfResponseMessagesType * messages =
			response.ews1__FindItemResponse->ResponseMessages;
	for (int i = 0; i < messages->__size_ArrayOfResponseMessagesType; i++) {
		__ews1__union_ArrayOfResponseMessagesType * message =
				messages->__union_ArrayOfResponseMessagesType + i;

		if (message->__union_ArrayOfResponseMessagesType
		    !=
		    SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_FindItemResponseMessage) {
			continue;
		}

		ews1__FindItemResponseMessageType * findItemResponseMessage =
				message->union_ArrayOfResponseMessagesType.FindItemResponseMessage;

		if (findItemResponseMessage->ResponseClass
		    != ews2__ResponseClassType__Success) {
			SAVE_ERROR(pError, findItemResponseMessage);

			return false;
		}

		if (findItemResponseMessage->RootFolder->__union_FindItemParentType
		    != SOAP_UNION__ews2__union_FindItemParentType_Items) {
			continue;
		}

		_ews2__union_FindItemParentType * findItemParentType =
				&findItemResponseMessage->RootFolder->union_FindItemParentType;
		ews2__ArrayOfRealItemsType * items =
				findItemParentType->Items;

		for (int j = 0;
		     j < items->__size_ArrayOfRealItemsType;
		     j++) {
			__ews2__union_ArrayOfRealItemsType * union_ArrayOfRealItems =
					items->__union_ArrayOfRealItemsType + j;

			CEWSItem::EWSItemType itemType = CEWSItem::Item_Message;
			ews2__ItemType * pItem = NULL;
            
			switch (union_ArrayOfRealItems->__union_ArrayOfRealItemsType) {
			case SOAP_UNION__ews2__union_ArrayOfRealItemsType_Message:
				itemType = CEWSItem::Item_Message;
				pItem = union_ArrayOfRealItems->union_ArrayOfRealItemsType.Message;
				break;
			case SOAP_UNION__ews2__union_ArrayOfRealItemsType_MeetingMessage:
				itemType = CEWSItem::Item_Message;
				pItem = union_ArrayOfRealItems->union_ArrayOfRealItemsType.MeetingMessage;
				break;
			case SOAP_UNION__ews2__union_ArrayOfRealItemsType_MeetingRequest:
				itemType = CEWSItem::Item_Message;
				pItem = union_ArrayOfRealItems->union_ArrayOfRealItemsType.MeetingRequest;
				break;
			case SOAP_UNION__ews2__union_ArrayOfRealItemsType_MeetingResponse:
				itemType = CEWSItem::Item_Message;
				pItem = union_ArrayOfRealItems->union_ArrayOfRealItemsType.MeetingResponse;
				break;
			case SOAP_UNION__ews2__union_ArrayOfRealItemsType_MeetingCancellation:
				itemType = CEWSItem::Item_Message;
				pItem = union_ArrayOfRealItems->union_ArrayOfRealItemsType.MeetingCancellation;
				break;
			case SOAP_UNION__ews2__union_ArrayOfRealItemsType_CalendarItem:
				itemType = CEWSItem::Item_Calendar;
				pItem = union_ArrayOfRealItems->union_ArrayOfRealItemsType.CalendarItem;
				break;
			case SOAP_UNION__ews2__union_ArrayOfRealItemsType_Task:
				itemType = CEWSItem::Item_Task;
				pItem = union_ArrayOfRealItems->union_ArrayOfRealItemsType.Task;
				break;
			default:
				continue;
			}

			std::auto_ptr<CEWSItemGsoapImpl> pEWSItem(
			    CEWSItemGsoapImpl::CreateInstance(itemType, pItem));

			callback->NewItem(pEWSItem.get());
		}
	} //for all response message

	return true;
}

bool CEWSItemOperationGsoapImpl::MarkItemsRead(CEWSItemList * pItemList,
                                               bool read,
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

	CEWSItemList::iterator it = pItemList->begin();

	while(it != pItemList->end()) {
		CEWSItem * pItem = *it;

		ews2__ItemChangeType * ItemChange =
				objPool.Create<ews2__ItemChangeType>();

		ItemChange->__union_ItemChangeType = SOAP_UNION__ews2__union_ItemChangeType_ItemId;

		ews2__ItemIdType * ItemId =
				objPool.Create<ews2__ItemIdType>();
		ItemId->Id = pItem->GetItemId();

		std::string * change_key =
				objPool.Create<std::string>(pItem->GetChangeKey().GetData());
		ItemId->ChangeKey = change_key;

		ItemChange->union_ItemChangeType.ItemId = ItemId;

		ews2__NonEmptyArrayOfItemChangeDescriptionsType * Updates =
				objPool.Create<ews2__NonEmptyArrayOfItemChangeDescriptionsType>();
		ItemChange->Updates = Updates;

		Updates->__size_NonEmptyArrayOfItemChangeDescriptionsType = 1;

		__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType * ItemChangeDesc =
				objPool.Create<__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType>();

		Updates->__union_NonEmptyArrayOfItemChangeDescriptionsType = ItemChangeDesc;

		ItemChangeDesc->__union_NonEmptyArrayOfItemChangeDescriptionsType =
				SOAP_UNION__ews2__union_NonEmptyArrayOfItemChangeDescriptionsType_SetItemField;

		ews2__SetItemFieldType * SetItemField =
				objPool.Create<ews2__SetItemFieldType>();
		ItemChangeDesc->union_NonEmptyArrayOfItemChangeDescriptionsType.SetItemField =
				SetItemField;

		SetItemField->__unionPath = SOAP_UNION__ews2__union_ChangeDescriptionType_FieldURI;

		ews2__PathToUnindexedFieldType * FieldURI =
				objPool.Create<ews2__PathToUnindexedFieldType>();
		SetItemField->__union_ChangeDescriptionType.FieldURI = FieldURI;
		FieldURI->FieldURI = ews2__UnindexedFieldURIType__message_x003aIsRead;

		SetItemField->__union_SetItemFieldType_ = SOAP_UNION__ews2__union_SetItemFieldType__Message;

		ews2__MessageType * Message =
				objPool.Create<ews2__MessageType>();
		SetItemField->union_SetItemFieldType_.Message = Message;

		bool * IsRead = objPool.Create<bool>(read);
		Message->IsRead = IsRead;

		ItemChanges.ItemChange.push_back(ItemChange);

		it++;
	}

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

		return true;
	}

	SET_INVALID_RESPONSE;
	return false;
}

CEWSInternetMessageHeader::CEWSInternetMessageHeader() {
}

CEWSInternetMessageHeader::~CEWSInternetMessageHeader() {
}

DEF_CREATE_INSTANCE(CEWSInternetMessageHeader)

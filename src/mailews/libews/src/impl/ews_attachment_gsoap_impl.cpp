/*
 * ews_attachment_gsoap_impl.cpp
 *
 *  Created on: Jul 24, 2014
 *      Author: stone
 */
#include "libews_gsoap.h"
#include <memory>
#include <map>

using namespace ews;

static void UpdateAttachment(CEWSAttachment * pEWSItem,
                             CEWSItem * pParentItem,
                             ews2__AttachmentType * pItem) {
	if (pItem->AttachmentId) {
		pEWSItem->SetAttachmentId(pItem->AttachmentId->Id.c_str());

		if (pParentItem && pItem->AttachmentId->RootItemChangeKey) {
			pParentItem->SetChangeKey(pItem->AttachmentId->RootItemChangeKey->c_str());
		}
	}

	if (pItem->ContentId) {
		pEWSItem->SetContentId(pItem->ContentId->c_str());
	}

	if (pItem->ContentLocation) {
		pEWSItem->SetContentLocation(pItem->ContentLocation->c_str());
	}

	if (pItem->ContentType) {
		pEWSItem->SetContentType(pItem->ContentType->c_str());
	}

	if (pItem->Name) {
		pEWSItem->SetName(pItem->Name->c_str());
	}
}

static void UpdateItemAttachment(CEWSItemAttachmentGsoapImpl * pEWSItem,
                                 ews2__ItemAttachmentType * pItem) {
	CEWSItem::EWSItemType itemType;
	ews2__ItemType * pAttachedItem = NULL;
	_ews2__union_ItemAttachmentType * union_ItemAttachmentType =
			&pItem->union_ItemAttachmentType;

	switch (pItem->__union_ItemAttachmentType) {
	case SOAP_UNION__ews2__union_ItemAttachmentType_Message:
		itemType = CEWSItem::Item_Message;
		pAttachedItem = union_ItemAttachmentType->Message;
		break;
	default:
		return;
	}

	pEWSItem->SetItemId(pAttachedItem->ItemId->Id.c_str());
	pEWSItem->SetItemType(itemType);
}

static void UpdateFileAttachment(CEWSFileAttachmentGsoapImpl * pEWSItem,
                                 ews2__FileAttachmentType * pItem) {
	if (pItem && pItem->Content)
		pEWSItem->SetContent(
		    XsdBase64BinaryToEWSString(pItem->soap, pItem->Content));
}

CEWSAttachmentGsoapImpl * CEWSAttachmentGsoapImpl::CreateInstance(
    EWSAttachmentType itemType, ews2__AttachmentType * pItem) {
	std::auto_ptr<CEWSAttachmentGsoapImpl> pEWSItem(NULL);

	switch (itemType) {
	case CEWSAttachment::Attachment_Item: {
		pEWSItem.reset(new CEWSItemAttachmentGsoapImpl());

		if (pEWSItem.get() && pItem) {
			UpdateItemAttachment(
			    dynamic_cast<CEWSItemAttachmentGsoapImpl *>(pEWSItem.get()),
			    (ews2__ItemAttachmentType *) pItem);
		}
	}
		break;
	case CEWSAttachment::Attachment_File: {
		pEWSItem.reset(new CEWSFileAttachmentGsoapImpl());

		if (pEWSItem.get() && pItem) {
			UpdateFileAttachment(
			    dynamic_cast<CEWSFileAttachmentGsoapImpl *>(pEWSItem.get()),
			    (ews2__FileAttachmentType *) pItem);
		}
	}
		break;
	}

	if (pEWSItem.get() && pItem) {
		UpdateAttachment(pEWSItem.get(), NULL, pItem);
	}

	if (pEWSItem.get()) {
		pEWSItem->SetAttachmentType(itemType);
	}
	return pEWSItem.release();
}

CEWSAttachment * CEWSAttachment::CreateInstance(EWSAttachmentType itemType) {
	return CEWSAttachmentGsoapImpl::CreateInstance(itemType, NULL);
}

bool CEWSAttachmentGsoapImpl::IsSame(CEWSAttachment * pAttachment) {
	if (this == pAttachment) {
		return true;
	}

	if (pAttachment == NULL) {
		return false;
	}

	return m_AttachmentId.CompareTo(pAttachment->GetAttachmentId()) == 0;
}

CEWSAttachmentOperationGsoapImpl::CEWSAttachmentOperationGsoapImpl(
    CEWSSessionGsoapImpl * pSession) :
	m_pSession(pSession) {
}

CEWSAttachmentOperationGsoapImpl::~CEWSAttachmentOperationGsoapImpl() {
}

CEWSAttachment * CEWSAttachmentOperationGsoapImpl::GetAttachment(
    const CEWSString & attachmentId, CEWSError * pError) {
	CEWSSessionRequestGuard guard(m_pSession);
	
	__ews1__GetAttachmentResponse response;
	ews1__GetAttachmentType getAttachment;

	ews2__AttachmentResponseShapeType attachmentShape;
	enum ews2__BodyTypeResponseType bodyType = ews2__BodyTypeResponseType__Best;
	attachmentShape.BodyType = &bodyType;
	getAttachment.AttachmentShape = &attachmentShape;

	ews2__NonEmptyArrayOfRequestAttachmentIdsType attachmentIds;
	attachmentIds.__size_NonEmptyArrayOfRequestAttachmentIdsType = 1;

	__ews2__union_NonEmptyArrayOfRequestAttachmentIdsType unionAttachmentIdsType;
	unionAttachmentIdsType.__union_NonEmptyArrayOfRequestAttachmentIdsType =
			SOAP_UNION__ews2__union_NonEmptyArrayOfRequestAttachmentIdsType_AttachmentId;

	ews2__AttachmentIdType attachmentIdType;
	attachmentIdType.Id = attachmentId;
	unionAttachmentIdsType.union_NonEmptyArrayOfRequestAttachmentIdsType.AttachmentId =
			&attachmentIdType;
	attachmentIds.__union_NonEmptyArrayOfRequestAttachmentIdsType =
			&unionAttachmentIdsType;
	getAttachment.AttachmentIds = &attachmentIds;

	int ret = m_pSession->GetProxy()->GetAttachment(&getAttachment, response);

	if (ret != SOAP_OK) {
		if (pError) {
			pError->SetErrorMessage(m_pSession->GetErrorMsg());
			pError->SetErrorCode(ret);
		}

		return NULL;
	}

	ews1__ArrayOfResponseMessagesType * messages =
			response.ews1__GetAttachmentResponse->ResponseMessages;

	for (int i = 0; i < messages->__size_ArrayOfResponseMessagesType; i++) {
		__ews1__union_ArrayOfResponseMessagesType * message =
				messages->__union_ArrayOfResponseMessagesType + i;

		if (message->__union_ArrayOfResponseMessagesType
		    !=
		    SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_GetAttachmentResponseMessage) {
			continue;
		}

		ews1__AttachmentInfoResponseMessageType * getAttachmentResponseMessage =
				message->union_ArrayOfResponseMessagesType.GetAttachmentResponseMessage;

		if (getAttachmentResponseMessage->ResponseClass
		    != ews2__ResponseClassType__Success) {
			SAVE_ERROR(pError, getAttachmentResponseMessage);

			return NULL;
		}

		if (0
		    == getAttachmentResponseMessage->Attachments->__size_ArrayOfAttachmentsType) {
			continue;
		}

		__ews2__union_ArrayOfAttachmentsType * unionAttachmentsType =
				getAttachmentResponseMessage->Attachments->__union_ArrayOfAttachmentsType;
		CEWSAttachment::EWSAttachmentType attachmentType;
		ews2__AttachmentType * attachment;

		switch (unionAttachmentsType->__union_ArrayOfAttachmentsType) {
		case SOAP_UNION__ews2__union_ArrayOfAttachmentsType_ItemAttachment:
			attachmentType = CEWSAttachment::Attachment_Item;
			attachment =
					unionAttachmentsType->union_ArrayOfAttachmentsType.ItemAttachment;
			break;
		case SOAP_UNION__ews2__union_ArrayOfAttachmentsType_FileAttachment:
			attachmentType = CEWSAttachment::Attachment_File;
			attachment =
					unionAttachmentsType->union_ArrayOfAttachmentsType.FileAttachment;
			break;
		default:
			continue;
		}

		return CEWSAttachmentGsoapImpl::CreateInstance(attachmentType,
		                                               attachment);
	}

	if (pError) {
		CEWSString msg("Attachment not found, attachment id is ");
		msg.Append(attachmentId);
		pError->SetErrorMessage(msg);
		pError->SetErrorCode(EWS_FAIL);
	}
	return NULL;
}

bool CEWSAttachmentOperationGsoapImpl::CreateAttachment(
    CEWSAttachment * pAttachment, CEWSItem * parentItem,
    CEWSError * pError) {
	CEWSAttachmentList list;
	list.push_back(pAttachment);

	return CreateAttachments(&list, parentItem, pError);
}

bool CEWSAttachmentOperationGsoapImpl::CreateAttachments(
    const CEWSAttachmentList * pAttachmentList,
    CEWSItem * parentItem, CEWSError * pError) {
	CEWSSessionRequestGuard guard(m_pSession);

	__ews1__CreateAttachmentResponse response;
	ews1__CreateAttachmentType createAttachment;

	ews2__ItemIdType parentItemIdType;
	parentItemIdType.Id = parentItem->GetItemId();
	createAttachment.ParentItemId = &parentItemIdType;

	ews2__NonEmptyArrayOfAttachmentsType attachmentsArrayType;
	attachmentsArrayType.__size_NonEmptyArrayOfAttachmentsType =
			pAttachmentList->size();

	CEWSObjectPool objPool;

	__ews2__union_NonEmptyArrayOfAttachmentsType * unionAttachmentsType =
			objPool.CreateArray<__ews2__union_NonEmptyArrayOfAttachmentsType>(pAttachmentList->size());

	__ews2__union_NonEmptyArrayOfAttachmentsType * tmp =
			unionAttachmentsType;

	for (int i = 0; i < pAttachmentList->size(); i++, tmp++) {
		CEWSAttachment * pAttachment = pAttachmentList->at(i);
		tmp->__union_NonEmptyArrayOfAttachmentsType =
				pAttachment->GetAttachmentType()
				== CEWSAttachment::Attachment_Item ?
				SOAP_UNION__ews2__union_NonEmptyArrayOfAttachmentsType_ItemAttachment :
				SOAP_UNION__ews2__union_NonEmptyArrayOfAttachmentsType_FileAttachment;

		if (pAttachment->GetAttachmentType()
		    == CEWSAttachment::Attachment_Item) {
			ews2__ItemAttachmentType * __attachmentType =
					tmp->union_NonEmptyArrayOfAttachmentsType.ItemAttachment =
					objPool.Create<ews2__ItemAttachmentType>();

			CEWSItemAttachment * pItemAttachment =
					dynamic_cast<CEWSItemAttachment *>(pAttachment);

			if (!pItemAttachment) {
				if (pError) {
					pError->SetErrorMessage(
					    "Invalid parameter, not a valid ItemAttachment");
					pError->SetErrorCode(EWS_FAIL);
				}
				return false;
			}

			__attachmentType->Name = objPool.Create<std::string>(
			    pAttachment->GetName().GetData());

			switch (pItemAttachment->GetItemType()) {
			case CEWSItem::Item_Message: {
				__attachmentType->__union_ItemAttachmentType =
						SOAP_UNION__ews2__union_ItemAttachmentType_Message;

				union _ews2__union_ItemAttachmentType * __union_ItemAttachmentType =
						&(__attachmentType->union_ItemAttachmentType);

				__union_ItemAttachmentType->Message = objPool.Create<
					ews2__MessageType>();

				__union_ItemAttachmentType->Message->ItemId = objPool.Create<
					ews2__ItemIdType>();

				__union_ItemAttachmentType->Message->ItemId->Id =
						pItemAttachment->GetItemId();
			}
				break;
			default:
				if (pError) {
					pError->SetErrorMessage(
					    "Unsupported Item type for create attachment");
					pError->SetErrorCode(EWS_FAIL);
				}
				return false;
			}
		} else {
			ews2__FileAttachmentType * __attachmentType = objPool.Create<
				ews2__FileAttachmentType>();

			__attachmentType->Name = objPool.Create<std::string>(
			    pAttachment->GetName().GetData());

			CEWSFileAttachment * pFileAttachment =
					dynamic_cast<CEWSFileAttachment *>(pAttachment);

			if (!pFileAttachment) {
				if (pError) {
					pError->SetErrorMessage(
					    "Invalid parameter, not a valid FileAttachment");
					pError->SetErrorCode(EWS_FAIL);
				}
				return false;
			}

			xsd__base64Binary * fileContent = EWSStringToXsdBase64Binary(
			    m_pSession->GetProxy(), pFileAttachment->GetContent());
			objPool.Add(fileContent);
			__attachmentType->Content = fileContent;

			tmp->union_NonEmptyArrayOfAttachmentsType.FileAttachment =
					__attachmentType;
		}
	}

	attachmentsArrayType.__union_NonEmptyArrayOfAttachmentsType =
			unionAttachmentsType;

	createAttachment.Attachments = &attachmentsArrayType;

	int ret = m_pSession->GetProxy()->CreateAttachment(&createAttachment,
	                                                   response);

	if (ret != SOAP_OK) {
		if (pError) {
			pError->SetErrorMessage(m_pSession->GetErrorMsg());
			pError->SetErrorCode(ret);
		}

		return false;
	}

	ews1__ArrayOfResponseMessagesType * messages =
			response.ews1__CreateAttachmentResponse->ResponseMessages;

	for (int i = 0; i < messages->__size_ArrayOfResponseMessagesType; i++) {
		__ews1__union_ArrayOfResponseMessagesType * message =
				messages->__union_ArrayOfResponseMessagesType + i;

		if (message->__union_ArrayOfResponseMessagesType
		    !=
		    SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_CreateAttachmentResponseMessage) {
			continue;
		}

		ews1__AttachmentInfoResponseMessageType * createAttachmentResponseMessage =
				message->union_ArrayOfResponseMessagesType.CreateAttachmentResponseMessage;

		if (createAttachmentResponseMessage->ResponseClass
		    != ews2__ResponseClassType__Success) {
			SAVE_ERROR(pError, createAttachmentResponseMessage);
			return false;
		}

		if (!createAttachmentResponseMessage->Attachments
		    || createAttachmentResponseMessage->Attachments->__size_ArrayOfAttachmentsType
		    < 1) {
			if (pError) {
				pError->SetErrorMessage(
				    "Invalid response, no attachments element");
				pError->SetErrorCode(EWS_FAIL);
			}

			return false;
		}

		//update item change key
		__ews2__union_ArrayOfAttachmentsType *__union_ArrayOfAttachmentsType =
				createAttachmentResponseMessage->Attachments->__union_ArrayOfAttachmentsType;
		for (int i = 0;
		     i
				     < createAttachmentResponseMessage->Attachments->__size_ArrayOfAttachmentsType;
		     i++, __union_ArrayOfAttachmentsType++) {
			CEWSAttachment * pAttachment = pAttachmentList->at(i);

			ews2__AttachmentType * __attachment = NULL;
			CEWSAttachment::EWSAttachmentType __attachmentType =
					CEWSAttachment::Attachment_Item;

			switch (__union_ArrayOfAttachmentsType->__union_ArrayOfAttachmentsType) {
			case SOAP_UNION__ews2__union_ArrayOfAttachmentsType_ItemAttachment:
				__attachmentType = CEWSAttachment::Attachment_Item;
				__attachment =
						__union_ArrayOfAttachmentsType->union_ArrayOfAttachmentsType.ItemAttachment;
				break;
			case SOAP_UNION__ews2__union_ArrayOfAttachmentsType_FileAttachment:
				__attachmentType = CEWSAttachment::Attachment_File;
				__attachment =
						__union_ArrayOfAttachmentsType->union_ArrayOfAttachmentsType.FileAttachment;
				break;
			default:
				continue;
			}

			UpdateAttachment(pAttachment, parentItem, __attachment);
		}
		return true;
	}

	SET_INVALID_RESPONSE;
	return false;
}

CEWSFileAttachment * CEWSFileAttachment::CreateInstance() {
	return dynamic_cast<CEWSFileAttachment *>(CEWSAttachment::CreateInstance(
	    CEWSAttachment::Attachment_File));
}

CEWSItemAttachment * CEWSItemAttachment::CreateInstance() {
	return dynamic_cast<CEWSItemAttachment *>(CEWSAttachment::CreateInstance(
	    CEWSAttachment::Attachment_Item));
}

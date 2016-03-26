/*
 * ews_item_message_gsoap_impl.cpp
 *
 *  Created on: Jul 28, 2014
 *      Author: stone
 */

#include "libews_gsoap.h"
#include <memory>
#include <map>

using namespace ews;

DLL_LOCAL void ToRecipient(const CEWSRecipient * pRecipient,
                        ews2__SingleRecipientType * pItem,
                        CEWSObjectPool * pObjPool);
DLL_LOCAL void ToArrayOfReceipients(const CEWSRecipientList * pRecipient,
                                 ews2__ArrayOfRecipientsType * pItem,
                                 CEWSObjectPool * pObjPool);

void CEWSMessageItemGsoapImpl::FromItemType(ews2__ItemType * pItem) {
    CEWSItemGsoapImpl::FromItemType(pItem);

    ews2__MessageType * pMessageItem = dynamic_cast<ews2__MessageType *>(pItem);

    if (pMessageItem) {
        FromMessageType(pMessageItem);
    }
}

void CEWSMessageItemGsoapImpl::FromArrayOfRecipientsType(
    CEWSRecipientList * pList, ews2__ArrayOfRecipientsType * pItem) {
    pList->Clear();

    if (pItem->__size_ArrayOfRecipientsType == 0) {
        return;
    }

    __ews2__union_ArrayOfRecipientsType * pRecipientType =
            pItem->__union_ArrayOfRecipientsType;
    for (int i = 0; i < pItem->__size_ArrayOfRecipientsType;
         i++, pRecipientType++) {
        if (SOAP_UNION__ews2__union_ArrayOfRecipientsType_Mailbox
            != pRecipientType->__union_ArrayOfRecipientsType)
            continue;

        pList->push_back(
            CEWSRecipientGsoapImpl::CreateInstance(
                pRecipientType->union_ArrayOfRecipientsType.Mailbox));
    }
}

void CEWSMessageItemGsoapImpl::FromMessageType(ews2__MessageType * pItem) {
    if (pItem->Sender) {
        SetSender(CEWSRecipientGsoapImpl::CreateInstance(pItem->Sender));
    }

    if (pItem->ToRecipients) {
        FromArrayOfRecipientsType(GetToRecipients(), pItem->ToRecipients);
    }

    if (pItem->CcRecipients) {
        FromArrayOfRecipientsType(GetCcRecipients(), pItem->CcRecipients);
    }

    if (pItem->BccRecipients) {
        FromArrayOfRecipientsType(GetBccRecipients(), pItem->BccRecipients);
    }

    if (pItem->IsReadReceiptRequested) {
        SetReadReceiptRequested(*pItem->IsReadReceiptRequested);
    }

    if (pItem->IsDeliveryReceiptRequested) {
        SetDeliveryReceiptRequested(*pItem->IsDeliveryReceiptRequested);
    }

    if (pItem->ConversationIndex) {
        SetConversationIndex(
            XsdBase64BinaryToEWSString(pItem->soap,
                                       pItem->ConversationIndex));
    }

    if (pItem->ConversationTopic) {
        SetConversationTopic(pItem->ConversationTopic->c_str());
    }

    if (pItem->From) {
        SetFrom(CEWSRecipientGsoapImpl::CreateInstance(pItem->From));
    }

    if (pItem->InternetMessageId) {
        SetInternetMessageId(pItem->InternetMessageId->c_str());
    }

    if (pItem->IsRead) {
        SetRead(*pItem->IsRead);
    }

    if (pItem->IsResponseRequested) {
        SetResponseRequested(*pItem->IsResponseRequested);
    }

    if (pItem->References) {
        SetReferences(pItem->References->c_str());
    }

    if (pItem->ReplyTo) {
        FromArrayOfRecipientsType(GetReplyTo(), pItem->ReplyTo);
    }

    if (pItem->ReceivedBy) {
        SetReceivedBy(
            CEWSRecipientGsoapImpl::CreateInstance(pItem->ReceivedBy));
    }

    if (pItem->ReceivedRepresenting) {
        SetReceivedRepresenting(
            CEWSRecipientGsoapImpl::CreateInstance(
                pItem->ReceivedRepresenting));
    }
}

void ToRecipient(const CEWSRecipient * pRecipient,
                        ews2__SingleRecipientType * pItem,
                        CEWSObjectPool * pObjPool) {
    pItem->__union_SingleRecipientType =
            SOAP_UNION__ews2__union_SingleRecipientType_Mailbox;
    union _ews2__union_SingleRecipientType * union_SingleRecipientType =
            &pItem->union_SingleRecipientType;
    union_SingleRecipientType->Mailbox =
            pObjPool->Create<ews2__EmailAddressType>();
    union_SingleRecipientType->Mailbox->EmailAddress = pObjPool->Create<std::string>(pRecipient->GetEmailAddress().GetData());
}

void ToArrayOfReceipients(
    const CEWSRecipientList * pRecipientList,
    ews2__ArrayOfRecipientsType * pItem,
    CEWSObjectPool * pObjPool) {
    pItem->__size_ArrayOfRecipientsType = pRecipientList->size();
    pItem->__union_ArrayOfRecipientsType =
            pObjPool->CreateArray<__ews2__union_ArrayOfRecipientsType>(pItem->__size_ArrayOfRecipientsType);

    __ews2__union_ArrayOfRecipientsType * tmp =
            pItem->__union_ArrayOfRecipientsType;

    for (int i = 0; i < pItem->__size_ArrayOfRecipientsType; i++, tmp++) {
        tmp->__union_ArrayOfRecipientsType =
                SOAP_UNION__ews2__union_ArrayOfRecipientsType_Mailbox;
        tmp->union_ArrayOfRecipientsType.Mailbox = pObjPool->Create<
            ews2__EmailAddressType>();
        tmp->union_ArrayOfRecipientsType.Mailbox->EmailAddress =
                pObjPool->Create<std::string>(
                    pRecipientList->at(i)->GetEmailAddress().GetData());
    }
}

MessageItemTypeBuilder::MessageItemTypeBuilder(soap * soap,
                                               const CEWSMessageItem * pEWSItem,
                                               CEWSObjectPool * pObjPool)
    : ItemTypeBuilder(soap, pEWSItem, pObjPool) {
}

MessageItemTypeBuilder::~MessageItemTypeBuilder() {
}

ews2__ItemType * MessageItemTypeBuilder::CreateItemType() {
	return new ews2__MessageType();
}

ews2__ItemType * MessageItemTypeBuilder::Build() {
	ews2__ItemType * pItem = ItemTypeBuilder::Build();

	if (!pItem)
		return NULL;

    ews2__MessageType * pMessage = dynamic_cast<ews2__MessageType *>(pItem);
    const CEWSMessageItem * pMessageItem =
            dynamic_cast<const CEWSMessageItem *>(m_pEWSItem);

    if (!pMessage || !pMessageItem) {
        delete pItem;
        return NULL;
    }

    if (pMessageItem->GetSender()) {
        pMessage->Sender = m_pObjPool->Create<ews2__SingleRecipientType>();
        ToRecipient(pMessageItem->GetSender(),
                    pMessage->Sender, m_pObjPool);
    }

    if (!pMessageItem->GetToRecipients()->empty()) {
        pMessage->ToRecipients =
                m_pObjPool->Create<ews2__ArrayOfRecipientsType>();
        ToArrayOfReceipients(
            pMessageItem->GetToRecipients(), pMessage->ToRecipients,
            m_pObjPool);
    }

    if (!pMessageItem->GetCcRecipients()->empty()) {
        pMessage->CcRecipients =
                m_pObjPool->Create<ews2__ArrayOfRecipientsType>();
        ToArrayOfReceipients(
            pMessageItem->GetCcRecipients(), pMessage->CcRecipients,
            m_pObjPool);
    }

    if (!pMessageItem->GetBccRecipients()->empty()) {
        pMessage->BccRecipients =
                m_pObjPool->Create<ews2__ArrayOfRecipientsType>();
        ToArrayOfReceipients(
            pMessageItem->GetBccRecipients(), pMessage->BccRecipients,
            m_pObjPool);
    }

    pMessage->IsReadReceiptRequested = m_pObjPool->Create<bool>();
    *pMessage->IsReadReceiptRequested = pMessageItem->IsReadReceiptRequested();

    pMessage->IsDeliveryReceiptRequested = m_pObjPool->Create<bool>();
    *pMessage->IsDeliveryReceiptRequested =
            pMessageItem->IsDeliveryReceiptRequested();

    if (!pMessageItem->GetConversationIndex().IsEmpty()) {
        pMessage->ConversationIndex =
                EWSStringToXsdBase64Binary(m_pSoap,
                                           pMessageItem->GetConversationIndex());
    }

    if (!pMessageItem->GetConversationTopic().IsEmpty()) {
        pMessage->ConversationTopic = m_pObjPool->Create<std::string>(
            pMessageItem->GetConversationTopic().GetData());
    }

    if (pMessageItem->GetFrom()) {
        pMessage->From = m_pObjPool->Create<ews2__SingleRecipientType>();
        ToRecipient(pMessageItem->GetFrom(),
                    pMessage->From, m_pObjPool);
    }

    if (!pMessageItem->GetInternetMessageId().IsEmpty()) {
        pMessage->InternetMessageId = m_pObjPool->Create<std::string>(
            pMessageItem->GetInternetMessageId().GetData());
    }

    pMessage->IsRead = m_pObjPool->Create<bool>();
    *pMessage->IsRead = pMessageItem->IsRead();

    pMessage->IsResponseRequested = m_pObjPool->Create<bool>();
    *pMessage->IsResponseRequested = pMessageItem->IsResponseRequested();

    if (!pMessageItem->GetReferences().IsEmpty()) {
        pMessage->References = m_pObjPool->Create<std::string>(
            pMessageItem->GetReferences().GetData());
    }

    if (!pMessageItem->GetReplyTo()->empty()) {
        pMessage->ReplyTo = m_pObjPool->Create<ews2__ArrayOfRecipientsType>();
        ToArrayOfReceipients(
            pMessageItem->GetReplyTo(), pMessage->ReplyTo, m_pObjPool);
    }

    if (pMessageItem->GetReceivedBy()) {
        pMessage->ReceivedBy = m_pObjPool->Create<ews2__SingleRecipientType>();
        ToRecipient(pMessageItem->GetReceivedBy(),
                    pMessage->ReceivedBy, m_pObjPool);
    }

    if (pMessageItem->GetReceivedRepresenting()) {
        pMessage->ReceivedRepresenting = m_pObjPool->Create<
            ews2__SingleRecipientType>();
        ToRecipient(
            pMessageItem->GetReceivedRepresenting(),
            pMessage->ReceivedRepresenting, m_pObjPool);
    }

    return pItem;
}

CEWSMessageItem * CEWSMessageItem::CreateInstance() {
    return dynamic_cast<CEWSMessageItem *>(CEWSItem::CreateInstance(
        CEWSItem::Item_Message));
}

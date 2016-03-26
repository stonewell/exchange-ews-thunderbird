/*
 * ews_recipient_gsoap_impl.cpp
 *
 *  Created on: Jul 25, 2014
 *      Author: stone
 */
#include "libews_gsoap.h"
#include <memory>
#include <map>

using namespace ews;

CEWSEmailAddress::CEWSEmailAddress() {
}

CEWSEmailAddress::~CEWSEmailAddress() {
}


CEWSRecipient * CEWSRecipient::CreateInstance() {
	return new CEWSRecipientGsoapImpl();
}

CEWSRecipientGsoapImpl * CEWSRecipientGsoapImpl::CreateInstance(
		ews2__SingleRecipientType * pItem) {
	if (pItem->__union_SingleRecipientType
			!= SOAP_UNION__ews2__union_SingleRecipientType_Mailbox)
		return NULL;

	_ews2__union_SingleRecipientType * pRecipientType =
			&pItem->union_SingleRecipientType;
	return CreateInstance(pRecipientType->Mailbox);
}

CEWSRecipientGsoapImpl * CEWSRecipientGsoapImpl::CreateInstance(
		ews2__EmailAddressType * pItem) {
	std::auto_ptr<CEWSRecipientGsoapImpl> pEWSRecipient(
			new CEWSRecipientGsoapImpl());

	if (pItem->EmailAddress) {
      pEWSRecipient->SetEmailAddress(pItem->EmailAddress->c_str());
	}

	if (pItem->ItemId) {
      pEWSRecipient->SetItemId(pItem->ItemId->Id.c_str());

      if (pItem->ItemId->ChangeKey) {
          pEWSRecipient->SetChangeKey(pItem->ItemId->ChangeKey->c_str());
      }
	}

	if (pItem->Name) {
      pEWSRecipient->SetName(pItem->Name->c_str());
	}

    if (pItem->RoutingType)
        pEWSRecipient->SetRoutingType(pItem->RoutingType->c_str());

    if (pItem->MailboxType) {
	    pEWSRecipient->SetMailboxType((CEWSEmailAddress::EWSMailboxTypeEnum)(*pItem->MailboxType));
    }

	return pEWSRecipient.release();
}

bool CEWSRecipientGsoapImpl::IsSame(CEWSRecipient * pRecipient) {
	if (this == pRecipient)
		return true;

	if (NULL == pRecipient)
		return false;

	return m_EmailAddress.CompareTo(pRecipient->GetEmailAddress()) == 0;
}

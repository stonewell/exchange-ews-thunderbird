#include "libews_gsoap.h"
#include <memory>
#include <map>

using namespace ews;

CEWSContactItem::CEWSContactItem() {
}

CEWSContactItem::~CEWSContactItem() {
}

CEWSCompleteName::CEWSCompleteName() {
}

CEWSCompleteName::~CEWSCompleteName() {
}

CEWSPhysicalAddress::CEWSPhysicalAddress() {
}

CEWSPhysicalAddress::~CEWSPhysicalAddress() {
}

CEWSContactItem * CEWSContactItem::CreateInstance() {
  return dynamic_cast<CEWSContactItem *>(CEWSItem::CreateInstance(
      CEWSItem::Item_Contact));
}

#define SET_PROPERTY(x, y, p)	\
	if (x->p) \
		y->Set##p(*x->p);

#define SET_PROPERTY_STR(x, y, p)	\
	if (x->p) \
		y->Set##p(x->p->c_str());

static void FromCompleteNameType(CEWSCompleteName * pCompleteName,
                             ews2__CompleteNameType * pCompleteNameType) {
	SET_PROPERTY_STR(pCompleteNameType, pCompleteName, Title);
	SET_PROPERTY_STR(pCompleteNameType, pCompleteName, FirstName);
	SET_PROPERTY_STR(pCompleteNameType, pCompleteName, MiddleName);
	SET_PROPERTY_STR(pCompleteNameType, pCompleteName, LastName);
	SET_PROPERTY_STR(pCompleteNameType, pCompleteName, Suffix);
	SET_PROPERTY_STR(pCompleteNameType, pCompleteName, Initials);
	SET_PROPERTY_STR(pCompleteNameType, pCompleteName, FullName);
	SET_PROPERTY_STR(pCompleteNameType, pCompleteName, Nickname);
	SET_PROPERTY_STR(pCompleteNameType, pCompleteName, YomiFirstName);
	SET_PROPERTY_STR(pCompleteNameType, pCompleteName, YomiLastName);
}

template<class T>
static void DictionaryTypeToStringList(T * pType,
                                       CEWSStringList * pStringList) {
	size_t count = pType->Entry.size();
	pStringList->resize(count);
	for(size_t i = 0; i < count; i++) {
		if ((size_t)pType->Entry[i]->Key >= pStringList->size()) {
			pStringList->resize(pType->Entry[i]->Key + 1);
		}
		CEWSString v(pType->Entry[i]->__item.c_str());
		(*pStringList)[pType->Entry[i]->Key] = v;
	}
}

#define DICTIONARY_TYPE_TO_STRING_LIST(x, y, p) \
	if (x->p) { \
		y->Set##p(new CEWSStringList()); \
		DictionaryTypeToStringList(x->p, \
		                           y->Get##p()); \
	}

static void FromPhysicalAddressType(ews2__PhysicalAddressDictionaryEntryType * pType,
                                    CEWSPhysicalAddress * pAddress) {
	SET_PROPERTY_STR(pType, pAddress, Street);
	SET_PROPERTY_STR(pType, pAddress, City);
	SET_PROPERTY_STR(pType, pAddress, State);
	SET_PROPERTY_STR(pType, pAddress, CountryOrRegion);
	SET_PROPERTY_STR(pType, pAddress, PostalCode);
}

static void PhysicalAddressTypeToList(ews2__PhysicalAddressDictionaryType * pType,
                                       CEWSPhysicalAddressList * pList) {
	size_t count = pType->Entry.size();
	pList->resize(count);
	for(size_t i = 0; i < count; i++) {
		if ((size_t)pType->Entry[i]->Key >= pList->size()) {
			pList->resize(pType->Entry[i]->Key + 1);
		}
		CEWSPhysicalAddressGsoapImpl * p = new CEWSPhysicalAddressGsoapImpl();
		FromPhysicalAddressType(pType->Entry[i], p);
		(*pList)[pType->Entry[i]->Key] = p;
	}
}

#define PHSICAL_ADDRESS_TYPE_TO_LIST(x, y, p)	  \
	if (x->p) { \
		y->Set##p(new CEWSPhysicalAddressList()); \
		PhysicalAddressTypeToList(x->p, \
		                           y->Get##p()); \
	}

static void FromContactItemType(CEWSContactItemGsoapImpl * pContactItem,
                                ews2__ContactItemType * pContactItemType) {
	SET_PROPERTY_STR(pContactItemType, pContactItem,DisplayName);
	SET_PROPERTY_STR(pContactItemType, pContactItem,GivenName);
	SET_PROPERTY_STR(pContactItemType, pContactItem,Initials);
	SET_PROPERTY_STR(pContactItemType, pContactItem,MiddleName);
	SET_PROPERTY_STR(pContactItemType, pContactItem,Nickname);
	SET_PROPERTY_STR(pContactItemType, pContactItem,CompanyName);
	SET_PROPERTY_STR(pContactItemType, pContactItem,AssistantName);
	SET_PROPERTY(pContactItemType, pContactItem,Birthday)
	SET_PROPERTY_STR(pContactItemType, pContactItem,BusinessHomePage);
	SET_PROPERTY_STR(pContactItemType, pContactItem,Department);
	SET_PROPERTY_STR(pContactItemType, pContactItem,Generation);
	SET_PROPERTY_STR(pContactItemType, pContactItem,JobTitle);
	SET_PROPERTY_STR(pContactItemType, pContactItem,Manager);
	SET_PROPERTY_STR(pContactItemType, pContactItem,Mileage);
	SET_PROPERTY_STR(pContactItemType, pContactItem,OfficeLocation);
	SET_PROPERTY_STR(pContactItemType, pContactItem,Profession);
	SET_PROPERTY_STR(pContactItemType, pContactItem,SpouseName);
	SET_PROPERTY_STR(pContactItemType, pContactItem,Surname);
	SET_PROPERTY(pContactItemType, pContactItem,WeddingAnniversary);

	if (pContactItemType->ContactSource) {
		pContactItem->SetContactSource((CEWSContactItem::EWSContactSourceEnum)(*pContactItemType->ContactSource));
	}

	if (pContactItemType->CompleteName) {
		CEWSCompleteNameGsoapImpl * pCompleteName =
				new CEWSCompleteNameGsoapImpl();
		pContactItem->SetCompleteName(pCompleteName);

		FromCompleteNameType(pCompleteName, pContactItemType->CompleteName);
	}

	DICTIONARY_TYPE_TO_STRING_LIST(pContactItemType,
	                               pContactItem,
	                               EmailAddresses);
	PHSICAL_ADDRESS_TYPE_TO_LIST(pContactItemType,
	                               pContactItem,
	                               PhysicalAddresses);
	DICTIONARY_TYPE_TO_STRING_LIST(pContactItemType,
	                               pContactItem,
	                               PhoneNumbers);
	ARRAY_OF_STRING_TYPE_TO_STRING_LIST(pContactItemType,
	                               pContactItem,
	                               Children);
	ARRAY_OF_STRING_TYPE_TO_STRING_LIST(pContactItemType,
	                               pContactItem,
	                               Companies);
	DICTIONARY_TYPE_TO_STRING_LIST(pContactItemType,
	                               pContactItem,
	                               ImAddresses);

	if (pContactItemType->PostalAddressIndex
	    && pContactItemType->PhysicalAddresses) {
		pContactItem->SetPostalAddress(pContactItem->GetPhysicalAddresses()->at(*pContactItemType->PostalAddressIndex));
	}
}

void CEWSContactItemGsoapImpl::FromItemType(ews2__ItemType * pItem) {
  CEWSItemGsoapImpl::FromItemType(pItem);

  ews2__ContactItemType * pContactItem = dynamic_cast<ews2__ContactItemType *>(pItem);

  if (pContactItem) {
	  FromContactItemType(this, pContactItem);
  }
}

CEWSContactOperation::CEWSContactOperation() {

}

CEWSContactOperation::~CEWSContactOperation() {

}

CEWSContactOperationGsoapImpl::CEWSContactOperationGsoapImpl(CEWSSessionGsoapImpl * pSession)
	: m_pSession(pSession) {

}

CEWSContactOperationGsoapImpl::~CEWSContactOperationGsoapImpl() {

}

CEWSItemList * CEWSContactOperationGsoapImpl::ResolveNames(const CEWSString & unresolvedEntry, CEWSError * pError) {
	CEWSSessionRequestGuard guard(m_pSession);

	ews1__ResolveNamesType request;
	__ews1__ResolveNamesResponse response;
	std::auto_ptr<CEWSItemList> pItemList(new CEWSItemList());

	request.ReturnFullContactData = true;
	request.SearchScope = ews2__ResolveNamesSearchScopeType__ContactsActiveDirectory;

	request.UnresolvedEntry = unresolvedEntry;

	int ret = m_pSession->GetProxy()->ResolveNames(&request, response);

	if (ret != SOAP_OK) {
		if (pError) {
			pError->SetErrorMessage(m_pSession->GetErrorMsg());
			pError->SetErrorCode(ret);
		}

		return NULL;
	}

	ews1__ArrayOfResponseMessagesType * messages =
			response.ews1__ResolveNamesResponse->ResponseMessages;
	for (int i = 0; i < messages->__size_ArrayOfResponseMessagesType; i++) {
		__ews1__union_ArrayOfResponseMessagesType * message =
				messages->__union_ArrayOfResponseMessagesType + i;

		if (message->__union_ArrayOfResponseMessagesType
				!=
				SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_ResolveNamesResponseMessage) {
			continue;
		}

		ews1__ResolveNamesResponseMessageType * responseMessage =
				message->union_ArrayOfResponseMessagesType.ResolveNamesResponseMessage;

		if (responseMessage->ResponseClass
				== ews2__ResponseClassType__Error) {
            enum ews1__ResponseCodeType * responseCode =
                    responseMessage->__ResponseMessageType_sequence->ResponseCode;
            
            if (responseCode &&
                ((*responseCode) == ews1__ResponseCodeType__ErrorNameResolutionNoResults
                 || (*responseCode) == ews1__ResponseCodeType__ErrorNameResolutionNoMailbox)) {
                //No Results 
                return pItemList.release();
            }

            SAVE_ERROR(pError, responseMessage);

			return NULL;
		}

		ews2__ArrayOfResolutionType * arrayOfResolutionType = responseMessage->ResolutionSet;

		std::vector<ews2__ResolutionType * >::iterator it =
				arrayOfResolutionType->Resolution.begin();

		while (it != arrayOfResolutionType->Resolution.end()) {
			ews2__ResolutionType * resolutionType = *it;

			if (!resolutionType->Contact && !resolutionType->Mailbox){
				it++;
				continue;
			}

			std::auto_ptr<CEWSContactItem> contactItem(dynamic_cast<CEWSContactItem*>(CEWSItemGsoapImpl::CreateInstance(CEWSItem::Item_Contact,
					resolutionType->Contact)));

			if (resolutionType->Mailbox) {
                SET_PROPERTY_STR(resolutionType->Mailbox, contactItem, Name);
                SET_PROPERTY_STR(resolutionType->Mailbox, contactItem, EmailAddress);
                SET_PROPERTY_STR(resolutionType->Mailbox, contactItem, RoutingType);
                
                if (resolutionType->Mailbox->MailboxType) {
                    contactItem->SetMailboxType((CEWSAttendee::EWSMailboxTypeEnum)(*resolutionType->Mailbox->MailboxType));
                    contactItem->SetMailList((*resolutionType->Mailbox->MailboxType
                                              == ews2__MailboxTypeType__PublicDL) ||
                                             (*resolutionType->Mailbox->MailboxType == ews2__MailboxTypeType__PrivateDL));
                    if (resolutionType->Mailbox->ItemId) {
                        contactItem->SetItemId(resolutionType->Mailbox->ItemId->Id.c_str());
      
                        if (resolutionType->Mailbox->ItemId->ChangeKey)
                            contactItem->SetChangeKey(resolutionType->Mailbox->ItemId->ChangeKey->c_str());
                    }
                    
                }
			}

			pItemList->push_back(contactItem.release());
			it++;
		}
	}

	return pItemList.release();
}

/*
 * libews_gsoap_utils.cpp
 *
 *  Created on: Jul 25, 2014
 *      Author: stone
 */
#include "libews.h"
#include "libews_gsoap.h"
#include "libews_gsoap_utils.h"

#include <memory>
#include "string.h"

ews::CEWSString ews::XsdBase64BinaryToEWSString(struct soap *soap,
		xsd__base64Binary * pBinary) {
	if (pBinary->__ptr && pBinary->__size > 0) {
		int n = 0;
		std::auto_ptr<const char> v(
				soap_base642s(soap, (const char *) pBinary->__ptr, NULL, 0,
						&n));

		return ews::CEWSString(v.get(), n);
	}

	return "";
}

xsd__base64Binary * ews::EWSStringToXsdBase64Binary(struct soap *soap,
		const ews::CEWSString & v) {
	std::auto_ptr<xsd__base64Binary> result(new xsd__base64Binary());
	result->__ptr = NULL;
	result->__size = 0;

	if (v.IsEmpty())
		return result.release();

	result->__ptr = (unsigned char *)v.GetData();
	result->__size = v.GetLength();

	return result.release();
}

ews2__DistinguishedFolderIdNameType ews::ConvertDistinguishedFolderIdName(
		ews::CEWSFolder::EWSDistinguishedFolderIdNameEnum name) {
	switch (name) {
	case ews::CEWSFolder::calendar:
		return ews2__DistinguishedFolderIdNameType__calendar;
	case ews::CEWSFolder::contacts:
		return ews2__DistinguishedFolderIdNameType__contacts;
	case ews::CEWSFolder::deleteditems:
		return ews2__DistinguishedFolderIdNameType__deleteditems;
	case ews::CEWSFolder::drafts:
		return ews2__DistinguishedFolderIdNameType__drafts;
	case ews::CEWSFolder::inbox:
		return ews2__DistinguishedFolderIdNameType__inbox;
	case ews::CEWSFolder::journal:
		return ews2__DistinguishedFolderIdNameType__journal;
	case ews::CEWSFolder::notes:
		return ews2__DistinguishedFolderIdNameType__notes;
	case ews::CEWSFolder::outbox:
		return ews2__DistinguishedFolderIdNameType__outbox;
	case ews::CEWSFolder::sentitems:
		return ews2__DistinguishedFolderIdNameType__sentitems;
	case ews::CEWSFolder::tasks:
		return ews2__DistinguishedFolderIdNameType__tasks;
	case ews::CEWSFolder::msgfolderroot:
		return ews2__DistinguishedFolderIdNameType__msgfolderroot;
	case ews::CEWSFolder::publicfoldersroot:
		return ews2__DistinguishedFolderIdNameType__publicfoldersroot;
	case ews::CEWSFolder::root:
		return ews2__DistinguishedFolderIdNameType__root;
	case ews::CEWSFolder::junkemail:
		return ews2__DistinguishedFolderIdNameType__junkemail;
	case ews::CEWSFolder::searchfolders:
		return ews2__DistinguishedFolderIdNameType__searchfolders;
	case ews::CEWSFolder::voicemail:
		return ews2__DistinguishedFolderIdNameType__voicemail;
	default:
		return (ews2__DistinguishedFolderIdNameType)name;
	}
}

void ews::convertNotifyTypeList(int notifyTypeFlags, ews::CEWSIntList & notifyTypesSoap) {

	if (notifyTypeFlags & ews::CEWSSubscription::COPY) {
		notifyTypesSoap.push_back(ews2__NotificationEventTypeType__CopiedEvent);
	}
	if (notifyTypeFlags & ews::CEWSSubscription::CREATE) {
		notifyTypesSoap.push_back(ews2__NotificationEventTypeType__CreatedEvent);
	}
	if (notifyTypeFlags & ews::CEWSSubscription::DEL) {
		notifyTypesSoap.push_back(ews2__NotificationEventTypeType__DeletedEvent);
	}
	if (notifyTypeFlags & ews::CEWSSubscription::MODIFY) {
		notifyTypesSoap.push_back(ews2__NotificationEventTypeType__ModifiedEvent);
	}
	if (notifyTypeFlags & ews::CEWSSubscription::MOVE) {
		notifyTypesSoap.push_back(ews2__NotificationEventTypeType__MovedEvent);
	}
	if (notifyTypeFlags & ews::CEWSSubscription::NEW_MAIL) {
		notifyTypesSoap.push_back(ews2__NotificationEventTypeType__NewMailEvent);
	}

}


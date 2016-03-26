/*
 * ews_subscription_gsoap_impl.cp p
 *
 *  Created on: 2014年9月3日
 *      Author: stone
 */

#include "libews_gsoap.h"
#include <memory>
#include <sstream>
#include "ews_gsoap_notification_handler.cpp"

using namespace ews;

CEWSSubscriptionOperationGsoapImpl::CEWSSubscriptionOperationGsoapImpl(CEWSSessionGsoapImpl * pSession) :
	m_pSession(pSession) {

}

CEWSSubscriptionOperationGsoapImpl::~CEWSSubscriptionOperationGsoapImpl() {

}

CEWSSubscription * CEWSSubscriptionOperationGsoapImpl::SubscribeWithPull(
    const CEWSStringList & folderIdList,
    const CEWSIntList & distinguishedIdNameList, int notifyTypeFlags,
    int timeout,
    CEWSError * pError) {
	return __Subscribe(folderIdList,
	                   distinguishedIdNameList,
	                   notifyTypeFlags,
	                   timeout,
                       NULL, //url
	                   NULL,//callback
	                   true,
	                   pError);
}

CEWSSubscription * CEWSSubscriptionOperationGsoapImpl::SubscribeWithPush(
    const CEWSStringList & folderIdList,
    const CEWSIntList & distinguishedIdNameList, int notifyTypeFlags,
    int timeout,
    const CEWSString & url,
    CEWSSubscriptionCallback * callback,
    CEWSError * pError) {
	return __Subscribe(folderIdList,
	                   distinguishedIdNameList,
	                   notifyTypeFlags,
	                   timeout,
                       url,
	                   callback,
	                   false,
	                   pError);
}

CEWSSubscription * CEWSSubscriptionOperationGsoapImpl::__Subscribe(
    const CEWSStringList & folderIdList,
    const CEWSIntList & distinguishedIdNameList, int notifyTypeFlags,
    int timeout,
    const CEWSString & url,
    CEWSSubscriptionCallback * callback,
	bool pullSubscription,
    CEWSError * pError) {
	std::auto_ptr<CEWSSubscriptionGsoapImpl> subscription(new CEWSSubscriptionGsoapImpl());

	CEWSSessionRequestGuard guard(m_pSession);

	CEWSObjectPool objPool;

	ews1__SubscribeType request;
	__ews1__SubscribeResponse response;

	if (pullSubscription)
		request.__union_SubscribeType = SOAP_UNION__ews1__union_SubscribeType_PullSubscriptionRequest;
	else
		request.__union_SubscribeType = SOAP_UNION__ews1__union_SubscribeType_PushSubscriptionRequest;

	ews2__PullSubscriptionRequestType pullSubscriptionRequestType;
	ews2__PushSubscriptionRequestType pushSubscriptionRequestType;

	_ews1__union_SubscribeType * union_SubscribeType = &request.union_SubscribeType;

	ews2__BaseSubscriptionRequestType * baseSubscriptionRequestType = NULL;
	if (pullSubscription) {
		baseSubscriptionRequestType =
				union_SubscribeType->PullSubscriptionRequest =
						&pullSubscriptionRequestType;
	}
	else {
		baseSubscriptionRequestType =
				union_SubscribeType->PushSubscriptionRequest =
						&pushSubscriptionRequestType;
	}

	//Folder Ids
	ews2__NonEmptyArrayOfBaseFolderIdsType folderIds;
	baseSubscriptionRequestType->FolderIds = &folderIds;

	int size = folderIdList.size() + distinguishedIdNameList.size();

	folderIds.__size_NonEmptyArrayOfBaseFolderIdsType = size;
	folderIds.__union_NonEmptyArrayOfBaseFolderIdsType =
			objPool.CreateArray<__ews2__union_NonEmptyArrayOfBaseFolderIdsType>(size);

	for (int i=0;i < folderIdList.size(); i++) {
		__ews2__union_NonEmptyArrayOfBaseFolderIdsType * baseFolderId =
				&folderIds.__union_NonEmptyArrayOfBaseFolderIdsType[i];

		baseFolderId->__union_NonEmptyArrayOfBaseFolderIdsType =
				SOAP_UNION__ews2__union_NonEmptyArrayOfBaseFolderIdsType_FolderId;
		baseFolderId->union_NonEmptyArrayOfBaseFolderIdsType.FolderId =
				objPool.Create<ews2__FolderIdType>();
		baseFolderId->union_NonEmptyArrayOfBaseFolderIdsType.FolderId->Id = folderIdList.at(i);
	}

	for (int i=0; i < distinguishedIdNameList.size(); i++) {
		__ews2__union_NonEmptyArrayOfBaseFolderIdsType * baseFolderId =
				&folderIds.__union_NonEmptyArrayOfBaseFolderIdsType[folderIdList.size() + i];

		baseFolderId->__union_NonEmptyArrayOfBaseFolderIdsType =
				SOAP_UNION__ews2__union_NonEmptyArrayOfBaseFolderIdsType_DistinguishedFolderId;
		baseFolderId->union_NonEmptyArrayOfBaseFolderIdsType.DistinguishedFolderId =
				objPool.Create<ews2__DistinguishedFolderIdType>();
		baseFolderId->union_NonEmptyArrayOfBaseFolderIdsType.DistinguishedFolderId->Id =
				ConvertDistinguishedFolderIdName((CEWSFolder::EWSDistinguishedFolderIdNameEnum)distinguishedIdNameList.at(i));
	}

	//EventType
	ews2__NonEmptyArrayOfNotificationEventTypesType eventTypes;
	baseSubscriptionRequestType->EventTypes = &eventTypes;

	CEWSIntList notifyTypesSoap;

	convertNotifyTypeList(notifyTypeFlags, notifyTypesSoap);

	eventTypes.__size_NonEmptyArrayOfNotificationEventTypesType = notifyTypesSoap.size();
	eventTypes.__union_NonEmptyArrayOfNotificationEventTypesType =
			objPool.CreateArray<__ews2__union_NonEmptyArrayOfNotificationEventTypesType>(notifyTypesSoap.size());

	for (int i=0;i<notifyTypesSoap.size();i++) {
		__ews2__union_NonEmptyArrayOfNotificationEventTypesType * t =
				eventTypes.__union_NonEmptyArrayOfNotificationEventTypesType + i;

		t->__union_NonEmptyArrayOfNotificationEventTypesType =
				SOAP_UNION__ews2__union_NonEmptyArrayOfNotificationEventTypesType_EventType;
		t->union_NonEmptyArrayOfNotificationEventTypesType.EventType =
				(ews2__NotificationEventTypeType)notifyTypesSoap.at(i);
	}

	//Timeout
	if (pullSubscription)
		pullSubscriptionRequestType.Timeout = timeout;

	if (!pullSubscription) {
		pushSubscriptionRequestType.StatusFrequency = 1;
		pushSubscriptionRequestType.URL = url.GetData();
	}

	int ret = m_pSession->GetProxy()->Subscribe(&request, response);

	if (ret != SOAP_OK) {
		if (pError) {
			pError->SetErrorMessage(m_pSession->GetErrorMsg());
			pError->SetErrorCode(ret);
		}

		return NULL;
	}

	do {
		if (1
		    != response.ews1__SubscribeResponse->ResponseMessages->__size_ArrayOfResponseMessagesType) {
			break;
		}

		__ews1__union_ArrayOfResponseMessagesType * responseMessage =
				response.ews1__SubscribeResponse->ResponseMessages->__union_ArrayOfResponseMessagesType;

		if (responseMessage->__union_ArrayOfResponseMessagesType
		    != SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_SubscribeResponseMessage) {
			break;
		}

		if (responseMessage->union_ArrayOfResponseMessagesType.SubscribeResponseMessage->ResponseClass
		    != ews2__ResponseClassType__Success) {
            SAVE_ERROR(pError,
                       responseMessage
                       ->union_ArrayOfResponseMessagesType
                       .SubscribeResponseMessage);

			return NULL;
		}

		subscription->SetSubscriptionId(responseMessage->union_ArrayOfResponseMessagesType.SubscribeResponseMessage->SubscriptionId->c_str());
		subscription->SetWaterMark(responseMessage->union_ArrayOfResponseMessagesType.SubscribeResponseMessage->Watermark->c_str());

		return subscription.release();
	} while (false);

	//INVALID_SERVER_RESPONSE
	if (pError) {
		pError->SetErrorMessage("Invalid Server Response");
		pError->SetErrorCode(EWS_FAIL);
	}

	return NULL;
}

bool CEWSSubscriptionOperationGsoapImpl::Unsubscribe(
    const CEWSSubscription * subscription, CEWSError * pError) {
	return Unsubscribe(subscription->GetSubscriptionId(),
	                   subscription->GetWaterMark(),
	                   pError);
}

bool CEWSSubscriptionOperationGsoapImpl::Unsubscribe(
    const CEWSString & subscriptionId,
    const CEWSString & waterMark,
    CEWSError * pError) {

	CEWSSessionRequestGuard guard(m_pSession);

	CEWSObjectPool objPool;

	ews1__UnsubscribeType request;
	__ews1__UnsubscribeResponse response;

	request.SubscriptionId = subscriptionId;

	int ret = m_pSession->GetProxy()->Unsubscribe(&request, response);

	if (ret != SOAP_OK) {
		if (pError) {
			pError->SetErrorMessage(m_pSession->GetErrorMsg());
			pError->SetErrorCode(ret);
		}

		return false;
	}

	do {
		if (1
		    != response.ews1__UnsubscribeResponse->ResponseMessages->__size_ArrayOfResponseMessagesType) {
			break;
		}

		__ews1__union_ArrayOfResponseMessagesType * responseMessage =
				response.ews1__UnsubscribeResponse->ResponseMessages->__union_ArrayOfResponseMessagesType;

		if (responseMessage->__union_ArrayOfResponseMessagesType
		    != SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_UnsubscribeResponseMessage) {
			break;
		}

		if (responseMessage->union_ArrayOfResponseMessagesType.UnsubscribeResponseMessage->ResponseClass
		    != ews2__ResponseClassType__Success) {
            SAVE_ERROR(pError,
                       responseMessage
                       ->union_ArrayOfResponseMessagesType
                       .UnsubscribeResponseMessage);

			return false;
		}

		return true;
	} while (false);

	//INVALID_SERVER_RESPONSE
	if (pError) {
		pError->SetErrorMessage("Invalid Server Response");
		pError->SetErrorCode(EWS_FAIL);
	}

	return false;
}

bool CEWSSubscriptionOperationGsoapImpl::GetEvents(
    const CEWSSubscription * subscription,
    CEWSSubscriptionCallback * callback,
    bool & moreEvents,
    CEWSError * pError) {
	return GetEvents(subscription->GetSubscriptionId(),
	                 subscription->GetWaterMark(),
	                 callback,
	                 moreEvents,
	                 pError);
}


bool CEWSSubscriptionOperationGsoapImpl::GetEvents(
    const CEWSString & subscriptionId,
    const CEWSString & waterMark,
    CEWSSubscriptionCallback * callback,
    bool & moreEvents,
    CEWSError * pError) {

	CEWSSessionRequestGuard guard(m_pSession);

	CEWSObjectPool objPool;

	ews1__GetEventsType request;
	__ews1__GetEventsResponse response;

	request.SubscriptionId = subscriptionId;
	request.Watermark = waterMark;

	int ret = m_pSession->GetProxy()->GetEvents(&request, response);

	if (ret != SOAP_OK) {
		if (pError) {
			pError->SetErrorMessage(m_pSession->GetErrorMsg());
			pError->SetErrorCode(ret);
		}

		return false;
	}

	do {
		if (1
		    != response.ews1__GetEventsResponse->ResponseMessages->__size_ArrayOfResponseMessagesType) {
			break;
		}

		__ews1__union_ArrayOfResponseMessagesType * responseMessage =
				response.ews1__GetEventsResponse->ResponseMessages->__union_ArrayOfResponseMessagesType;

		if (responseMessage->__union_ArrayOfResponseMessagesType
		    != SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_GetEventsResponseMessage) {
			break;
		}

		ews1__GetEventsResponseMessageType * getEventsResponseMessage =
				responseMessage->union_ArrayOfResponseMessagesType.GetEventsResponseMessage;
		if (getEventsResponseMessage->ResponseClass
		    != ews2__ResponseClassType__Success) {
            SAVE_ERROR(pError,
                       getEventsResponseMessage);

			return false;
		}

		ews2__NotificationType *Notification = getEventsResponseMessage->Notification;
		moreEvents = Notification->MoreEvents;

		ProcessNotificationMessage(Notification,
				callback);
		return true;
	} while (false);

	//INVALID_SERVER_RESPONSE
	if (pError) {
		pError->SetErrorMessage("Invalid Server Response");
		pError->SetErrorCode(EWS_FAIL);
	}

	return false;
}

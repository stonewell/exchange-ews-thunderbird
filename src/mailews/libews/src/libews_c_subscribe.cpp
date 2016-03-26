/*
 * libews_c_subscribe.cpp
 *
 *  Created on: Jan 5, 2015
 *      Author: stone
 */

#include "libews.h"
#include <memory>
#include <stdlib.h>
#include <map>
#include <string>
#include <stdio.h>

using namespace ews;

#define ON_ERROR_RETURN(x)	  \
	if ((x)) { \
		if (pp_err_msg) \
			*pp_err_msg = strdup(error.GetErrorMessage()); \
		return error.GetErrorCode(); \
	}

#define SAFE_FREE(x)	  \
	if ((x)) free((x));

#include "libews_c_subscribe_callback.h"

typedef std::map<std::string, ews::c::CLocalEWSSubscriptionCallback_C *> CEWSPushSubscriptionCallbackMap;

static CEWSPushSubscriptionCallbackMap g_push_subscription_callback_map;

static int __ews_subscribe_to_folders(ews_session * session,
                                      const char ** folder_ids,
                                      int folder_id_count,
                                      int * distinguished_ids,
                                      int distinguished_id_count,
                                      int notifyFlags,
                                      int timeout,
                                      const char * url,
                                      ews_event_notification_callback * callback,
                                      bool pull_subscription,
                                      ews_subscription ** pp_subscription,
                                      char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !url)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSSubscriptionOperation> pItemOp(
	    pSession->CreateSubscriptionOperation());
	CEWSError error;
	CEWSStringList folderIds;
	CEWSIntList distinguishedIds;

	if (folder_ids && (folder_id_count > 0)) {
		for (int i=0;i<folder_id_count;i++) {
            if (!folder_ids[i]) continue;
            
			folderIds.push_back(folder_ids[i]);
		}
	}

	if (distinguished_ids && (distinguished_id_count > 0)) {
		for (int i=0;i<distinguished_id_count;i++) {
			distinguishedIds.push_back(distinguished_ids[i]);
		}
	}
	
	std::auto_ptr<CEWSSubscription> pSubscription(NULL);
	std::auto_ptr<ews::c::CLocalEWSSubscriptionCallback_C> pCallback(new ews::c::CLocalEWSSubscriptionCallback_C(callback));
	
	if (pull_subscription) {
		pSubscription.reset(pItemOp->SubscribeWithPull(folderIds,
		                                               distinguishedIds,
		                                               notifyFlags,
		                                               timeout,
		                                               &error));
	} else {
		//need to register the push subscribe to system
		pSubscription.reset(pItemOp->SubscribeWithPush(folderIds,
		                                               distinguishedIds,
		                                               notifyFlags,
		                                               timeout,
                                                       url,
		                                               pCallback.get(),
		                                               &error));
	}
	ON_ERROR_RETURN(pSubscription.get() == NULL);

	if (!pull_subscription) {
		std::string subscription_id = (const char *)pSubscription->GetSubscriptionId();
		g_push_subscription_callback_map.insert(std::pair<std::string, ews::c::CLocalEWSSubscriptionCallback_C *>(subscription_id, pCallback.get()));
		pCallback.release();
	}
	
	std::auto_ptr<ews_subscription> p((ews_subscription*)malloc(sizeof(ews_subscription)));
	p->subscription_id = strdup(pSubscription->GetSubscriptionId());
	p->water_mark = strdup(pSubscription->GetWaterMark());

	*pp_subscription = p.release();
	
	return EWS_SUCCESS;
}

int ews_subscribe_to_folders_with_pull(ews_session * session,
                                       const char ** folder_ids,
                                       int folder_id_count,
                                       int * distinguished_ids,
                                       int distinguished_id_count,
                                       int notifyFlags,
                                       int timeout,
                                       ews_subscription ** pp_subscription,
                                       char ** pp_error_msg) {
	return __ews_subscribe_to_folders(session,
	                                  folder_ids,
	                                  folder_id_count,
	                                  distinguished_ids,
	                                  distinguished_id_count,
	                                  notifyFlags,
	                                  timeout,
                                      NULL,
	                                  NULL,
	                                  true,
	                                  pp_subscription,
	                                  pp_error_msg);
	                                  
}

int ews_subscribe_to_folders_with_push(ews_session * session,
                                       const char ** folder_ids,
                                       int folder_id_count,
                                       int * distinguished_ids,
                                       int distinguished_id_count,
                                       int notifyFlags,
                                       int timeout,
                                       const char * url,
                                       ews_event_notification_callback * callback,
                                       ews_subscription ** pp_subscription,
                                       char ** pp_error_msg)  {
	return __ews_subscribe_to_folders(session,
	                                  folder_ids,
	                                  folder_id_count,
	                                  distinguished_ids,
	                                  distinguished_id_count,
	                                  notifyFlags,
	                                  timeout,
                                      url,
	                                  callback,
	                                  false,
	                                  pp_subscription,
	                                  pp_error_msg);
	                                  
}

int ews_unsubscribe(ews_session * session,
                    ews_subscription * subscription,
                    char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !subscription)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSSubscriptionOperation> pItemOp(
	    pSession->CreateSubscriptionOperation());
	CEWSError error;

	ON_ERROR_RETURN(!pItemOp->Unsubscribe(subscription->subscription_id,
	                                      subscription->water_mark,
	                                      &error));
	CEWSPushSubscriptionCallbackMap::iterator it =
			g_push_subscription_callback_map.find(subscription->subscription_id);

	if (it != g_push_subscription_callback_map.end()) {
		delete it->second;
		g_push_subscription_callback_map.erase(it);
	}

	return EWS_SUCCESS;
}

int ews_get_events(ews_session * session,
                   ews_subscription * subscription,
                   ews_event_notification_callback * callback,
                   bool * more_item,
                   char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !callback || !subscription || !more_item)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSSubscriptionOperation> pItemOp(
	    pSession->CreateSubscriptionOperation());
	CEWSError error;

	std::auto_ptr<ews::c::CLocalEWSSubscriptionCallback_C> pCallback(new ews::c::CLocalEWSSubscriptionCallback_C(callback));

	ON_ERROR_RETURN(!pItemOp->GetEvents(subscription->subscription_id,
	                                    subscription->water_mark,
	                                    pCallback.get(),
	                                    *more_item,
	                                    &error));

	return EWS_SUCCESS;
}

void ews_free_subscription(ews_subscription * subscription) {
	if (!subscription) return;
	SAFE_FREE(subscription->subscription_id);
	SAFE_FREE(subscription->water_mark);
	SAFE_FREE(subscription);
}

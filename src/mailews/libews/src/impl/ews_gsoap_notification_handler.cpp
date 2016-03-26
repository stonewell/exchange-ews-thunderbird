/*
 * ews_gsoap_notification_handler.cpp
 *
 *  Created on: 2015年1月10日
 *      Author: stone
 */

#include "libews.h"

void ProcessNotificationMessage(ews2__NotificationType* Notification,
		ews::CEWSSubscriptionCallback * callback) {
    if (!Notification)
        return;
     
	for (int i = 0;
			i < Notification->__size_NotificationType;
			i++) {
		__ews2__union_NotificationType * union_NotificationType =
				Notification->__union_NotificationType
						+ i;

		switch (union_NotificationType->__union_NotificationType) {
		case SOAP_UNION__ews2__union_NotificationType_CopiedEvent: {
			ews2__MovedCopiedEventType * e =
					union_NotificationType->union_NotificationType.CopiedEvent;
			_ews2__union_MovedCopiedEventType_ * old =
					&e->union_MovedCopiedEventType_;

			_ews2__union_BaseObjectChangedEventType * n =
					&e->union_BaseObjectChangedEventType;

			if (e->__union_MovedCopiedEventType_
					== SOAP_UNION__ews2__union_MovedCopiedEventType__OldFolderId
					&& e->__union_BaseObjectChangedEventType
							== SOAP_UNION__ews2__union_BaseObjectChangedEventType_FolderId) {
				callback->CopyFolder(e->OldParentFolderId->Id.c_str(),
						e->ParentFolderId->Id.c_str(),
						old->OldFolderId->Id.c_str(), n->FolderId->Id.c_str());
			} else if (e->__union_MovedCopiedEventType_
					== SOAP_UNION__ews2__union_MovedCopiedEventType__OldItemId
					&& e->__union_BaseObjectChangedEventType
							== SOAP_UNION__ews2__union_BaseObjectChangedEventType_ItemId) {
				callback->CopyItem(e->OldParentFolderId->Id.c_str(),
						e->ParentFolderId->Id.c_str(),
						old->OldItemId->Id.c_str(), n->ItemId->Id.c_str());
			}
		}
			break;
		case SOAP_UNION__ews2__union_NotificationType_CreatedEvent: {
			ews2__BaseObjectChangedEventType * e =
					union_NotificationType->union_NotificationType.CreatedEvent;

			_ews2__union_BaseObjectChangedEventType * n =
					&e->union_BaseObjectChangedEventType;

			if (e->__union_BaseObjectChangedEventType
					== SOAP_UNION__ews2__union_BaseObjectChangedEventType_FolderId) {
				callback->CreateFolder(e->ParentFolderId->Id.c_str(),
						n->FolderId->Id.c_str());
			} else if (e->__union_BaseObjectChangedEventType
					== SOAP_UNION__ews2__union_BaseObjectChangedEventType_ItemId) {
				callback->CreateItem(e->ParentFolderId->Id.c_str(),
						n->ItemId->Id.c_str());
			}
		}
			break;
		case SOAP_UNION__ews2__union_NotificationType_DeletedEvent: {
			ews2__BaseObjectChangedEventType * e =
					union_NotificationType->union_NotificationType.DeletedEvent;

			_ews2__union_BaseObjectChangedEventType * n =
					&e->union_BaseObjectChangedEventType;

			if (e->__union_BaseObjectChangedEventType
					== SOAP_UNION__ews2__union_BaseObjectChangedEventType_FolderId) {
				callback->DeleteFolder(e->ParentFolderId->Id.c_str(),
						n->FolderId->Id.c_str());
			} else if (e->__union_BaseObjectChangedEventType
					== SOAP_UNION__ews2__union_BaseObjectChangedEventType_ItemId) {
				callback->DeleteItem(e->ParentFolderId->Id.c_str(),
						n->ItemId->Id.c_str());
			}
		}
			break;
		case SOAP_UNION__ews2__union_NotificationType_ModifiedEvent: {
			ews2__ModifiedEventType * e =
					union_NotificationType->union_NotificationType.ModifiedEvent;

			_ews2__union_BaseObjectChangedEventType * n =
					&e->union_BaseObjectChangedEventType;

			if (e->__union_BaseObjectChangedEventType
					== SOAP_UNION__ews2__union_BaseObjectChangedEventType_FolderId) {
				callback->ModifyFolder(e->ParentFolderId->Id.c_str(),
                                       n->FolderId->Id.c_str(),
                                       e->UnreadCount ? *e->UnreadCount : -1);
			} else if (e->__union_BaseObjectChangedEventType
					== SOAP_UNION__ews2__union_BaseObjectChangedEventType_ItemId) {
				callback->ModifyItem(e->ParentFolderId->Id.c_str(),
                                     n->ItemId->Id.c_str(),
                                     e->UnreadCount ? *e->UnreadCount : -1);
			}
		}
			break;
		case SOAP_UNION__ews2__union_NotificationType_MovedEvent: {
			ews2__MovedCopiedEventType * e =
					union_NotificationType->union_NotificationType.MovedEvent;
			_ews2__union_MovedCopiedEventType_ * old =
					&e->union_MovedCopiedEventType_;

			_ews2__union_BaseObjectChangedEventType * n =
					&e->union_BaseObjectChangedEventType;

			if (e->__union_MovedCopiedEventType_
					== SOAP_UNION__ews2__union_MovedCopiedEventType__OldFolderId
					&& e->__union_BaseObjectChangedEventType
							== SOAP_UNION__ews2__union_BaseObjectChangedEventType_FolderId) {
				callback->MoveFolder(e->OldParentFolderId->Id.c_str(),
						e->ParentFolderId->Id.c_str(),
						old->OldFolderId->Id.c_str(), n->FolderId->Id.c_str());
			} else if (e->__union_MovedCopiedEventType_
					== SOAP_UNION__ews2__union_MovedCopiedEventType__OldItemId
					&& e->__union_BaseObjectChangedEventType
							== SOAP_UNION__ews2__union_BaseObjectChangedEventType_ItemId) {
				callback->MoveItem(e->OldParentFolderId->Id.c_str(),
						e->ParentFolderId->Id.c_str(),
						old->OldItemId->Id.c_str(), n->ItemId->Id.c_str());
			}
		}
			break;
		case SOAP_UNION__ews2__union_NotificationType_NewMailEvent: {
			ews2__BaseObjectChangedEventType * e =
					union_NotificationType->union_NotificationType.NewMailEvent;

			_ews2__union_BaseObjectChangedEventType * n =
					&e->union_BaseObjectChangedEventType;

			if (e->__union_BaseObjectChangedEventType
					== SOAP_UNION__ews2__union_BaseObjectChangedEventType_ItemId) {
				callback->NewMail(e->ParentFolderId->Id.c_str(),
						n->ItemId->Id.c_str());
			}
		}
			break;
		default:
			continue;
		}
	}
}




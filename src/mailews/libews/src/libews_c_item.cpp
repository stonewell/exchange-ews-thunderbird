/*
 * libews_c_item.cpp
 *
 *  Created on: Aug 28, 2014
 *      Author: stone
 */

#include "libews.h"
#include <memory>
#include <stdlib.h>
#include <stdio.h>

using namespace ews;

static
void *
safe_malloc(size_t s) {
	void * p = malloc(s);

	memset(p, 0, s);

	return p;
}

#define ON_ERROR_RETURN(x)                                  \
	if ((x)) {                                              \
		if (pp_err_msg)                                     \
			*pp_err_msg = strdup(error.GetErrorMessage());  \
		return error.GetErrorCode();                        \
	}

#define SAFE_FREE(x)                            \
	if ((x)) free((x));

#define SET_VALUE_STR(x, x_n, y, y_n)           \
	y->y_n = strdup(x->Get##x_n().GetData())
#define SET_VALUE(x, x_n, y, y_n)               \
	y->y_n = x->Get##x_n()
#define SET_VALUE_B(x, x_n, y, y_n)             \
	y->y_n = x->Is##x_n()
#define LIST_TO_POINTER_POINTER(x, x_n, y, y_n) \
	list_to_pointer_pointer(x->Get##x_n(),      \
	                        y->y_n##_count,     \
	                        y->y_n)

#define GET_VALUE_STR(x, x_n, y, y_n)           \
	if (y->y_n)                                 \
		x->Set##x_n(y->y_n);
#define GET_VALUE(x, x_n, y, y_n)               \
	x->Set##x_n(y->y_n);
#define GET_VALUE_5(x, x_n, x_t, y, y_n)        \
	x->Set##x_n((x_t)y->y_n);

#define LIST_FROM_POINTER_POINTER(x, x_n, y, y_n)   \
	if (y->y_n##_count > 0 && y->y_n) {             \
		x->Set##x_n(list_from_pointer_pointer(      \
		    y->y_n##_count,                         \
		    y->y_n));                               \
	}

#define LIST_SAFE_FREE_2(x, ___item_free)       \
	if (x) {                                    \
		int c = x##_count;                      \
		for(int i =0;i < c; i++) {              \
			___item_free(x[i]);                 \
		}                                       \
		free(x);                                \
	}

#define LIST_SAFE_FREE(x) LIST_SAFE_FREE_2(x, free)

static ews_item * cewsitem_to_item(const CEWSItem * pItem);
static ews_msg_item * cewsmsgitem_to_msg_item(const CEWSMessageItem * pItem);
static ews_calendar_item * cewscalendaritem_to_calendar_item(const CEWSCalendarItem * pItem);
static ews_task_item * cewstaskitem_to_task_item(const CEWSTaskItem * pItem);
static void cewsitem_to_item_3(const CEWSItem * pItem, ews_item * item);
static void cewsmsgitem_to_msg_item_3(const CEWSMessageItem * pItem, ews_msg_item * msg_item);
static void cewscalendaritem_to_calendar_item_3(const CEWSCalendarItem * pItem, ews_calendar_item * cal_item);
static void cewstaskitem_to_task_item_3(const CEWSTaskItem * pItem, ews_task_item * cal_item);
static CEWSItem * item_to_cewsitem(ews_item * item);
static void __item_to_cewsitem(const ews_item * item, CEWSItem * pItem);
static void __cewsitem_to_item(const CEWSItem * pItem, ews_item * item);
static void cewsitem_to_item_2(const CEWSItem * pItem, ews_item * item);
static CEWSFolder::EWSFolderTypeEnum folder_type_from_item_type(int item_type);
static CEWSMessageItem * msg_item_to_cewsmsgitem(ews_msg_item * item);
static void cewscontactitem_to_contact_item_3(const CEWSContactItem * pItem,
                                              ews_contact_item * item);
static void ews_free_contact_item(ews_contact_item * contact_item);
static void ews_free_calendar_item(ews_calendar_item * calendar_item);
static ews_contact_item * cewscontactitem_to_contact_item(const CEWSContactItem * pItem);
static CEWSCalendarItem * calendar_item_to_cewscalendaritem(ews_calendar_item * item);
static
CEWSTaskItem *
task_item_to_cewstaskitem(ews_task_item * item);
static void ews_free_task_item(ews_task_item * task_item);

namespace ews {
namespace c {
class DLL_LOCAL CEWSItemOperationCallback_C : public CEWSItemOperationCallback {
public:
	CEWSItemOperationCallback_C(ews_sync_item_callback * callback) :
			m_p_callback(callback),
			m_SyncState(""),
			m_IgnoreList() {
		if (m_p_callback->get_sync_state)
			m_SyncState = m_p_callback->get_sync_state(m_p_callback->user_data);

		m_IgnoreList.clear();

		if (m_p_callback->get_ignored_item_ids) {
			int count = 0;
			char ** item_ids = m_p_callback->get_ignored_item_ids(&count, m_p_callback->user_data);

			for(int i = 0; i < count; i++) {
				m_IgnoreList.push_back(item_ids[i]);
			}
		}

	}

	virtual ~CEWSItemOperationCallback_C() {

	}

public:
	virtual const CEWSString & GetSyncState() const {
		return m_SyncState;
	}

	virtual void SetSyncState(const CEWSString & syncState) {
		if (m_p_callback->set_sync_state)
			m_p_callback->set_sync_state(syncState, m_p_callback->user_data);
	}

	virtual int GetSyncMaxReturnItemCount() const {
		if (m_p_callback->get_max_return_item_count) {
			return m_p_callback->get_max_return_item_count(m_p_callback->user_data);
		}

		return 0;
	}

	virtual const CEWSStringList & GetIgnoreItemIdList() const {
		return m_IgnoreList;
	}

	virtual void NewItem(const CEWSItem * pItem) {
		if (m_p_callback->new_item) {
			ews_item *item(cewsitem_to_item(pItem));

			m_p_callback->new_item(item, m_p_callback->user_data);

			ews_free_item(item);
		}
	}

	virtual void UpdateItem(const CEWSItem * pItem) {
		if (m_p_callback->update_item) {
			ews_item *item(cewsitem_to_item(pItem));

			m_p_callback->update_item(item, m_p_callback->user_data);
			ews_free_item(item);
		}
	}

	virtual void DeleteItem(const CEWSItem * pItem) {
		if (m_p_callback->delete_item) {
			ews_item *item(cewsitem_to_item(pItem));

			m_p_callback->delete_item(item, m_p_callback->user_data);
			ews_free_item(item);
		}
	}

	virtual void ReadItem(const CEWSItem * pItem, bool bRead) {
		if (m_p_callback->read_item) {
			ews_item *item(cewsitem_to_item(pItem));

			m_p_callback->read_item(item, bRead ? 1 : 0, m_p_callback->user_data);
			ews_free_item(item);
		}
	}
private:
	ews_sync_item_callback * m_p_callback;
	CEWSString m_SyncState;
	CEWSStringList m_IgnoreList;
};

}
}

int ews_sync_items(ews_session * session, const char * folder_id, ews_sync_item_callback * callback, char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !callback)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSItemOperation> pItemOp(
	    pSession->CreateItemOperation());
	CEWSError error;

	ews::c::CEWSItemOperationCallback_C _callback(callback);

	ON_ERROR_RETURN(!pItemOp->SyncItems(folder_id, &_callback, &error));

	return EWS_SUCCESS;
}

int ews_get_unread_items(ews_session * session, const char * folder_id, ews_sync_item_callback * callback, char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !callback)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSItemOperation> pItemOp(
	    pSession->CreateItemOperation());
	CEWSError error;

	ews::c::CEWSItemOperationCallback_C _callback(callback);

	ON_ERROR_RETURN(!pItemOp->GetUnreadItems(folder_id, &_callback, &error));

	return EWS_SUCCESS;
}

int ews_get_item(ews_session * session,
                 const char * item_id,
                 int flags,
                 ews_item ** pp_item,
                 char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !pp_item)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSItemOperation> pItemOp(
	    pSession->CreateItemOperation());
	CEWSError error;

	CEWSItem * pItem = pItemOp->GetItem(item_id, flags, &error);

	ON_ERROR_RETURN(!pItem);

	*pp_item = cewsitem_to_item(pItem);

	return EWS_SUCCESS;
}

int ews_create_item_by_id(ews_session * session, const char * folder_id, ews_item * item, char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !item)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSItemOperation> pItemOp(
	    pSession->CreateItemOperation());
	CEWSError error;

	std::auto_ptr<CEWSItem> pItem(item_to_cewsitem(item));
	std::auto_ptr<CEWSFolder> pSaveFolder(CEWSFolder::CreateInstance(folder_type_from_item_type(item->item_type),""));
	pSaveFolder->SetFolderId(folder_id);

	ON_ERROR_RETURN(!pItemOp->CreateItem(pItem.get(), pSaveFolder.get(), &error));

	cewsitem_to_item_2(pItem.get(), item);
	
	return EWS_SUCCESS;
}

int ews_create_item_by_distinguished_id_name(ews_session * session, int distinguished_id_name, ews_item * item, char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !item)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSItemOperation> pItemOp(
	    pSession->CreateItemOperation());
	CEWSError error;

	std::auto_ptr<CEWSItem> pItem(item_to_cewsitem(item));

	ON_ERROR_RETURN(!pItemOp->CreateItem(pItem.get(),
	                                     (CEWSFolder::EWSDistinguishedFolderIdNameEnum)distinguished_id_name,
	                                     &error));

	cewsitem_to_item_2(pItem.get(), item);

	return EWS_SUCCESS;
}

int ews_delete_item(ews_session * session, const char * item_id, const char * change_key, int moveToDeleted, char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSItemOperation> pItemOp(
	    pSession->CreateItemOperation());
	CEWSError error;

	std::auto_ptr<CEWSItem> pItem(CEWSItem::CreateInstance(CEWSItem::Item_Message));
	pItem->SetItemId(item_id);
	pItem->SetChangeKey(change_key);

	ON_ERROR_RETURN(!pItemOp->DeleteItem(pItem.get(),
	                                     moveToDeleted ? true : false,
	                                     &error));
	return EWS_SUCCESS;
}

int ews_send_item(ews_session * session, ews_msg_item * msg_item, char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !msg_item)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSItemOperation> pItemOp(
	    pSession->CreateItemOperation());
	CEWSError error;

	std::auto_ptr<CEWSMessageItem> pItem(msg_item_to_cewsmsgitem(msg_item));

	ON_ERROR_RETURN(!pItemOp->SendItem(pItem.get(), &error));

	return EWS_SUCCESS;
}

static ews_item * cewsitem_to_item(const CEWSItem * pItem) {
	switch(pItem->GetItemType()) {
	case CEWSItem::Item_Message:
		return (ews_item*)cewsmsgitem_to_msg_item(dynamic_cast<const CEWSMessageItem *>(pItem));
	case CEWSItem::Item_Calendar:
		return (ews_item*)cewscalendaritem_to_calendar_item(dynamic_cast<const CEWSCalendarItem *>(pItem));
	case CEWSItem::Item_Contact:
		return (ews_item*)cewscontactitem_to_contact_item(dynamic_cast<const CEWSContactItem *>(pItem));
	case CEWSItem::Item_Task:
		return (ews_item*)cewstaskitem_to_task_item(dynamic_cast<const CEWSTaskItem *>(pItem));
	default:
		break;
	}
	return NULL;
}

static void cewsitem_to_item_3(const CEWSItem * pItem, ews_item * item) {
	switch(pItem->GetItemType()) {
	case CEWSItem::Item_Message:
		cewsmsgitem_to_msg_item_3(dynamic_cast<const CEWSMessageItem *>(pItem),
		                          (ews_msg_item*)item);
		break;
	case CEWSItem::Item_Contact:
		cewscontactitem_to_contact_item_3(dynamic_cast<const CEWSContactItem *>(pItem),
		                                  (ews_contact_item*)item);
		break;
	case CEWSItem::Item_Calendar:
		cewscalendaritem_to_calendar_item_3(dynamic_cast<const CEWSCalendarItem *>(pItem),
		                                    (ews_calendar_item*)item);
		break;
	case CEWSItem::Item_Task:
		cewstaskitem_to_task_item_3(dynamic_cast<const CEWSTaskItem *>(pItem),
		                            (ews_task_item*)item);
		break;
	default:
		break;
	}
}

static CEWSItem * item_to_cewsitem(ews_item * item) {
	switch(item->item_type) {
	case CEWSItem::Item_Message:
		return msg_item_to_cewsmsgitem((ews_msg_item*)item);
	case CEWSItem::Item_Calendar:
		return calendar_item_to_cewscalendaritem((ews_calendar_item*)item);
	case CEWSItem::Item_Task:
		return task_item_to_cewstaskitem((ews_task_item*)item);
	default:
		return NULL;
	}
}

static CEWSFolder::EWSFolderTypeEnum folder_type_from_item_type(int item_type) {
	switch(item_type) {
	case CEWSItem::Item_Message:
		return CEWSFolder::Folder_Mail;
	default:
		return CEWSFolder::Folder_Mail;
	}
}

static CEWSRecipient * recipient_to_cewsrecipient(ews_recipient * recipient) {
	std::auto_ptr<CEWSRecipient> pRecipient(CEWSRecipient::CreateInstance());
	pRecipient->SetEmailAddress(recipient->mailbox.email);
	pRecipient->SetItemId(recipient->mailbox.item_id);
	pRecipient->SetName(recipient->mailbox.name);
	pRecipient->SetChangeKey(recipient->mailbox.change_key);
	pRecipient->SetRoutingType(recipient->mailbox.routing_type);
	pRecipient->SetMailboxType((CEWSEmailAddress::EWSMailboxTypeEnum)recipient->mailbox.mailbox_type);
	return pRecipient.release();
}

static CEWSMessageItem * msg_item_to_cewsmsgitem(ews_msg_item * item) {
	std::auto_ptr<CEWSMessageItem> pItem(dynamic_cast<CEWSMessageItem*>(CEWSItem::CreateInstance(CEWSItem::Item_Message)));

	__item_to_cewsitem(&item->item, pItem.get());

	if (item->to_recipients_count > 0) {
		for(int i=0;i<item->to_recipients_count;i++) {
			pItem->GetToRecipients()->push_back(recipient_to_cewsrecipient(&item->to_recipients[i]));
		}
	}

	if (item->cc_recipients_count > 0) {
		for(int i=0;i<item->cc_recipients_count;i++) {
			pItem->GetCcRecipients()->push_back(recipient_to_cewsrecipient(&item->cc_recipients[i]));
		}
	}

	if (item->bcc_recipients_count > 0) {
		for(int i=0;i<item->bcc_recipients_count;i++) {
			pItem->GetBccRecipients()->push_back(recipient_to_cewsrecipient(&item->bcc_recipients[i]));
		}
	}

	pItem->SetReadReceiptRequested(!!item->read_receeipt_requested);
	pItem->SetDeliveryReceiptRequested(!!item->delivery_receipt_requested);

	if (item->conversation_index)
		pItem->SetConversationIndex(item->conversation_index);
	if (item->conversation_topic)
		pItem->SetConversationTopic(item->conversation_topic);
	if (item->internet_message_id)
		pItem->SetInternetMessageId(item->internet_message_id);

	pItem->SetRead(!!item->read);
	pItem->SetResponseRequested(!!item->response_requested);

	if (item->references)
		pItem->SetReferences(item->references);

	if (item->reply_to_count > 0) {
		for(int i=0;i<item->reply_to_count;i++) {
			pItem->GetBccRecipients()->push_back(recipient_to_cewsrecipient(&item->reply_to[i]));
		}
	}

	return pItem.release();
}

static void cewsrecipient_to_recipient(CEWSRecipient * pRecipient, ews_recipient * recipient) {
	recipient->mailbox.email = strdup(pRecipient->GetEmailAddress());
	recipient->mailbox.item_id = strdup(pRecipient->GetItemId());
	recipient->mailbox.name = strdup(pRecipient->GetName());
	recipient->mailbox.change_key = strdup(pRecipient->GetChangeKey());
	recipient->mailbox.routing_type = strdup(pRecipient->GetRoutingType());
	recipient->mailbox.mailbox_type = pRecipient->GetMailboxType();
}

static ews_msg_item * cewsmsgitem_to_msg_item(const CEWSMessageItem * pItem) {
	std::auto_ptr<ews_msg_item> item((ews_msg_item *)safe_malloc(sizeof(ews_msg_item)));
	memset(item.get(), 0, sizeof(ews_msg_item));

	cewsmsgitem_to_msg_item_3(pItem, item.get());

	return item.release();
}

static void cewsmsgitem_to_msg_item_3(const CEWSMessageItem * pItem,
                                      ews_msg_item * item) {
	__cewsitem_to_item(pItem, &item->item);

	if (pItem->GetSender()) {
		cewsrecipient_to_recipient(pItem->GetSender(), &item->sender);
	}

	if (pItem->GetFrom()) {
		cewsrecipient_to_recipient(pItem->GetFrom(), &item->from);
	}

	if (pItem->GetReceivedBy()) {
		cewsrecipient_to_recipient(pItem->GetReceivedBy(), &item->received_by);
	}

	if (pItem->GetReceivedRepresenting()) {
		cewsrecipient_to_recipient(pItem->GetReceivedRepresenting(),
		                           &item->received_representing);
	}

	if (pItem->GetToRecipients()) {
		item->to_recipients_count = pItem->GetToRecipients()->size();
		if (item->to_recipients_count > 0) {
			item->to_recipients = (ews_recipient *) safe_malloc(
			    sizeof(ews_recipient) * item->to_recipients_count);
			memset(item->to_recipients, 0, sizeof(ews_recipient) * item->to_recipients_count);
            
			for (int i = 0; i < item->to_recipients_count; i++) {
				cewsrecipient_to_recipient(pItem->GetToRecipients()->at(i),
				                           &item->to_recipients[i]);
			}
		}
	}

	if (pItem->GetCcRecipients()) {
		item->cc_recipients_count = pItem->GetCcRecipients()->size();
		if (item->cc_recipients_count > 0) {
			item->cc_recipients = (ews_recipient *) safe_malloc(
			    sizeof(ews_recipient) * item->cc_recipients_count);
			memset(item->cc_recipients, 0, sizeof(ews_recipient) * item->cc_recipients_count);

			for (int i = 0; i < item->cc_recipients_count; i++) {
				cewsrecipient_to_recipient(pItem->GetCcRecipients()->at(i),
				                           &item->cc_recipients[i]);
			}
		}
	}

	if (pItem->GetBccRecipients()) {
		item->bcc_recipients_count = pItem->GetBccRecipients()->size();
		if (item->bcc_recipients_count > 0) {
			item->bcc_recipients = (ews_recipient *) safe_malloc(
			    sizeof(ews_recipient) * item->bcc_recipients_count);
			memset(item->bcc_recipients, 0, sizeof(ews_recipient) * item->bcc_recipients_count);

			for (int i = 0; i < item->bcc_recipients_count; i++) {
				cewsrecipient_to_recipient(pItem->GetBccRecipients()->at(i),
				                           &item->bcc_recipients[i]);
			}
		}
	}

	item->read_receeipt_requested = pItem->IsReadReceiptRequested();
	item->delivery_receipt_requested = pItem->IsDeliveryReceiptRequested();

	item->conversation_index = strdup(pItem->GetConversationIndex());
	item->conversation_topic = strdup(pItem->GetConversationTopic());
	item->internet_message_id = strdup(pItem->GetInternetMessageId());

	item->read = pItem->IsRead();
	item->response_requested = pItem->IsResponseRequested();

	item->references = strdup(pItem->GetReferences());

	if (pItem->GetReplyTo()) {
		item->reply_to_count = pItem->GetReplyTo()->size();
		if (item->reply_to_count > 0) {
			item->reply_to = (ews_recipient *) safe_malloc(
			    sizeof(ews_recipient) * item->reply_to_count);
			memset(item->reply_to, 0, sizeof(ews_recipient) * item->reply_to_count);

			for (int i = 0; i < item->reply_to_count; i++) {
				cewsrecipient_to_recipient(pItem->GetReplyTo()->at(i),
				                           &item->reply_to[i]);
			}
		}
	}
}

extern ews_attachment * cewsattachment_to_attachment(CEWSAttachment* pAttachment);

static void cewsitem_to_item_2(const CEWSItem * pItem, ews_item * item) {
	if (item->item_id) free(item->item_id);
	if (item->change_key) free(item->change_key);
	
	item->item_id = strdup(pItem->GetItemId());
	item->change_key = strdup(pItem->GetChangeKey());
}

static void list_to_pointer_pointer(CEWSStringList * pList,
                                    int & pp_count,
                                    char ** & pp){
	if (pList == NULL) {
		pp = NULL;
		pp_count = 0;
		return;
	}

	pp_count = pList->size();

	if (pp_count == 0) {
		pp = NULL;
		return;
	}
    
	pp = (char **)safe_malloc(pp_count * sizeof(char *));

	for(int i=0;i < pp_count;i++) {
		pp[i] = strdup(pList->at(i).GetData());
	}
}

static void list_to_pointer_pointer(CEWSInternetMessageHeaderList * pList,
                                    int & pp_count,
                                    ews_internet_message_header ** & pp){
	if (pList == NULL) {
		pp = NULL;
		pp_count = 0;
		return;
	}

	pp_count = pList->size();

	if (pp_count == 0) {
		pp = NULL;
		return;
	}
    
	pp = (ews_internet_message_header **)safe_malloc(pp_count * sizeof(ews_internet_message_header *));

	for(int i=0;i < pp_count;i++) {
		pp[i] = (ews_internet_message_header*)safe_malloc(sizeof(ews_internet_message_header));
		SET_VALUE_STR(pList->at(i), Value, pp[i], value);
		SET_VALUE_STR(pList->at(i), HeaderName, pp[i], header_name);
	}
}

static void __cewsitem_to_item(const CEWSItem * pItem, ews_item * item) {
	item->item_id = strdup(pItem->GetItemId());
	item->body_type = pItem->GetBodyType();
	item->body = strdup(pItem->GetBody());
	item->mime_content = strdup(pItem->GetMimeContent());
	item->create_time = pItem->GetCreateTime();
	item->received_time = pItem->GetReceivedTime();
	item->sent_time = pItem->GetSentTime();
	item->display_cc = strdup(pItem->GetDisplayCc());
	item->display_to = strdup(pItem->GetDisplayTo());
	item->inreply_to = strdup(pItem->GetInReplyTo());
	item->subject = strdup(pItem->GetSubject());
	item->item_type = pItem->GetItemType();
	item->change_key = strdup(pItem->GetChangeKey());
	item->has_attachments = pItem->HasAttachments();
    
	if (pItem->GetAttachments()) {
		item->attachment_count = pItem->GetAttachments()->size();

		if (item->attachment_count > 0) {
			item->attachments = (ews_attachment **)safe_malloc(sizeof(ews_attachment *) * item->attachment_count);

			for(int i=0;i<item->attachment_count;i++) {
				CEWSAttachment * pAttachment = pItem->GetAttachments()->at(i);
				item->attachments[i] =
						cewsattachment_to_attachment(pAttachment);
			}
		}
	}

	SET_VALUE_STR(pItem,ParentFolderId, item, parent_folder_id);
	SET_VALUE_STR(pItem,ParentFolderChangeKey, item, parent_folder_change_key);
	SET_VALUE_STR(pItem,ItemClass, item, item_class);
	SET_VALUE(pItem,Sensitivity,
	          item, sensitivity);
	SET_VALUE(pItem,Size, item, size);
	LIST_TO_POINTER_POINTER(pItem,Categories, item, categories);
	SET_VALUE(pItem,Importance,
	          item, importance);
	SET_VALUE_B(pItem,Submitted, item, is_submitted);
	SET_VALUE_B(pItem,Draft, item, is_draft);
	SET_VALUE_B(pItem,FromMe, item, is_from_me);
	SET_VALUE_B(pItem,Resend, item, is_resend);
	SET_VALUE_B(pItem,Unmodified, item, isunmodified);
	LIST_TO_POINTER_POINTER(pItem,
	                        InternetMessageHeaders,
	                        item,
	                        internet_message_headers);
	SET_VALUE(pItem,ReminderDueBy, item, reminder_due_by);
	SET_VALUE(pItem,ReminderIsSet, item, reminder_is_set);
	SET_VALUE_STR(pItem,ReminderMinutesBeforeStart, item, reminder_minutes_before_start);
	SET_VALUE_STR(pItem,LastModifiedName, item, last_modified_name);
	SET_VALUE(pItem,LastModifiedTime, item, last_modified_time);
}

extern CEWSAttachment* attachment_to_cewsattachment(ews_attachment* attachment);

static
CEWSStringList *
list_from_pointer_pointer(int pp_count,
                          char ** pp){
	if (pp_count == 0 || !pp) {
		return NULL;
	}

	CEWSStringList * pList = new CEWSStringList;

	for(int i = 0;i < pp_count; i++) {
		pList->push_back(pp[i]);
	}

	return pList;
}

static
CEWSInternetMessageHeaderList *
list_from_pointer_pointer(int pp_count,
                          ews_internet_message_header ** pp){
	if (pp_count == 0 || !pp) {
		return NULL;
	}

	CEWSInternetMessageHeaderList * pList = new CEWSInternetMessageHeaderList;

	for(int i = 0;i < pp_count; i++) {
		CEWSInternetMessageHeader * p =
				CEWSInternetMessageHeader::CreateInstance();

		p->SetValue(pp[i]->value);
		p->SetHeaderName(pp[i]->header_name);
	    
		pList->push_back(p);
	}

	return pList;
}

static void __item_to_cewsitem(const ews_item * item, CEWSItem * pItem) {
	if (item->item_id) pItem->SetItemId(item->item_id);
	pItem->SetBodyType((CEWSItem::EWSBodyType)item->body_type);
	if (item->body) pItem->SetBody(item->body);
	if (item->mime_content) pItem->SetMimeContent(item->mime_content);
	pItem->SetCreateTime(item->create_time);
	pItem->SetReceivedTime(item->received_time);
	pItem->SetSentTime(item->sent_time);
	if (item->display_cc) pItem->SetDisplayCc(item->display_cc);
	if (item->display_to) pItem->SetDisplayTo(item->display_to);
	if (item->inreply_to) pItem->SetInReplyTo(item->inreply_to);
	if (item->subject) pItem->SetSubject(item->subject);
	pItem->SetItemType((CEWSItem::EWSItemType)item->item_type);
	if (item->change_key) pItem->SetChangeKey(item->change_key);
	pItem->SetHasAttachments(item->has_attachments);

	for (int i = 0; i < item->attachment_count; i++) {
		ews_attachment * attachment = item->attachments[i];

		CEWSAttachment * pAttachment =
				attachment_to_cewsattachment(attachment);

		if (pAttachment)
			pItem->GetAttachments()->push_back(pAttachment);
	}

	GET_VALUE_STR(pItem,ParentFolderId, item, parent_folder_id);
	GET_VALUE_STR(pItem,ParentFolderChangeKey, item, parent_folder_change_key);
	GET_VALUE_STR(pItem,ItemClass, item, item_class);
	GET_VALUE_5(pItem,Sensitivity,
	            enum CEWSItem::EWSSensitivityTypeEnum, item, sensitivity);
	GET_VALUE(pItem,Size, item, size);
	LIST_FROM_POINTER_POINTER(pItem,Categories, item, categories);
	GET_VALUE_5(pItem,Importance,
	            enum CEWSItem::EWSImportanceTypeEnum,
	            item, importance);
	GET_VALUE(pItem,Submitted, item, is_submitted);
	GET_VALUE(pItem,Draft, item, is_draft);
	GET_VALUE(pItem,FromMe, item, is_from_me);
	GET_VALUE(pItem,Resend, item, is_resend);
	GET_VALUE(pItem,Unmodified, item, isunmodified);
	LIST_FROM_POINTER_POINTER(pItem,
	                          InternetMessageHeaders,
	                          item,
	                          internet_message_headers);
	GET_VALUE(pItem,ReminderDueBy, item, reminder_due_by);
	GET_VALUE(pItem,ReminderIsSet, item, reminder_is_set);
	GET_VALUE_STR(pItem,ReminderMinutesBeforeStart, item, reminder_minutes_before_start);
	GET_VALUE_STR(pItem,LastModifiedName, item, last_modified_name);
	GET_VALUE(pItem,LastModifiedTime, item, last_modified_time);
}

static void ews_free_recipient(ews_recipient * recipient) {
	SAFE_FREE(recipient->mailbox.email);
	SAFE_FREE(recipient->mailbox.item_id);
	SAFE_FREE(recipient->mailbox.name);
	SAFE_FREE(recipient->mailbox.change_key);
	SAFE_FREE(recipient->mailbox.routing_type);
}

static void ews_free_recipient_array(int count, ews_recipient * recipient) {
	for(int i=0;i<count;i++) {
		ews_free_recipient(&recipient[i]);
	}
}

static void ews_free_msg_item(ews_msg_item * msg_item) {
	ews_free_recipient_array(msg_item->bcc_recipients_count, msg_item->bcc_recipients);
	SAFE_FREE(msg_item->bcc_recipients);
	ews_free_recipient_array(msg_item->cc_recipients_count, msg_item->cc_recipients);
	SAFE_FREE(msg_item->cc_recipients);
	SAFE_FREE(msg_item->conversation_index);
	SAFE_FREE(msg_item->conversation_topic);
	ews_free_recipient(&msg_item->from);
	SAFE_FREE(msg_item->internet_message_id);
	ews_free_recipient(&msg_item->received_by);
	ews_free_recipient(&msg_item->received_representing);
	SAFE_FREE(msg_item->references);
	ews_free_recipient(&msg_item->sender);
	ews_free_recipient_array(msg_item->to_recipients_count, msg_item->to_recipients);
	SAFE_FREE(msg_item->to_recipients);
	ews_free_recipient_array(msg_item->reply_to_count, msg_item->reply_to);
	SAFE_FREE(msg_item->reply_to);
}

extern void ews_free_attachment(ews_attachment * attachment);

static
void
free_internet_message_header(ews_internet_message_header * p) {
	if (!p) return;
	
	SAFE_FREE(p->value);
	SAFE_FREE(p->header_name);
	free(p);
}

void __ews_free_item(ews_item * item) {
	switch(item->item_type) {
	case CEWSItem::Item_Message:
		ews_free_msg_item((ews_msg_item *)item);
		break;
	case CEWSItem::Item_Contact:
		ews_free_contact_item((ews_contact_item *)item);
		break;
	case CEWSItem::Item_Calendar:
		ews_free_calendar_item((ews_calendar_item *)item);
		break;
	case CEWSItem::Item_Task:
		ews_free_task_item((ews_task_item *)item);
		break;
	default:
		break;
	}

	SAFE_FREE(item->body);
	SAFE_FREE(item->mime_content);
	SAFE_FREE(item->change_key);
	SAFE_FREE(item->display_cc);
	SAFE_FREE(item->display_to);
	SAFE_FREE(item->inreply_to);
	SAFE_FREE(item->item_id);
	SAFE_FREE(item->subject);

	if (item->attachments) {
		for(int i=0;i<item->attachment_count;i++) {
			ews_free_attachment(item->attachments[i]);
		}

		free(item->attachments);
	}
	
	SAFE_FREE(item->parent_folder_id);
	SAFE_FREE(item->parent_folder_change_key);
	SAFE_FREE(item->item_class);
	SAFE_FREE(item->reminder_minutes_before_start);
	SAFE_FREE(item->last_modified_name);
	LIST_SAFE_FREE(item->categories);
	LIST_SAFE_FREE_2(item->internet_message_headers, free_internet_message_header);
}

void ews_free_item(ews_item * item) {
	if (!item) return;

	__ews_free_item(item);

	free(item);
}

void ews_free_items(ews_item ** item, int item_count) {
	if (!item) return;

	for(int i=0;i<item_count;i++) {
		ews_free_item(item[i]);
	}

	free(item);
}

int ews_get_items(ews_session * session,
                  const char ** item_id,
                  int & item_count,
                  int flags,
                  ews_item *** pp_item,
                  char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !pp_item)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSItemOperation> pItemOp(
	    pSession->CreateItemOperation());
	CEWSError error;
    
	CEWSItemList itemList;
	CEWSStringList itemIdList;

	for(int i=0;i < item_count; i++) {
		itemIdList.push_back(item_id[i]);
	}

	if (pItemOp->GetItems(itemIdList, &itemList, flags, &error)) {
		item_count = itemList.size();
		
		ews_item ** tmp_items
				= *pp_item
				= (ews_item **)safe_malloc(item_count * sizeof(ews_item *));
		memset(*pp_item, 0, item_count * sizeof(ews_item *));

		for(int i = 0; i < item_count; i++) {
			CEWSItem * pItem = itemList.at(i);
			if (pItem) {
				tmp_items[i] = cewsitem_to_item(pItem);
			}
		}//for
        
	} else {
		ON_ERROR_RETURN(true);
	}

	return EWS_SUCCESS;
}

#define SAFE_CLONE(n, o)                        \
	n = o ? strdup(o) : NULL;

void clone_recipient(ews_recipient * n, ews_recipient * o) {
	SAFE_CLONE(n->mailbox.item_id, o->mailbox.item_id);
	SAFE_CLONE(n->mailbox.name, o->mailbox.name);
	SAFE_CLONE(n->mailbox.email, o->mailbox.email);
	SAFE_CLONE(n->mailbox.change_key, o->mailbox.change_key);
	SAFE_CLONE(n->mailbox.routing_type, o->mailbox.routing_type);
	n->mailbox.mailbox_type = o->mailbox.mailbox_type;
}

ews_msg_item * ews_msg_item_clone(ews_msg_item * v) {
	std::auto_ptr<CEWSMessageItem> ewsItem(msg_item_to_cewsmsgitem(v));
	ews_msg_item * new_item = cewsmsgitem_to_msg_item(ewsItem.get());

	clone_recipient(&new_item->sender, &v->sender);
	clone_recipient(&new_item->from, &v->from);
	clone_recipient(&new_item->received_by, &v->received_by);
	clone_recipient(&new_item->received_representing, &v->received_representing);

	return new_item;
}

int ews_delete_items(ews_session * session,
                     ews_item * items,
                     int item_count,
                     int moveToDeleted,
                     char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSItemOperation> pItemOp(
	    pSession->CreateItemOperation());
	CEWSError error;

	CEWSItemList itemList;

	//TODO:Need to check item type to create item
	for(int i=0;i < item_count;i++) {
		ews_item * tmp = items + i;
		std::auto_ptr<CEWSItem> pItem(CEWSItem::CreateInstance(CEWSItem::Item_Message));
		pItem->SetItemId(tmp->item_id);
		pItem->SetChangeKey(tmp->change_key);

		itemList.push_back(pItem.release());
	}

	ON_ERROR_RETURN(!pItemOp->DeleteItems(&itemList,
	                                      moveToDeleted ? true : false,
	                                      &error));
	return EWS_SUCCESS;
}

int ews_mark_items_read(ews_session * session,
                        ews_item * items,
                        int item_count,
                        int read,
                        char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSItemOperation> pItemOp(
	    pSession->CreateItemOperation());
	CEWSError error;

	CEWSItemList itemList;

	//TODO:Need to check item type to create item
	for(int i=0;i < item_count;i++) {
		ews_item * tmp = items + i;
		std::auto_ptr<CEWSItem> pItem(CEWSItem::CreateInstance(CEWSItem::Item_Message));
		pItem->SetItemId(tmp->item_id);
		pItem->SetChangeKey(tmp->change_key);

		itemList.push_back(pItem.release());
	}

	ON_ERROR_RETURN(!pItemOp->MarkItemsRead(&itemList,
	                                        read ? true : false,
	                                        &error));
	return EWS_SUCCESS;
}

int ews_resolve_names(ews_session * session,
                      const char * unresolvedEntry,
                      ews_item *** pp_items,
                      int & items_count,
                      char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !pp_items)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSContactOperation> pContactOp(
	    pSession->CreateContactOperation());
	CEWSError error;
    
	std::auto_ptr<CEWSItemList> pContactItems(
	    pContactOp->ResolveNames(unresolvedEntry, &error));

	if(pContactItems.get() == NULL) {
		ON_ERROR_RETURN(true);
	}

	items_count = pContactItems->size();
    
	ews_item ** tmp_items
			= *pp_items
			= (ews_item **)safe_malloc(items_count * sizeof(ews_item *));
	memset(*pp_items, 0, items_count * sizeof(ews_item *));

	for(int i = 0; i < items_count; i++) {
		CEWSItem * pItem = pContactItems->at(i);
		if (pItem) {
			tmp_items[i] = cewsitem_to_item(pItem);
		}
	}//for
    
	return EWS_SUCCESS;
}

static void list_to_pointer_pointer(CEWSPhysicalAddressList * pList,
                                    int & pp_count,
                                    ews_physical_address ** & pp){
	if (!pList) {
		pp_count = 0;
		pp = NULL;
		return;
	}
	
	pp_count = pList->size();

	if (pp_count == 0) {
		pp = NULL;
		return;
	}
    
	pp = (ews_physical_address **)safe_malloc(pp_count * sizeof(ews_physical_address *));

	for(int i = 0;i < pp_count; i++) {
		pp[i] = (ews_physical_address*)safe_malloc(sizeof(ews_physical_address));
		CEWSPhysicalAddress * pAddr = pList->at(i);
        
		SET_VALUE_STR( pAddr , Street , pp[i], street );
		SET_VALUE_STR( pAddr , City , pp[i], city );
		SET_VALUE_STR( pAddr , State , pp[i], state );
		SET_VALUE_STR( pAddr , CountryOrRegion , pp[i], country_or_region );
		SET_VALUE_STR( pAddr , PostalCode , pp[i], postal_code );
	}
}

static
CEWSPhysicalAddressList *
list_from_pointer_pointer(int pp_count,
                          ews_physical_address ** pp){
	if (pp_count == 0 || !pp) {
		return NULL;
	}

	CEWSPhysicalAddressList * pList = new CEWSPhysicalAddressList;

	for(int i = 0;i < pp_count; i++) {
		CEWSPhysicalAddress * pAddr =
				CEWSPhysicalAddress::CreateInstance();
        
		GET_VALUE_STR( pAddr , Street , pp[i], street );
		GET_VALUE_STR( pAddr , City , pp[i], city );
		GET_VALUE_STR( pAddr , State , pp[i], state );
		GET_VALUE_STR( pAddr , CountryOrRegion , pp[i], country_or_region );
		GET_VALUE_STR( pAddr , PostalCode , pp[i], postal_code );

		pList->push_back(pAddr);
	}

	return pList;
}

static void cewscontactitem_to_contact_item_3(const CEWSContactItem * pItem,
                                              ews_contact_item * item) {
	__cewsitem_to_item(pItem, &item->item);

	item->item.item_type = EWS_Item_Contact;
    
	SET_VALUE_STR( pItem , Name , item, name );
	SET_VALUE_STR( pItem , EmailAddress , item, email_address );
	SET_VALUE_STR( pItem , RoutingType , item, routing_type );
	SET_VALUE( pItem, MailboxType, item, mailbox_type);
	SET_VALUE_STR( pItem , DisplayName , item, display_name );
	SET_VALUE_STR( pItem , GivenName , item, given_name );
	SET_VALUE_STR( pItem , Initials , item, initials );
	SET_VALUE_STR( pItem , MiddleName , item, middle_name );
	SET_VALUE_STR( pItem , Nickname , item, nick_name );
	SET_VALUE_STR( pItem , CompanyName , item, company_name );
	SET_VALUE_STR( pItem , AssistantName , item, assistant_name );
	SET_VALUE(pItem, Birthday, item, birthday);
	SET_VALUE_STR( pItem , BusinessHomePage , item, business_home_page );
	SET_VALUE(pItem, ContactSource, item, contact_source);
	SET_VALUE_STR( pItem , Department , item, department );
	SET_VALUE_STR( pItem , Generation , item, generation );
	SET_VALUE_STR( pItem , JobTitle , item, job_title );
	SET_VALUE_STR( pItem , Manager , item, manager );
	SET_VALUE_STR( pItem , Mileage , item, mileage );
	SET_VALUE_STR( pItem , OfficeLocation , item, office_location );
	SET_VALUE_STR( pItem , Profession , item, profession );
	SET_VALUE_STR( pItem , SpouseName , item, spouse_name );
	SET_VALUE_STR( pItem , Surname , item, sur_name );
	SET_VALUE(pItem, WeddingAnniversary, item, wedding_anniversary);

	item->is_maillist = pItem->IsMailList();

	LIST_TO_POINTER_POINTER(pItem, EmailAddresses, item, email_addresses);
	LIST_TO_POINTER_POINTER(pItem, PhysicalAddresses, item, physical_addresses);
	LIST_TO_POINTER_POINTER(pItem, PhoneNumbers, item, phone_numbers);
	LIST_TO_POINTER_POINTER(pItem, Children, item, children);
	LIST_TO_POINTER_POINTER(pItem, Companies, item, companies);
	LIST_TO_POINTER_POINTER(pItem, ImAddresses, item, im_addresses);

	if (pItem->GetCompleteName()) {
		SET_VALUE_STR( pItem->GetCompleteName() , Title , (&item->complete_name), title );
		SET_VALUE_STR( pItem->GetCompleteName() , FirstName , (&item->complete_name), first_name );
		SET_VALUE_STR( pItem->GetCompleteName() , MiddleName , (&item->complete_name), middle_name );
		SET_VALUE_STR( pItem->GetCompleteName() , LastName , (&item->complete_name), last_name );
		SET_VALUE_STR( pItem->GetCompleteName() , Suffix , (&item->complete_name), suffix );
		SET_VALUE_STR( pItem->GetCompleteName() , Initials , (&item->complete_name), initials );
		SET_VALUE_STR( pItem->GetCompleteName() , FullName , (&item->complete_name), full_name );
		SET_VALUE_STR( pItem->GetCompleteName() , YomiFirstName , (&item->complete_name), yomi_first_name );
		SET_VALUE_STR( pItem->GetCompleteName() , YomiLastName , (&item->complete_name), yomi_last_name );
	}

	if (pItem->GetPostalAddress()) {
		for(size_t i=0;i<pItem->GetPhysicalAddresses()->size();i++) {
			if (pItem->GetPostalAddress() ==
			    pItem->GetPhysicalAddresses()->at(i)) {
				item->postal_address = item->physical_addresses[i];
			}
		}
	}
}

#define PHYSICAL_ADDRESS_SAFE_FREE(x)           \
	if (x) {                                    \
		SAFE_FREE(x->street );                  \
		SAFE_FREE(x->city );                    \
		SAFE_FREE(x->state );                   \
		SAFE_FREE(x->country_or_region );       \
		SAFE_FREE(x->postal_code );             \
		free(x);                                \
	}

#define PHYSICAL_ADDRESS_LIST_SAFE_FREE(x)      \
	if (x) {                                    \
		int c = x##_count;                      \
		for(int i =0;i < c; i++) {              \
			PHYSICAL_ADDRESS_SAFE_FREE(x[i]);   \
		}                                       \
		free(x);                                \
	}

static void ews_free_contact_item(ews_contact_item * item) {
	SAFE_FREE(item-> name );
	SAFE_FREE(item-> email_address );
	SAFE_FREE(item-> routing_type );
	SAFE_FREE(item-> display_name );
	SAFE_FREE(item-> given_name );
	SAFE_FREE(item-> initials );
	SAFE_FREE(item-> middle_name );
	SAFE_FREE(item-> nick_name );
	SAFE_FREE(item-> company_name );
	SAFE_FREE(item-> assistant_name );
	SAFE_FREE(item-> business_home_page );
	SAFE_FREE(item-> department );
	SAFE_FREE(item-> generation );
	SAFE_FREE(item-> job_title );
	SAFE_FREE(item-> manager );
	SAFE_FREE(item-> mileage );
	SAFE_FREE(item-> office_location );
	SAFE_FREE(item-> profession );
	SAFE_FREE(item-> spouse_name );
	SAFE_FREE(item-> sur_name );

	LIST_SAFE_FREE(item-> email_addresses);
	PHYSICAL_ADDRESS_LIST_SAFE_FREE(item-> physical_addresses);
	LIST_SAFE_FREE(item-> phone_numbers);
	LIST_SAFE_FREE(item-> children);
	LIST_SAFE_FREE(item-> companies);
	LIST_SAFE_FREE(item-> im_addresses);
    
	SAFE_FREE(item->complete_name.title );
	SAFE_FREE(item->complete_name.first_name );
	SAFE_FREE(item->complete_name.middle_name );
	SAFE_FREE(item->complete_name.last_name );
	SAFE_FREE(item->complete_name.suffix );
	SAFE_FREE(item->complete_name.initials );
	SAFE_FREE(item->complete_name.full_name );
	SAFE_FREE(item->complete_name.yomi_first_name );
	SAFE_FREE(item->complete_name.yomi_last_name );
}

int
ews_sync_calendar(ews_session * session,
                  ews_sync_item_callback * callback,
                  char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !callback)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSCalendarOperation> pItemOp(
	    pSession->CreateCalendarOperation());
	CEWSError error;

	ews::c::CEWSItemOperationCallback_C _callback(callback);

	ON_ERROR_RETURN(!pItemOp->SyncCalendar(&_callback, &error));

	return EWS_SUCCESS;
}

static
ews_calendar_item *
cewscalendaritem_to_calendar_item(const CEWSCalendarItem * pItem) {
	std::auto_ptr<ews_calendar_item> item((ews_calendar_item *)safe_malloc(sizeof(ews_calendar_item)));
	memset(item.get(), 0, sizeof(ews_calendar_item));

	cewscalendaritem_to_calendar_item_3(pItem, item.get());

	return item.release();
}

static
ews_contact_item *
cewscontactitem_to_contact_item(const CEWSContactItem * pItem) {
	std::auto_ptr<ews_contact_item> item((ews_contact_item *)safe_malloc(sizeof(ews_contact_item)));
	memset(item.get(), 0, sizeof(ews_contact_item));

	cewscontactitem_to_contact_item_3(pItem, item.get());

	return item.release();
}

static
void
list_to_pointer_pointer(CEWSAttendeeList * pList,
                        int & pp_count,
                        ews_attendee ** & pp){
	if (!pList) {
		pp_count = 0;
		pp = NULL;
		return;
	}

	pp_count = pList->size();

	if (pp_count == 0) {
		pp = NULL;
		return;
	}
    
	pp = (ews_attendee **)safe_malloc(pp_count * sizeof(ews_attendee *));

	for(int i = 0;i < pp_count; i++) {
		pp[i] = (ews_attendee*)safe_malloc(sizeof(ews_attendee));

		CEWSAttendee * pAddr = pList->at(i);
        
		SET_VALUE_STR( pAddr , ItemId , (&(pp[i]->email_address)), item_id );
		SET_VALUE_STR( pAddr , Name , (&(pp[i]->email_address)), name);
		SET_VALUE_STR( pAddr , EmailAddress , (&(pp[i]->email_address)), email);
		SET_VALUE_STR( pAddr , ChangeKey , (&(pp[i]->email_address)), change_key );
		SET_VALUE_STR( pAddr , RoutingType , (&(pp[i]->email_address)), routing_type );
		SET_VALUE( pAddr , MailboxType , (&(pp[i]->email_address)), mailbox_type );
		SET_VALUE( pAddr , ResponseType , pp[i], response_type );
		SET_VALUE( pAddr , LastResponseTime , pp[i], last_response_time );
	}
}

static
CEWSAttendeeList *
list_from_pointer_pointer(int pp_count,
                          ews_attendee ** pp) {
	if (pp_count == 0 || !pp) {
		return NULL;
	}

	CEWSAttendeeList * pList = new CEWSAttendeeList;
    
	for(int i = 0;i < pp_count; i++) {
		CEWSAttendee * pAddr = CEWSAttendee::CreateInstance();
        
		GET_VALUE_STR( pAddr , ItemId , (&(pp[i]->email_address)), item_id );
		GET_VALUE_STR( pAddr , Name , (&(pp[i]->email_address)), name);
		GET_VALUE_STR( pAddr , EmailAddress , (&(pp[i]->email_address)), email);
		GET_VALUE_STR( pAddr , ChangeKey , (&(pp[i]->email_address)), change_key );
		GET_VALUE_STR( pAddr , RoutingType , (&(pp[i]->email_address)), routing_type );
		GET_VALUE_5( pAddr ,
		             MailboxType ,
		             CEWSEmailAddress::EWSMailboxTypeEnum,
		             (&(pp[i]->email_address)),
		             mailbox_type );
		GET_VALUE_5( pAddr ,
		             ResponseType ,
		             enum CEWSAttendee::EWSResponseTypeEnum,
		             pp[i],
		             response_type );
		GET_VALUE( pAddr , LastResponseTime , pp[i], last_response_time );

		pList->push_back(pAddr);
	}

	return pList;
}

static
void
list_to_pointer_pointer(CEWSItemList * pList,
                        int & pp_count,
                        ews_item ** & pp){
	if (!pList) {
		pp_count = 0;
		pp = NULL;
		return;
	}

	pp_count = pList->size();

	if (pp_count == 0) {
		pp = NULL;
		return;
	}
    
	pp = (ews_item **)safe_malloc(pp_count * sizeof(ews_item *));

	for(int i = 0;i < pp_count; i++) {
		pp[i] = (ews_item*)safe_malloc(sizeof(ews_item));
		CEWSItem * pAddr = pList->at(i);

		pp[i] = cewsitem_to_item(pAddr);
	}
}

static
CEWSItemList *
list_from_pointer_pointer(int pp_count,
                          ews_item ** pp){
	if (pp_count == 0 || !pp) {
		return NULL;
	}
    
	CEWSItemList * pList = new CEWSItemList;
    
	for(int i = 0;i < pp_count; i++) {
		pList->push_back(item_to_cewsitem(pp[i]));
	}
    
	return pList;
}

static
void
from(ews_recurrence_range * r, const CEWSRecurrenceRange * p) {
	if (!p) return;
    
	r->range_type = p->GetRecurrenceRangeType();

	SET_VALUE_STR(p, StartDate, r, start_date);
	switch(r->range_type) {
	case EWSRecurrenceRangeType_EndDate:
		SET_VALUE_STR(dynamic_cast<const CEWSEndDateRecurrenceRange*>(p),
		              EndDate,
		              r,
		              end_date);
		break;
	case EWSRecurrenceRangeType_Numbered:
		SET_VALUE(dynamic_cast<const CEWSNumberedRecurrenceRange*>(p),
		          NumberOfOccurrences,
		          r,
		          number_of_occurrences);
		break;
	case EWSRecurrenceRangeType_NoEnd:
	default:
		break;
	}
}

static
CEWSRecurrenceRange *
to(ews_recurrence_range * r) {
	if (!r) return NULL;

	ews::CEWSRecurrenceRange::EWSRecurrenceRangeTypeEnum rangeType=
			(ews::CEWSRecurrenceRange::EWSRecurrenceRangeTypeEnum)r->range_type;
    
	CEWSRecurrenceRange * p =
			CEWSRecurrenceRange::CreateInstance(rangeType);
    
	GET_VALUE_STR(p, StartDate, r, start_date);
	switch(r->range_type) {
	case EWSRecurrenceRangeType_EndDate:
		GET_VALUE_STR(dynamic_cast<CEWSEndDateRecurrenceRange*>(p),
		              EndDate,
		              r,
		              end_date);
		break;
	case EWSRecurrenceRangeType_Numbered:
		GET_VALUE(dynamic_cast<CEWSNumberedRecurrenceRange*>(p),
		          NumberOfOccurrences,
		          r,
		          number_of_occurrences);
		break;
	case EWSRecurrenceRangeType_NoEnd:
	default:
		break;
	}

	return p;
}

static
void
from(ews_recurrence_pattern * r, const CEWSRecurrencePattern * p) {
	if (!p) return;

	r->pattern_type = p->GetRecurrenceType();

	switch(r->pattern_type) {
	case EWS_Recurrence_RelativeYearly:
		SET_VALUE(dynamic_cast<const CEWSRecurrencePatternRelativeYearly*>(p),
		          DaysOfWeek,
		          r,
		          days_of_week);
		SET_VALUE(dynamic_cast<const CEWSRecurrencePatternRelativeYearly*>(p),
		          DayOfWeekIndex,
		          r,
		          day_of_week_index);
		SET_VALUE(dynamic_cast<const CEWSRecurrencePatternRelativeYearly*>(p),
		          Month,
		          r,
		          month);
		break;
	case EWS_Recurrence_AbsoluteYearly:
		SET_VALUE(dynamic_cast<const CEWSRecurrencePatternAbsoluteYearly*>(p),
		          DayOfMonth,
		          r,
		          day_of_month);
		SET_VALUE(dynamic_cast<const CEWSRecurrencePatternAbsoluteYearly*>(p),
		          Month,
		          r,
		          month);
		break;
	case EWS_Recurrence_RelativeMonthly:
		SET_VALUE(dynamic_cast<const CEWSRecurrencePatternRelativeMonthly*>(p),
		          DaysOfWeek,
		          r,
		          days_of_week);
		SET_VALUE(dynamic_cast<const CEWSRecurrencePatternRelativeMonthly*>(p),
		          DayOfWeekIndex,
		          r,
		          day_of_week_index);
		SET_VALUE(dynamic_cast<const CEWSRecurrencePatternInterval*>(p),
		          Interval,
		          r,
		          interval);
		break;
	case EWS_Recurrence_AbsoluteMonthly:
		SET_VALUE(dynamic_cast<const CEWSRecurrencePatternAbsoluteMonthly*>(p),
		          DayOfMonth,
		          r,
		          day_of_month);
		SET_VALUE(dynamic_cast<const CEWSRecurrencePatternInterval*>(p),
		          Interval,
		          r,
		          interval);
		break;
	case EWS_Recurrence_Weekly:
		SET_VALUE(dynamic_cast<const CEWSRecurrencePatternWeekly*>(p),
		          DaysOfWeek,
		          r,
		          days_of_week);
		SET_VALUE(dynamic_cast<const CEWSRecurrencePatternInterval*>(p),
		          Interval,
		          r,
		          interval);
		break;
	case EWS_Recurrence_Daily:
		SET_VALUE(dynamic_cast<const CEWSRecurrencePatternInterval*>(p),
		          Interval,
		          r,
		          interval);
		break;
	default:
		break;
	}
}

static
CEWSRecurrencePattern *
to(ews_recurrence_pattern * r) {
	if (!r) return NULL;

	ews::CEWSRecurrencePattern::EWSRecurrenceTypeEnum patternType =
			(ews::CEWSRecurrencePattern::EWSRecurrenceTypeEnum)r->pattern_type;
    
	CEWSRecurrencePattern * p =
			CEWSRecurrencePattern::CreateInstance(patternType);

	switch(r->pattern_type) {
	case EWS_Recurrence_RelativeYearly:
		GET_VALUE_5(dynamic_cast< CEWSRecurrencePatternRelativeYearly*>(p),
		            DaysOfWeek,
		            ews::CEWSRecurrencePattern::EWSDayOfWeekEnum,
		            r,
		            days_of_week);
		GET_VALUE_5(dynamic_cast< CEWSRecurrencePatternRelativeYearly*>(p),
		            DayOfWeekIndex,
		            ews::CEWSRecurrencePattern::EWSDayOfWeekIndexEnum,
		            r,
		            day_of_week_index);
		GET_VALUE_5(dynamic_cast< CEWSRecurrencePatternRelativeYearly*>(p),
		            Month,
		            ews::CEWSRecurrencePattern::EWSMonthNamesEnum,
		            r,
		            month);
		break;
	case EWS_Recurrence_AbsoluteYearly:
		GET_VALUE(dynamic_cast< CEWSRecurrencePatternAbsoluteYearly*>(p),
		          DayOfMonth,
		          r,
		          day_of_month);
		GET_VALUE_5(dynamic_cast< CEWSRecurrencePatternAbsoluteYearly*>(p),
		            Month,
		            ews::CEWSRecurrencePattern::EWSMonthNamesEnum,
		            r,
		            month);
		break;
	case EWS_Recurrence_RelativeMonthly:
		GET_VALUE_5(dynamic_cast< CEWSRecurrencePatternRelativeMonthly*>(p),
		            DaysOfWeek,
		            ews::CEWSRecurrencePattern::EWSDayOfWeekEnum,
		            r,
		            days_of_week);
		GET_VALUE_5(dynamic_cast< CEWSRecurrencePatternRelativeMonthly*>(p),
		            DayOfWeekIndex,
		            ews::CEWSRecurrencePattern::EWSDayOfWeekIndexEnum,
		            r,
		            day_of_week_index);
		GET_VALUE(dynamic_cast< CEWSRecurrencePatternInterval*>(p),
		          Interval,
		          r,
		          interval);
		break;
	case EWS_Recurrence_AbsoluteMonthly:
		GET_VALUE(dynamic_cast< CEWSRecurrencePatternAbsoluteMonthly*>(p),
		          DayOfMonth,
		          r,
		          day_of_month);
		GET_VALUE(dynamic_cast< CEWSRecurrencePatternInterval*>(p),
		          Interval,
		          r,
		          interval);
		break;
	case EWS_Recurrence_Weekly:
		GET_VALUE_5(dynamic_cast< CEWSRecurrencePatternWeekly*>(p),
		            DaysOfWeek,
		            ews::CEWSRecurrencePattern::EWSDayOfWeekEnum,
		            r,
		            days_of_week);
		GET_VALUE(dynamic_cast< CEWSRecurrencePatternInterval*>(p),
		          Interval,
		          r,
		          interval);
		break;
	case EWS_Recurrence_Daily:
		GET_VALUE(dynamic_cast< CEWSRecurrencePatternInterval*>(p),
		          Interval,
		          r,
		          interval);
		break;
	default:
		break;
	}

	return p;
}

static
ews_recurrence *
from(const CEWSRecurrence * p) {
	if (!p)
		return NULL;

	ews_recurrence * r = (ews_recurrence *)safe_malloc(sizeof(ews_recurrence));
	memset(r, 0, sizeof(ews_recurrence));

	from(&r->range, p->GetRecurrenceRange());
	from(&r->pattern, p->GetRecurrencePattern());

	return r;
}

static
CEWSRecurrence *
to(ews_recurrence * r) {
	if (!r)
		return NULL;

	CEWSRecurrence * p = CEWSRecurrence::CreateInstance();
	p->SetRecurrenceRange(to(&r->range));
	p->SetRecurrencePattern(to(&r->pattern));

	return p;
}

static
ews_occurrence_info *
from(const CEWSOccurrenceInfo * p) {
	if (!p)
		return NULL;

	ews_occurrence_info * r = (ews_occurrence_info*)safe_malloc(sizeof(ews_occurrence_info));

	SET_VALUE_STR(p, ItemId, r, item_id);
	SET_VALUE_STR(p, ChangeKey, r, change_key);
	SET_VALUE(p, Start, r, start);
	SET_VALUE(p, End, r, end);
	SET_VALUE(p, OriginalStart, r, original_start);
    
	return r;
}

static
CEWSOccurrenceInfo *
to(ews_occurrence_info * r) {
	if (!r)
		return NULL;

	CEWSOccurrenceInfo * p =
			CEWSOccurrenceInfo::CreateInstance();
    
	GET_VALUE_STR(p, ItemId, r, item_id);
	GET_VALUE_STR(p, ChangeKey, r, change_key);
	GET_VALUE(p, Start, r, start);
	GET_VALUE(p, End, r, end);
	GET_VALUE(p, OriginalStart, r, original_start);
    
	return p;
}

static
void
list_to_pointer_pointer(CEWSOccurrenceInfoList * pList,
                        int & pp_count,
                        ews_occurrence_info ** & pp){
	if (!pList) {
		pp_count = 0;
		pp = NULL;
		return;
	}

	pp_count = pList->size();

	if (pp_count == 0) {
		pp = NULL;
		return;
	}
    
	pp = (ews_occurrence_info **)safe_malloc(pp_count * sizeof(ews_occurrence_info *));

	for(int i = 0;i < pp_count; i++) {
		pp[i] = from(pList->at(i));
	}
}

static
CEWSOccurrenceInfoList *
list_from_pointer_pointer(int pp_count,
                          ews_occurrence_info ** pp){
	if (pp_count == 0 || !pp) {
		return NULL;
	}
    
	CEWSOccurrenceInfoList * pList = new CEWSOccurrenceInfoList;

	for(int i = 0;i < pp_count; i++) {
		pList->push_back(to(pp[i]));
	}

	return pList;
}

static
ews_deleted_occurrence_info *
from(const CEWSDeletedOccurrenceInfo * p) {
	if (!p) return NULL;

	ews_deleted_occurrence_info * r = (ews_deleted_occurrence_info*)safe_malloc(sizeof(ews_deleted_occurrence_info));

	SET_VALUE(p, Start, r, start);
    
	return r;
}

static
CEWSDeletedOccurrenceInfo *
to(ews_deleted_occurrence_info * r) {
	if (!r) return NULL;

	CEWSDeletedOccurrenceInfo * p =
			CEWSDeletedOccurrenceInfo::CreateInstance();
    
	GET_VALUE(p, Start, r, start);
    
	return p;
}

static
void
list_to_pointer_pointer(CEWSDeletedOccurrenceInfoList * pList,
                        int & pp_count,
                        ews_deleted_occurrence_info ** & pp){
	if (!pList) {
		pp_count = 0;
		pp = NULL;
		return;
	}

	pp_count = pList->size();

	if (pp_count == 0) {
		pp = NULL;
		return;
	}
    
	pp = (ews_deleted_occurrence_info **)safe_malloc(pp_count * sizeof(ews_deleted_occurrence_info *));

	for(int i = 0;i < pp_count; i++) {
		pp[i] = from(pList->at(i));
	}
}

static
CEWSDeletedOccurrenceInfoList *
list_from_pointer_pointer(int pp_count,
                          ews_deleted_occurrence_info ** pp){
	if (pp_count == 0 || !pp) {
		return NULL;
	}

	CEWSDeletedOccurrenceInfoList * pList = new CEWSDeletedOccurrenceInfoList;
    
	for(int i = 0;i < pp_count; i++) {
		pList->push_back(to(pp[i]));
	}

	return pList;
}

static
void
from(ews_time_change_sequence * r, const CEWSTimeChangeSequence *p) {
	if (!p) return;

	r->sequence_type = p->GetTimeChangeSequenceType();
    
	switch(r->sequence_type) {
	case EWS_TimeChange_Sequence_RelativeYearly:
		if (dynamic_cast<const CEWSTimeChangeSequenceRelativeYearly*>(p)->GetRelativeYearly()) {
			SET_VALUE(dynamic_cast<const CEWSTimeChangeSequenceRelativeYearly*>(p)->GetRelativeYearly(),
			          DaysOfWeek,
			          r,
			          days_of_week);
			SET_VALUE(dynamic_cast<const CEWSTimeChangeSequenceRelativeYearly*>(p)->GetRelativeYearly(),
			          DayOfWeekIndex,
			          r,
			          day_of_week_index);
			SET_VALUE(dynamic_cast<const CEWSTimeChangeSequenceRelativeYearly*>(p)->GetRelativeYearly(),
			          Month,
			          r,
			          month);
		}
		break;
	case EWS_TimeChange_Sequence_AbsoluteDate:
		SET_VALUE_STR(dynamic_cast<const CEWSTimeChangeSequenceAbsoluteDate *>(p),
		              AbsoluteDate,
		              r,
		              absolute_date);
		break;
	default:
		break;
	}
}

static
CEWSTimeChangeSequence *
to(ews_time_change_sequence * r) {
	if (!r) return NULL;

	CEWSTimeChangeSequence::EWSTimeChangeSequenceTypeEnum sequenceType =
			(CEWSTimeChangeSequence::EWSTimeChangeSequenceTypeEnum)r->sequence_type;
    
	CEWSTimeChangeSequence *p =
			CEWSTimeChangeSequence::CreateInstance(sequenceType);
    
	switch(r->sequence_type) {
	case EWS_TimeChange_Sequence_RelativeYearly: {
		ews_recurrence_pattern pattern;
		pattern.pattern_type = EWS_Recurrence_RelativeYearly;
		pattern.days_of_week = r->days_of_week;
		pattern.day_of_week_index = r->day_of_week_index;
		pattern.month = r->month;

		CEWSRecurrencePatternRelativeYearly * ry =
				dynamic_cast<CEWSRecurrencePatternRelativeYearly *>(to(&pattern));
        
		dynamic_cast<CEWSTimeChangeSequenceRelativeYearly*>(p)->SetRelativeYearly(ry);
	}
		break;
	case EWS_TimeChange_Sequence_AbsoluteDate:
		GET_VALUE_STR(dynamic_cast<CEWSTimeChangeSequenceAbsoluteDate *>(p),
		              AbsoluteDate,
		              r,
		              absolute_date);
		break;
	default:
		break;
	}

	return p;
}

static
void
from(ews_time_change ** rr, const CEWSTimeChange * p) {
	if (!p) {
		*rr = NULL;
		return;
	}

	ews_time_change * r = *rr = (ews_time_change *)malloc(sizeof(ews_time_change));
	memset(r, 0, sizeof(ews_time_change));
    
	SET_VALUE(p, Offset, r, offset);
	SET_VALUE_STR(p, Time, r, time);
	SET_VALUE_STR(p, TimeZoneName, r, time_zone_name);

	from(&r->time_change_sequence, p->GetTimeChangeSequence());
}

static
CEWSTimeChange *
to(ews_time_change * r) {
	if (!r) return NULL;

	CEWSTimeChange * p =
			CEWSTimeChange::CreateInstance();
    
	GET_VALUE(p, Offset, r, offset);
	GET_VALUE_STR(p, Time, r, time);
	GET_VALUE_STR(p, TimeZoneName, r, time_zone_name);

	p->SetTimeChangeSequence(to(&r->time_change_sequence));

	return p;
}

static
ews_time_zone *
from(const CEWSTimeZone * p) {
	if (!p) return NULL;

	ews_time_zone * r = (ews_time_zone*)safe_malloc(sizeof(ews_time_zone));
	memset(r, 0, sizeof(ews_time_zone));

	SET_VALUE_STR(p, TimeZoneName, r, time_zone_name);
	SET_VALUE(p, BaseOffset, r, base_offset);

	from(&r->standard, p->GetStandard());
	from(&r->daylight, p->GetDaylight());
	return r;
}

static
CEWSTimeZone *
to(ews_time_zone * r) {
	if (!r) return NULL;

	CEWSTimeZone * p =
			CEWSTimeZone::CreateInstance();

	GET_VALUE_STR(p, TimeZoneName, r, time_zone_name);
	GET_VALUE(p, BaseOffset, r, base_offset);

	p->SetStandard(to(r->standard));
	p->SetDaylight(to(r->daylight));

	return p;
}

void
cewscalendaritem_to_calendar_item_3(const CEWSCalendarItem * pItem,
                                    ews_calendar_item * item) {
	__cewsitem_to_item(pItem, &item->item);
	item->item.item_type = EWS_Item_Calendar;

	SET_VALUE_STR(pItem, UID, item, uid);
	SET_VALUE(pItem, RecurrenceId, item, recurrence_id);
	SET_VALUE(pItem, DateTimeStamp, item, date_time_stamp);
	SET_VALUE(pItem, Start, item, start);
	SET_VALUE(pItem, End, item, end);
	SET_VALUE(pItem, OriginalStart, item, original_start);
	SET_VALUE_B(pItem, AllDayEvent, item, all_day_event);
	SET_VALUE(pItem, LegacyFreeBusyStatus, item, legacy_free_busy_status);
	SET_VALUE_STR(pItem, Location, item, location);
	SET_VALUE_STR(pItem, When, item, when);
	SET_VALUE_B(pItem, Meeting, item, is_meeting);
	SET_VALUE_B(pItem, Cancelled, item, is_cancelled);
	SET_VALUE_B(pItem, Recurring, item, is_recurring);
	SET_VALUE(pItem, MeetingRequestWasSent, item, meeting_request_was_sent);
	SET_VALUE_B(pItem, ResponseRequested, item, is_response_requested);
	SET_VALUE(pItem, CalendarItemType, item, calendar_item_type);
	SET_VALUE(pItem, MyResponseType, item, my_response_type);

	if (pItem->GetOrganizer()) {
		item->organizer = (ews_recipient*)safe_malloc(sizeof(ews_recipient));
		memset(item->organizer, 0, sizeof(ews_recipient));
		cewsrecipient_to_recipient(pItem->GetOrganizer(), item->organizer);
	}

	LIST_TO_POINTER_POINTER(pItem, RequiredAttendees, item, required_attendees);
	LIST_TO_POINTER_POINTER(pItem, OptionalAttendees, item, optional_attendees);
	LIST_TO_POINTER_POINTER(pItem, Resources, item, resources);
	LIST_TO_POINTER_POINTER(pItem, ConflictingMeetings, item, conflicting_meetings);
	LIST_TO_POINTER_POINTER(pItem, AdjacentMeetings, item, adjacent_meetings);
	SET_VALUE_STR(pItem, Duration, item, duration);
	SET_VALUE_STR(pItem, TimeZone, item, time_zone);
	SET_VALUE(pItem, AppointmentReplyTime, item, appointment_reply_time);
	SET_VALUE(pItem, AppointmentSequenceNumber, item, appointment_sequence_number);
	SET_VALUE(pItem, AppointmentState, item, appointment_state);

	item->recurrence = from(pItem->GetRecurrence());
	item->first_occurrence = from(pItem->GetFirstOccurrence());
	item->last_occurrence = from(pItem->GetLastOccurrence());
    
	LIST_TO_POINTER_POINTER(pItem, ModifiedOccurrences, item, modified_occurrences);

	LIST_TO_POINTER_POINTER(pItem, DeletedOccurrences, item, deleted_occurrences);

	item->meeting_time_zone = from(pItem->GetMeetingTimeZone());
    
	SET_VALUE(pItem, ConferenceType, item, conference_type);
	SET_VALUE_B(pItem, AllowNewTimeProposal, item, is_allow_new_time_proposal);
	SET_VALUE_B(pItem, OnlineMeeting, item, is_online_meeting);
	SET_VALUE_STR(pItem, MeetingWorkspaceUrl, item, meeting_workspace_url);
	SET_VALUE_STR(pItem, NetShowUrl, item, net_show_url);
}

static
void ews_free_occurrence_info(ews_occurrence_info * p) {
	if (!p) return;
    
	SAFE_FREE(p->item_id);
	SAFE_FREE(p->change_key);
	SAFE_FREE(p);
}

static
void
ews_free_attendee(ews_attendee * p) {
	if (!p) return;

	SAFE_FREE(p->email_address.item_id);
	SAFE_FREE(p->email_address.change_key);
	SAFE_FREE(p->email_address.name);
	SAFE_FREE(p->email_address.email);
	SAFE_FREE(p->email_address.routing_type);
    
	SAFE_FREE(p);
}

void
ews_free_calendar_item(ews_calendar_item * calendar_item) {
	if (!calendar_item) return;
    
	SAFE_FREE(calendar_item->uid);
	SAFE_FREE(calendar_item->location);
	SAFE_FREE(calendar_item->when);

	if (calendar_item->organizer)
		ews_free_recipient(calendar_item->organizer);
	SAFE_FREE(calendar_item->organizer);

	LIST_SAFE_FREE_2(calendar_item->required_attendees, ews_free_attendee);
	LIST_SAFE_FREE_2(calendar_item->optional_attendees, ews_free_attendee);
	LIST_SAFE_FREE_2(calendar_item->resources, ews_free_attendee);

	LIST_SAFE_FREE_2(calendar_item->conflicting_meetings, ews_free_item);
	LIST_SAFE_FREE_2(calendar_item->adjacent_meetings, ews_free_item);

	SAFE_FREE(calendar_item->duration);
	SAFE_FREE(calendar_item->time_zone);

	if (calendar_item->recurrence) {
		SAFE_FREE(calendar_item->recurrence->range.start_date);
		SAFE_FREE(calendar_item->recurrence->range.end_date);
	}
	SAFE_FREE(calendar_item->recurrence);

	ews_free_occurrence_info(calendar_item->first_occurrence);
	ews_free_occurrence_info(calendar_item->last_occurrence);
    
	LIST_SAFE_FREE_2(calendar_item-> modified_occurrences, ews_free_occurrence_info);

	LIST_SAFE_FREE(calendar_item->deleted_occurrences);

	if (calendar_item->meeting_time_zone) {
		SAFE_FREE(calendar_item->meeting_time_zone->time_zone_name);

		if (calendar_item->meeting_time_zone->standard) {
			SAFE_FREE(calendar_item->meeting_time_zone->standard->time);
			SAFE_FREE(calendar_item->meeting_time_zone->standard->time_zone_name);
			SAFE_FREE(calendar_item->
			          meeting_time_zone->standard->time_change_sequence.absolute_date);
		}

		if (calendar_item->meeting_time_zone->daylight) {
			SAFE_FREE(calendar_item->meeting_time_zone->daylight->time);
			SAFE_FREE(calendar_item->meeting_time_zone->daylight->time_zone_name);
			SAFE_FREE(calendar_item->
			          meeting_time_zone->daylight->time_change_sequence.absolute_date);
		}
	}
	SAFE_FREE(calendar_item->meeting_time_zone);
    
	SAFE_FREE(calendar_item->meeting_workspace_url);
	SAFE_FREE(calendar_item->net_show_url);

	SAFE_FREE(calendar_item->recurring_master_id);
}

int
ews_cal_get_recurrence_master_id(ews_session * session,
                                 const char * item_id,
                                 char ** mast_item_id,
                                 char ** master_uid,
                                 char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !item_id)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSCalendarOperation> pItemOp(
	    pSession->CreateCalendarOperation());
	CEWSError error;

	CEWSString masterId;
	CEWSString masterUid;
    
	ON_ERROR_RETURN(!pItemOp->GetRecurrenceMasterItemId(item_id,
	                                                    masterId,
	                                                    masterUid,
	                                                    &error));

	if (mast_item_id)
		*mast_item_id = strdup(masterId.GetData());

	if (master_uid)
		*master_uid = strdup(masterUid.GetData());

	return EWS_SUCCESS;
}

static CEWSCalendarItem * calendar_item_to_cewscalendaritem(ews_calendar_item * item) {
	std::auto_ptr<CEWSCalendarItem> pItem(dynamic_cast<CEWSCalendarItem*>(CEWSItem::CreateInstance(CEWSItem::Item_Calendar)));

	//it will handle mime_content here
	__item_to_cewsitem(&item->item, pItem.get());

	GET_VALUE_STR(pItem, UID, item, uid);
	GET_VALUE(pItem, RecurrenceId, item, recurrence_id);
	GET_VALUE(pItem, DateTimeStamp, item, date_time_stamp);
	GET_VALUE(pItem, Start, item, start);
	GET_VALUE(pItem, End, item, end);
	GET_VALUE(pItem, OriginalStart, item, original_start);
	GET_VALUE(pItem, AllDayEvent, item, all_day_event);
	GET_VALUE_5(pItem,
	            LegacyFreeBusyStatus,
	            enum ews::CEWSCalendarItem::EWSLegacyFreeBusyTypeEnum,
	            item, legacy_free_busy_status);
	GET_VALUE_STR(pItem, Location, item, location);
	GET_VALUE_STR(pItem, When, item, when);
	GET_VALUE(pItem, Meeting, item, is_meeting);
	GET_VALUE(pItem, Cancelled, item, is_cancelled);
	GET_VALUE(pItem, Recurring, item, is_recurring);
	GET_VALUE(pItem, MeetingRequestWasSent, item, meeting_request_was_sent);
	GET_VALUE(pItem, ResponseRequested, item, is_response_requested);
	GET_VALUE_5(pItem,
	            CalendarItemType,
	            enum ews::CEWSCalendarItem::EWSCalendarItemTypeEnum,
	            item,
	            calendar_item_type);
	GET_VALUE_5(pItem, MyResponseType,
	            enum ews::CEWSAttendee::EWSResponseTypeEnum,
	            item, my_response_type);

	if (item->organizer) {
		pItem->SetOrganizer(recipient_to_cewsrecipient(item->organizer));
	}

	LIST_FROM_POINTER_POINTER(pItem, RequiredAttendees, item, required_attendees);
	LIST_FROM_POINTER_POINTER(pItem, OptionalAttendees, item, optional_attendees);
	LIST_FROM_POINTER_POINTER(pItem, Resources, item, resources);
	LIST_FROM_POINTER_POINTER(pItem, ConflictingMeetings, item, conflicting_meetings);
	LIST_FROM_POINTER_POINTER(pItem, AdjacentMeetings, item, adjacent_meetings);
    
	GET_VALUE_STR(pItem, Duration, item, duration);
	GET_VALUE_STR(pItem, TimeZone, item, time_zone);
	GET_VALUE(pItem, AppointmentReplyTime, item, appointment_reply_time);
	GET_VALUE(pItem, AppointmentSequenceNumber, item, appointment_sequence_number);
	GET_VALUE(pItem, AppointmentState, item, appointment_state);

	if (item->recurrence)
		pItem->SetRecurrence(to(item->recurrence));

	if (item->first_occurrence)
		pItem->SetFirstOccurrence(to(item->first_occurrence));
    
	if (item->last_occurrence)
		pItem->SetLastOccurrence(to(item->last_occurrence));
    
	LIST_FROM_POINTER_POINTER(pItem, ModifiedOccurrences, item, modified_occurrences);
	LIST_FROM_POINTER_POINTER(pItem, DeletedOccurrences, item, deleted_occurrences);

	if (item->meeting_time_zone)
		pItem->SetMeetingTimeZone(to(item->meeting_time_zone));
    
	GET_VALUE(pItem, ConferenceType, item, conference_type);
	GET_VALUE(pItem, AllowNewTimeProposal, item, is_allow_new_time_proposal);
	GET_VALUE(pItem, OnlineMeeting, item, is_online_meeting);
	GET_VALUE_STR(pItem, MeetingWorkspaceUrl, item, meeting_workspace_url);
	GET_VALUE_STR(pItem, NetShowUrl, item, net_show_url);
    
	return pItem.release();
}

int
ews_calendar_accept_event(ews_session * session,
                          ews_calendar_item * item,
                          char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !item)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSCalendarOperation> pItemOp(
	    pSession->CreateCalendarOperation());
	CEWSError error;

	std::auto_ptr<CEWSCalendarItem> ewsItem(dynamic_cast<CEWSCalendarItem*>(CEWSItem::CreateInstance(CEWSItem::Item_Calendar)));
    
	ewsItem->SetItemId(item->item.item_id);
	ewsItem->SetChangeKey(item->item.change_key);

	ON_ERROR_RETURN(!pItemOp->AcceptEvent(ewsItem.get(),
	                                      &error));

	cewsitem_to_item_2(ewsItem.get(), &item->item);
	return EWS_SUCCESS;
}

int ews_calendar_decline_event(ews_session * session,
                               ews_calendar_item * item,
                               char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !item)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSCalendarOperation> pItemOp(
	    pSession->CreateCalendarOperation());
	CEWSError error;

	std::auto_ptr<CEWSCalendarItem> ewsItem(dynamic_cast<CEWSCalendarItem*>(CEWSItem::CreateInstance(CEWSItem::Item_Calendar)));
    
	ewsItem->SetItemId(item->item.item_id);
	ewsItem->SetChangeKey(item->item.change_key);

	ON_ERROR_RETURN(!pItemOp->DeclineEvent(ewsItem.get(),
	                                       &error));

	cewsitem_to_item_2(ewsItem.get(), &item->item);
    
	return EWS_SUCCESS;
}

int
ews_calendar_tentatively_accept_event(ews_session * session,
                                      ews_calendar_item * item,
                                      char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !item)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSCalendarOperation> pItemOp(
	    pSession->CreateCalendarOperation());
	CEWSError error;

	std::auto_ptr<CEWSCalendarItem> ewsItem(dynamic_cast<CEWSCalendarItem*>(CEWSItem::CreateInstance(CEWSItem::Item_Calendar)));
    
	ewsItem->SetItemId(item->item.item_id);
	ewsItem->SetChangeKey(item->item.change_key);

	ON_ERROR_RETURN(!pItemOp->TentativelyAcceptEvent(ewsItem.get(),
	                                                 &error));

	cewsitem_to_item_2(ewsItem.get(), &item->item);

	return EWS_SUCCESS;
}

// int
// ews_calendar_item_id_to_uid(ews_session * session,
//                             const char * item_id,
//                             char ** pp_uid,
//                             char ** pp_err_msg) {
// 	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

// 	if (!session || !pSession || !item_id || !pp_uid)
// 		return EWS_INVALID_PARAM;

// 	std::auto_ptr<CEWSCalendarOperation> pItemOp(
//         pSession->CreateCalendarOperation());
// 	CEWSError error;

//     CEWSString uid;
    
// 	ON_ERROR_RETURN(!pItemOp->ItemIdToUID(item_id,
//                                           uid,
//                                           &error));
    
//     *pp_uid = strdup(uid.GetData());
    
// 	return EWS_SUCCESS;
// }
    
int
ews_calendar_delete_occurrence(ews_session * session,
                               const char * master_id,
                               int occurrence_instance,
                               char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !master_id)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSCalendarOperation> pItemOp(
	    pSession->CreateCalendarOperation());
	CEWSError error;

	ON_ERROR_RETURN(!pItemOp->DeleteOccurrence(master_id,
	                                           occurrence_instance,
	                                           &error));

	return EWS_SUCCESS;
}

int
ews_calendar_update_event(ews_session * session,
                          ews_calendar_item * item,
                          int flags,
                          char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !item)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSCalendarOperation> pItemOp(
	    pSession->CreateCalendarOperation());
	CEWSError error;

	std::auto_ptr<CEWSCalendarItem> pCalendarItem(calendar_item_to_cewscalendaritem(item));
	
	ON_ERROR_RETURN(!pItemOp->UpdateEvent(pCalendarItem.get(),
	                                      flags,
	                                      &error));

	cewsitem_to_item_2(pCalendarItem.get(), &item->item);
	return EWS_SUCCESS;
}

int
ews_calendar_update_event_occurrence(ews_session * session,
                                     ews_calendar_item * item,
                                     int instanceIndex,
                                     int flags,
                                     char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !item)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSCalendarOperation> pItemOp(
	    pSession->CreateCalendarOperation());
	CEWSError error;

	std::auto_ptr<CEWSCalendarItem> pCalendarItem(calendar_item_to_cewscalendaritem(item));
	
	ON_ERROR_RETURN(!pItemOp->UpdateEventOccurrence(pCalendarItem.get(),
	                                                instanceIndex,
	                                                flags,
	                                                &error));

	cewsitem_to_item_2(pCalendarItem.get(), &item->item);
	return EWS_SUCCESS;
}

int
ews_sync_task(ews_session * session,
              ews_sync_item_callback * callback,
              char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !callback)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSTaskOperation> pItemOp(
	    pSession->CreateTaskOperation());
	CEWSError error;

	ews::c::CEWSItemOperationCallback_C _callback(callback);

	ON_ERROR_RETURN(!pItemOp->SyncTask(&_callback, &error));

	return EWS_SUCCESS;
}

static
ews_task_item *
cewstaskitem_to_task_item(const CEWSTaskItem * pItem) {
	std::auto_ptr<ews_task_item> item((ews_task_item *)safe_malloc(sizeof(ews_task_item)));
	memset(item.get(), 0, sizeof(ews_task_item));

	cewstaskitem_to_task_item_3(pItem, item.get());

	return item.release();
}

static
void
cewstaskitem_to_task_item_3(const CEWSTaskItem * pItem,
                            ews_task_item * item) {
	__cewsitem_to_item(pItem, &item->item);
	item->item.item_type = EWS_Item_Task;

	SET_VALUE(pItem, ActualWork, item, actual_work);
	SET_VALUE(pItem, AssignedTime, item, assigned_time);
	SET_VALUE_STR(pItem, BillingInformation, item, billing_information);
	SET_VALUE(pItem, ChangeCount, item, change_count);
	LIST_TO_POINTER_POINTER(pItem, Companies, item, companies);
	SET_VALUE(pItem, CompleteDate, item, complete_date);
	LIST_TO_POINTER_POINTER(pItem, Contacts, item, contacts);
	SET_VALUE(pItem, DelegationState, item, delegation_state);
	SET_VALUE_STR(pItem, Delegator, item, delegator);
	SET_VALUE(pItem, DueDate, item, due_date);
	SET_VALUE(pItem, IsAssignmentEditable, item, is_assignment_editable);
	SET_VALUE_B(pItem, Complete, item, is_complete);
	SET_VALUE_B(pItem, Recurring, item, is_recurring);
	SET_VALUE_B(pItem, TeamTask, item, is_team_task);
	SET_VALUE_STR(pItem, Mileage, item, mileage);
	SET_VALUE_STR(pItem, Owner, item, owner);
	SET_VALUE(pItem, PercentComplete, item, percent_complete);

	item->recurrence = from(pItem->GetRecurrence());

	SET_VALUE(pItem, StartDate, item, start_date);
	SET_VALUE(pItem, Status, item, status);
	SET_VALUE_STR(pItem, StatusDescription, item, status_description);
	SET_VALUE(pItem, TotalWork, item, total_work);
}

static
CEWSTaskItem *
task_item_to_cewstaskitem(ews_task_item * item) {
	std::auto_ptr<CEWSTaskItem> pItem(dynamic_cast<CEWSTaskItem*>(CEWSItem::CreateInstance(CEWSItem::Item_Task)));

	//it will handle mime_content here
	__item_to_cewsitem(&item->item, pItem.get());

	GET_VALUE(pItem, ActualWork, item, actual_work);
	GET_VALUE(pItem, AssignedTime, item, assigned_time);
	GET_VALUE_STR(pItem, BillingInformation, item, billing_information);
	GET_VALUE(pItem, ChangeCount, item, change_count);
	LIST_FROM_POINTER_POINTER(pItem, Companies, item, companies);
	GET_VALUE(pItem, CompleteDate, item, complete_date);
	LIST_FROM_POINTER_POINTER(pItem, Contacts, item, contacts);
	GET_VALUE_5(pItem,
	            DelegationState,
	            enum ews::CEWSTaskItem::DelegateStateTypeEnum,
	            item,
	            delegation_state);
	GET_VALUE_STR(pItem, Delegator, item, delegator);
	GET_VALUE(pItem, DueDate, item, due_date);
	GET_VALUE(pItem, IsAssignmentEditable, item, is_assignment_editable);
	GET_VALUE(pItem, Complete, item, is_complete);
	GET_VALUE(pItem, Recurring, item, is_recurring);
	GET_VALUE(pItem, TeamTask, item, is_team_task);
	GET_VALUE_STR(pItem, Mileage, item, mileage);
	GET_VALUE_STR(pItem, Owner, item, owner);
	GET_VALUE(pItem, PercentComplete, item, percent_complete);

	if (item->recurrence)
		pItem->SetRecurrence(to(item->recurrence));

	GET_VALUE(pItem, StartDate, item, start_date);
	GET_VALUE_5(pItem,
	            Status,
	            enum ews::CEWSTaskItem::StatusTypeEnum,
	            item,
	            status);
	GET_VALUE_STR(pItem, StatusDescription, item, status_description);
	GET_VALUE(pItem, TotalWork, item, total_work);

	return pItem.release();
}

void
ews_free_task_item(ews_task_item * item) {
	if (!item)
		return;
    
	SAFE_FREE(item->billing_information);
	LIST_SAFE_FREE(item->companies);
	LIST_SAFE_FREE(item->contacts);
	SAFE_FREE(item->delegator);
	SAFE_FREE(item->mileage);
	SAFE_FREE(item->owner);
	if (item->recurrence) {
		SAFE_FREE(item->recurrence->range.start_date);
		SAFE_FREE(item->recurrence->range.end_date);
	}
	SAFE_FREE(item->recurrence);
	SAFE_FREE(item->status_description);
}

int
ews_calendar_update_task(ews_session * session,
                         ews_task_item * item,
                         int flags,
                         char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !item)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSTaskOperation> pItemOp(
	    pSession->CreateTaskOperation());
	CEWSError error;

	std::auto_ptr<CEWSTaskItem> pItem(task_item_to_cewstaskitem(item));
	
	ON_ERROR_RETURN(!pItemOp->UpdateTask(pItem.get(),
	                                     flags,
	                                     &error));

	cewsitem_to_item_2(pItem.get(), &item->item);
	return EWS_SUCCESS;
}

int
ews_calendar_update_task_occurrence(ews_session * session,
                                    ews_task_item * item,
                                    int instanceIndex,
                                    int flags,
                                    char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !item)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSTaskOperation> pItemOp(
	    pSession->CreateTaskOperation());
	CEWSError error;

	std::auto_ptr<CEWSTaskItem> pItem(task_item_to_cewstaskitem(item));
	
	ON_ERROR_RETURN(!pItemOp->UpdateTaskOccurrence(pItem.get(),
	                                               instanceIndex,
	                                               flags,
	                                               &error));

	cewsitem_to_item_2(pItem.get(), &item->item);
	return EWS_SUCCESS;
}

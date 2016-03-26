/*
 * libews_c_attachment.cpp
 *
 *  Created on: Aug 28, 2014
 *      Author: stone
 */

#include "libews.h"
#include <memory>
#include <stdlib.h>

using namespace ews;

#define ON_ERROR_RETURN(x) \
		if ((x)) { \
			if (pp_err_msg) \
		*pp_err_msg = strdup(error.GetErrorMessage()); \
		return error.GetErrorCode(); \
	}

#define SAFE_FREE(x) \
		if ((x)) free((x));

void ews_free_attachment(ews_attachment * attachment) {
    if (!attachment) return;
    
	switch (attachment->attachment_type) {
	case CEWSAttachment::Attachment_File: {
        ews_file_attachment* file_attachment = (ews_file_attachment*)attachment;
        SAFE_FREE(file_attachment->content);
	}
		break;
	case CEWSAttachment::Attachment_Item: {
        ews_item_attachment* item_attachment = (ews_item_attachment*)attachment;
        SAFE_FREE(item_attachment->item_id);
	}
		break;
	default:
        break;
    }

    SAFE_FREE(attachment->attachment_id);
    SAFE_FREE(attachment->content_id);
    SAFE_FREE(attachment->content_location);
    SAFE_FREE(attachment->content_type);
    SAFE_FREE(attachment->name);

    SAFE_FREE(attachment);
}

ews_attachment * cewsattachment_to_attachment(CEWSAttachment* pAttachment) {
	ews_attachment * attachment = NULL;

	switch (pAttachment->GetAttachmentType()) {
	case CEWSAttachment::Attachment_File: {
		ews_file_attachment* file_attachment = (ews_file_attachment*) (malloc(
				sizeof(ews_file_attachment)));
		attachment= (ews_attachment*) (file_attachment);
		file_attachment->content = strdup(
				dynamic_cast<CEWSFileAttachment*>(pAttachment)->GetContent());
        attachment->attachment_type = CEWSAttachment::Attachment_File;
	}
		break;
	case CEWSAttachment::Attachment_Item: {
		ews_item_attachment* item_attachment = (ews_item_attachment*) (malloc(
				sizeof(ews_item_attachment)));
		attachment = (ews_attachment*) (item_attachment);
		item_attachment->item_id = strdup(
				dynamic_cast<CEWSItemAttachment*>(pAttachment)->GetItemId());
		item_attachment->item_type =
				dynamic_cast<CEWSItemAttachment*>(pAttachment)->GetItemType();
        attachment->attachment_type = CEWSAttachment::Attachment_Item;
	}
		break;
	default:
		return NULL;
	}

	attachment->attachment_id = strdup(
			pAttachment->GetAttachmentId());
	attachment->content_id = strdup(pAttachment->GetContentId());
	attachment->content_location = strdup(
			pAttachment->GetContentLocation());
	attachment->content_type = strdup(pAttachment->GetContentType());
	attachment->name = strdup(pAttachment->GetName());

	return attachment;
}

CEWSAttachment* attachment_to_cewsattachment(ews_attachment* attachment) {
	CEWSAttachment* pAttachment = NULL;

	switch (attachment->attachment_type) {
	case CEWSAttachment::Attachment_File: {
		pAttachment = CEWSAttachment::CreateInstance(
				CEWSAttachment::Attachment_File);
        if (((ews_file_attachment*) (attachment))->content)
            dynamic_cast<CEWSFileAttachment*>(pAttachment)->SetContent(
				((ews_file_attachment*) (attachment))->content);
	}
		break;
	case CEWSAttachment::Attachment_Item: {
		pAttachment = CEWSAttachment::CreateInstance(
				CEWSAttachment::Attachment_Item);
        if(((ews_item_attachment*) (attachment))->item_id) {
            dynamic_cast<CEWSItemAttachment*>(pAttachment)->SetItemId(
				((ews_item_attachment*) (attachment))->item_id);
            dynamic_cast<CEWSItemAttachment*>(pAttachment)->SetItemType(
				(CEWSItem::EWSItemType) (((ews_item_attachment*) (attachment))->item_type));
        }
	}
		break;
	default:
		return NULL;
	}

	return pAttachment;
}

int ews_get_attachment(ews_session * session, const char * attachment_id, ews_attachment ** pp_attachment, char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !attachment_id || !pp_attachment)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSAttachmentOperation> pAttachmentOp(
			pSession->CreateAttachmentOperation());
	CEWSError error;

	std::auto_ptr<CEWSAttachment> pAttachment(pAttachmentOp->GetAttachment(attachment_id, &error));

	ON_ERROR_RETURN(!pAttachment.get());

	*pp_attachment = cewsattachment_to_attachment(pAttachment.get());

	return EWS_SUCCESS;
}

int ews_create_attachments(ews_session * session, const char * item_id, ews_attachment * attachments, int attachment_count, char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !item_id || !attachments)
		return EWS_INVALID_PARAM;

	if (!attachment_count)
		return EWS_SUCCESS;

	std::auto_ptr<CEWSAttachmentOperation> pAttachmentOp(
			pSession->CreateAttachmentOperation());
	CEWSError error;

	CEWSAttachmentList attachmentList;
	std::auto_ptr<CEWSItem> pParentItem(CEWSItem::CreateInstance(CEWSItem::Item_Message));
	pParentItem->SetItemId(item_id);

	for(int i=0;i<attachment_count;i++) {
		attachmentList.push_back(attachment_to_cewsattachment(&attachments[i]));
	}

	ON_ERROR_RETURN(!pAttachmentOp->CreateAttachments(&attachmentList, pParentItem.get(), &error));

	return EWS_SUCCESS;
}

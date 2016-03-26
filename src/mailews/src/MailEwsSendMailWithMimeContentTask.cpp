#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgIncomingServer.h"
#include "nsArrayUtils.h"

#include "libews.h"

#include "MailEwsSendMailWithMimeContentTask.h"
#include "MailEwsMsgUtils.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"

#include "MailEwsLog.h"

SendMailWithMimeContentTask::SendMailWithMimeContentTask(nsIMsgIncomingServer * pIncomingServer,
                                                         const char * mimeContent,
                                                         int32_t len,
                                                         nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
	, m_pIncomingServer(pIncomingServer) {
	m_MimeContent.Assign(mimeContent, len);
}

SendMailWithMimeContentTask::~SendMailWithMimeContentTask() {
}
	
NS_IMETHODIMP SendMailWithMimeContentTask::Run() {
	int ret = EWS_FAIL;
	nsCString err_msg("");
	
	if (EWS_FAIL == NotifyBegin(this)) {
		return NS_OK;
	}
	
    ret = msg_server_send_mail(m_pIncomingServer,
                               m_MimeContent,
                               err_msg);
    
    NotifyError(this, ret, err_msg.get());

	return NS_OK;
}


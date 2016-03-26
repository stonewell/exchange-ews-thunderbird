#include "MailEwsRunnable.h"
#include "IMailEwsTaskCallback.h"
#include "nsArrayUtils.h"
#include "MailEwsLog.h"

MailEwsRunnable::MailEwsRunnable(nsISupportsArray * callbacks)
	: m_pEwsTaskCallbacks(callbacks) {
}

MailEwsRunnable::~MailEwsRunnable() {
}

void MailEwsRunnable::NotifyError(nsIRunnable * runnable,
                                   int code,
                                   const char * err_msg) {
	if (code != EWS_SUCCESS) {
		PRUint32 length;
		m_pEwsTaskCallbacks->Count(&length);

		for (PRUint32 i=0; i<length; ++i) {
            nsCOMPtr<nsISupports> _supports;
            m_pEwsTaskCallbacks->GetElementAt(i, getter_AddRefs(_supports));
            nsCOMPtr<IMailEwsTaskCallback> element =
                    do_QueryInterface(_supports);
            
			element->OnTaskError(runnable, code, err_msg ? nsCString(err_msg) : nsCString(""));
		}
	}
}

int MailEwsRunnable::NotifyBegin(nsIRunnable * runnable) {
	PRUint32 length;
	m_pEwsTaskCallbacks->Count(&length);

	for (PRUint32 i=0; i<length; ++i) {
        nsCOMPtr<nsISupports> _supports;
        m_pEwsTaskCallbacks->GetElementAt(i, getter_AddRefs(_supports));
        nsCOMPtr<IMailEwsTaskCallback> element =
                do_QueryInterface(_supports);
        if (NS_FAILED(element->OnTaskBegin(runnable)))
	        return EWS_FAIL;
	}

	return EWS_SUCCESS;
}

void MailEwsRunnable::NotifyEnd(nsIRunnable * runnable, int result, void * taskData) {
	PRUint32 length;
	m_pEwsTaskCallbacks->Count(&length);

	for (PRUint32 i=0; i<length; ++i) {
        nsCOMPtr<nsISupports> _supports;
        m_pEwsTaskCallbacks->GetElementAt(i, getter_AddRefs(_supports));
        nsCOMPtr<IMailEwsTaskCallback> element =
                do_QueryInterface(_supports);

        element->OnTaskDone(runnable, result, taskData);
	}
}

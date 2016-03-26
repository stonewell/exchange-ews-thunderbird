#ifndef __MAIL_EWS_RUNNABLE_H__
#define __MAIL_EWS_RUNNABLE_H__

#include "nsThreadUtils.h"
#include "nsISupportsArray.h"
#include "libews.h"
#include "MailEwsCommon.h"

class DLL_LOCAL MailEwsRunnable : public nsRunnable {
public:
	MailEwsRunnable(nsISupportsArray *);
	virtual ~MailEwsRunnable();

protected:
	void NotifyError(nsIRunnable * runnable,
	                 int code,
	                 const char * err_msg);
	int  NotifyBegin(nsIRunnable * runnable);
	void NotifyEnd(nsIRunnable * runnable, int result, void * taskData = NULL);
	
protected:
	nsCOMPtr<nsISupportsArray> m_pEwsTaskCallbacks;
};

#endif //__MAIL_EWS_RUNNABLE_H__


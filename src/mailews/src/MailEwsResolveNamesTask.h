#pragma once
#ifndef __MAIL_EWS_RESOLVE_NAMES_TASK_H__
#define __MAIL_EWS_RESOLVE_NAMES_TASK_H__

#include "libews.h"

#include "nsIMsgIncomingServer.h"
#include "MailEwsRunnable.h"

class nsIAbDirSearchListener;
class nsIAbDirectory;

class DLL_LOCAL ResolveNamesTask : public MailEwsRunnable
{
public:
	ResolveNamesTask(const nsACString & unresolvedEntry,
	                 nsIAbDirSearchListener * aListener,
                     nsIAbDirectory * aParent,
	                 const nsACString & uuid,
	                 int32_t aResultLimit,
                     bool doSyncCall,
	                 nsIMsgIncomingServer * pIncomingServer,
                     int32_t aSearchContext,
	                 nsISupportsArray * ewsTaskCallbacks);

	virtual ~ResolveNamesTask();

	NS_IMETHOD Run();

private:
	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
	nsCString m_UnresolvedEntry;
	nsCOMPtr<nsIAbDirSearchListener> m_pListener;
	int32_t m_ResultLimit;
	nsCString m_Uuid;
    bool m_DoSyncCall;
    nsCOMPtr<nsIAbDirectory> m_pParent;
    int32_t m_SearchContext;
};

#endif //__MAIL_EWS_RESOLVE_NAMES_TASK_H__

#ifndef __MAIL_EWS_SERVICE_H__
#define __MAIL_EWS_SERVICE_H__

#include "xpcom-config.h"
#include "nsStringAPI.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsITimer.h"
#include "nspr.h"
#include "nscore.h"

#include "calIEvent.h"

#include "libews.h"
#include "MailEwsCommon.h"

#include "IMailEwsService.h"
#include "IMailEwsTaskCallback.h"

class MailEwsErrorInfo;
class nsIThread;
class nsIRunnable;
class MailEwsSubscriptionCallback;

class MailEwsService : public IMailEwsService,
                       public IMailEwsTaskCallback
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
	NS_DECL_IMAILEWSSERVICE
	NS_DECL_IMAILEWSTASKCALLBACK

	MailEwsService();

private:
	virtual ~MailEwsService();

protected:
	/* additional members */
	NS_IMETHOD CreateSession();
    NS_IMETHODIMP CreateSession(ews_session ** ppSession);

	void AsyncPromptPassword();
	void HandleEwsError(MailEwsErrorInfo * pError);

    NS_IMETHOD StartSmtpProxy(IMailEwsTaskCallback* callback);
    
	ews_session * m_pSession;
	nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
	nsCOMPtr<nsIThread> m_WorkerThread;
	nsCOMPtr<nsIThread> m_NotificationThread;
	nsCOMPtr<nsIThread> m_SmtpProxyThread;
	nsCOMArray<nsIRunnable> m_TaskQueue;
	nsTArray<nsCString> m_FolderIdNamesToFolderId;
	std::auto_ptr<MailEwsSubscriptionCallback> m_SubscriptionCallback;
	bool m_IsShutdown;
	nsCOMPtr<nsITimer> m_PlaybackTimer;
	nsCOMArray<nsIRunnable> m_DelayedTaskQueue;
	PRLock * m_Lock;
    ews_session_params m_SessionParams;
    bool m_SessionParamsInitialized;
	nsCOMPtr<nsIThread> m_CalendarThread;
	
	NS_IMETHOD QueueTask(nsIRunnable * runnable);
    NS_IMETHOD __UpdateFolderPropertiesWithId(const nsACString & folder_id,
                                              int32_t folder_id_name,
                                              IMailEwsMsgFolder *folder,
                                              IMailEwsTaskCallback *callback);
	NS_IMETHOD CreatePlaybackTimer();
	static void PlaybackTimerCallback(nsITimer *aTimer, void *aClosure);
    NS_IMETHOD InitSessionParams();

    nsresult GetAllFolderIds(nsIMsgFolder *rootFolder,
                    nsTArray<nsCString> & folderIds);
    nsresult GetAllFolderIds(nsTArray<nsCString> & folderIds);
    
    friend class PromptPasswordTask;
};

#endif

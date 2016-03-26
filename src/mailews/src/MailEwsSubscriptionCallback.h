#pragma once
#ifndef __MAIL_EWS_SUBSCRIPTION_CALLBACK_H__
#define __MAIL_EWS_SUBSCRIPTION_CALLBACK_H__

#include "libews.h"

#include "MailEwsRunnable.h"
#include "nsIMsgIncomingServer.h"

class DLL_LOCAL MailEwsSubscriptionCallback {
public:
    MailEwsSubscriptionCallback(nsIMsgIncomingServer * pIncomingServer);
    virtual ~MailEwsSubscriptionCallback();

public:
	virtual void NewMail(const char * parentFolderId,
	                     const char * itemId);

	virtual void MoveItem(const char * oldParentFolderId,
	                      const char * parentFolderId,
	                      const char * oldItemId,
	                      const char * itemId);
	
	virtual void ModifyItem(const char * parentFolderId,
	                        const char * itemId,
                            int unreadCount);

	virtual void CopyItem(const char * oldParentFolderId,
	                      const char * parentFolderId,
	                      const char * oldItemId,
	                      const char * itemId);
	
	virtual void DeleteItem(const char * parentFolderId,
	                        const char * itemId);
	
	virtual void CreateItem(const char * parentFolderId,
	                        const char * itemId);

	virtual void MoveFolder(const char * oldParentFolderId,
	                        const char * parentFolderId,
	                        const char * oldFolderId,
	                        const char * folderId);
    
	virtual void ModifyFolder(const char * parentFolderId,
	                          const char * folderId,
                              int unreadCount);
	
	virtual void CopyFolder(const char * oldParentFolderId,
	                        const char * parentFolderId,
	                        const char * oldFolderId,
	                        const char * folderId);
	
	virtual void DeleteFolder(const char * parentFolderId,
	                          const char * folderId);
	
	virtual void CreateFolder(const char * parentFolderId,
	                          const char * folderId);

    const char * GetUrl() const { return m_URL; }
    void SetUrl(char * url) { if (m_URL) free(m_URL); m_URL = url; }

    ews_event_notification_callback * GetEwsCallback() { return &m_Callback; };

    nsTArray<ews_subscription *> * GetEwsSubscriptions() { return &m_Subscriptions; }
    void AddEwsSubscription(ews_subscription * s) { m_Subscriptions.AppendElement(s); }
    ews_notification_server * GetNotificationServer() { return m_NotifyServer; }
    void SetNotificationServer(ews_notification_server * s) {
        if (m_NotifyServer) {
            ews_notification_server * tmp_server = m_NotifyServer;
            m_NotifyServer = NULL;
            ews_free_notification_server(tmp_server);
        }
        
        m_NotifyServer = s;
    }

    void StopNotificationServer() {
	    if (m_NotifyServer)
		    ews_notification_server_stop(m_NotifyServer);
    }
private:
    char * m_URL;
    ews_event_notification_callback m_Callback;
    nsTArray<ews_subscription *> m_Subscriptions;
    nsCOMPtr<nsIMsgIncomingServer> m_pIncomingServer;
    ews_notification_server * m_NotifyServer;

    void Initialize();
    nsresult GetFolderWithId(const nsCString & folderId,
                             nsIMsgFolder ** pFolder,
                             bool & calendarOrTaskFolder);
};

#endif //__MAIL_EWS_SUBSCRIPTION_CALLBACK_H__


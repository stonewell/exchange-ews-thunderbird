#ifndef __MAIL_EWS_INCOMING_SERVER_H__
#define __MAIL_EWS_INCOMING_SERVER_H__

#include "nsMsgBaseCID.h"
#include "MsgIncomingServerBase.h"
#include "libews.h"
#include "MailEwsCommon.h"

#include "IMailEwsMsgIncomingServer.h"
#include "IMailEwsService.h"

class SyncFolderCallback;
class MailEwsErrorInfo;
class calICalendar;

class MailEwsMsgIncomingServer
		: public MsgIncomingServerBase
		, public IMailEwsMsgIncomingServer {
public:
	NS_DECL_IMAILEWSMSGINCOMINGSERVER
	NS_DECL_ISUPPORTS_INHERITED
	
	MailEwsMsgIncomingServer();

	virtual nsresult CreateRootFolderFromUri(const nsCString&, nsIMsgFolder**) override;
	NS_IMETHOD PerformExpand(nsIMsgWindow *aMsgWindow) override;
	NS_IMETHOD GetLocalStoreType(nsACString & aLocalStoreType) override;
    NS_IMETHOD Shutdown() override;
	NS_IMETHOD VerifyLogon(nsIUrlListener *aUrlListener,
	                       nsIMsgWindow *aMsgWindow,
	                       nsIURI **aURL) override;
	NS_IMETHOD PerformBiff(nsIMsgWindow* aMsgWindow) override;
    NS_IMETHOD CloseCachedConnections() override;
private:
	bool m_SyncingFolder;
	bool m_PromptingPassword;
	nsCOMPtr<IMailEwsService> m_pService;
	std::auto_ptr<SyncFolderCallback> m_pSyncFolderCallback;
    bool m_IsShuttingDown;

	NS_IMETHOD GetNewMessagesForFolder(nsIMsgFolder * rootFolder,
	                                   bool deep,
	                                   nsIMsgWindow * aMsgWindow);
    NS_IMETHOD CreateContactDirectory();
    NS_IMETHOD CreateCalendar();
    
    nsresult CreateCalendarUriAndName(nsIURI ** _ret,
                                     nsCString & name);
protected:
	virtual ~MailEwsMsgIncomingServer();

public:
};

#endif //__MAIL_EWS_INCOMING_SERVER_H__

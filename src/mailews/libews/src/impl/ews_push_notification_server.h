/*
 * ews_push_notification_server.h
 *
 *  Created on: 2015年1月10日
 *      Author: stone
 */

#ifndef EWS_PUSH_NOTIFICATION_SERVER_H_
#define EWS_PUSH_NOTIFICATION_SERVER_H_

#include "libews.h"
#include <memory>

namespace ewsn {
class DLL_LOCAL CLocalEwsGsoapNotificationServerImpl;

class DLL_LOCAL CLocalEWSPushNotificationServer {
public:
	CLocalEWSPushNotificationServer(ews::CEWSSession * pSession,
                                    ews::CEWSSubscriptionCallback * pCallback,
                                    char * ip,
                                    int port,
                                    bool useHttps);
	virtual ~CLocalEWSPushNotificationServer();

public:
	const ews::CEWSString & GetURL() const;
	const ews::CEWSError * GetLastError() const;

	int Bind();
	int Run();
	int Stop();

private:
	ews::CEWSSession * m_pSession;
	ews::CEWSSubscriptionCallback * m_pCallback;
	std::auto_ptr<CLocalEwsGsoapNotificationServerImpl> m_pServerImpl;
};
};


#endif /* EWS_PUSH_NOTIFICATION_SERVER_H_ */

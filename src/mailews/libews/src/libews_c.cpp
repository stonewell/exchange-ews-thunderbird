/*
 * libews_c.cpp
 *
 *  Created on: Aug 27, 2014
 *      Author: stone
 */

#include "libews.h"
#include <memory>
#include <string.h>

using namespace ews;

int ews_session_init(ews_session_params * params,
                     ews_session ** pp_session,
                     char ** pp_err_msg) {
	if (!params || !pp_session || (params && !params->endpoint)) {
		if (pp_err_msg)
			*pp_err_msg = strdup("invalid parameters");
		return EWS_INVALID_PARAM;
	}

	CEWSError error;
    EWSProxyInfo proxyInfo;
    
	if (params->proxy_host) {
		proxyInfo.Host = (params->proxy_host);
		proxyInfo.ProxyType = ((EWSProxyInfo::EWSProxyTypeEnum)params->proxy_type);

		if (params->proxy_password)
			proxyInfo.Password = (params->proxy_password);

		if (params->proxy_user)
			proxyInfo.User = (params->proxy_user);

		if (params->proxy_port > 0)
			proxyInfo.Port = (params->proxy_port);
	}

    EWSConnectionInfo connInfo;
    EWSAccountInfo accInfo;

    if (params->endpoint)
        connInfo.Endpoint = params->endpoint;
    connInfo.AuthMethod = (EWSConnectionInfo::EWSAuthMethodEnum)params->auth_method;

    if (params->timezone_id)
        connInfo.TimezoneId = params->timezone_id;

    if (params->user)
        accInfo.UserName = params->user;
    if (params->password)
        accInfo.Password = params->password;
    if (params->domain)
        accInfo.Domain = params->domain;
    if (params->email)
        accInfo.EmailAddress = params->email;
    
	std::auto_ptr<CEWSSession> pSession(CEWSSession::CreateSession(&connInfo,
                                                                   &accInfo,
                                                                   &proxyInfo,
                                                                   &error));
    

	if (!pSession.get()) {
		if (pp_err_msg)
			*pp_err_msg = strdup(error.GetErrorMessage());

		return error.GetErrorCode();
	}

	*pp_session = pSession.release();
	return EWS_SUCCESS;
}

void
ews_session_cleanup(ews_session * session) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (pSession) {
		delete pSession;
	}
}

void
ews_session_set_timezone_id(ews_session * session,
                            const char * tz_id) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

    if (pSession) {
        EWSConnectionInfo * pConnInfo =
                pSession->GetConnectionInfo();

        pConnInfo->TimezoneId = tz_id;
    }
}

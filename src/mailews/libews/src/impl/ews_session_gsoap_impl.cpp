/*
 * ews_session_impl.cpp
 *
 *  Created on: Mar 28, 2014
 *      Author: stone
 */

#include "libews_gsoap.h"
#include <memory>
#include <sstream>

extern "C" bool soap_create_local_time;
using namespace ews;

CEWSOperatorProvider::CEWSOperatorProvider() {

}

CEWSOperatorProvider::~CEWSOperatorProvider() {

}

CEWSSession * CEWSSession::CreateSession(EWSConnectionInfo * connInfo,
                                         EWSAccountInfo * accountInfo,
                                         EWSProxyInfo * proxyInfo,
                                         CEWSError * pError) {
	std::auto_ptr<CEWSSessionGsoapImpl> pSession(
        new CEWSSessionGsoapImpl(connInfo, accountInfo, proxyInfo));

	if (pSession->Initialize(pError)) {
		return pSession.release();
	}

	return NULL;
}

bool CEWSSessionGsoapImpl::m_envInitialized = false;

CEWSSessionGsoapImpl::CEWSSessionGsoapImpl(EWSConnectionInfo * connInfo,
                                           EWSAccountInfo * accountInfo,
                                           EWSProxyInfo * proxyInfo)
    : m_Proxy(connInfo ? connInfo->Endpoint : "",
              SOAP_IO_KEEPALIVE | SOAP_IO_STORE | SOAP_C_UTFSTRING | SOAP_XML_NOTYPE)
    , m_pCUrlHelper(NULL) {
    
    if (connInfo) {
        m_ConnInfo.Endpoint = connInfo->Endpoint;
        m_ConnInfo.AuthMethod = connInfo->AuthMethod;
    }

    if (accountInfo) {
        m_AccountInfo.UserName = accountInfo->UserName;
        m_AccountInfo.Password = accountInfo->Password;
        m_AccountInfo.Domain = accountInfo->Domain;
        m_AccountInfo.EmailAddress = accountInfo->EmailAddress;
    }

    if (proxyInfo) {
        m_ProxyInfo.Host = proxyInfo->Host;
        m_ProxyInfo.User = proxyInfo->User;
        m_ProxyInfo.Password = proxyInfo->Password;
        m_ProxyInfo.ProxyType = proxyInfo->ProxyType;
        m_ProxyInfo.Port = proxyInfo->Port;
    }
}

CEWSSessionGsoapImpl::~CEWSSessionGsoapImpl() {
	CleanupCurlSoapHelper(&m_Proxy);
}

bool CEWSSessionGsoapImpl::Initialize(CEWSError * pError) {
	if (!m_envInitialized) {
#ifndef _WIN32
		soap_ssl_init();

		if (soap_ssl_client_context(&m_Proxy,
                                    SOAP_SSL_NO_AUTHENTICATION,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL)) {
			if (pError) {
				//we should use soap_stream_fault here
				//since curl is not take over this function
				std::stringstream ss;
				soap_stream_fault(&m_Proxy, ss);
				pError->SetErrorMessage(ss.str().c_str());
				pError->SetErrorCode(EWS_FAIL);
			}
			return false;
		}
#endif
		m_envInitialized = true;
	}

	m_Proxy.userid = m_AccountInfo.UserName.GetData();
	m_Proxy.passwd = m_AccountInfo.Password.GetData();

	m_Proxy.ssl_flags = SOAP_SSL_NO_AUTHENTICATION;
	m_Proxy.mode = SOAP_IO_KEEPALIVE | SOAP_IO_STORE | SOAP_C_UTFSTRING | SOAP_XML_NOTYPE;
	m_Proxy.authrealm = m_AccountInfo.Domain.GetData();
	m_Proxy.ntlm_challenge = "";

	if (GetProxyInfo() && !GetProxyInfo()->Host.IsEmpty()) {
		m_Proxy.proxy_host = GetProxyInfo()->Host.GetData();
		m_Proxy.proxy_userid = GetProxyInfo()->User.GetData();
		m_Proxy.proxy_passwd = GetProxyInfo()->Password.GetData();
		m_Proxy.proxy_port = GetProxyInfo()->Port;
		m_Proxy.proxy_from =
				reinterpret_cast<const char *>((long) GetProxyInfo()->ProxyType);
	}

	memset(m_Proxy.endpoint, 0, sizeof(m_Proxy.endpoint) / sizeof(char));
    m_Proxy.soap_endpoint = m_ConnInfo.Endpoint;
	m_ConnInfo.Endpoint.CopyTo(m_Proxy.endpoint,
                         sizeof(m_Proxy.endpoint) / sizeof(char));

	m_pCUrlHelper = SetupCurlSoapHelper(&m_Proxy);
    UpdateCurlAuthMethod(m_pCUrlHelper, m_ConnInfo.AuthMethod);
	return true;
}

void CEWSSessionGsoapImpl::InitForEachRequest() {
	_ews2__RequestServerVersion * requestServerVersion =
			soap_new__ews2__RequestServerVersion(&m_Proxy);
	requestServerVersion->Version =
			ews2__ExchangeVersionType__Exchange2007_USCORESP1;

    ews2__TimeZoneContextType *ews2__TimeZoneContext = NULL;

    if (!m_ConnInfo.TimezoneId.IsEmpty()) {
        ews2__TimeZoneContext =
                soap_new_ews2__TimeZoneContextType(&m_Proxy);
        ews2__TimeZoneContext->TimeZoneDefinition =
                soap_new_ews2__TimeZoneDefinitionType(&m_Proxy);

        m_TimeZoneId = new std::string(m_ConnInfo.TimezoneId.GetData());
        
        ews2__TimeZoneContext->TimeZoneDefinition->Id =
                m_TimeZoneId;

        ::soap_create_local_time = true;
    } else {
        m_TimeZoneId = NULL;
    }
    
	m_Proxy.ntlm_challenge = "";
	m_Proxy.userid = m_AccountInfo.UserName.GetData();
	m_Proxy.passwd = m_AccountInfo.Password.GetData();
	m_Proxy.authrealm = m_AccountInfo.Domain.GetData();
	m_Proxy.soap_header(NULL,
                        NULL,
                        NULL,
                        requestServerVersion,
                        NULL,
                        NULL,
                        ews2__TimeZoneContext);
}

void CEWSSessionGsoapImpl::CleanupForEachRequest() {
	soap_dealloc(&m_Proxy, NULL);

    if (m_TimeZoneId) {
        ::soap_create_local_time = false;
        
        delete m_TimeZoneId;
        m_TimeZoneId = NULL;
    }
    
}

CEWSFolderOperation * CEWSSessionGsoapImpl::CreateFolderOperation() {
	return new CEWSFolderOperationGsoapImpl(this);
}

CEWSItemOperation * CEWSSessionGsoapImpl::CreateItemOperation() {
	return new CEWSItemOperationGsoapImpl(this);
}

CEWSAttachmentOperation * CEWSSessionGsoapImpl::CreateAttachmentOperation() {
	return new CEWSAttachmentOperationGsoapImpl(this);
}

CEWSString CEWSSessionGsoapImpl::GetErrorMsg() {
    std::string err = CurlSaopHelprGetErrorMsg(&m_Proxy);

    CEWSString msg;

    msg.Append(err.c_str());

    return msg;
}

void CEWSSessionGsoapImpl::AutoDiscover(CEWSError * pError) {
	ews_session_params params;
	char * endpoint = NULL;
	char * oab_url = NULL;
	char * err_msg = NULL;
    int auth_method = EWSConnectionInfo::Auth_NTLM;
    
	memset(&params, 0, sizeof(ews_session_params));
	params.domain = m_AccountInfo.Domain.GetData();
	params.user = m_AccountInfo.UserName.GetData();
	params.password = m_AccountInfo.Password.GetData();
    params.email = m_AccountInfo.EmailAddress.GetData();
    
	if (GetProxyInfo()) {
		params.proxy_host = GetProxyInfo()->Host;
		params.proxy_user = GetProxyInfo()->User;
		params.proxy_password = GetProxyInfo()->Password;
		params.proxy_type = (int) GetProxyInfo()->ProxyType;
		params.proxy_port = (long) GetProxyInfo()->Port;
	}

	int retcode = ews_autodiscover(&params,
                                   &endpoint,
                                   &oab_url,
                                   &auth_method,
                                   &err_msg);

	if (retcode == EWS_SUCCESS && endpoint) {
		m_ConnInfo.Endpoint = endpoint;
        m_Proxy.soap_endpoint = m_ConnInfo.Endpoint;
		m_ConnInfo.Endpoint.CopyTo(m_Proxy.endpoint,
                             sizeof(m_Proxy.endpoint) / sizeof(char));
        m_ConnInfo.AuthMethod = (EWSConnectionInfo::EWSAuthMethodEnum)auth_method;

        CleanupCurlSoapHelper(&m_Proxy);
        m_pCUrlHelper = SetupCurlSoapHelper(&m_Proxy);
        UpdateCurlAuthMethod(m_pCUrlHelper, m_ConnInfo.AuthMethod);
	} else if (pError) {
		if (err_msg)
			pError->SetErrorMessage(err_msg);
		pError->SetErrorCode(retcode);
	}

	ews_autodiscover_free(&params, endpoint, oab_url, err_msg);
}

CEWSSubscriptionOperation * CEWSSessionGsoapImpl::CreateSubscriptionOperation() {
	return new CEWSSubscriptionOperationGsoapImpl(this);
}

CEWSContactOperation * CEWSSessionGsoapImpl::CreateContactOperation() {
	return new CEWSContactOperationGsoapImpl(this);
}

CEWSCalendarOperation * CEWSSessionGsoapImpl::CreateCalendarOperation() {
	return new CEWSCalendarOperationGsoapImpl(this);
}

CEWSTaskOperation * CEWSSessionGsoapImpl::CreateTaskOperation() {
	return new CEWSTaskOperationGsoapImpl(this);
}

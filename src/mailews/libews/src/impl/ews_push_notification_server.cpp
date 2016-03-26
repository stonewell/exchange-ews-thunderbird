#include "libews.h"
#include "ewsnNotificationServiceBindingService.h"
#include "ewsnNotificationServiceBinding12Service.h"
#include "ews_push_notification_server.h"

#include <iostream>
#include <sstream>
#include <memory>

#include "urlparser.h"
#include "libews_c_subscribe_callback.h"

#ifndef _WIN32
#define  closesocket close
#endif

namespace ewsn {

static bool get_sock_port(int sock, int & port) {
	struct sockaddr_in name;
	socklen_t namelen = sizeof(name);
	int err = getsockname(sock, (struct sockaddr*) &name, &namelen);

	if (err == -1) {
		return false;
	}

	port = ntohs(name.sin_port);

	return true;
}

static bool get_bind_ip(const char * ews_server, int ews_server_port, char* buffer, size_t buflen) 
{
	bool ret = false;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    int err;
    struct sockaddr_in serv;
    const char * p = NULL;
    struct hostent *server;

    if (sock == -1)
	    return false;
    
    server = gethostbyname(ews_server);
    if (server == NULL) {
        return false;
    }
    
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    memmove((char *)&serv.sin_addr.s_addr,
            (char *)server->h_addr, 
         server->h_length);
    serv.sin_port = htons(ews_server_port);

    err = connect(sock, (const struct sockaddr*) &serv, sizeof(serv));

    if (err == -1)
	    goto END;
    
    err = getsockname(sock, (struct sockaddr*) &name, &namelen);

    if (err == -1)
	    goto END;
    
    p = inet_ntop(AF_INET, &name.sin_addr, buffer, buflen);

    if (!p)
	    goto END;

    ret = true;
    
END:
    closesocket(sock);

    return ret;
}

typedef ewsn2__NotificationType ews2__NotificationType;
typedef ewsn2__BaseObjectChangedEventType ews2__BaseObjectChangedEventType;
typedef _ewsn2__union_BaseObjectChangedEventType _ews2__union_BaseObjectChangedEventType;
typedef ewsn2__ModifiedEventType ews2__ModifiedEventType;
typedef ewsn2__MovedCopiedEventType ews2__MovedCopiedEventType;
typedef _ewsn2__union_MovedCopiedEventType_ _ews2__union_MovedCopiedEventType_;
typedef __ewsn2__union_NotificationType __ews2__union_NotificationType;
#define SOAP_UNION__ews2__union_NotificationType_CopiedEvent SOAP_UNION__ewsn2__union_NotificationType_CopiedEvent
#define SOAP_UNION__ews2__union_MovedCopiedEventType__OldFolderId SOAP_UNION__ewsn2__union_MovedCopiedEventType__OldFolderId
#define SOAP_UNION__ews2__union_BaseObjectChangedEventType_FolderId SOAP_UNION__ewsn2__union_BaseObjectChangedEventType_FolderId
#define SOAP_UNION__ews2__union_MovedCopiedEventType__OldItemId SOAP_UNION__ewsn2__union_MovedCopiedEventType__OldItemId
#define SOAP_UNION__ews2__union_BaseObjectChangedEventType_ItemId SOAP_UNION__ewsn2__union_BaseObjectChangedEventType_ItemId
#define SOAP_UNION__ews2__union_NotificationType_CreatedEvent SOAP_UNION__ewsn2__union_NotificationType_CreatedEvent
#define SOAP_UNION__ews2__union_BaseObjectChangedEventType_FolderId SOAP_UNION__ewsn2__union_BaseObjectChangedEventType_FolderId
#define SOAP_UNION__ews2__union_BaseObjectChangedEventType_ItemId SOAP_UNION__ewsn2__union_BaseObjectChangedEventType_ItemId
#define SOAP_UNION__ews2__union_NotificationType_DeletedEvent SOAP_UNION__ewsn2__union_NotificationType_DeletedEvent
#define SOAP_UNION__ews2__union_NotificationType_ModifiedEvent SOAP_UNION__ewsn2__union_NotificationType_ModifiedEvent
#define SOAP_UNION__ews2__union_NotificationType_MovedEvent SOAP_UNION__ewsn2__union_NotificationType_MovedEvent
#define SOAP_UNION__ews2__union_NotificationType_NewMailEvent SOAP_UNION__ewsn2__union_NotificationType_NewMailEvent

#include "ews_gsoap_notification_handler.cpp"

class DLL_LOCAL CLocalEwsGsoapNotificationServerImpl : public NotificationServiceBindingService {
public:
	CLocalEwsGsoapNotificationServerImpl(ews::CEWSSession * pSession, ews::CEWSSubscriptionCallback * pCallback,
                                         char * ip,
                                         int port,
                                         bool useHttps)
			: m_pSession(pSession)
			, m_pCallback(pCallback)
			, m_Url("")
			, m_Error()
            , m_Ip(ip)
            , m_Port(port)
            , m_UseHttps(useHttps)
            , m_Running(false){

	}

	virtual ~CLocalEwsGsoapNotificationServerImpl() {

	}

public:
	const ews::CEWSError * GetLastError() const { return &m_Error; }
	const ews::CEWSString & GetURL() const { return m_Url; }

	virtual int SendNotification(ewsn::ewsn__SendNotificationResponseType* response,
	                             ewsn::ewsn__SendNotificationResultType&);

	virtual int run(int port);
    virtual int stop();
	virtual SOAP_SOCKET bind(const char *host, int port, int backlog);
	virtual bool BuildUrl();
private:
	ews::CEWSSession * m_pSession;
    std::auto_ptr<ews::CEWSSubscriptionCallback> m_pCallback;
	ews::CEWSString m_Url;
	ews::CEWSError m_Error;
public:
    char * m_Ip;
    int m_Port;
    bool m_UseHttps;
    bool m_Running;
};
}

using namespace ewsn;

int ewsn::NotificationServiceBindingService::SendNotification(ewsn::ewsn__SendNotificationResponseType*,
                                                              ewsn::ewsn__SendNotificationResultType&) {
	return SOAP_SVR_FAULT;
}

int ewsn::NotificationServiceBinding12Service::SendNotification(ewsn::ewsn__SendNotificationResponseType*,
                                                                ewsn::ewsn__SendNotificationResultType&) {
	return SOAP_SVR_FAULT;
}

CLocalEWSPushNotificationServer::CLocalEWSPushNotificationServer(ews::CEWSSession * pSession,
                                                                 ews::CEWSSubscriptionCallback * pCallback,
                                                                 char * ip,
                                                                 int port,
                                                                 bool useHttps)
	: m_pSession(pSession)
	, m_pCallback(pCallback)
	, m_pServerImpl(new CLocalEwsGsoapNotificationServerImpl(m_pSession,
                                                             m_pCallback,
                                                             ip,
                                                             port,
                                                             useHttps)) {

}

CLocalEWSPushNotificationServer::~CLocalEWSPushNotificationServer() {

}

int CLocalEWSPushNotificationServer::Bind() {
	SOAP_SOCKET s =
			m_pServerImpl->bind(m_pServerImpl->m_Ip, m_pServerImpl->m_Port /*port*/, 100);
	if (!soap_valid_socket(s))
		return EWS_FAIL;

	if (!m_pServerImpl->BuildUrl()) {
		return EWS_FAIL;
	}

	return EWS_SUCCESS;
}

int CLocalEWSPushNotificationServer::Run() {
	int ret = m_pServerImpl->run(0);
	
	if (ret == SOAP_OK)
		return EWS_SUCCESS;
	else {
		return EWS_FAIL;
	}
}

int CLocalEWSPushNotificationServer::Stop() {
    m_pServerImpl->stop();
	return EWS_SUCCESS;
}

const ews::CEWSString & CLocalEWSPushNotificationServer::GetURL() const {
	return m_pServerImpl->GetURL();
}

const ews::CEWSError * CLocalEWSPushNotificationServer::GetLastError() const {
	return m_pServerImpl->GetLastError();
}

int CLocalEwsGsoapNotificationServerImpl::SendNotification(ewsn::ewsn__SendNotificationResponseType* response,
                                                           ewsn::ewsn__SendNotificationResultType&) {
	ewsn::__ewsn__union_ArrayOfResponseMessagesType * __union_ArrayOfResponseMessagesType =
			response->ResponseMessages->__union_ArrayOfResponseMessagesType;
	ewsn::ewsn2__NotificationType * Notification =
			__union_ArrayOfResponseMessagesType->union_ArrayOfResponseMessagesType.SendNotificationResponseMessage->Notification;
	ewsn::ProcessNotificationMessage(Notification,
	                                 m_pCallback.get());
	return SOAP_OK;
}

bool CLocalEwsGsoapNotificationServerImpl::BuildUrl() {
	if (!soap_valid_socket(this->master)) {
		m_Error.SetErrorCode(EWS_FAIL);
		m_Error.SetErrorMessage("Build Url failed, invalid socket.");
		return false;
	}
	
	//build url
	int port = m_Port;
	char bind_ip[255] = {0};
	char port_buf[255] = {0};

    if (!m_Ip) {
        UrlParser parser(m_pSession->GetConnectionInfo()->Endpoint.GetData());
        std::string endpoint_host;
        int endpoint_port = 0;

        endpoint_host = parser.GetHost();

        if (parser.GetPort().empty()) {
            std::string schema = parser.GetSchema();
            UrlParser::strToLower(schema);
            if (!schema.compare(std::string("https"))) {
                endpoint_port = 443;
            } else {
                endpoint_port = 80;
            }
        } else {
            endpoint_port = atoi(parser.GetPort().c_str());
        }
		
        if (!get_sock_port(this->master, port)
            || !get_bind_ip(endpoint_host.c_str(), endpoint_port, bind_ip, 254)) {
			
            m_Error.SetErrorCode(EWS_FAIL);
            m_Error.SetErrorMessage("Build Url failed, unable to get port or ip.");
            return false;
        }
    } else {
        sprintf(bind_ip, "%s", m_Ip);
    }

	sprintf(port_buf, "%d", (port & 0xFFFF));

	m_Url.Clear();

    if (m_UseHttps) {
        m_Url.Append("https://");
    } else {
        m_Url.Append("http://");
    }
	m_Url.Append(bind_ip);
	m_Url.Append(":");
	m_Url.Append(port_buf);

	return true;
}

int CLocalEwsGsoapNotificationServerImpl::stop() {
    m_Running = false;
    return 0;
}

int CLocalEwsGsoapNotificationServerImpl::run(int port) {
    this->accept_timeout = 5;
    m_Running = true;
    if (soap_valid_socket(this->master) || soap_valid_socket(bind(NULL, port, 100))) {
        while(m_Running) {
            SOAP_SOCKET s = accept();

            if (!m_Running) {
                break;
            }
            
            if (!soap_valid_socket(s)) {
                if (this->errnum || !m_Running)
                    break;
                continue;
            }

            if (serve())
                break;
            
			soap_destroy(this);
			soap_end(this);
		}
	}
	
	if (this->error != SOAP_OK && m_Running) {
		m_Error.SetErrorCode(EWS_FAIL);

		std::stringstream os;
		os << "error code:"
		   << this->error
		   << std::endl;
		soap_stream_fault(os);
		m_Error.SetErrorMessage(os.str().c_str());
	}

    m_Running = false;

	return this->error;
}

SOAP_SOCKET CLocalEwsGsoapNotificationServerImpl::bind(const char *host, int port, int backlog) {
	int ret = NotificationServiceBindingService::bind(host, port, backlog);
	
	if (this->error != SOAP_OK) {
		m_Error.SetErrorCode(EWS_FAIL);

		std::stringstream os;
        os << "error code:"
		   << this->error
           << std::endl;
		soap_stream_fault(os);
		m_Error.SetErrorMessage(os.str().c_str());
	}

	return ret;
}

int ews_notification_server_bind(ews_notification_server * server) {
    CLocalEWSPushNotificationServer * s = (CLocalEWSPushNotificationServer*)server;
    return s->Bind();
}

int ews_notification_server_run(ews_notification_server * server) {
    CLocalEWSPushNotificationServer * s = (CLocalEWSPushNotificationServer*)server;
    return s->Run();
}

int ews_notification_server_stop(ews_notification_server * server) {
    CLocalEWSPushNotificationServer * s = (CLocalEWSPushNotificationServer*)server;
    return s->Stop();
}

int ews_free_notification_server(ews_notification_server * server) {
    CLocalEWSPushNotificationServer * s = (CLocalEWSPushNotificationServer*)server;
    delete s;

    return EWS_SUCCESS;
}

int ews_notification_server_get_url(ews_notification_server * server, char ** pp_url) {
    CLocalEWSPushNotificationServer * s = (CLocalEWSPushNotificationServer*)server;

    std::string url = s->GetURL().GetData();

    *pp_url = strdup(url.c_str());

    return EWS_SUCCESS;
}

ews_notification_server * ews_create_notification_server(ews_session * session,
                                                         ews_event_notification_callback * pcallback,
                                                         ews_notification_server_params * params
                                                         ) {
    std::auto_ptr<ews::c::CLocalEWSSubscriptionCallback_C> callback(new ews::c::CLocalEWSSubscriptionCallback_C(pcallback));

    std::auto_ptr<CLocalEWSPushNotificationServer> server(new CLocalEWSPushNotificationServer((ews::CEWSSession *)session,
                                                                                              callback.release(),
                                                                                              params? params->ip : NULL,
                                                                                              params? params->port : 0,
                                                                                              params? (params->use_https ? true : false) : false));

    return (ews_notification_server*)server.release();
}


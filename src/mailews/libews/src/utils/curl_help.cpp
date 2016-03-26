/*
 * curl_help.cpp
 *
 *  Created on: Jul 21, 2014
 *      Author: stone
 */
#include "ewsExchangeServiceBindingProxy.h"
#include "curl_help.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <string>
#include <sstream>

#include "libews.h"
#include "Event.h"

#define RECV_TIMEOUT (30 * 1000)
class HINT {
public:
	CURL * curl;
	struct curl_slist * pHeaders;
	std::string send_data;
	std::string recv_data;
	char err_buf[CURL_ERROR_SIZE + 1];
	CURLcode retCode;
    int auth_method;
	TextProcess::Utils::Impl::CEvent recv_event;
};

static SOAP_SOCKET CURLSocksOpen(struct soap *soap, const char *endpoint,
		const char *host, int port);
static int CURLSocksClose(struct soap *soap);
static int CURLSend(struct soap*, const char*, size_t);
static size_t CURLRecv(struct soap*, char*, size_t);
static int CURLFinalSend(struct soap*);
static size_t callback_curl_read(char *bufptr, size_t size, size_t nitems,
		void *userp);
static size_t callback_curl_write(char *bufptr, size_t size, size_t nitems,
		void *userp);
static int CURLAddHeader(struct soap*, const char*, const char*);

CUrlHelper * SetupCurlSoapHelper(soap * pSoap) {
	curl_global_init(CURL_GLOBAL_ALL);

	HINT * pHint = new HINT();
	pHint->curl = NULL;
	pHint->pHeaders = NULL;
    pHint->auth_method = EWS_AUTH_NTLM;

	pSoap->user = pHint;
	pSoap->fopen = CURLSocksOpen;
	pSoap->fclose = CURLSocksClose;
	pSoap->fsend = CURLSend;
	pSoap->frecv = CURLRecv;
	pSoap->fpreparefinalsend = CURLFinalSend;
	pSoap->fposthdr = CURLAddHeader;
	return pHint;
}

void CleanupCurlSoapHelper(soap * soap) {
	HINT *pHint = reinterpret_cast<HINT*>(soap->user);

	CURLSocksClose(soap);

	if (pHint) {
		if (pHint->pHeaders)
			curl_slist_free_all(pHint->pHeaders);

		delete pHint;
	}
	soap->user = NULL;
	//curl_global_cleanup();
}

static SOAP_SOCKET CURLSocksOpen(struct soap *soap, const char *endpoint,
		const char *host, int port) {
	//get the "hint" structure back
	HINT *pHint = reinterpret_cast<HINT*>(soap->user);

	std::string user;

	if (soap->authrealm && strlen(soap->authrealm) > 0) {
		user = soap->authrealm;
		user.append("\\");
		user.append(soap->userid);
	} else {
		user = soap->userid;
	}

	pHint->err_buf[0] = 0;
	//Allocate a new CURL object, set options. Note we only
	//want CURL to connect the socket for us so we set CURLOPT_CONNECT_ONLY
	//to 1
	CURL* hCurl = curl_easy_init();
	curl_easy_setopt(hCurl, CURLOPT_URL, host);
	curl_easy_setopt(hCurl, CURLOPT_PORT, port);
	curl_easy_setopt(hCurl, CURLOPT_CONNECT_ONLY, 1);
	curl_easy_setopt(hCurl, CURLOPT_USERNAME, user.c_str());
	curl_easy_setopt(hCurl, CURLOPT_PASSWORD, soap->passwd);
#ifdef DEBUG
	curl_easy_setopt(hCurl, CURLOPT_VERBOSE, 1);
#endif
    if (pHint->auth_method == EWS_AUTH_NTLM) {
        curl_easy_setopt(hCurl, CURLOPT_HTTPAUTH, CURLAUTH_NTLM);
    } else {
        curl_easy_setopt(hCurl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    }
	curl_easy_setopt(hCurl, CURLOPT_ERRORBUFFER, pHint->err_buf);
	curl_easy_setopt(hCurl, CURLOPT_SSLVERSION, CURL_SSLVERSION_DEFAULT);
	curl_easy_setopt(hCurl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(hCurl, CURLOPT_SSL_VERIFYHOST, 0L);
	
	//set proxy info
	if (soap->proxy_host && strlen(soap->proxy_host) > 0) {
		curl_easy_setopt(hCurl, CURLOPT_PROXY, soap->proxy_host);
		curl_easy_setopt(hCurl, CURLOPT_PROXYUSERNAME, soap->proxy_userid);
		curl_easy_setopt(hCurl, CURLOPT_PROXYPASSWORD, soap->proxy_passwd);

		if (soap->proxy_port >= 0) {
			curl_easy_setopt(hCurl, CURLOPT_PROXYPORT, soap->proxy_port);
		}

		ews::EWSProxyInfo::EWSProxyTypeEnum proxyType =
				static_cast<ews::EWSProxyInfo::EWSProxyTypeEnum>((long)soap->proxy_from);

		switch(proxyType) {
		case ews::EWSProxyInfo::HTTP:
			curl_easy_setopt(hCurl, CURLOPT_PROXYTYPE,  CURLPROXY_HTTP);
			break;
		case ews::EWSProxyInfo::SOCKS4:
			curl_easy_setopt(hCurl, CURLOPT_PROXYTYPE,  CURLPROXY_SOCKS4);
			break;
		case ews::EWSProxyInfo::SOCKS5:
			curl_easy_setopt(hCurl, CURLOPT_PROXYTYPE,  CURLPROXY_SOCKS5);
			break;
		default:
			break;
		}
	}

	//Perform the connection and get the FD. Clean up if error happens
	//do not do connect here, just provide a fake socket fd
	//curl_easy_perform(hCurl);
	long sockFd = 1000;//SOAP_INVALID_SOCKET;
	//curl_easy_getinfo(hCurl, CURLINFO_LASTSOCKET, &sockFd);
	//failed. cleanup and return
	if (sockFd < 0) {
		soap->sendfd = SOAP_INVALID_SOCKET;
		soap->recvfd = SOAP_INVALID_SOCKET;
		soap->error = SOAP_TCP_ERROR;
		curl_easy_cleanup(hCurl);
		return SOAP_INVALID_SOCKET;
	} else {
		soap->sendfd = sockFd;
		soap->recvfd = sockFd;
		pHint->curl = hCurl;
		return sockFd;
	}
}

static int CURLSocksClose(struct soap *soap) {
	HINT *pHint = reinterpret_cast<HINT*>(soap->user);
	if (pHint && pHint->curl) {
		curl_easy_cleanup(pHint->curl);
		pHint->curl = NULL;
		soap->recvfd = SOAP_INVALID_SOCKET;
		soap->sendfd = SOAP_INVALID_SOCKET;
	}
	return 0;
}

static int CURLSend(struct soap* soap, const char* buf, size_t len) {
	if (soap->mode & SOAP_IO_LENGTH)
		return SOAP_OK;
	HINT *pHint = reinterpret_cast<HINT*>(soap->user);

	if (pHint) {
		pHint->send_data.append(buf, len);
		return SOAP_OK;
	}

	return SOAP_OK;
}

static size_t CURLRecv(struct soap* soap, char* buf, size_t len) {
	if (soap->mode & SOAP_IO_LENGTH)
		return 0;
	HINT *pHint = reinterpret_cast<HINT*>(soap->user);

	if (pHint) {
		pHint->recv_event.Wait(RECV_TIMEOUT);
		if (pHint->recv_data.length() == 0)
			return 0;

		int tmp_len =
				pHint->recv_data.length() < len ?
						pHint->recv_data.length() : len;
		memmove(buf, pHint->recv_data.c_str(), tmp_len);

		pHint->recv_data = pHint->recv_data.substr(tmp_len);

		return tmp_len;
	}

	return 0;
}

static int CURLFinalSend(struct soap* soap) {
	if (soap->mode & SOAP_IO_LENGTH)
		return SOAP_OK;
	HINT *pHint = reinterpret_cast<HINT*>(soap->user);

	if (pHint && pHint->curl) {
		curl_easy_setopt(pHint->curl, CURLOPT_URL, soap->endpoint);
		curl_easy_setopt(pHint->curl, CURLOPT_POST, 1);
		curl_easy_setopt(pHint->curl, CURLOPT_HTTP_VERSION,
				CURL_HTTP_VERSION_1_1);
		curl_easy_setopt(pHint->curl, CURLOPT_POSTFIELDSIZE,
				pHint->send_data.length());
		curl_easy_setopt(pHint->curl, CURLOPT_HEADER, 0);
		curl_easy_setopt(pHint->curl, CURLOPT_CONNECT_ONLY, 0);
		curl_easy_setopt(pHint->curl, CURLOPT_HTTPHEADER, pHint->pHeaders);

		curl_easy_setopt(pHint->curl, CURLOPT_READFUNCTION, callback_curl_read);
		curl_easy_setopt(pHint->curl, CURLOPT_READDATA, pHint);

		curl_easy_setopt(pHint->curl, CURLOPT_WRITEDATA, pHint);
		curl_easy_setopt(pHint->curl, CURLOPT_WRITEFUNCTION,
				callback_curl_write);
		
		pHint->err_buf[0] = 0;
		pHint->recv_event.Reset();
		pHint->retCode = curl_easy_perform(pHint->curl);

		if (pHint->retCode == CURLE_SSL_CONNECT_ERROR) {
			for(int i=0;i<3;i++) {
#ifdef _WIN32
				Sleep(1000);
#else
				sleep(1);
#endif
				pHint->retCode = curl_easy_perform(pHint->curl);
				if (pHint->retCode != CURLE_SSL_CONNECT_ERROR)
					break;
			}
		}
		
		if (pHint->pHeaders) {
			curl_slist_free_all(pHint->pHeaders);
			pHint->pHeaders = NULL;
		}

		long httpStatus = 200;
		curl_easy_getinfo(pHint->curl, CURLINFO_HTTP_CODE, &httpStatus);

		if (httpStatus > 200 && httpStatus < 600) {
			pHint->retCode = CURLE_OK;
			return httpStatus;
		}

		return CURLE_OK == pHint->retCode ? SOAP_OK : 600 + pHint->retCode;
	}
	return SOAP_OK;
}

static size_t callback_curl_read(char *bufptr, size_t size, size_t nitems,
		void *userp) {
	size_t real_size = size * nitems;
	HINT *pHint = reinterpret_cast<HINT*>(userp);

	if (pHint) {
		if (pHint->send_data.length() == 0) {
			pHint->recv_data.clear();
			return 0;
		}

		int tmp_len =
				pHint->send_data.length() < real_size ?
						pHint->send_data.length() : real_size;
		memmove(bufptr, pHint->send_data.c_str(), tmp_len);

		pHint->send_data = pHint->send_data.substr(tmp_len);
		return tmp_len;
	}

	return 0;
}

static size_t callback_curl_write(char *bufptr, size_t size, size_t nitems,
		void *userp) {
	size_t real_size = size * nitems;
	HINT *pHint = reinterpret_cast<HINT*>(userp);

	if (pHint) {
		pHint->recv_event.Set();
		pHint->recv_data.append(bufptr, real_size);
		return real_size;
	}

	return 0;
}

static int CURLAddHeader(struct soap* soap, const char* name, const char* v) {
	//save send data pos
	HINT *pHint = reinterpret_cast<HINT*>(soap->user);

	if (pHint && name && v) {
		std::string header(name);
		header.append(":").append(v);

		if (header.find(':') == std::string::npos
				|| !strcmp(name, "Authorization")
				|| !strcmp(name, "Content-Length"))
			return SOAP_OK;

		pHint->pHeaders = curl_slist_append(pHint->pHeaders, header.c_str());
	}

	return SOAP_OK;
}

std::string CurlSaopHelprGetErrorMsg(soap * soap) {
	HINT *pHint = reinterpret_cast<HINT*>(soap->user);

	if (pHint && pHint->retCode != CURLE_OK) {
		if (strlen(pHint->err_buf) > 0) {
			return pHint->err_buf;
		}

        return curl_easy_strerror(pHint->retCode);
	}

	std::stringstream ss;
	soap_stream_fault(soap, ss);

	return ss.str();
}

void UpdateCurlAuthMethod(CUrlHelper * pHelper, int auth_method) {
    HINT * pHint = reinterpret_cast<HINT*>(pHelper);

    if (pHint)
        pHint->auth_method = auth_method;
}

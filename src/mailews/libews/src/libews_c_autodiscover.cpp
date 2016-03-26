/*
 * libews_c_autodiscover.cpp
 *
 *  Created on: Aug 29, 2014
 *      Author: stone
 */

#include "libews.h"
#include <memory>
#include <stdlib.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

static const char * autodiscover_url_list[] = {
    "https://${domain}/autodiscover/autodiscover.xml",
    "https://autodiscover.${domain}/autodiscover/autodiscover.xml",
    "http://${domain}/autodiscover/autodiscover.xml",
    "http://autodiscover.${domain}/autodiscover/autodiscover.xml"
};

static const char * request_template = "<?xml version=\"1.0\" encoding=\"utf-8\"?>" \
        "<Autodiscover xmlns=\"http://schemas.microsoft.com/exchange/autodiscover/outlook/requestschema/2006\">" \
        "  <Request> " \
        "    <EMailAddress>${email_address}</EMailAddress> " \
        "    <AcceptableResponseSchema>http://schemas.microsoft.com/exchange/autodiscover/outlook/responseschema/2006a</AcceptableResponseSchema>" \
        "  </Request>" \
        "</Autodiscover>";

static int replace_all(std::string& str,  const std::string& pattern,  const std::string& newpat)
{
    int count = 0;
    const size_t nsize = newpat.size();
    const size_t psize = pattern.size();

    for(size_t pos = str.find(pattern, 0);
        pos != std::string::npos;
        pos = str.find(pattern,pos + nsize))
    {
        str.replace(pos, psize, newpat);
        count++;
    }

    return count;
}

size_t get_response(char * ptr, size_t size, size_t nmemb, void * userdata) {
    size_t len = size * nmemb;

    std::string * value = reinterpret_cast<std::string *>(userdata);

    value->append(ptr, len);

    return len;
}

int get_content(const std::string & response, const std::string & element, char ** content) {
    std::string begin("<");
    begin.append(element);
    begin.append(">");

    std::string end("</");
    end.append(element);
    end.append(">");

    int pos = response.find(begin);

    if (pos == std::string::npos) {
        *content = strdup("");
        return 1;
    }

    pos += begin.length();

    int pos2 = response.find(end, pos);

    if (pos2 == std::string::npos) {
        *content = strdup("");
        return 2;
    }

    *content = strdup(response.substr(pos, pos2 - pos).c_str());

    return 0;
}

// trim from start
static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
static inline std::string & trim(std::string &s) {
    return ltrim(rtrim(s));
}

static std::string get_header_from_data(const std::string & data, const std::string & header_name) {
    std::stringstream ss(data);

    std::string prefix(header_name);
    prefix.push_back(':');
    
    while(true) {
        std::string line;
        std::getline(ss, line);

        if (line.length() == 0)
            break;

        int pos = line.find(prefix);
        if (line.find(prefix) != std::string::npos) {
            std::string v = line.substr(pos + prefix.length());
            return trim(v);
        }
    }

    return "";
}

int ews_autodiscover(ews_session_params * params,
                     char ** pp_endpoint,
                     char ** pp_oab_url,
                     int * auth_method,
                     char ** pp_err_msg) {
	char err_buf[CURL_ERROR_SIZE + 1];

    if (!params || !pp_endpoint || !pp_oab_url)
        return EWS_INVALID_PARAM;
    
	*pp_endpoint = NULL;
	*pp_oab_url = NULL;
	if (pp_err_msg) *pp_err_msg = NULL;

    const char * domain = params->email;

    //find domain
    while (domain) {
        if (*domain == '@') 
            break;
        domain++;
    }

    if (domain) 
        domain++;
    else {
        if (pp_err_msg)
            *pp_err_msg = strdup("invalid email address.");
        return EWS_INVALID_PARAM;
    }

    std::string request(request_template);
    replace_all(request, "${email_address}", params->email);

    std::string response;
    std::string header;
    std::string err_str;
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: */*");
    headers = curl_slist_append(headers, "Content-Type: text/xml");
    headers = curl_slist_append(headers, "charsets: utf-8");

    //initial the auth method with ntlm, will change we get 401
    *auth_method = EWS_AUTH_NTLM;
    
	curl_global_init(CURL_GLOBAL_ALL);

	CURL* hCurl = curl_easy_init();
	curl_easy_setopt(hCurl, CURLOPT_POST, 1);
	curl_easy_setopt(hCurl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
	curl_easy_setopt(hCurl, CURLOPT_CONNECT_ONLY, 0);
	curl_easy_setopt(hCurl, CURLOPT_USERNAME, params->user);
	curl_easy_setopt(hCurl, CURLOPT_PASSWORD, params->password);
#ifdef DEBUG
	curl_easy_setopt(hCurl, CURLOPT_VERBOSE, 1);
#endif
    curl_easy_setopt(hCurl, CURLOPT_HTTPAUTH, CURLAUTH_NTLM);
	curl_easy_setopt(hCurl, CURLOPT_ERRORBUFFER, err_buf);
	curl_easy_setopt(hCurl, CURLOPT_WRITEFUNCTION, get_response);
	curl_easy_setopt(hCurl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(hCurl, CURLOPT_HEADERDATA, &header);
    curl_easy_setopt(hCurl, CURLOPT_HEADER, 1);
    curl_easy_setopt(hCurl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(hCurl, CURLOPT_SSLVERSION, CURL_SSLVERSION_DEFAULT);
	curl_easy_setopt(hCurl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(hCurl, CURLOPT_SSL_VERIFYHOST, 0L);

	//set proxy info
	if (params->proxy_host && strlen(params->proxy_host) > 0) {
        response.clear();
		curl_easy_setopt(hCurl, CURLOPT_PROXY, params->proxy_host);
		curl_easy_setopt(hCurl, CURLOPT_PROXYUSERNAME, params->proxy_user);
		curl_easy_setopt(hCurl, CURLOPT_PROXYPASSWORD, params->proxy_password);

		if (params->proxy_port >= 0) {
			curl_easy_setopt(hCurl, CURLOPT_PROXYPORT, params->proxy_port);
		}

		ews::EWSProxyInfo::EWSProxyTypeEnum proxyType =
				static_cast<ews::EWSProxyInfo::EWSProxyTypeEnum>((long)params->proxy_type);

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

    //loop all candiate urls
    int count = sizeof(autodiscover_url_list) / sizeof(const char *);
    int result = EWS_FAIL;
    int auth_changed = 0;

    for(int i=0;i<count;i++) {
        std::string host(autodiscover_url_list[i]);
        err_buf[0] = 0;
        auth_changed = 0;

        replace_all(host, "${domain}", domain);

  RETRY:
        response.clear();
        header.clear();
        curl_easy_setopt(hCurl, CURLOPT_URL, host.c_str());
        curl_easy_setopt(hCurl, CURLOPT_POSTFIELDSIZE,
                         request.length());
        curl_easy_setopt(hCurl, CURLOPT_POSTFIELDS,
                         request.c_str());

        int retCode = curl_easy_perform(hCurl);

        if (CURLE_OK != retCode) {
            err_str.append("try host:").append(host).append(" failed with error:");
            err_str.append(err_buf);
            err_str.append("\n");
            continue;
        }

        long httpStatus = 200;
        curl_easy_getinfo(hCurl, CURLINFO_HTTP_CODE, &httpStatus);

        if (httpStatus >300 && httpStatus < 400) {
            //Handle 3xx move
            //Get new location
            host = get_header_from_data(header, "Location");

            if (host.length() >0) {
                goto RETRY;
            }
            goto HTTP_ERROR;
        } else if (httpStatus == 401) {
            if (!auth_changed) {
                //try anther authmethod
                auth_changed = 1;
                if (*auth_method == EWS_AUTH_NTLM) {
                    *auth_method = EWS_AUTH_BASIC;
                    curl_easy_setopt(hCurl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
                } else {
                    *auth_method = EWS_AUTH_NTLM;
                    curl_easy_setopt(hCurl, CURLOPT_HTTPAUTH, CURLAUTH_NTLM);
                }
                    
                goto RETRY;
            }
            goto HTTP_ERROR;
        } else if (httpStatus > 200 && httpStatus < 600) {
      HTTP_ERROR:
            err_str.append("try host:").append(host).append(" failed with http error:");
            char buf[255] = {0};
#ifdef _WIN32
            sprintf_s(buf, "%ld", httpStatus);
#else
            sprintf(buf, "%ld", httpStatus);
#endif
            err_str.append(buf);
            err_str.append("\n");
            continue;
        }

        //get the discovered info
        get_content(response, "EwsUrl", pp_endpoint);
        get_content(response, "OABUrl", pp_oab_url);

        result = EWS_SUCCESS;
        break;
    }

	curl_global_cleanup();
    curl_slist_free_all(headers);

    if (result != EWS_SUCCESS && pp_err_msg && err_str.length() > 0)
        *pp_err_msg = strdup(err_str.c_str());
	return result;
}

int ews_autodiscover_free(ews_session_params * params,
	char * endpoint,
	char * oab_url,
	char * err_msg)
{
	if (endpoint) free(endpoint);
	if (oab_url) free(oab_url);
	if (err_msg) free(err_msg);

	return 0;
}

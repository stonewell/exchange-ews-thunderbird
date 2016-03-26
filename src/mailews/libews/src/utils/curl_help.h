/*
 * curl_help.h
 *
 *  Created on: Jul 21, 2014
 *      Author: stone
 */

#ifndef CURL_HELP_H_
#define CURL_HELP_H_

typedef void CUrlHelper;

CUrlHelper * SetupCurlSoapHelper(soap * soap);
void CleanupCurlSoapHelper(soap * soap);
std::string CurlSaopHelprGetErrorMsg(soap * soap);
void UpdateCurlAuthMethod(CUrlHelper * pHelper, int auth_method);
#endif /* CURL_HELP_H_ */

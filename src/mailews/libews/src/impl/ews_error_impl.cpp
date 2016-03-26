/*
 * ews_error_impl.cpp
 *
 *  Created on: Jun 9, 2014
 *      Author: stone
 */
#include "libews.h"
#include <iostream>

using namespace ews;

CEWSError::CEWSError() :m_ErrorMsg(""), m_ErrorCode(EWS_SUCCESS) {
  m_ErrorMsg.Reserve(1024);
}

CEWSError::CEWSError(const CEWSString & msg) : m_ErrorMsg(msg),m_ErrorCode(EWS_FAIL) {

}

CEWSError::CEWSError(int errorCode, const CEWSString & msg) : m_ErrorMsg(msg), m_ErrorCode(errorCode) {

}

CEWSError::~CEWSError() {

}

const CEWSString & CEWSError::GetErrorMessage() const {
	return m_ErrorMsg;
}

void CEWSError::SetErrorMessage(const CEWSString & msg) {
  m_ErrorMsg = msg;
}

int CEWSError::GetErrorCode() const {
	return m_ErrorCode;
}

void CEWSError::SetErrorCode(int errorCode) {
	m_ErrorCode = errorCode;
}

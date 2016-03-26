#pragma once
#ifndef __MAIL_EWS_ERROR_INFO_H__
#define __MAIL_EWS_ERROR_INFO_H__

#include <string.h>

class MailEwsErrorInfo {
private:
	char * m_Msg;
	int m_ErrorCode;

public:
	MailEwsErrorInfo()
			: m_Msg(NULL)
			, m_ErrorCode(EWS_SUCCESS) {
	}
	
	MailEwsErrorInfo(int err, const char * msg)
			: m_Msg(strdup(msg))
			, m_ErrorCode(err) {
	}
			
	~MailEwsErrorInfo() {
		if (m_Msg) free(m_Msg);
		m_Msg = NULL;
		m_ErrorCode = EWS_SUCCESS;
	}

	const char * GetErrorMessage() const { return m_Msg ? m_Msg : ""; }
	int GetErrorCode() const { return m_ErrorCode; }

	void SetErrorMessage(const char * msg) {
		if (m_Msg) free(m_Msg);
		m_Msg = strdup(msg);
	}

	void SetErrorCode(int errorCode) {
		m_ErrorCode = errorCode;
	}
};

#endif // __MAIL_EWS_ERROR_INFO_H__


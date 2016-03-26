#ifndef __MAIL_EWS_MSG_PROTOCOL_INFO_H__
#define __MAIL_EWS_MSG_PROTOCOL_INFO_H__

#include "nsIMsgProtocolInfo.h"

class MailEwsMsgProtocolInfo: public nsIMsgProtocolInfo {
public:
  NS_DECL_THREADSAFE_ISUPPORTS;
  NS_DECL_NSIMSGPROTOCOLINFO;
  
  MailEwsMsgProtocolInfo();

private:
  virtual ~MailEwsMsgProtocolInfo();

};

#endif //__MAIL_EWS_MSG_PROTOCOL_INFO_H__

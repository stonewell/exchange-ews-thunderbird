#ifndef __MAIL_EWS_ACCOUNT_MANAGER_EXTENSIONS_H__
#define __MAIL_EWS_ACCOUNT_MANAGER_EXTENSIONS_H__

#include "nsIMsgAccountManager.h"

class MailEwsServerExt : public nsIMsgAccountManagerExtension {
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIMSGACCOUNTMANAGEREXTENSION

  MailEwsServerExt();

protected:
  virtual ~MailEwsServerExt();
};

class MailEwsAuthExt : public nsIMsgAccountManagerExtension {
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIMSGACCOUNTMANAGEREXTENSION

  MailEwsAuthExt();

protected:
  virtual ~MailEwsAuthExt();
};
#endif

#pragma once
#ifndef __MAIL_EWS_ITEMS_CALLBACK_H__
#define __MAIL_EWS_ITEMS_CALLBACK_H__

#include "nsStringAPI.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "IMailEwsItemsCallback.h"
#include "nsIMsgFolder.h"
#include <map>
#include <string>

class MailEwsItemsCallback : public IMailEwsItemsCallback
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_IMAILEWSITEMSCALLBACK

  MailEwsItemsCallback();

protected:
  virtual ~MailEwsItemsCallback();

protected:
  /* additional members */
    nsTArray<nsCString> m_ItemIds;
	nsCOMPtr<nsIMsgFolder> m_Folder;
	char * m_RemoteMsg;
	int m_RemoteResult;
};

class MailEwsLoadItemsCallback : public MailEwsItemsCallback {
public:
	NS_DECL_ISUPPORTS_INHERITED
    MailEwsLoadItemsCallback();

	NS_IMETHOD LocalOperation(void *severResponse) override;
	NS_IMETHOD RemoteOperation(void **severResponse) override;
	NS_IMETHOD FreeRemoteResponse(void *severResponse) override;
protected:
    virtual ~MailEwsLoadItemsCallback();
    NS_IMETHOD LocalProcessMsgItem(ews_msg_item * msg_item);
};

class MailEwsDeleteItemsCallback : public MailEwsItemsCallback {
public:
    MailEwsDeleteItemsCallback(bool localOnly);
	NS_DECL_ISUPPORTS_INHERITED

	NS_IMETHOD LocalOperation(void *severResponse) override;
protected:
    virtual ~MailEwsDeleteItemsCallback();

    bool m_LocalOnly;
};

class MailEwsReadItemsCallback : public MailEwsItemsCallback {
public:
    MailEwsReadItemsCallback(bool localOnly);
	NS_DECL_ISUPPORTS_INHERITED

	NS_IMETHOD LocalOperation(void *severResponse) override;

    std::map<std::string, bool> * GetItemReadMap() {
        return &m_ItemReadMap;
    }
protected:
    virtual ~MailEwsReadItemsCallback();

    bool m_LocalOnly;
    std::map<std::string, bool> m_ItemReadMap;
};

#endif // __MAIL_EWS_ITEMS_CALLBACK_H__


#pragma once
#ifndef __MAIL_EWS_ITEM_OP_H__
#define __MAIL_EWS_ITEM_OP_H__

#include "xpcom-config.h"
#include "nsStringAPI.h"
#include "IMailEwsItemOp.h"
#include "nsCOMPtr.h"
#include "nsIMsgOfflineImapOperation.h"

class MailEwsItemOp : public IMailEwsItemOp {
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_IMAILEWSITEMOP

    MailEwsItemOp(nsIMsgOfflineImapOperation * op);
private:
    virtual ~MailEwsItemOp();
    
    nsCOMPtr <nsIMsgOfflineImapOperation> m_OfflineOp;
};

#endif// __MAIL_EWS_ITEM_OP_H__

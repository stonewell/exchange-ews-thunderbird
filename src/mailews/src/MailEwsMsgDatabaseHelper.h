#pragma once
#ifndef __MAIL_EWS_MSG_DATABASE_HELPER_H__
#define __MAIL_EWS_MSG_DATABASE_HELPER_H__

#include "nsIMsgDatabase.h"
#include "nsCOMPtr.h"
#include "IMailEwsItemOp.h"

class MailEwsMsgDatabaseHelper {
public:
	MailEwsMsgDatabaseHelper(nsIMsgDatabase * db);

public:
	virtual ~MailEwsMsgDatabaseHelper();

	NS_IMETHOD CreateItemOp(IMailEwsItemOp ** ppItemOp);
	NS_IMETHOD RemoveItemOp(const nsACString & itemId, int itemOp);
	NS_IMETHOD GetItemOps(nsIArray ** itemOps);

private:
	nsCOMPtr <nsIMsgDatabase> m_db;
};

#endif //__MAIL_EWS_MSG_DATABASE_HELPER_H__

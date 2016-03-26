#include "nsComponentManagerUtils.h"
#include "nsIArray.h"
#include "nsIMutableArray.h"
#include "MailEwsMsgDatabaseHelper.h"
#include "MailEwsItemOp.h"
#include "nsIMsgOfflineImapOperation.h"

MailEwsMsgDatabaseHelper::MailEwsMsgDatabaseHelper(nsIMsgDatabase * db)
	: m_db(db) {
}

MailEwsMsgDatabaseHelper::~MailEwsMsgDatabaseHelper() {
}

NS_IMETHODIMP MailEwsMsgDatabaseHelper::CreateItemOp(IMailEwsItemOp ** ppItemOp) {

	nsMsgKey fakeKey;
	m_db->GetNextFakeOfflineMsgKey(&fakeKey);

	nsCOMPtr <nsIMsgOfflineImapOperation> op;
	nsresult rv = m_db->GetOfflineOpForKey(fakeKey, true, getter_AddRefs(op));
	NS_ENSURE_SUCCESS(rv, rv);
	
	MailEwsItemOp * newItemOp = new MailEwsItemOp(op);

	if (!newItemOp)
		return NS_ERROR_OUT_OF_MEMORY;

	NS_ADDREF(*ppItemOp = newItemOp);
	
	return NS_OK;
}

NS_IMETHODIMP MailEwsMsgDatabaseHelper::RemoveItemOp(const nsACString & itemId, int itemOp) {
	nsTArray<nsMsgKey> offlineOps;
	nsresult rv;

    //Add a fake msg key to avoid crash, don't known why
    offlineOps.AppendElement(1000);

	rv = m_db->ListAllOfflineOpIds(&offlineOps);
	NS_ENSURE_SUCCESS(rv, rv);
      
	nsCOMPtr<nsIMsgOfflineImapOperation> currentOp;
    //start from index 1 to skip the fake key
	for (uint32_t opIndex = 1; opIndex < offlineOps.Length(); opIndex++) {
		rv = m_db->GetOfflineOpForKey(offlineOps[opIndex], false,
		                              getter_AddRefs(currentOp));
	      
		if (NS_SUCCEEDED(rv) && currentOp) {
			nsCOMPtr<IMailEwsItemOp> newItemOp = new MailEwsItemOp(currentOp);
			if (!newItemOp)
				return NS_ERROR_OUT_OF_MEMORY;
			
			nsCString _itemId;
			int _itemOp;
			newItemOp->GetItemId(_itemId);
			newItemOp->GetOpType(&_itemOp);

			if (_itemId.Equals(itemId) && _itemOp == itemOp) {
				m_db->RemoveOfflineOp(currentOp);
				break;
			}
		}
	}

	return NS_OK;
}

NS_IMETHODIMP MailEwsMsgDatabaseHelper::GetItemOps(nsIArray ** itemOps) {
	nsTArray<nsMsgKey> offlineOps;
	nsresult rv;

    //Add a fake msg key to avoid crash, don't known why
    offlineOps.AppendElement(1000);

	rv = m_db->ListAllOfflineOpIds(&offlineOps);
	NS_ENSURE_SUCCESS(rv, rv);
      
	nsCOMPtr<nsIMutableArray> foundItemOps(do_CreateInstance(NS_ARRAY_CONTRACTID, &rv));
	
	nsCOMPtr<nsIMsgOfflineImapOperation> currentOp;
    //skip the fake msgKey
	for (uint32_t opIndex = 1; opIndex < offlineOps.Length(); opIndex++) {
		rv = m_db->GetOfflineOpForKey(offlineOps[opIndex], false,
		                              getter_AddRefs(currentOp));
	      
		if (NS_SUCCEEDED(rv) && currentOp) {
			nsCOMPtr<IMailEwsItemOp> newItemOp(new MailEwsItemOp(currentOp));
			if (!newItemOp)
				return NS_ERROR_OUT_OF_MEMORY;
			
			foundItemOps->AppendElement(newItemOp, false);
		}
	}

    if (itemOps)
        NS_IF_ADDREF(*itemOps = foundItemOps);
    
	return NS_OK;
}

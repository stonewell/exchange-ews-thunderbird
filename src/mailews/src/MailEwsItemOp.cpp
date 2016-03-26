#include "MailEwsItemOp.h"

NS_IMPL_ISUPPORTS(MailEwsItemOp, IMailEwsItemOp)

MailEwsItemOp::MailEwsItemOp(nsIMsgOfflineImapOperation * op)
:m_OfflineOp(op) {
}

MailEwsItemOp::~MailEwsItemOp() {
}

NS_IMETHODIMP MailEwsItemOp::GetItemId(nsACString & itemId) {
	nsCString _itemId;
	
	nsresult rv = m_OfflineOp->GetDestinationFolderURI(getter_Copies(_itemId));
	NS_ENSURE_SUCCESS(rv, rv);

	itemId = _itemId;
	return NS_OK;
}

NS_IMETHODIMP MailEwsItemOp::SetItemId(const nsACString & itemId) {
	return m_OfflineOp->SetDestinationFolderURI(nsCString(itemId).get());
}

NS_IMETHODIMP MailEwsItemOp::GetChangeKey(nsACString & changeKey) {
	nsCString _changeKey;
	nsresult rv = m_OfflineOp->GetSourceFolderURI(getter_Copies(_changeKey));
	NS_ENSURE_SUCCESS(rv, rv);

	changeKey = _changeKey;

	return NS_OK;
}

NS_IMETHODIMP MailEwsItemOp::SetChangeKey(const nsACString & changeKey) {
	return m_OfflineOp->SetSourceFolderURI(nsCString(changeKey).get());
}

NS_IMETHODIMP MailEwsItemOp::GetOpType(int * opType) {
	return m_OfflineOp->GetMsgSize((uint32_t*)opType);
}

NS_IMETHODIMP MailEwsItemOp::SetOpType(int opType) {
	return m_OfflineOp->SetMsgSize((uint32_t)opType);
}

NS_IMETHODIMP MailEwsItemOp::GetRead(bool * read) {
	return m_OfflineOp->GetPlayingBack(read);
}

NS_IMETHODIMP MailEwsItemOp::SetRead(bool read) {
	return m_OfflineOp->SetPlayingBack(read);
}

#pragma once
#ifndef __LIB_EWS_C_SUBSCRIBE_CALLBACK_H__
#define __LIB_EWS_C_SUBSCRIBE_CALLBACK_H__

namespace ews {
namespace c {
class DLL_LOCAL CLocalEWSSubscriptionCallback_C : public CEWSSubscriptionCallback {
public:
	CLocalEWSSubscriptionCallback_C(ews_event_notification_callback * callback)
			:m_pCallback(callback) {
	}
	
	virtual ~CLocalEWSSubscriptionCallback_C() {
	}
	
private:
	ews_event_notification_callback * m_pCallback;

public:
	virtual void NewMail(const CEWSString & parentFolderId,
	                     const CEWSString & itemId) {
		if (m_pCallback->new_mail)
			m_pCallback->new_mail(parentFolderId, itemId, true, m_pCallback->user_data);
	}

	virtual void MoveItem(const CEWSString & oldParentFolderId,
	                      const CEWSString & parentFolderId,
	                      const CEWSString & oldItemId,
	                      const CEWSString & itemId) {
		if (m_pCallback->move)
			m_pCallback->move(oldParentFolderId,
			                  parentFolderId,
			                  oldItemId,
			                  itemId,
			                  true, m_pCallback->user_data);
	}
	
	virtual void ModifyItem(const CEWSString & parentFolderId,
	                        const CEWSString & itemId,
                            int unreadCount) {
		if (m_pCallback->modify)
			m_pCallback->modify(parentFolderId, itemId, unreadCount, true, m_pCallback->user_data);
	}

	virtual void CopyItem(const CEWSString & oldParentFolderId,
	                      const CEWSString & parentFolderId,
	                      const CEWSString & oldItemId,
	                      const CEWSString & itemId) {
		if (m_pCallback->copy)
			m_pCallback->copy(oldParentFolderId,
			                  parentFolderId,
			                  oldItemId,
			                  itemId,
			                  true, m_pCallback->user_data);
	}
	
	virtual void DeleteItem(const CEWSString & parentFolderId,
	                        const CEWSString & itemId) {
		if (m_pCallback->remove)
			m_pCallback->remove(parentFolderId, itemId, true, m_pCallback->user_data);
	}
	
	virtual void CreateItem(const CEWSString & parentFolderId,
	                        const CEWSString & itemId) {
		if (m_pCallback->create)
			m_pCallback->create(parentFolderId, itemId, true, m_pCallback->user_data);
	}		

	virtual void MoveFolder(const CEWSString & oldParentFolderId,
	                        const CEWSString & parentFolderId,
	                        const CEWSString & oldFolderId,
	                        const CEWSString & folderId) {
		if (m_pCallback->move)
			m_pCallback->move(oldParentFolderId,
			                  parentFolderId,
			                  oldFolderId,
			                  folderId,
			                  false, m_pCallback->user_data);
	}
	
	virtual void ModifyFolder(const CEWSString & parentFolderId,
	                          const CEWSString & folderId,
	                          int unreadCount) {
		if (m_pCallback->modify)
			m_pCallback->modify(parentFolderId, folderId, unreadCount, false, m_pCallback->user_data);
	}
	
	virtual void CopyFolder(const CEWSString & oldParentFolderId,
	                        const CEWSString & parentFolderId,
	                        const CEWSString & oldFolderId,
	                        const CEWSString & folderId) {
		if (m_pCallback->copy)
			m_pCallback->copy(oldParentFolderId,
			                  parentFolderId,
			                  oldFolderId,
			                  folderId,
			                  false, m_pCallback->user_data);
	}
	
	virtual void DeleteFolder(const CEWSString & parentFolderId,
	                          const CEWSString & folderId) {
		if (m_pCallback->remove)
			m_pCallback->remove(parentFolderId, folderId, false, m_pCallback->user_data);
	}
	
	virtual void CreateFolder(const CEWSString & parentFolderId,
	                          const CEWSString & folderId) {
		if (m_pCallback->create)
			m_pCallback->create(parentFolderId, folderId, false, m_pCallback->user_data);
	}		
};
}
};

#endif //__LIB_EWS_C_SUBSCRIBE_CALLBACK_H__

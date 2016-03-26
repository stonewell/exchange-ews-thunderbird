/*
 * libews_c_folder.cpp
 *
 *  Created on: Aug 27, 2014
 *      Author: stone
 */

#include "libews.h"
#include <memory>
#include <stdlib.h>
#include <iostream>

using namespace ews;

static ews_folder * cewsfolder_to_ews_folder(const CEWSFolder * pFolder);
static void cewsfolder_to_ews_folder_2(const CEWSFolder * pFolder, ews_folder * folder, bool id_only);
static CEWSFolder * ews_folder_to_cewsfolder(const ews_folder * folder);

#define ON_ERROR_RETURN(x)                                  \
    if ((x)) {                                              \
		if (pp_err_msg)                                     \
			*pp_err_msg = strdup(error.GetErrorMessage());  \
		return error.GetErrorCode();                        \
	}

namespace ews {
namespace c {
class CEWSFolderOperationCallback_C: public CEWSFolderOperationCallback {
public:
	CEWSFolderOperationCallback_C(ews_sync_folder_callback * callback) :
        m_p_callback(callback),
        m_SyncState(""){
		if (m_p_callback->get_sync_state)
			m_SyncState = m_p_callback->get_sync_state(m_p_callback->user_data);
	}

	virtual ~CEWSFolderOperationCallback_C() {

	}

public:
	virtual const CEWSString & GetSyncState() const {
		return m_SyncState;
	}

	virtual void SetSyncState(const CEWSString & syncState) {
		if (m_p_callback->set_sync_state)
			m_p_callback->set_sync_state(syncState, m_p_callback->user_data);
	}

	virtual CEWSFolder::EWSFolderTypeEnum GetFolderType() const {
		if (m_p_callback->get_folder_type) {
			return (CEWSFolder::EWSFolderTypeEnum) m_p_callback->get_folder_type(m_p_callback->user_data);
		}
		return CEWSFolder::Folder_All_Types;
	}

	virtual void NewFolder(const CEWSFolder * pFolder) {
		if (m_p_callback->new_folder) {
			ews_folder * folder(cewsfolder_to_ews_folder(pFolder));

			m_p_callback->new_folder(folder, m_p_callback->user_data);

			ews_free_folder(folder);
		}
	}

	virtual void UpdateFolder(const CEWSFolder * pFolder) {
		if (m_p_callback->update_folder) {
			ews_folder *folder(cewsfolder_to_ews_folder(pFolder));

			m_p_callback->update_folder(folder, m_p_callback->user_data);

			ews_free_folder(folder);
		}
	}

	virtual void DeleteFolder(const CEWSFolder * pFolder) {
		if (m_p_callback->delete_folder) {
			ews_folder *folder(cewsfolder_to_ews_folder(pFolder));

			m_p_callback->delete_folder(folder, m_p_callback->user_data);

			ews_free_folder(folder);
		}
	}
private:
	ews_sync_folder_callback * m_p_callback;
	CEWSString m_SyncState;
};
}
}

/* folder operation */
int ews_sync_folder(ews_session * session, ews_sync_folder_callback * callback,
                    char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !callback)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSFolderOperation> pFolderOp(
        pSession->CreateFolderOperation());
	CEWSError error;

	ews::c::CEWSFolderOperationCallback_C folder_callback(callback);

	ON_ERROR_RETURN(!pFolderOp->SyncFolders(&folder_callback, &error));

	return EWS_SUCCESS;
}

int ews_get_folders_by_id(ews_session * session,
                          const char ** folder_id,
                          int count,
                          ews_folder ** ppfolder,
                          char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !ppfolder)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSFolderOperation> pFolderOp(
        pSession->CreateFolderOperation());
	CEWSError error;
	CEWSStringList folder_id_list;

	for(int i=0;i<count;i++) {
		folder_id_list.push_back(folder_id[i]);
	}

	std::auto_ptr<CEWSFolderList> pFolderList(pFolderOp->GetFolders(folder_id_list,
                                                                    &error));

	ON_ERROR_RETURN(!pFolderList.get());
	CEWSFolderList::iterator it;

	*ppfolder = (ews_folder *)malloc(sizeof(ews_folder) * pFolderList->size());
	memset(*ppfolder, 0, sizeof(ews_folder) * pFolderList->size());

	int i=0;
	for(it=pFolderList->begin();it != pFolderList->end(); it++) {
		CEWSFolder * pFolder = *it;

		if (!pFolder) {
			i++;
			continue;
		}
		
		cewsfolder_to_ews_folder_2(pFolder, &(*ppfolder)[i++], false);
	}
	
	return EWS_SUCCESS;
}

int ews_get_folders_by_distinguished_id_name(ews_session * session,
                                             int *distinguished_id_name,
                                             int count,
                                             ews_folder ** ppfolder,
                                             char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !ppfolder)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSFolderOperation> pFolderOp(
        pSession->CreateFolderOperation());
	CEWSError error;

	std::auto_ptr<CEWSFolderList> pFolderList(pFolderOp->GetFolders(distinguished_id_name,
                                                                    count,
                                                                    &error));

	ON_ERROR_RETURN(!pFolderList.get());
	CEWSFolderList::iterator it;

	*ppfolder = (ews_folder *)malloc(sizeof(ews_folder) * pFolderList->size());
	memset(*ppfolder, 0, sizeof(ews_folder) * pFolderList->size());

	int i=0;
	for(it=pFolderList->begin();it != pFolderList->end(); it++) {
		CEWSFolder * pFolder = *it;
		
		if (!pFolder) {
			i++;
			continue;
		}
		
		cewsfolder_to_ews_folder_2(pFolder, &(*ppfolder)[i++], false);
	}
	
	return EWS_SUCCESS;
}

int ews_create_folder(ews_session * session, ews_folder * folder,
                      const char * parent_folder_id,
                      char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !folder)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSFolderOperation> pFolderOp(
        pSession->CreateFolderOperation());
	CEWSError error;

	std::auto_ptr<CEWSFolder> pParentFolder(CEWSFolder::CreateInstance((CEWSFolder::EWSFolderTypeEnum)folder->folder_type,""));
	pParentFolder->SetFolderId(parent_folder_id);

	std::auto_ptr<CEWSFolder> pFolder(ews_folder_to_cewsfolder(folder));

	ON_ERROR_RETURN(!pFolderOp->Create(pFolder.get(), pParentFolder.get(), &error));

	cewsfolder_to_ews_folder_2(pFolder.get(), folder, true);

	return EWS_SUCCESS;
}

int ews_update_folder(ews_session * session, ews_folder * folder,
                      char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !folder)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSFolderOperation> pFolderOp(
        pSession->CreateFolderOperation());
	CEWSError error;

	std::auto_ptr<CEWSFolder> pFolder(ews_folder_to_cewsfolder(folder));

	ON_ERROR_RETURN(!pFolderOp->Update(pFolder.get(), &error));

	cewsfolder_to_ews_folder_2(pFolder.get(), folder, true);

	return EWS_SUCCESS;
}

int ews_delete_folder(ews_session * session,
                      ews_folder * folder,
                      bool hardDelete,
                      char ** pp_err_msg) {
	CEWSSession * pSession = reinterpret_cast<CEWSSession *>(session);

	if (!session || !pSession || !folder)
		return EWS_INVALID_PARAM;

	std::auto_ptr<CEWSFolderOperation> pFolderOp(
        pSession->CreateFolderOperation());
	CEWSError error;

	std::auto_ptr<CEWSFolder> pFolder(ews_folder_to_cewsfolder(folder));

	ON_ERROR_RETURN(!pFolderOp->Delete(pFolder.get(), hardDelete, &error));

	return EWS_SUCCESS;
}

int ews_delete_folder_by_id(ews_session * session,
                            const char * folder_id,
                            const char * folder_change_key,
                            bool hardDelete,
                            char ** pp_err_msg) {
	ews_folder folder;
	memset(&folder, 0, sizeof(ews_folder));
	folder.change_key = const_cast<char *>(folder_change_key);
	folder.id = const_cast<char *>(folder_id);
	folder.folder_type = EWS_FOLDER_MAIL;

	return ews_delete_folder(session, &folder, hardDelete, pp_err_msg);
}

#define FREE_MEMBER(folder) {                                           \
		if ((folder)->change_key) free((folder)->change_key);           \
		if ((folder)->display_name) free((folder)->display_name);       \
		if ((folder)->id) free((folder)->id);                           \
		if ((folder)->parent_folder_id) free((folder)->parent_folder_id); \
	}

void ews_free_folder(ews_folder * folder) {
	if (!folder) return;

	FREE_MEMBER(folder);
	
	free(folder);
}

void ews_free_folders(ews_folder * folder, int count) {
	int i = 0;
	if (!folder) return;

	for(i=0;i<count;i++) {
		FREE_MEMBER((&folder[i]));
	}
	
	free(folder);
}

static ews_folder * cewsfolder_to_ews_folder(const CEWSFolder * pFolder) {
	std::auto_ptr<ews_folder> folder((ews_folder*)malloc(sizeof(ews_folder)));
	memset(folder.get(), 0, sizeof(ews_folder));

	cewsfolder_to_ews_folder_2(pFolder, folder.get(), false);
	
	return folder.release();
}

static void cewsfolder_to_ews_folder_2(const CEWSFolder * pFolder, ews_folder * folder, bool id_only) {
	if (id_only) {
		if (folder->change_key) free(folder->change_key);
		if (folder->id) free(folder->id);
	} else {
		FREE_MEMBER(folder);
	}
	
	folder->change_key = strdup(pFolder->GetChangeKey());
	folder->id = strdup(pFolder->GetFolderId());

	if (!id_only) {
		folder->display_name = strdup(pFolder->GetDisplayName());
		folder->folder_type = pFolder->GetFolderType();
		folder->total_count = pFolder->GetTotalCount();
		folder->unread_count = pFolder->GetUnreadCount();
		folder->is_visible = pFolder->IsVisible();
		folder->parent_folder_id = strdup(pFolder->GetParentFolderId());
	}
}

static CEWSFolder * ews_folder_to_cewsfolder(const ews_folder * folder) {
	std::auto_ptr<CEWSFolder> pFolder(
        CEWSFolder::CreateInstance(
            (CEWSFolder::EWSFolderTypeEnum)folder->folder_type,
            folder->display_name ? folder->display_name : ""));

	if (folder->change_key)
		pFolder->SetChangeKey(folder->change_key);

	if (folder->id)
		pFolder->SetFolderId(folder->id);

	pFolder->SetTotalCount(folder->total_count);
	pFolder->SetUnreadCount(folder->unread_count);

	return pFolder.release();
}

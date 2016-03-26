#include "nsIMsgFolder.h"
#include "nsMsgBaseCID.h"
#include "nsIMsgIncomingServer.h"
#include "msgCore.h"
#include "nsThreadUtils.h"

#include "libews.h"
#include "IMailEwsMsgIncomingServer.h"
#include "IMailEwsMsgFolder.h"

#include "MailEwsSyncFolderUtils.h"
#include "MailEwsSyncFolderTask.h"
#include "MailEwsErrorInfo.h"

#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsIMsgAccountManager.h"
#include "nsIMsgIdentity.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "nsMsgFolderFlags.h"

#include "MailEwsLog.h"

#define USER_PTR(x) ((SyncFolderCallback*)x)

static const char * get_sync_state(void * user_data);
static void set_sync_state(const char * sync_state, void * user_data);
static int get_folder_type(void * user_data);
static void new_folder(ews_folder * folder, void * user_data);
static void update_folder(ews_folder * folder, void * user_data);
static void delete_folder(ews_folder * folder, void * user_data);

SyncFolderCallback::SyncFolderCallback(nsIMsgIncomingServer * pIncomingServer)
	: m_SyncState("")
	, m_pIncomingServer(pIncomingServer)
	, m_pCallback(new ews_sync_folder_callback)
	, m_Initialized(false)
	, m_FullSync(false) {

	
}

SyncFolderCallback::~SyncFolderCallback() {
}

NS_IMETHODIMP SyncFolderCallback::UpdateRootFolderId() {
	if (!m_RootFolder.get())
		return NS_ERROR_FAILURE;

	nsCOMPtr<IMailEwsMsgFolder> rootFolder(do_QueryInterface(m_RootFolder));
	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer));
	nsCOMPtr<IMailEwsService> ewsService;

	nsresult rv3 = ewsServer->GetService(getter_AddRefs(ewsService));
	NS_ENSURE_SUCCESS(rv3, rv3);
	
	nsCString savedFolderId;
	nsresult rv = rootFolder->GetFolderId(savedFolderId);

	nsCString change_key;
	nsresult rv2 = rootFolder->GetChangeKey(change_key);
	
	if (NS_FAILED(rv) || savedFolderId.Length() == 0
	    || NS_FAILED(rv2)
	    || change_key.Length() == 0) {
		ewsService->UpdateFolderPropertiesWithIdName(EWSDistinguishedFolderIdName_msgfolderroot,
		                                             rootFolder, nullptr);
	}

	return rv;
}

NS_IMETHODIMP SyncFolderCallback::Initialize() {
	if (m_Initialized)
		return NS_OK;
	
	nsresult rv = m_pIncomingServer->GetRootFolder(getter_AddRefs(m_RootFolder));

	NS_ENSURE_SUCCESS(rv, rv);

	//setup sync folder callback
	m_pCallback->get_sync_state = get_sync_state;
	m_pCallback->set_sync_state = set_sync_state;
	m_pCallback->get_folder_type = get_folder_type;
	m_pCallback->new_folder = new_folder;
	m_pCallback->update_folder = update_folder;
	m_pCallback->delete_folder = delete_folder;
	m_pCallback->user_data = this;

	m_Initialized = true;
	
	return NS_OK;
}

void SyncFolderCallback::Reset() {
	m_NewFolderInfos.clear();
	m_UpdateFolderInfos.clear();
	m_DelFolderInfos.clear();
}

const std::string & SyncFolderCallback::GetSyncState() const {
	return m_SyncState;
}

void SyncFolderCallback::SetSyncState(const std::string & syncState) {
	m_SyncState = syncState;
}

int SyncFolderCallback::GetFolderType() const {
	return ews::CEWSFolder::Folder_Mail;
}

void SyncFolderCallback::NewFolder(ews_folder * pFolder) {
	if (pFolder->is_visible) {
		FolderInfo fInfo(pFolder);
		m_NewFolderInfos.push_back(fInfo);
	}
}

void SyncFolderCallback::UpdateFolder(ews_folder * pFolder) {
	FolderInfo fInfo(pFolder);
	m_UpdateFolderInfos.push_back(fInfo);
}

void SyncFolderCallback::DeleteFolder(ews_folder * pFolder) {
	FolderInfo fInfo(pFolder);

	m_DelFolderInfos.push_back(fInfo);
}

void SyncFolderCallback::TaskBegin() {
	m_FullSync = m_SyncState.empty();

	nsCOMPtr<IMailEwsMsgIncomingServer> server(do_QueryInterface(m_pIncomingServer));

	if (server)
		server->SetIsSyncingFolder(true);
}

void SyncFolderCallback::TaskDone() {
    mailews_logger << "sync folders TaskDone call begin"
                   << std::endl;
	UpdateRootFolderId();
	
	if (!m_RootFolder)
		return;
	
	std::map<std::string, std::string> folder_idsmap;
	
	std::vector<FolderInfo>::iterator it;
	for(it=m_NewFolderInfos.begin();it != m_NewFolderInfos.end(); it++) {
		folder_idsmap[(*it).folderId] = (*it).folderId;
		if (!AddSubFolder(&(*it))) {
			mailews_logger << "-----------Unable to add sub folder:"
                           << (*it).folderId
                           << ", parentId:" << (*it).parentFolderId
                           << ", name:" << (*it).displayName
                           << std::endl;
		}
	}

	for(it=m_UpdateFolderInfos.begin();it != m_UpdateFolderInfos.end(); it++) {
		if (!UpdateSubFolder(&(*it))) {
			mailews_logger << "-----------Unable to Update sub folder:"
                           << (*it).folderId
                           << ", parentId:" << (*it).parentFolderId
                           << ", name:" << (*it).displayName
                           << std::endl;
		}
	}

	for(it=m_DelFolderInfos.begin();it != m_DelFolderInfos.end(); it++) {
		folder_idsmap.erase((*it).folderId);
		if (!RemoveSubFolder(&(*it))) {
			mailews_logger << "-----------Unable to Del sub folder:"
                           << (*it).folderId
                           << ", parentId:" << (*it).parentFolderId
                           << ", name:" << (*it).displayName
                           << std::endl;
		}
	}

	if (m_FullSync) {
        mailews_logger << "sync folders TaskDone call full sync"
                       << std::endl;
		UpdateIdentity();
		UpdateSpecialFolder();
		if (GetErrorInfo()->GetErrorCode() == EWS_SUCCESS) {
			VerifyFolders();
		}
        
        mailews_logger << "sync folders TaskDone call full sync done."
                       << std::endl;
	}

	nsCOMPtr<IMailEwsMsgIncomingServer> server(do_QueryInterface(m_pIncomingServer));

    m_pIncomingServer->SetCharValue("folder_sync_state",
                                    nsCString(m_SyncState.c_str()));
    if (server) {
		server->SetIsSyncingFolder(false);

		nsCOMPtr<IMailEwsService> ewsService;

		nsresult rv = server->GetService(getter_AddRefs(ewsService));

		if (NS_SUCCEEDED(rv) && ewsService) {
			nsTArray<nsCString> folderIds;
			std::map<std::string, std::string>::iterator map_it =
					folder_idsmap.begin();
			while(map_it != folder_idsmap.end()) {
				nsCString id(map_it->first.c_str());
				folderIds.AppendElement(id);
				map_it++;
			}
			
            mailews_logger << "sync folders TaskDone do subscription"
                           << std::endl;
            ewsService->SubscribePushNotification(
			    &folderIds,
			    nullptr);
            mailews_logger << "sync folders TaskDone do subscription done"
                           << std::endl;
		}
    }
    
    mailews_logger << "sync folders TaskDone call done."
                   << std::endl;
}

bool SyncFolderCallback::AddSubFolder(FolderInfo * pFolderInfo) {
	nsCOMPtr<nsIMsgFolder> parentFolder;

	mailews_logger << "Add Folder"
                   << pFolderInfo->parentFolderId << ","
                   << pFolderInfo->folderId << ","
                   << pFolderInfo->displayName << std::endl;

	nsCOMPtr<IMailEwsMsgFolder> rootFolder(do_QueryInterface(m_RootFolder));
    nsCString rootFolderId;

    if (NS_SUCCEEDED(rootFolder->GetChildWithFolderId(
	        nsCString(pFolderInfo->parentFolderId.c_str()),
	        true,
	        getter_AddRefs(parentFolder))) && parentFolder) {
    } else if (NS_SUCCEEDED(rootFolder->GetFolderId(rootFolderId)) && rootFolderId.Equals(nsCString(pFolderInfo->parentFolderId.c_str()))) {
        parentFolder = m_RootFolder;
    } else {
        mailews_logger << "Root Folder ID:"
                       << rootFolderId.get()
                       << ", pFolderInfo Id"
                       << pFolderInfo->parentFolderId.c_str()
                       << std::endl;
    }
		
	if (parentFolder) {
		nsCOMPtr<nsIMsgFolder> child;
		nsCString str(pFolderInfo->displayName.c_str());
		nsString ustr;
		nsresult rv = NS_CStringToUTF16(str, NS_CSTRING_ENCODING_UTF8, ustr);
		if (rv != NS_OK) {
			mailews_logger << "invalid folder name:%ld, %s\n"
                           << pFolderInfo->displayName.length()
                           << pFolderInfo->displayName
                           << std::endl;
			ustr = (const char16_t*) pFolderInfo->displayName.c_str();
		}
			
		rv = parentFolder->AddSubfolder(ustr, getter_AddRefs(child));
			
		if (NS_SUCCEEDED(rv) && rv != NS_MSG_FOLDER_EXISTS) {

			nsCOMPtr<IMailEwsMsgFolder> ews_child(do_QueryInterface(child));
			ews_child->SetFolderId(nsCString(pFolderInfo->folderId.c_str()));
			ews_child->SetChangeKey(nsCString(pFolderInfo->changeKey.c_str()));
			ews_child->SetSyncState(nsCString(""));
			parentFolder->NotifyItemAdded(child);
		} else if (rv == NS_MSG_FOLDER_EXISTS) {
			nsCOMPtr<IMailEwsMsgFolder> ews_parent(do_QueryInterface(parentFolder));
            
			if (NS_SUCCEEDED(ews_parent->GetChildWithFolderId(
			        nsCString(pFolderInfo->folderId.c_str()),
			        false,
			        getter_AddRefs(child))) && child) {
				nsCOMPtr<IMailEwsMsgFolder> ews_child(do_QueryInterface(child));
				ews_child->SetFolderId(nsCString(pFolderInfo->folderId.c_str()));
				ews_child->SetChangeKey(nsCString(pFolderInfo->changeKey.c_str()));

				nsCString state;
				if (NS_FAILED(ews_child->GetSyncState(state)) ) {
					ews_child->SetSyncState(nsCString(""));
                    NS_ASSERTION(false, "add sub folder exiting with same folder id, update sync state");
				}

				mailews_logger << "Folder Name:" << str.get()
                               << std::endl;
			} else if (NS_SUCCEEDED(ews_parent->GetChildWithName(
                ustr,
                false,
                getter_AddRefs(child))) && child) {
				nsCOMPtr<IMailEwsMsgFolder> ews_child(do_QueryInterface(child));
				ews_child->SetFolderId(nsCString(pFolderInfo->folderId.c_str()));
				ews_child->SetChangeKey(nsCString(pFolderInfo->changeKey.c_str()));
				nsCString state;
				if (NS_FAILED(ews_child->GetSyncState(state)) ) {
					ews_child->SetSyncState(nsCString(""));
                    NS_ASSERTION(false, "add sub folder exiting with same name, update sync state");
				}
				mailews_logger << "Folder Name:" << str.get()
                               << std::endl;
            }
		} else {
			return false;
		}			
			
		return true;
	}

	mailews_logger << "unable to get parent folder"
                   << std::endl;

	return false;
}

NS_IMETHODIMP SyncFolderCallback::UpdateFolder(nsIMsgFolder * pFolder,
                                               FolderInfo * pFolderInfo) {
	nsresult rv = NS_OK;

	nsCOMPtr<IMailEwsMsgFolder> rootFolder(do_QueryInterface(m_RootFolder));

	nsCOMPtr<IMailEwsMsgFolder> folder(do_QueryInterface(pFolder));

	nsCString savedFolderId;
	rv = folder->GetFolderId(savedFolderId);
	NS_ENSURE_SUCCESS(rv, rv);

	rv = UpdateFolderName(pFolder, pFolderInfo->displayName.c_str());
	NS_ENSURE_SUCCESS(rv, rv);

	//Update folder name will change pFolder
	nsCOMPtr<nsIMsgFolder> pNewFolder;
	if (!NS_SUCCEEDED(rootFolder->GetChildWithFolderId(
	        savedFolderId,
	        true,
	        getter_AddRefs(pNewFolder)))) {
		return NS_ERROR_FAILURE;
	}
	rv = MoveFolder(pNewFolder, pFolderInfo->parentFolderId.c_str());
	NS_ENSURE_SUCCESS(rv, rv);

	return rv;
}

NS_IMETHODIMP SyncFolderCallback::UpdateFolderName(nsIMsgFolder * pFolder,
                                                   const char * name) {
	//Update folder name
	nsCString str(name);
	nsString ustr;
	nsresult rv = NS_CStringToUTF16(str, NS_CSTRING_ENCODING_UTF8, ustr);
	if (NS_FAILED(rv)) {
		mailews_logger << "invalid folder name:"
                       << name
                       << std::endl;
		ustr = (const char16_t*) name;
	}

	nsString oldName;
	rv = pFolder->GetName(oldName);

	NS_ENSURE_SUCCESS(rv, rv);

	bool canRename = false;
		
	if (NS_SUCCEEDED(pFolder->GetCanRename(&canRename))
	    && canRename
	    && !oldName.Equals(ustr)) {

		nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(pFolder, &rv));
		NS_ENSURE_SUCCESS(rv, rv);
		
		rv = ewsFolder->ClientRename(ustr, nullptr);
	}

	return rv;
}

NS_IMETHODIMP SyncFolderCallback::MoveFolder(nsIMsgFolder * pFolder,
                                             const char * newParentFolderId) {
	nsCOMPtr<nsIMsgFolder> parentFolder;

	nsCString strNewParentFolderId(newParentFolderId);
	
	nsresult rv = pFolder->GetParent(getter_AddRefs(parentFolder));
	NS_ENSURE_SUCCESS(rv, rv);

	if (!parentFolder) {
		return NS_OK;
	}

	nsCOMPtr<IMailEwsMsgFolder> rootFolder(do_QueryInterface(m_RootFolder));

	nsCOMPtr<IMailEwsMsgFolder> folder(do_QueryInterface(parentFolder));

	nsCString savedFolderId;
	rv = folder->GetFolderId(savedFolderId);
	NS_ENSURE_SUCCESS(rv, rv);

	if (strNewParentFolderId.Equals(savedFolderId))
		return NS_OK;

	nsCOMPtr<nsIMsgFolder> pNewParent;
	if (!NS_SUCCEEDED(rootFolder->GetChildWithFolderId(
	        nsCString(newParentFolderId),
	        true,
	        getter_AddRefs(pNewParent)))) {
		return NS_ERROR_FAILURE;
	}
	
	nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(pFolder, &rv));
	NS_ENSURE_SUCCESS(rv, rv);

	return ewsFolder->ClientMoveTo(pNewParent, nullptr);
}

bool SyncFolderCallback::UpdateSubFolder(FolderInfo * pFolderInfo) {
	nsCOMPtr<nsIMsgFolder> folder;

	mailews_logger << "Update Folder"
                   << pFolderInfo->parentFolderId << ","
                   << pFolderInfo->folderId << ","
                   << pFolderInfo->displayName << std::endl;
		
	nsCOMPtr<IMailEwsMsgFolder> rootFolder(do_QueryInterface(m_RootFolder));
	
	if (NS_SUCCEEDED(rootFolder->GetChildWithFolderId(
	        nsCString(pFolderInfo->folderId.c_str()),
	        true,
	        getter_AddRefs(folder)))) {

		nsresult rv = UpdateFolder(folder, pFolderInfo);
		
		return NS_SUCCEEDED(rv);
	}

	return false;
}

bool SyncFolderCallback::RemoveSubFolder(FolderInfo * pFolderInfo) {
	nsCOMPtr<nsIMsgFolder> parentFolder;
	nsCOMPtr<nsIMsgFolder> child;

	mailews_logger << "Remove Fodler:"
                   << pFolderInfo->parentFolderId << ","
                   << pFolderInfo->folderId << ","
                   << pFolderInfo->displayName << std::endl;

	nsCOMPtr<IMailEwsMsgFolder> rootFolder(do_QueryInterface(m_RootFolder));
	
	if (NS_SUCCEEDED(rootFolder->GetChildWithFolderId(
	        nsCString(pFolderInfo->folderId.c_str()),
	        true,
	        getter_AddRefs(child)))) {
		nsresult rv = child->GetParent(getter_AddRefs(parentFolder));

		if (NS_FAILED(rv)) {
			return false;
		}
		
		nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(parentFolder, &rv));
		NS_ENSURE_SUCCESS(rv, NS_SUCCEEDED(rv));
		
		rv = ewsFolder->ClientRemoveSubFolder(child);
		NS_ENSURE_SUCCESS(rv, NS_SUCCEEDED(rv));
	}

	return false;
}

void SyncFolderCallback::VerifyLocalFolder(nsIMsgFolder * pMsgFolder,
                                           std::map<std::string, std::string> * pFolderMap) {
	nsCString savedFolderId;
	nsresult rv = NS_OK;
	nsCOMPtr<IMailEwsMsgFolder> ewsFolder1(do_QueryInterface(pMsgFolder, &rv));
	NS_ENSURE_SUCCESS_VOID(rv);
	
	rv = ewsFolder1->GetFolderId(savedFolderId);

	if (NS_SUCCEEDED(rv)) {
		nsCOMPtr<nsIMsgFolder> parentFolder;
		rv = pMsgFolder->GetParent(getter_AddRefs(parentFolder));

		if (NS_SUCCEEDED(rv) && parentFolder) {
			nsCOMPtr<IMailEwsMsgFolder> ewsFolder(do_QueryInterface(parentFolder, &rv));
			NS_ENSURE_SUCCESS_VOID(rv);

			nsCString savedParentFolderId;
			rv = ewsFolder->GetFolderId(savedParentFolderId);

			if (NS_SUCCEEDED(rv)) {
				std::string key(savedFolderId.get());
				key.append(",");
				key.append(savedParentFolderId.get());

				if (pFolderMap->find(key) == pFolderMap->end()) {
                    //Check Archives folder, Archives may not enable on server,but can be enabled on client
                    nsCOMPtr<nsIMsgAccountManager> accountMgr = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
                    NS_ENSURE_SUCCESS_VOID(rv);

                    nsCOMPtr<nsIMsgIdentity> identity;
                    rv = accountMgr->GetFirstIdentityForServer(m_pIncomingServer,
                                                               getter_AddRefs(identity));
                    if (NS_SUCCEEDED(rv) && identity) {
                        bool archiveEnabled;
                        identity->GetArchiveEnabled(&archiveEnabled);
                        if (archiveEnabled) {
                            nsCString folderUri;
                            nsCString uri;
                            pMsgFolder->GetURI(uri);
                            identity->GetArchiveFolder(folderUri);

                            if (uri.Equals(folderUri))
                                return;
                        }
                    }
                    
					mailews_logger << "------------------"
                                   << "remove folder "
                                   << savedFolderId.get()
                                   << " from parent folder "
                                   << savedParentFolderId.get()
                                   << std::endl;
		
					ewsFolder->ClientRemoveSubFolder(pMsgFolder);
					return;
				} else {
                    //check if the folder has the right name
                    nsString name;
                    nsCOMPtr<nsIMsgFolder> child;
                    name.AppendLiteral((pFolderMap->find(key)->second).c_str());
                    
                    rv = ewsFolder->GetChildWithName(name, false, getter_AddRefs(child));
                    if (NS_SUCCEEDED(rv) && child) {
                        nsCString uri;
                        pMsgFolder->GetURI(uri);

                        nsCString uri2;
                        child->GetURI(uri2);
                        
                        if (!uri.Equals(uri2)) {
                            mailews_logger << "------------------"
                                           << "remove folder because name is not match: "
                                           << uri.get()
                                           << ","
                                           << uri2.get()
                                           << ","
                                           << savedFolderId.get()
                                           << " from parent folder "
                                           << savedParentFolderId.get()
                                           << std::endl;
                            ewsFolder->ClientRemoveSubFolder(pMsgFolder);
                            return;
                        }//uri not same
                    }
                }//find folder with key
			}
		}
	}

	//loop all sub folders
	nsCOMPtr<nsISimpleEnumerator> subFolders;
	rv = pMsgFolder->GetSubFolders(getter_AddRefs(subFolders));

	if (NS_FAILED(rv)) {
		return;
	}

	bool hasMore;
	while (NS_SUCCEEDED(subFolders->HasMoreElements(&hasMore)) && hasMore) {
		nsCOMPtr<nsISupports> item;

		rv = subFolders->GetNext(getter_AddRefs(item));

		nsCOMPtr<nsIMsgFolder> msgFolder(do_QueryInterface(item));
		if (!msgFolder)
			continue;

		VerifyLocalFolder(msgFolder,
		                  pFolderMap);
	}
}

void SyncFolderCallback::UpdateSpecialFolder() {
	nsCString folderUri("");
	nsCString existingUri;
	
	CheckSpecialFolder(nullptr,
	                   folderUri,
	                   nsMsgFolderFlags::Trash,
	                   existingUri);

	folderUri = "";
	                   
	CheckSpecialFolder(nullptr,
	                   folderUri,
	                   nsMsgFolderFlags::Queue,
	                   existingUri);
	
	folderUri = "";
	                   
	CheckSpecialFolder(nullptr,
	                   folderUri,
	                   nsMsgFolderFlags::Junk,
	                   existingUri);
}

void SyncFolderCallback::VerifyFolders() {
	std::map<std::string, std::string> folder_list;
	
	std::vector<FolderInfo>::iterator it;
	for(it=m_NewFolderInfos.begin();it != m_NewFolderInfos.end(); it++) {
		std::string key((*it).folderId);
		key.append(",");
		key.append((*it).parentFolderId);
		
		folder_list[key] = (*it).displayName;
	}

	VerifyLocalFolder(m_RootFolder, &folder_list);
}

bool SyncFolderCallback::CheckSpecialFolder(nsIRDFService *rdf,
                                            nsCString &folderUri,
                                            uint32_t folderFlag,
                                            nsCString &existingUri) {
	bool foundExistingFolder = false;
	nsCOMPtr<nsIRDFResource> res;
	nsCOMPtr<nsIMsgFolder> folder;
	nsresult rv = NS_OK;

	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer, &rv));

	if (NS_SUCCEEDED(rv) && ewsServer) {
		rv = ewsServer->GetFolderWithFlags(folderFlag, getter_AddRefs(folder));

		if (NS_SUCCEEDED(rv) && folder) {
			folder->GetURI(existingUri);
			folder->SetFlag(folderFlag);

			nsString folderName;
			folder->GetPrettyName(folderName);
			// this will set the localized name based on the folder flag.
			folder->SetPrettyName(folderName);

			return true;
		}
	}

	if (!folderUri.IsEmpty() &&
	    NS_SUCCEEDED(rdf->GetResource(folderUri, getter_AddRefs(res)))) {
		folder = do_QueryInterface(res, &rv);
		if (NS_SUCCEEDED(rv)) {
			nsCOMPtr<nsIMsgFolder> parent;
			folder->GetParent(getter_AddRefs(parent));
			// if the default folder doesn't really exist, check if the server
			// told us about an existing one.
			if (!parent) {
				nsCOMPtr<nsIMsgFolder> existingFolder;
				m_RootFolder->GetFolderWithFlags(folderFlag, getter_AddRefs(existingFolder));
				if (existingFolder) {
					existingFolder->GetURI(existingUri);
					foundExistingFolder = true;
				}
			}
			if (!foundExistingFolder)
				folder->SetFlag(folderFlag);

			nsString folderName;
			folder->GetPrettyName(folderName);
			// this will set the localized name based on the folder flag.
			folder->SetPrettyName(folderName);
		}
	}
	return foundExistingFolder;
}

NS_IMETHODIMP SyncFolderCallback::UpdateIdentity() {
	nsresult rv = NS_OK;
	
	if (m_RootFolder)
	{
		nsCOMPtr<nsIRDFService> rdf(do_GetService("@mozilla.org/rdf/rdf-service;1",
		                                          &rv));
		NS_ENSURE_SUCCESS(rv, rv);

		nsCOMPtr<nsIMsgAccountManager> accountMgr = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
		NS_ENSURE_SUCCESS(rv, rv);

		nsCOMPtr<nsIMsgIdentity> identity;
		rv = accountMgr->GetFirstIdentityForServer(m_pIncomingServer,
		                                           getter_AddRefs(identity));
		if (NS_SUCCEEDED(rv) && identity)
		{
			nsCString folderUri;
			identity->GetFccFolder(folderUri);
			nsCString existingUri;

			if (CheckSpecialFolder(rdf, folderUri, nsMsgFolderFlags::SentMail,
			                       existingUri))
			{
				identity->SetFccFolder(existingUri);
				identity->SetFccFolderPickerMode(NS_LITERAL_CSTRING("1"));
			}
			identity->GetDraftFolder(folderUri);
			if (CheckSpecialFolder(rdf, folderUri, nsMsgFolderFlags::Drafts,
			                       existingUri))
			{
				identity->SetDraftFolder(existingUri);
				identity->SetDraftsFolderPickerMode(NS_LITERAL_CSTRING("1"));
			}
			bool archiveEnabled;
			identity->GetArchiveEnabled(&archiveEnabled);
			if (archiveEnabled)
			{
				identity->GetArchiveFolder(folderUri);
				if (CheckSpecialFolder(rdf, folderUri, nsMsgFolderFlags::Archive,
				                       existingUri))
				{
					identity->SetArchiveFolder(existingUri);
					identity->SetArchivesFolderPickerMode(NS_LITERAL_CSTRING("1"));
				}
			}
			identity->GetStationeryFolder(folderUri);
			nsCOMPtr<nsIRDFResource> res;
			if (!folderUri.IsEmpty() && NS_SUCCEEDED(rdf->GetResource(folderUri, getter_AddRefs(res))))
			{
				nsCOMPtr<nsIMsgFolder> folder(do_QueryInterface(res, &rv));
				if (NS_SUCCEEDED(rv))
					rv = folder->SetFlag(nsMsgFolderFlags::Templates);
			}
		}
	}

	return NS_OK;
}

static const char * get_sync_state(void * user_data) {
	return USER_PTR(user_data)->GetSyncState().c_str();
}

static void set_sync_state(const char * sync_state, void * user_data) {
	USER_PTR(user_data)->SetSyncState(sync_state);
}

static int get_folder_type(void * user_data) {
	return (int)USER_PTR(user_data)->GetFolderType();
}

static void new_folder(ews_folder * folder, void * user_data){
	USER_PTR(user_data)->NewFolder(folder);
}

static void update_folder(ews_folder * folder, void * user_data) {
	USER_PTR(user_data)->UpdateFolder(folder);
}

static void delete_folder(ews_folder * folder, void * user_data) {
	USER_PTR(user_data)->DeleteFolder(folder);
}

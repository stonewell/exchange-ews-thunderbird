/*
 * ews_folder_impl.cpp
 *
 *  Created on: Apr 2, 2014
 *      Author: stone
 */

#include "libews_gsoap.h"
#include <memory>
#include <sstream>
#include <map>

using namespace ews;

namespace ews {
class DLL_LOCAL CEWSListFolderCallback: public CEWSFolderOperationGsoapCallback {
public:
	CEWSListFolderCallback(CEWSFolder::EWSFolderTypeEnum folderType,
                           const CEWSFolder * pParentFolder)
            : m_FolderType(folderType)
            , m_pParentFolder(pParentFolder)
            , m_FolderList(new CEWSFolderList())
            , m_idToFolderMap() {

	}

	virtual ~CEWSListFolderCallback() {

	}

public:
	virtual const CEWSString & GetSyncState() const {
		static CEWSString empty("");
		return empty;
	}

	virtual void SetSyncState(const CEWSString & syncState) {

	}

	virtual CEWSFolder::EWSFolderTypeEnum GetFolderType() const {
		return m_FolderType;
	}

	virtual void NewFolder(const CEWSFolder * pFolder) {
		//should not go here
	}

	virtual void UpdateFolder(const CEWSFolder * pFolder) {
		//should not go here
	}

	virtual void DeleteFolder(const CEWSFolder * pFolder) {
		//should not go here
	}

	virtual void NewFolder(std::auto_ptr<CEWSFolderGsoapImpl> * pFolder) {
		if (!m_pParentFolder
            || (m_pParentFolder
                && !m_pParentFolder->GetFolderId().CompareTo(
                    (*pFolder)->GetParentFolderId()))) {
			CEWSFolderGsoapImpl * pEWSFolder = pFolder->release();
			m_FolderList->push_back(pEWSFolder);

			//Save folder id for later usage
			if (m_pParentFolder == NULL) {
				//will update parent folder when all folder created
				m_idToFolderMap[pEWSFolder->GetFolderId()] = pEWSFolder;
			} else {
				pEWSFolder->SetParentFolder(
                    const_cast<CEWSFolder *>(m_pParentFolder));
			}
		}
	}

	virtual void UpdateFolder(std::auto_ptr<CEWSFolderGsoapImpl> * pFolder) {
		//for list folder, new and update are same, because SyncState are empty
		NewFolder(pFolder);
	}

	virtual void DeleteFolder(std::auto_ptr<CEWSFolderGsoapImpl> * pFolder) {
	}

	CEWSFolderList * DetachFolderList() {
		UpdateParentFolder();
		return m_FolderList.release();
	}

private:
	void UpdateParentFolder() {
		//update parent folder
		if (m_pParentFolder == NULL) {
			for (CEWSFolderList::iterator it = m_FolderList->begin();
                 it != m_FolderList->end(); it++) {
				CEWSFolderGsoapImpl * pEWSFolder =
						dynamic_cast<CEWSFolderGsoapImpl *>((*it));

				if (pEWSFolder->GetParentFolderId().IsEmpty())
					continue;

				std::map<CEWSString, CEWSFolderGsoapImpl *>::iterator mapIt =
						m_idToFolderMap.find(pEWSFolder->GetParentFolderId());

				if (mapIt != m_idToFolderMap.end()) {
					pEWSFolder->SetParentFolder(mapIt->second);
				} else {
					CEWSString msg("Missing parent folder for:");
					msg.Append(pEWSFolder->GetDisplayName());
					msg.Append(",parent folder id=");
					msg.Append(pEWSFolder->GetParentFolderId());
				}
			}
		}
	}
private:
	CEWSFolder::EWSFolderTypeEnum m_FolderType;
	const CEWSFolder * m_pParentFolder;
	std::auto_ptr<CEWSFolderList> m_FolderList;
	std::map<CEWSString, CEWSFolderGsoapImpl *> m_idToFolderMap;
};

class DLL_LOCAL CEWSDefaultFolderOperationGsoapCallback: public CEWSFolderOperationGsoapCallback {
public:
	CEWSDefaultFolderOperationGsoapCallback(
        CEWSFolderOperationCallback * pCallback) :
			m_pCallback(pCallback) {
	}

	virtual ~CEWSDefaultFolderOperationGsoapCallback() {

	}

public:
	virtual const CEWSString & GetSyncState() const {
		return m_pCallback->GetSyncState();
	}

	virtual void SetSyncState(const CEWSString & syncState) {
		m_pCallback->SetSyncState(syncState);
	}

	virtual CEWSFolder::EWSFolderTypeEnum GetFolderType() const {
		return m_pCallback->GetFolderType();
	}

	virtual void NewFolder(const CEWSFolder * pFolder) {
		m_pCallback->NewFolder(pFolder);
	}

	virtual void UpdateFolder(const CEWSFolder * pFolder) {
		m_pCallback->UpdateFolder(pFolder);
	}

	virtual void DeleteFolder(const CEWSFolder * pFolder) {
		m_pCallback->DeleteFolder(pFolder);
	}

	virtual void NewFolder(std::auto_ptr<CEWSFolderGsoapImpl> * pFolder) {
		NewFolder(pFolder->get());
	}

	virtual void UpdateFolder(std::auto_ptr<CEWSFolderGsoapImpl> * pFolder) {
		UpdateFolder(pFolder->get());
	}

	virtual void DeleteFolder(std::auto_ptr<CEWSFolderGsoapImpl> * pFolder) {
		DeleteFolder(pFolder->get());
	}

private:
	CEWSFolderOperationCallback * m_pCallback;
};
}

CEWSFolderOperationGsoapCallback::CEWSFolderOperationGsoapCallback() {

}

CEWSFolderOperationGsoapCallback::~CEWSFolderOperationGsoapCallback() {

}

static void UpdateBaseFolderProperties(CEWSFolder * pEWSFolder,
                                   ews2__BaseFolderType * pFolder) {
    pEWSFolder->SetFolderId(pFolder->FolderId->Id.c_str());

	if (pFolder->FolderClass) {
		pEWSFolder->SetVisible(!pFolder->FolderClass->compare("IPF.Note"));
	}

	if (pFolder->FolderId->ChangeKey)
        pEWSFolder->SetChangeKey(pFolder->FolderId->ChangeKey->c_str());

	if (pFolder->TotalCount)
		pEWSFolder->SetTotalCount(*pFolder->TotalCount);
}

static void UpdateFolderProperties(CEWSFolder * pEWSFolder,
                                   ews2__FolderType * pFolder) {
    UpdateBaseFolderProperties(pEWSFolder,
                               pFolder);
	if (pFolder->UnreadCount)
		pEWSFolder->SetUnreadCount(*pFolder->UnreadCount);
}

static
CEWSFolderGsoapImpl *
CreateFolderInstance(CEWSFolder::EWSFolderTypeEnum type, ews2__BaseFolderType * pFolder) {
	if (!pFolder) {
		return new CEWSFolderGsoapImpl(type, "");
	}

	std::auto_ptr<CEWSFolderGsoapImpl> pEWSFolder(
        new CEWSFolderGsoapImpl(type, pFolder->DisplayName->c_str()));

	UpdateBaseFolderProperties(pEWSFolder.get(), pFolder);

	if (pFolder->ParentFolderId) {
        pEWSFolder->SetParentFolderId(pFolder->ParentFolderId->Id.c_str());
	}

	return pEWSFolder.release();
}

CEWSFolderGsoapImpl * CEWSFolderGsoapImpl::CreateInstance(
    EWSFolderTypeEnum type, ews2__FolderType * pFolder) {

    CEWSFolderGsoapImpl * pEWSFolder =
            CreateFolderInstance(type, pFolder);

    if (pFolder) {
        UpdateFolderProperties(pEWSFolder, pFolder);
    }

	return pEWSFolder;
}

CEWSFolder * CEWSFolder::CreateInstance(EWSFolderTypeEnum type,
                                        const CEWSString & displayName) {
	CEWSFolderGsoapImpl * pFolder = CEWSFolderGsoapImpl::CreateInstance(type,
                                                                        NULL);

	pFolder->SetDisplayName(displayName);

	return pFolder;
}

CEWSFolderOperationGsoapImpl::CEWSFolderOperationGsoapImpl(
    CEWSSessionGsoapImpl * pSession) :
    m_pSession(pSession) {

}

CEWSFolderOperationGsoapImpl::~CEWSFolderOperationGsoapImpl() {

}

CEWSFolderList * CEWSFolderOperationGsoapImpl::List(
    CEWSFolder::EWSFolderTypeEnum type,
    CEWSFolder::EWSDistinguishedFolderIdNameEnum distinguishedFolderId,
    CEWSError * pError) {
	CEWSListFolderCallback callback(type, NULL);

	__ews2__union_NonEmptyArrayOfBaseFolderIdsType targetFolder;
	targetFolder.__union_NonEmptyArrayOfBaseFolderIdsType =
			SOAP_UNION__ews2__union_NonEmptyArrayOfBaseFolderIdsType_DistinguishedFolderId;

	ews2__DistinguishedFolderIdType DistinguishedFolderIdType;
	DistinguishedFolderIdType.Id = ConvertDistinguishedFolderIdName(distinguishedFolderId);
	targetFolder.union_NonEmptyArrayOfBaseFolderIdsType.DistinguishedFolderId =
			&DistinguishedFolderIdType;

	if (!__FindFolder(&callback, &targetFolder, pError)) {
		return NULL;
	}

	return callback.DetachFolderList();
}

CEWSFolderList * CEWSFolderOperationGsoapImpl::List(
    CEWSFolder::EWSFolderTypeEnum type, const CEWSFolder * pParentFolder,
    CEWSError * pError) {

	if (!pParentFolder) {
		return List(type, CEWSFolder::msgfolderroot, pError);
	}

	CEWSListFolderCallback callback(type, NULL);

	__ews2__union_NonEmptyArrayOfBaseFolderIdsType targetFolder;
	targetFolder.__union_NonEmptyArrayOfBaseFolderIdsType =
			SOAP_UNION__ews2__union_NonEmptyArrayOfBaseFolderIdsType_FolderId;

	ews2__FolderIdType FolderId;
	FolderId.Id = pParentFolder->GetFolderId();
	targetFolder.union_NonEmptyArrayOfBaseFolderIdsType.FolderId=
			&FolderId;

	if (!__FindFolder(&callback, &targetFolder, pError)) {
		return NULL;
	}

	return callback.DetachFolderList();
}

bool CEWSFolderOperationGsoapImpl::SyncFolders(
    CEWSFolderOperationCallback * pFolderCallback, CEWSError * pError) {
	CEWSDefaultFolderOperationGsoapCallback defaultCallback(pFolderCallback);

	return __SyncFolders(&defaultCallback, NULL, pError);
}

bool CEWSFolderOperationGsoapImpl::__SyncFolders(
    CEWSFolderOperationGsoapCallback * pFolderCallback,
    ews2__TargetFolderIdType * targetFolder,
    CEWSError * pError) {
	CEWSSessionRequestGuard guard(m_pSession);

	__ews1__SyncFolderHierarchyResponse response;
	ews2__FolderResponseShapeType FolderShape;
	FolderShape.BaseShape = ews2__DefaultShapeNamesType__AllProperties;

	CEWSObjectPool objPool;

	ews1__SyncFolderHierarchyType FolderHierarchyType;
	FolderHierarchyType.FolderShape = &FolderShape;

	if (targetFolder)
		FolderHierarchyType.SyncFolderId = targetFolder;

	if (!pFolderCallback->GetSyncState().IsEmpty())
        FolderHierarchyType.SyncState = objPool.Create
                <std::string>(
                    pFolderCallback->GetSyncState().GetData());

	int ret = m_pSession->GetProxy()->SyncFolderHierarchy(&FolderHierarchyType,
                                                          response);

	if (ret != SOAP_OK) {
		if (pError) {
			pError->SetErrorMessage(m_pSession->GetErrorMsg());
			pError->SetErrorCode(ret);
		}
		return false;
	}

	ews1__ArrayOfResponseMessagesType * messages =
			response.ews1__SyncFolderHierarchyResponse->ResponseMessages;
	for (int i = 0; i < messages->__size_ArrayOfResponseMessagesType; i++) {
		__ews1__union_ArrayOfResponseMessagesType * message =
				messages->__union_ArrayOfResponseMessagesType + i;

		if (message->__union_ArrayOfResponseMessagesType
            !=
            SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_SyncFolderHierarchyResponseMessage) {
			continue;
		}

		ews1__SyncFolderHierarchyResponseMessageType * syncFolderResponseMesage =
				message->union_ArrayOfResponseMessagesType.SyncFolderHierarchyResponseMessage;

		if (syncFolderResponseMesage->ResponseClass
            != ews2__ResponseClassType__Success) {
            SAVE_ERROR(pError, syncFolderResponseMesage);
			return false;
		}

		CEWSString syncState(syncFolderResponseMesage->SyncState->c_str());
		pFolderCallback->SetSyncState(syncState);
		for (int j = 0;
             j
                     < syncFolderResponseMesage->Changes->__size_SyncFolderHierarchyChangesType;
             j++) {
			__ews2__union_SyncFolderHierarchyChangesType * unionFolderChange =
					syncFolderResponseMesage->Changes->__union_SyncFolderHierarchyChangesType
                    + j;

			//Only take create or updated folder, deleted folder don't needed
			ews2__SyncFolderHierarchyCreateOrUpdateType * createOrUpdateFolder =
					NULL;
			bool update = false;

			switch (unionFolderChange->__union_SyncFolderHierarchyChangesType) {
			case SOAP_UNION__ews2__union_SyncFolderHierarchyChangesType_Create:
				createOrUpdateFolder =
						unionFolderChange->union_SyncFolderHierarchyChangesType.Create;
				break;
			case SOAP_UNION__ews2__union_SyncFolderHierarchyChangesType_Update:
				createOrUpdateFolder =
						unionFolderChange->union_SyncFolderHierarchyChangesType.Update;
				update = true;
				break;
			case SOAP_UNION__ews2__union_SyncFolderHierarchyChangesType_Delete: {
				ews2__SyncFolderHierarchyDeleteType * deleteType =
						unionFolderChange->union_SyncFolderHierarchyChangesType.Delete;

				std::auto_ptr<CEWSFolderGsoapImpl> pEWSFolder(
                    CEWSFolderGsoapImpl::CreateInstance(
                        CEWSFolder::Folder_Mail, NULL));
				pEWSFolder->SetFolderId(deleteType->FolderId->Id.c_str());

				pFolderCallback->DeleteFolder(&pEWSFolder);
			}
				continue;
			default:
				continue;
			}

			//Only list the folder match input type
			bool filterFolder = true;
			if (((pFolderCallback->GetFolderType() & CEWSFolder::Folder_Mail)
                 == CEWSFolder::Folder_Mail)
                && createOrUpdateFolder->__union_SyncFolderHierarchyCreateOrUpdateType
                ==
                SOAP_UNION__ews2__union_SyncFolderHierarchyCreateOrUpdateType_Folder) {
				filterFolder = false;
			}

			if (filterFolder)
				continue;

			//Mail Folder
			if (createOrUpdateFolder->__union_SyncFolderHierarchyCreateOrUpdateType
                ==
                SOAP_UNION__ews2__union_SyncFolderHierarchyCreateOrUpdateType_Folder) {
				_ews2__union_SyncFolderHierarchyCreateOrUpdateType * createOrUpdate =
						&createOrUpdateFolder->union_SyncFolderHierarchyCreateOrUpdateType;
				ews2__FolderType * pFolder = createOrUpdate->Folder;

				std::auto_ptr<CEWSFolderGsoapImpl> pEWSFolder(
                    CEWSFolderGsoapImpl::CreateInstance(
                        CEWSFolder::Folder_Mail, pFolder));

				if (update) {
					pFolderCallback->UpdateFolder(&pEWSFolder);
				} else {
					pFolderCallback->NewFolder(&pEWSFolder);
				}
			} //Mail Folder
		} //for all folder change
	} //for all response message

	return true;
}

bool CEWSFolderOperationGsoapImpl::Delete(CEWSFolder * pFolder,
                                          bool hardDelete,
                                          CEWSError * pError) {
	CEWSSessionRequestGuard guard(m_pSession);

	__ews1__DeleteFolderResponse response;
	ews1__DeleteFolderType * pDeleteFolderType =
			soap_new_ews1__DeleteFolderType(m_pSession->GetProxy());

	if (hardDelete) {
		pDeleteFolderType->DeleteType = ews2__DisposalType__HardDelete;
	} else {
		pDeleteFolderType->DeleteType = ews2__DisposalType__MoveToDeletedItems;
	}
	
	_ews2__union_NonEmptyArrayOfBaseFolderIdsType folderId;
	folderId.FolderId = soap_new_ews2__FolderIdType(m_pSession->GetProxy());
	folderId.FolderId->Id = pFolder->GetFolderId();

	std::string changeKey(pFolder->GetChangeKey());

	folderId.FolderId->ChangeKey = &changeKey;

	__ews2__union_NonEmptyArrayOfBaseFolderIdsType * folderIdArray =
			soap_new_req___ews2__union_NonEmptyArrayOfBaseFolderIdsType(
                m_pSession->GetProxy(),
                SOAP_UNION__ews2__union_NonEmptyArrayOfBaseFolderIdsType_FolderId,
                folderId);

	pDeleteFolderType->FolderIds =
			soap_new_req_ews2__NonEmptyArrayOfBaseFolderIdsType(
                m_pSession->GetProxy(), 1, folderIdArray);

	int ret = m_pSession->GetProxy()->DeleteFolder(pDeleteFolderType, response);

	if (ret != SOAP_OK) {
		if (pError) {
			pError->SetErrorMessage(m_pSession->GetErrorMsg());
			pError->SetErrorCode(ret);
		}

		return false;
	}

	do {
		if (1
            != response.ews1__DeleteFolderResponse->ResponseMessages->__size_ArrayOfResponseMessagesType) {
			break;
		}

		__ews1__union_ArrayOfResponseMessagesType * responseMessage =
				response.ews1__DeleteFolderResponse->ResponseMessages->__union_ArrayOfResponseMessagesType;

		if (responseMessage->__union_ArrayOfResponseMessagesType
            != SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_DeleteFolderResponseMessage) {
			break;
		}

		if (responseMessage->union_ArrayOfResponseMessagesType.DeleteFolderResponseMessage->ResponseClass
            != ews2__ResponseClassType__Success) {
            SAVE_ERROR(pError,
                       responseMessage
                       ->union_ArrayOfResponseMessagesType
                       .DeleteFolderResponseMessage);

			return false;
		}

		return true;
	} while (false);

	//INVALID_SERVER_RESPONSE
	if (pError) {
		pError->SetErrorMessage("Invalid Server Response");
		pError->SetErrorCode(EWS_FAIL);
	}

	return false;
}

bool CEWSFolderOperationGsoapImpl::Update(CEWSFolder * pFolder,
                                          CEWSError * pError) {
	CEWSSessionRequestGuard guard(m_pSession);

	__ews1__UpdateFolderResponse response;
	ews1__UpdateFolderType * pUpdateFolderType =
			soap_new_ews1__UpdateFolderType(m_pSession->GetProxy());

	ews2__NonEmptyArrayOfFolderChangesType arrayOfFolderchanges;

	ews2__FolderChangeType folderChange;
	folderChange.__union_FolderChangeType =
            SOAP_UNION__ews2__union_FolderChangeType_FolderId;

	//set folder Id
	_ews2__union_FolderChangeType * unionFolderchangeType =
			&folderChange.union_FolderChangeType;
	unionFolderchangeType->FolderId = soap_new_ews2__FolderIdType(
        m_pSession->GetProxy());
	unionFolderchangeType->FolderId->Id = pFolder->GetFolderId();

	std::string changeKey(pFolder->GetChangeKey());
	unionFolderchangeType->FolderId->ChangeKey = &changeKey;

	//set update
	ews2__NonEmptyArrayOfFolderChangeDescriptionsType updates;
	updates.__size_NonEmptyArrayOfFolderChangeDescriptionsType = 1;
	__ews2__union_NonEmptyArrayOfFolderChangeDescriptionsType unionUpdates;
	unionUpdates.__union_NonEmptyArrayOfFolderChangeDescriptionsType =
			SOAP_UNION__ews2__union_NonEmptyArrayOfFolderChangeDescriptionsType_SetFolderField;

	ews2__SetFolderFieldType setFolderField;
	setFolderField.__unionPath =
            SOAP_UNION__ews2__union_ChangeDescriptionType_FieldURI;
	_ews2__union_ChangeDescriptionType * unionChangeDesc =
			&setFolderField.__union_ChangeDescriptionType;
	ews2__PathToUnindexedFieldType fieldURI;
	fieldURI.FieldURI = ews2__UnindexedFieldURIType__folder_x003aDisplayName;
	unionChangeDesc->FieldURI = &fieldURI;
	setFolderField.__union_SetFolderFieldType_ =
            SOAP_UNION__ews2__union_SetFolderFieldType__Folder;
	_ews2__union_SetFolderFieldType_ * unionSetFolderField =
			&setFolderField.union_SetFolderFieldType_;
	ews2__FolderType folderType;
	std::string displayName(pFolder->GetDisplayName());
	folderType.DisplayName = &displayName;
	unionSetFolderField->Folder = &folderType;

	unionUpdates.union_NonEmptyArrayOfFolderChangeDescriptionsType.SetFolderField =
			&setFolderField;

	updates.__union_NonEmptyArrayOfFolderChangeDescriptionsType = &unionUpdates;
	folderChange.Updates = &updates;

	arrayOfFolderchanges.FolderChange.push_back(&folderChange);

	pUpdateFolderType->FolderChanges = &arrayOfFolderchanges;

	int ret = m_pSession->GetProxy()->UpdateFolder(pUpdateFolderType, response);

	if (ret != SOAP_OK) {
		if (pError) {
			pError->SetErrorMessage(m_pSession->GetErrorMsg());
			pError->SetErrorCode(ret);
		}

		return false;
	}

	std::stringstream err_msg;

	do {
		if (1
            != response.ews1__UpdateFolderResponse->ResponseMessages->__size_ArrayOfResponseMessagesType) {
			break;
		}

		__ews1__union_ArrayOfResponseMessagesType * responseMessage =
				response.ews1__UpdateFolderResponse->ResponseMessages->__union_ArrayOfResponseMessagesType;

		if (responseMessage->__union_ArrayOfResponseMessagesType
            != SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_UpdateFolderResponseMessage) {
			err_msg << "invalid response type";
			break;
		}

		if (responseMessage->union_ArrayOfResponseMessagesType.UpdateFolderResponseMessage->ResponseClass
            != ews2__ResponseClassType__Success) {
            SAVE_ERROR(pError,
                       responseMessage
                       ->union_ArrayOfResponseMessagesType
                       .GetFolderResponseMessage);

			return false;
		}

		ews2__ArrayOfFoldersType * responseFolders =
				responseMessage->union_ArrayOfResponseMessagesType.UpdateFolderResponseMessage->Folders;

		if (responseFolders->__size_ArrayOfFoldersType != 1) {
			err_msg << "invalid response folder size";
			break;
		}
		__ews2__union_ArrayOfFoldersType * unionArrayOfFolderTypes =
				responseFolders->__union_ArrayOfFoldersType;

		if (unionArrayOfFolderTypes->__union_ArrayOfFoldersType
            != SOAP_UNION__ews2__union_ArrayOfFoldersType_Folder) {
			err_msg << "invalid response folder type";
			break;
		}

		if (pFolder->GetFolderId().CompareTo(
				unionArrayOfFolderTypes->union_ArrayOfFoldersType.Folder->FolderId->Id.c_str())) {
			err_msg << "folder Id changed";
			break;
		}

		UpdateFolderProperties(pFolder,
                               unionArrayOfFolderTypes->union_ArrayOfFoldersType.Folder);

		return true;
	} while (false);

	//INVALID_SERVER_RESPONSE
	if (pError) {
		CEWSString msg("Invalid Server Response");

		if (err_msg.gcount() > 0) {
			msg.Append(":");
			msg.Append(err_msg.str().c_str());
		}
		pError->SetErrorMessage(msg);
		pError->SetErrorCode(EWS_FAIL);
	}

	return false;
}

bool CEWSFolderOperationGsoapImpl::Create(CEWSFolder * pFolder,
                                          const CEWSFolder * pParentFolder, CEWSError * pError) {
	CEWSSessionRequestGuard guard(m_pSession);

	__ews1__CreateFolderResponse response;
	ews1__CreateFolderType * pCreateFolderType =
			soap_new_ews1__CreateFolderType(m_pSession->GetProxy());

	//1.Create ParentFolderId
	ews2__TargetFolderIdType * parentFolderId =
			soap_new_ews2__TargetFolderIdType(m_pSession->GetProxy());

	if (pParentFolder) {
		parentFolderId->__union_TargetFolderIdType =
                SOAP_UNION__ews2__union_TargetFolderIdType_FolderId;
		_ews2__union_TargetFolderIdType * targetFolderIdType =
				&parentFolderId->union_TargetFolderIdType;
		targetFolderIdType->FolderId = soap_new_ews2__FolderIdType(
            m_pSession->GetProxy());

		targetFolderIdType->FolderId->Id = pParentFolder->GetFolderId();
	} else {
		parentFolderId->__union_TargetFolderIdType =
                SOAP_UNION__ews2__union_TargetFolderIdType_DistinguishedFolderId;
		_ews2__union_TargetFolderIdType * targetFolderIdType =
				&parentFolderId->union_TargetFolderIdType;
		targetFolderIdType->DistinguishedFolderId =
				soap_new_ews2__DistinguishedFolderIdType(
                    m_pSession->GetProxy());

		switch (pFolder->GetFolderType()) {
		case CEWSFolder::Folder_Mail:
			targetFolderIdType->DistinguishedFolderId->Id =
					ews2__DistinguishedFolderIdNameType__msgfolderroot;
			break;
		default:
			if (pError) {
				pError->SetErrorMessage("Invalid Message Type");
				pError->SetErrorCode(EWS_FAIL);
			}
			return false;
		}
	}
	pCreateFolderType->ParentFolderId = parentFolderId;

	//2.Create Folder List
	ews2__NonEmptyArrayOfFoldersType * folders =
			soap_new_ews2__NonEmptyArrayOfFoldersType(m_pSession->GetProxy());

	folders->__size_NonEmptyArrayOfFoldersType = 1;
	__ews2__union_NonEmptyArrayOfFoldersType * arrayOfFolderType =
			soap_new___ews2__union_NonEmptyArrayOfFoldersType(
                m_pSession->GetProxy());
	folders->__union_NonEmptyArrayOfFoldersType = arrayOfFolderType;
	arrayOfFolderType->__union_NonEmptyArrayOfFoldersType =
            SOAP_UNION__ews2__union_NonEmptyArrayOfFoldersType_Folder;
	arrayOfFolderType->union_NonEmptyArrayOfFoldersType.Folder =
			soap_new_req_ews2__FolderType(m_pSession->GetProxy());

	std::string displayName(pFolder->GetDisplayName());

	arrayOfFolderType->union_NonEmptyArrayOfFoldersType.Folder->DisplayName =
			&displayName;

	pCreateFolderType->Folders = folders;

	int ret = m_pSession->GetProxy()->CreateFolder(pCreateFolderType, response);

	if (ret != SOAP_OK) {
		if (pError) {
			pError->SetErrorMessage(m_pSession->GetErrorMsg());
			pError->SetErrorCode(ret);
		}

		return false;
	}

	do {
		if (response.ews1__CreateFolderResponse->ResponseMessages->__size_ArrayOfResponseMessagesType
            != 1) {
			break;
		}

		__ews1__union_ArrayOfResponseMessagesType * responseMessage =
				response.ews1__CreateFolderResponse->ResponseMessages->__union_ArrayOfResponseMessagesType;

		if (responseMessage->__union_ArrayOfResponseMessagesType
            != SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_CreateFolderResponseMessage) {
			break;
		}

		if (responseMessage->union_ArrayOfResponseMessagesType.CreateFolderResponseMessage->ResponseClass
            != ews2__ResponseClassType__Success) {
            SAVE_ERROR(pError,
                       responseMessage
                       ->union_ArrayOfResponseMessagesType
                       .GetFolderResponseMessage);

			return false;
		}

		ews2__ArrayOfFoldersType * responseFolders =
				responseMessage->union_ArrayOfResponseMessagesType.CreateFolderResponseMessage->Folders;

		if (responseFolders->__size_ArrayOfFoldersType != 1) {
			break;
		}
		__ews2__union_ArrayOfFoldersType * unionArrayOfFolderTypes =
				responseFolders->__union_ArrayOfFoldersType;

		if (unionArrayOfFolderTypes->__union_ArrayOfFoldersType
            != SOAP_UNION__ews2__union_ArrayOfFoldersType_Folder) {
			break;
		}

		UpdateFolderProperties(pFolder,
                               unionArrayOfFolderTypes->union_ArrayOfFoldersType.Folder);

		return true;

	} while (false);

	//INVALID_SERVER_RESPONSE
	if (pError) {
		pError->SetErrorMessage("Invalid Server Response");
		pError->SetErrorCode(EWS_FAIL);
	}

	return false;
}

CEWSFolder * CEWSFolderOperationGsoapImpl::GetFolder(
    CEWSFolder::EWSDistinguishedFolderIdNameEnum distinguishedFolderId,
    CEWSError * pError) {
	std::auto_ptr<CEWSFolderList> pFolderList(GetFolders((int*)&distinguishedFolderId, 1, pError));

	CEWSFolder * pFolder = *pFolderList->begin();
	pFolderList->erase(pFolderList->begin());

	return pFolder;
}

CEWSFolderList * CEWSFolderOperationGsoapImpl::GetFolders(
    int *distinguishedFolderId,
    int count,
    CEWSError * pError) {
	CEWSObjectPool objPool;
	
	__ews2__union_NonEmptyArrayOfBaseFolderIdsType * union_NonEmptyArrayOfBaseFolderIdsType =
			objPool.CreateArray<__ews2__union_NonEmptyArrayOfBaseFolderIdsType>(count);
	
	for(int i=0;i<count;i++) {
		union_NonEmptyArrayOfBaseFolderIdsType[i].__union_NonEmptyArrayOfBaseFolderIdsType =
				SOAP_UNION__ews2__union_NonEmptyArrayOfBaseFolderIdsType_DistinguishedFolderId;

		ews2__DistinguishedFolderIdType * DistinguishedFolderId =
				objPool.Create<ews2__DistinguishedFolderIdType>();
		DistinguishedFolderId->Id = ConvertDistinguishedFolderIdName((CEWSFolder::EWSDistinguishedFolderIdNameEnum)distinguishedFolderId[i]);
		union_NonEmptyArrayOfBaseFolderIdsType[i].union_NonEmptyArrayOfBaseFolderIdsType.DistinguishedFolderId = DistinguishedFolderId;
	}
	
	return __GetFolder(union_NonEmptyArrayOfBaseFolderIdsType, count, false, pError);
}

CEWSFolder * CEWSFolderOperationGsoapImpl::GetFolder(const CEWSString & folderId,
                                                     CEWSError * pError) {
	CEWSStringList folder_id_list;
	folder_id_list.push_back(folderId);
	
	std::auto_ptr<CEWSFolderList> pFolderList(GetFolders(folder_id_list, pError));

	CEWSFolder * pFolder = *pFolderList->begin();
	pFolderList->erase(pFolderList->begin());

	return pFolder;
}

CEWSFolderList * CEWSFolderOperationGsoapImpl::GetFolders(const CEWSStringList & folderId,
                                                          CEWSError * pError) {
	CEWSObjectPool objPool;
	
	__ews2__union_NonEmptyArrayOfBaseFolderIdsType * union_NonEmptyArrayOfBaseFolderIdsType =
			objPool.CreateArray<__ews2__union_NonEmptyArrayOfBaseFolderIdsType>(folderId.size());
	
	for(int i=0;i<folderId.size();i++) {
		union_NonEmptyArrayOfBaseFolderIdsType[i].__union_NonEmptyArrayOfBaseFolderIdsType =
				SOAP_UNION__ews2__union_NonEmptyArrayOfBaseFolderIdsType_FolderId;

		ews2__FolderIdType * FolderId = objPool.Create<ews2__FolderIdType>();
		FolderId->Id = folderId[i];
		union_NonEmptyArrayOfBaseFolderIdsType[i].union_NonEmptyArrayOfBaseFolderIdsType.FolderId = FolderId;
	}

	return __GetFolder(union_NonEmptyArrayOfBaseFolderIdsType, folderId.size(), false, pError);
}

CEWSFolder * CEWSFolderOperationGsoapImpl::GetFolder(const CEWSFolder * pFolder,
                                                     CEWSError * pError) {
	if (!pFolder) {
		if (pError) {
			pError->SetErrorMessage("Invalid parameter, pFolder is NULL");
			pError->SetErrorCode(EWS_FAIL);
		}
		return NULL;
	}

	return GetFolder(pFolder->GetFolderId(), pError);
}

CEWSFolderList * CEWSFolderOperationGsoapImpl::__GetFolder(__ews2__union_NonEmptyArrayOfBaseFolderIdsType * union_NonEmptyArrayOfBaseFolderIdsType,
                                                           int count,
                                                           bool idOnly, CEWSError * pError) {
	CEWSSessionRequestGuard guard(m_pSession);

	__ews1__GetFolderResponse response;
	ews1__GetFolderType GetFolderType;

	ews2__FolderResponseShapeType FolderShape;
	FolderShape.BaseShape = idOnly ? ews2__DefaultShapeNamesType__IdOnly:
			ews2__DefaultShapeNamesType__AllProperties;
	GetFolderType.FolderShape = &FolderShape;

	ews2__NonEmptyArrayOfBaseFolderIdsType ArrayOfBaseFolderIdsType;
	ArrayOfBaseFolderIdsType.__size_NonEmptyArrayOfBaseFolderIdsType = count;

	ArrayOfBaseFolderIdsType.__union_NonEmptyArrayOfBaseFolderIdsType = union_NonEmptyArrayOfBaseFolderIdsType;

	GetFolderType.FolderIds = &ArrayOfBaseFolderIdsType;

	int ret = m_pSession->GetProxy()->GetFolder(&GetFolderType, response);

	if (ret != SOAP_OK) {
		if (pError) {
			pError->SetErrorMessage(m_pSession->GetErrorMsg());
			pError->SetErrorCode(ret);
		}

		return NULL;
	}

	do {
		if (response.ews1__GetFolderResponse->ResponseMessages->__size_ArrayOfResponseMessagesType
            != count) {
			break;
		}

		std::auto_ptr<CEWSFolderList> pFolderList(new CEWSFolderList());

		for(int i=0;i<count;i++) {
			__ews1__union_ArrayOfResponseMessagesType * responseMessage =
					response.ews1__GetFolderResponse->ResponseMessages->__union_ArrayOfResponseMessagesType + i;

			if (responseMessage->__union_ArrayOfResponseMessagesType
			    != SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_GetFolderResponseMessage) {
				pFolderList->push_back(NULL);
				continue;
			}

			if (responseMessage->union_ArrayOfResponseMessagesType.GetFolderResponseMessage->ResponseClass
			    != ews2__ResponseClassType__Success) {
                SAVE_ERROR(pError,
                           responseMessage
                           ->union_ArrayOfResponseMessagesType
                           .GetFolderResponseMessage);

				pFolderList->push_back(NULL);
				continue;
			}

			ews2__ArrayOfFoldersType * responseFolders =
					responseMessage->union_ArrayOfResponseMessagesType.GetFolderResponseMessage->Folders;

			__ews2__union_ArrayOfFoldersType * unionArrayOfFolderTypes =
					responseFolders->__union_ArrayOfFoldersType;

			switch (unionArrayOfFolderTypes->__union_ArrayOfFoldersType) {
            case SOAP_UNION__ews2__union_ArrayOfFoldersType_Folder: {
                std::auto_ptr<CEWSFolderGsoapImpl> pEWSFolder(
                    CEWSFolderGsoapImpl::CreateInstance(CEWSFolder::Folder_Mail,
                                                        unionArrayOfFolderTypes->union_ArrayOfFoldersType.Folder));
                pFolderList->push_back(pEWSFolder.release());
                break;
            }
            case SOAP_UNION__ews2__union_ArrayOfFoldersType_CalendarFolder: {
                std::auto_ptr<CEWSFolderGsoapImpl> pEWSFolder(
                    CreateFolderInstance(CEWSFolder::Folder_Mail,
                                         unionArrayOfFolderTypes->union_ArrayOfFoldersType.CalendarFolder));
                pFolderList->push_back(pEWSFolder.release());
                break;
            }
            case SOAP_UNION__ews2__union_ArrayOfFoldersType_ContactsFolder: {
                std::auto_ptr<CEWSFolderGsoapImpl> pEWSFolder(
                    CreateFolderInstance(CEWSFolder::Folder_Mail,
                                         unionArrayOfFolderTypes->union_ArrayOfFoldersType.ContactsFolder));
                pFolderList->push_back(pEWSFolder.release());
                break;
            }
            case SOAP_UNION__ews2__union_ArrayOfFoldersType_SearchFolder: {
                std::auto_ptr<CEWSFolderGsoapImpl> pEWSFolder(
                    CreateFolderInstance(CEWSFolder::Folder_Mail,
                                         unionArrayOfFolderTypes->union_ArrayOfFoldersType.SearchFolder));
                pFolderList->push_back(pEWSFolder.release());
                break;
            }
            case SOAP_UNION__ews2__union_ArrayOfFoldersType_TasksFolder: {
                std::auto_ptr<CEWSFolderGsoapImpl> pEWSFolder(
                    CreateFolderInstance(CEWSFolder::Folder_Mail,
                                         unionArrayOfFolderTypes->union_ArrayOfFoldersType.TasksFolder));
                pFolderList->push_back(pEWSFolder.release());
                break;
            }
            default:
				pFolderList->push_back(NULL);
				break;
			}
		}
		
		return pFolderList.release();

	} while (false);

	//INVALID_SERVER_RESPONSE
	if (pError) {
		pError->SetErrorMessage("Invalid Server Response");
		pError->SetErrorCode(EWS_FAIL);
	}

	return NULL;
}

bool CEWSFolderOperationGsoapImpl::__FindFolder(
    CEWSFolderOperationGsoapCallback * pFolderCallback,
    __ews2__union_NonEmptyArrayOfBaseFolderIdsType * parentFolder,
    CEWSError * pError) {
	CEWSSessionRequestGuard guard(m_pSession);

	__ews1__FindFolderResponse response;
	ews2__FolderResponseShapeType FolderShape;
	FolderShape.BaseShape = ews2__DefaultShapeNamesType__AllProperties;

	CEWSObjectPool objPool;

	ews1__FindFolderType FindFolderType;
	FindFolderType.FolderShape = &FolderShape;

	ews2__NonEmptyArrayOfBaseFolderIdsType ParentFolderIds;

	FindFolderType.ParentFolderIds = &ParentFolderIds;

	ParentFolderIds.__size_NonEmptyArrayOfBaseFolderIdsType = 1;

	ParentFolderIds.__union_NonEmptyArrayOfBaseFolderIdsType =
			parentFolder;

	int ret = m_pSession->GetProxy()->FindFolder(&FindFolderType,
                                                 response);
	if (ret != SOAP_OK) {
		if (pError) {
			pError->SetErrorMessage(m_pSession->GetErrorMsg());
			pError->SetErrorCode(ret);
		}
		return false;
	}

	ews1__ArrayOfResponseMessagesType * messages =
			response.ews1__FindFolderResponse->ResponseMessages;
	for (int i = 0; i < messages->__size_ArrayOfResponseMessagesType; i++) {
		__ews1__union_ArrayOfResponseMessagesType * message =
				messages->__union_ArrayOfResponseMessagesType + i;

		if (message->__union_ArrayOfResponseMessagesType
            !=
            SOAP_UNION__ews1__union_ArrayOfResponseMessagesType_FindFolderResponseMessage) {
			continue;
		}

		ews1__FindFolderResponseMessageType * syncFolderResponseMesage =
				message->union_ArrayOfResponseMessagesType.FindFolderResponseMessage;

		if (syncFolderResponseMesage->ResponseClass
            != ews2__ResponseClassType__Success) {
            SAVE_ERROR(pError, syncFolderResponseMesage);

			return false;
		}

		for (int j = 0;
             j
                     < syncFolderResponseMesage->RootFolder->Folders->__size_ArrayOfFoldersType;
             j++) {
			__ews2__union_ArrayOfFoldersType * union_ArrayOfFoldersType =
					syncFolderResponseMesage->RootFolder->Folders->__union_ArrayOfFoldersType
                    + j;

			//MailFolder Only
			if (union_ArrayOfFoldersType->__union_ArrayOfFoldersType !=
                SOAP_UNION__ews2__union_ArrayOfFoldersType_Folder)
				continue;

			ews2__FolderType * pFolder =
					union_ArrayOfFoldersType->union_ArrayOfFoldersType.Folder;

			std::auto_ptr<CEWSFolderGsoapImpl> pEWSFolder(
                CEWSFolderGsoapImpl::CreateInstance(CEWSFolder::Folder_Mail,
                                                    pFolder));
			pFolderCallback->NewFolder(&pEWSFolder);
		} //for all folder change
	} //for all response message

	return true;
}

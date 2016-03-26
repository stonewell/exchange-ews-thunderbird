#include "MailEwsMsgFolder.h"
#include "nsArrayEnumerator.h"
#include "nsMsgDBCID.h"
#include "nsServiceManagerUtils.h"
#include "nsMsgIncomingServer.h"
#include "libews.h"

#include "MailEwsIncomingServer.h"
#include "nsIFile.h"
#include "nsIFolderListener.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "nsMsgDBCID.h"
#include "nsMsgFolderFlags.h"
#include "nsISeekableStream.h"
#include "nsThreadUtils.h"
#include "MsgUtils.h"
#include "nsIMsgMailSession.h"
#include "nsMsgKeyArray.h"
#include "nsMsgBaseCID.h"
#include "nsMsgLocalCID.h"
#include "nsIMsgCopyService.h"
#include "nsICopyMsgStreamListener.h"
#include "nsIMsgFolderCacheElement.h"
#include "nsTextFormatter.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "MsgI18N.h"
#include "nsICacheSession.h"
#include "nsIDOMWindow.h"
#include "nsIMsgFilter.h"
#include "nsIMsgFilterService.h"
#include "nsIMsgSearchCustomTerm.h"
#include "nsIMsgSearchTerm.h"
#include "nsIPrompt.h"
#include "nsIPromptService.h"
#include "nsIDocShell.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsUnicharUtils.h"
#include "nsIMessenger.h"
#include "nsIMsgSearchAdapter.h"
#include "nsIProgressEventSink.h"
#include "nsIMsgWindow.h"
#include "nsIMsgFolder.h" // TO include biffState enum. Change to bool later...
#include "nsIMsgAccountManager.h"
#include "nsQuickSort.h"
#include "nsIWebNavigation.h"
#include "nsNetUtil.h"
#include "nsIMsgFolderCompactor.h"
#include "nsMsgMessageFlags.h"
#include "nsIMimeHeaders.h"
#include "nsIMsgMdnGenerator.h"
#include "nsISpamSettings.h"
#include <time.h>
#include "nsIMsgMailNewsUrl.h"
#include "nsEmbedCID.h"
#include "nsIMsgComposeService.h"
#include "nsMsgCompCID.h"
#include "nsICacheEntryDescriptor.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIMsgIdentity.h"
#include "nsIMsgFolderNotificationService.h"
#include "nsNativeCharsetUtils.h"
#include "nsIExternalProtocolService.h"
#include "nsCExternalHandlerService.h"
#include "prprf.h"
#include "nsIMutableArray.h"
#include "nsArrayUtils.h"
#include "nsArrayEnumerator.h"
#include "nsIMsgFilterCustomAction.h"
#include "nsMsgReadStateTxn.h"
#include "nsIStringEnumerator.h"
#include "nsIMsgStatusFeedback.h"
#include "nsAlgorithm.h"
#include "nsMsgLineBuffer.h"
#include <algorithm>
#include "MailEwsLog.h"
#include "MailEwsErrorInfo.h"
#include "MailEwsSyncItemUtils.h"
#include "MailEwsMsgUtils.h"
#include "MailEwsCID.h"
#include "MailEwsMsgDatabaseHelper.h"

#include "nsIMsgParseMailMsgState.h"

static const char kEwsRootURI[] = "ews:/";
static const char kEwsMessageRootURI[] = "ews-message:/";

static NS_DEFINE_CID(kRDFServiceCID, NS_RDFSERVICE_CID);

NS_IMPL_ADDREF_INHERITED(MailEwsMsgFolder, MsgDBFolderBase)
NS_IMPL_RELEASE_INHERITED(MailEwsMsgFolder, MsgDBFolderBase)
NS_IMPL_QUERY_HEAD(MailEwsMsgFolder)
NS_IMPL_QUERY_BODY(IMailEwsMsgFolder)
NS_IMPL_QUERY_TAIL_INHERITING(MsgDBFolderBase)

MailEwsMsgFolder::MailEwsMsgFolder(): m_initialized(false)
        , m_pSyncItemCallback(new SyncItemCallback(this))
        , m_FolderId("")
        , m_ChangeKey("")
        , m_SyncState("")
        , m_FolderIdLoaded(false)
        , m_ChangeKeyLoaded(false)
        , m_SyncStateLoaded(false) {
}

MailEwsMsgFolder::~MailEwsMsgFolder() {
}

void MailEwsMsgFolder::GetIncomingServerType(nsCString& serverType) {
	serverType.AssignLiteral("ews");
}

nsresult MailEwsMsgFolder::CreateChildFromURI(const nsCString &uri,
                                                   nsIMsgFolder **folder) {
	MailEwsMsgFolder * newFolder = new MailEwsMsgFolder;

	if (!newFolder)
		return NS_ERROR_OUT_OF_MEMORY;

	NS_ADDREF(*folder = newFolder);

	newFolder->Init(uri.get());

	return NS_OK;
}

nsresult MailEwsMsgFolder::GetDatabase() {
	nsresult rv = NS_OK;
	if (!mDatabase) {
		nsCOMPtr < nsIMsgDBService > msgDBService = do_GetService(
		    NS_MSGDB_SERVICE_CONTRACTID, &rv);
		NS_ENSURE_SUCCESS(rv, rv);

		// Create the database, blowing it away if it needs to be rebuilt
		rv = msgDBService->OpenFolderDB(this, false, getter_AddRefs(mDatabase));
		if (NS_FAILED(rv))
			rv = msgDBService->CreateNewDB(this, getter_AddRefs(mDatabase));

		NS_ENSURE_SUCCESS(rv, rv);

		// UpdateNewMessages/UpdateSummaryTotals can null mDatabase, so we save a local copy
		nsCOMPtr<nsIMsgDatabase> database(mDatabase);
		UpdateNewMessages();
		if (mAddListener)
			database->AddListener(this);
		UpdateSummaryTotals(true);
		mDatabase = database;
	}
	return rv;
}

NS_IMETHODIMP MailEwsMsgFolder::GetSubFolders(nsISimpleEnumerator **aResult) {
	nsCOMPtr<nsIMsgFolder> rootFolder;
	nsresult rv = NS_OK;

	rv = GetRootFolder(getter_AddRefs(rootFolder));
	NS_ENSURE_SUCCESS(rv, rv);

	bool isServer;
	rv = GetIsServer(&isServer);
	NS_ENSURE_SUCCESS(rv, rv);

	if (!m_initialized) {
		nsCOMPtr<nsIFile> pathFile;
		rv = GetFilePath(getter_AddRefs(pathFile));
		if (NS_FAILED(rv)) return rv;

		// host directory does not need .sbd tacked on
		if (!isServer)
		{
			rv = AddDirectorySeparator(pathFile);
			if(NS_FAILED(rv)) return rv;
		}

		m_initialized = true;      // need to set this here to avoid infinite recursion from CreateSubfolders.
		// we have to treat the root folder specially, because it's name
		// doesn't end with .sbd
		int32_t newFlags = nsMsgFolderFlags::Mail;
		bool isDirectory = false;
		pathFile->IsDirectory(&isDirectory);
		if (isDirectory)
		{
			newFlags |= (nsMsgFolderFlags::Directory | nsMsgFolderFlags::Elided);
			if (!mIsServer)
				SetFlag(newFlags);
			rv = CreateSubFolders(pathFile);
		}
	
        //Load cached folder_id, changed_key, sync_state
        nsCString str;
        if (NS_FAILED(GetFolderId(str))) {
            SetFolderId(nsCString(""));
            mailews_logger << "get sub folder, update folder id"
                           << std::endl;
        }
        if (NS_FAILED(GetChangeKey(str))) {
            SetChangeKey(nsCString(""));
            mailews_logger << "get sub folder, update change key"
                           << std::endl;
        }
        if (NS_FAILED(GetSyncState(str))) {
            SetSyncState(nsCString(""));
            NS_ASSERTION(false, "get sub folder, update sync state");
        }
        
		int32_t count = mSubFolders.Count();
		for (int32_t i = 0; i < count; i++)
			mSubFolders[i]->GetSubFolders(nullptr);

		UpdateSummaryTotals(false);
		if (NS_FAILED(rv)) return rv;

		//Trigger a Sync folder when root folder done.
		if (rootFolder.get() == (nsIMsgFolder*)this) {
			nsCOMPtr<nsIMsgIncomingServer> server;
			rv = GetServer(getter_AddRefs(server));
			NS_ENSURE_SUCCESS(rv, rv);

			nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(server, &rv));
			NS_ENSURE_SUCCESS(rv, rv);

			nsCOMPtr<IMailEwsService> ewsService;
			rv = ewsServer->GetService(getter_AddRefs(ewsService));
			NS_ENSURE_SUCCESS(rv, rv);
			
			rv = ewsService->SyncFolder(nullptr);

			NS_ENSURE_SUCCESS(rv, rv);
		}
	}

	return aResult ?
			NS_NewArrayEnumerator(aResult, mSubFolders) : NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP MailEwsMsgFolder::GetDBFolderInfoAndDB(
    nsIDBFolderInfo * *folderInfo, nsIMsgDatabase * *db) {
	NS_ENSURE_ARG_POINTER(folderInfo);
	NS_ENSURE_ARG_POINTER(db);

	nsresult rv = GetDatabase();
	if (NS_FAILED(rv))
		return rv;

	NS_ADDREF(*db = mDatabase);

	rv = (*db)->GetDBFolderInfo(folderInfo);

	return rv;
}

NS_IMETHODIMP MailEwsMsgFolder::GetNewMessages(nsIMsgWindow * aMsgWindow, nsIUrlListener * aListener) {
    nsCString folder_id;
    GetFolderId(folder_id);

    if (folder_id.IsEmpty()) {
        return NS_OK;
    }
    
	nsCOMPtr<nsIMsgIncomingServer> server;
	nsresult rv = GetServer(getter_AddRefs(server));

    nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(server, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<IMailEwsService> ewsService;
    rv = ewsServer->GetService(getter_AddRefs(ewsService));
    NS_ENSURE_SUCCESS(rv, rv);

    return ewsService->SyncItems(this, nullptr);
}

static bool
nsShouldIgnoreFile(nsString& name)
{
	int32_t len = name.Length();
	if (len > 4 && name.RFind(SUMMARY_SUFFIX, true) == len - 4)
	{
		name.SetLength(len-4); // truncate the string
		return false;
	}
	return true;
}

nsresult MailEwsMsgFolder::CreateSubFolders(nsIFile *path)
{
	nsresult rv = NS_OK;
	nsAutoString currentFolderNameStr;    // online name
	nsAutoString currentFolderDBNameStr;  // possibly munged name
	nsCOMPtr<nsIMsgFolder> child;

	nsCOMPtr<nsIMsgIncomingServer> server;
	rv = GetServer(getter_AddRefs(server));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsISimpleEnumerator> children;
	rv = path->GetDirectoryEntries(getter_AddRefs(children));
	NS_ENSURE_SUCCESS(rv, rv);

	bool more = false;
	if (children)
		children->HasMoreElements(&more);
	nsCOMPtr<nsIFile> dirEntry;

	while (more)
	{
		nsCOMPtr<nsISupports> supports;
		rv = children->GetNext(getter_AddRefs(supports));
		dirEntry = do_QueryInterface(supports);
		if (NS_FAILED(rv) || !dirEntry)
			break;
		rv = children->HasMoreElements(&more);
		if (NS_FAILED(rv)) return rv;

		nsCOMPtr <nsIFile> currentFolderPath = do_QueryInterface(dirEntry);
		currentFolderPath->GetLeafName(currentFolderNameStr);
		if (nsShouldIgnoreFile(currentFolderNameStr))
			continue;

		// OK, here we need to get the online name from the folder cache if we can.
		// If we can, use that to create the sub-folder
		nsCOMPtr <nsIMsgFolderCacheElement> cacheElement;
		nsCOMPtr <nsIFile> curFolder = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
		NS_ENSURE_SUCCESS(rv, rv);
		nsCOMPtr <nsIFile> dbFile = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
		NS_ENSURE_SUCCESS(rv, rv);
		dbFile->InitWithFile(currentFolderPath);
		curFolder->InitWithFile(currentFolderPath);
		// don't strip off the .msf in currentFolderPath.
		currentFolderPath->SetLeafName(currentFolderNameStr);
		currentFolderDBNameStr = currentFolderNameStr;
		nsAutoString utf7LeafName = currentFolderNameStr;

		if (curFolder)
		{
			rv = GetFolderCacheElemFromFile(dbFile, getter_AddRefs(cacheElement));
			if (NS_SUCCEEDED(rv) && cacheElement)
			{
				nsCString onlineFullUtf7Name;

				uint32_t folderFlags;
				rv = cacheElement->GetInt32Property("flags", (int32_t *) &folderFlags);
				if (NS_SUCCEEDED(rv) && folderFlags & nsMsgFolderFlags::Virtual) //ignore virtual folders
					continue;
				rv = cacheElement->GetStringProperty("onlineName", onlineFullUtf7Name);
				if (NS_SUCCEEDED(rv) && !onlineFullUtf7Name.IsEmpty())
				{
                    mailews::CopyMUTF7toUTF16(onlineFullUtf7Name, currentFolderNameStr);

					// take the utf7 full online name, and determine the utf7 leaf name
					CopyASCIItoUTF16(onlineFullUtf7Name, utf7LeafName);
				}
			}
		}

		nsCOMPtr <nsIFile> msfFilePath = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
		NS_ENSURE_SUCCESS(rv, rv);
		msfFilePath->InitWithFile(currentFolderPath);
		if (NS_SUCCEEDED(rv) && msfFilePath)
		{
			// leaf name is the db name w/o .msf (nsShouldIgnoreFile strips it off)
			// so this trims the .msf off the file spec.
			msfFilePath->SetLeafName(currentFolderDBNameStr);
		}
		// use the utf7 name as the uri for the folder.
		AddSubfolderWithPath(utf7LeafName, msfFilePath, getter_AddRefs(child));
		if (child) {
			// use the unicode name as the "pretty" name. Set it so it won't be
			// automatically computed from the URI, which is in utf7 form.
			if (!currentFolderNameStr.IsEmpty())
				child->SetPrettyName(currentFolderNameStr);
			child->SetMsgDatabase(nullptr);
		}
	}
	return rv;
}

nsresult MailEwsMsgFolder::AddSubfolderWithPath(nsAString& name, nsIFile *dbPath,
                                                nsIMsgFolder **child, bool brandNew)
{
	NS_ENSURE_ARG_POINTER(child);
	nsresult rv;
	nsCOMPtr<nsIRDFService> rdf(do_GetService(kRDFServiceCID, &rv));
	NS_ENSURE_SUCCESS(rv, rv);

	nsAutoCString uri(mURI);
	uri.Append('/');

	nsAutoCString escapedName;
	rv = mailews::NS_MsgEscapeEncodeURLPath(name, escapedName);
	NS_ENSURE_SUCCESS(rv, rv);
	uri.Append(escapedName);

	bool isServer;
	rv = GetIsServer(&isServer);
	NS_ENSURE_SUCCESS(rv, rv);

	bool isInbox = isServer && name.LowerCaseEqualsLiteral("inbox");

	//will make sure mSubFolders does not have duplicates because of bogus msf files.
	nsCOMPtr <nsIMsgFolder> msgFolder;
	rv = GetChildWithURI(uri, false/*deep*/, isInbox /*case Insensitive*/, getter_AddRefs(msgFolder));
	if (NS_SUCCEEDED(rv) && msgFolder)
		return NS_MSG_FOLDER_EXISTS;

	nsCOMPtr<nsIRDFResource> res;
	rv = rdf->GetResource(uri, getter_AddRefs(res));
	if (NS_FAILED(rv))
		return rv;

	nsCOMPtr<nsIMsgFolder> folder(do_QueryInterface(res, &rv));
	NS_ENSURE_SUCCESS(rv, rv);

	folder->SetFilePath(dbPath);

	uint32_t flags = 0;
	folder->GetFlags(&flags);

	folder->SetParent(this);
	flags |= nsMsgFolderFlags::Mail;

	uint32_t pFlags;
	GetFlags(&pFlags);
	bool isParentInbox = pFlags & nsMsgFolderFlags::Inbox;

	//Only set these if these are top level children or parent is inbox
	if (isInbox)
		flags |= nsMsgFolderFlags::Inbox;
	else if (isServer || isParentInbox)
	{
		if (name.Equals(NS_LITERAL_STRING("Deleted Items")))
			flags |= nsMsgFolderFlags::Trash;
	}

	folder->SetFlags(flags);

    nsCOMPtr<IMailEwsMsgFolder> ews_child(do_QueryInterface(folder));

    if (ews_child) {
        nsCString folderId;
        ews_child->GetFolderId(folderId);
        mailews_logger << uri.get() << "," << folderId.get() << std::endl;
    } else {
        mailews_logger << "not a ews folder" << std::endl;
    }

	if (folder)
		mSubFolders.AppendObject(folder);
	folder.swap(*child);
	return NS_OK;
}

NS_IMETHODIMP MailEwsMsgFolder::ClientRemoveSubFolder (nsIMsgFolder *which)
{
	nsresult rv;
	nsCOMPtr<nsIMutableArray> folders(do_CreateInstance(NS_ARRAY_CONTRACTID, &rv));
	NS_ENSURE_TRUE(folders, rv);
	nsCOMPtr<nsISupports> folderSupport = do_QueryInterface(which, &rv);
	NS_ENSURE_SUCCESS(rv, rv);
	folders->AppendElement(folderSupport, false);
	rv = MsgDBFolderBase::DeleteSubFolders(folders, nullptr);
	which->Delete();
	return rv;
}

NS_IMETHODIMP MailEwsMsgFolder::ClientMoveTo(nsIMsgFolder * pNewParent, nsIMsgWindow *msgWindow)
{
	nsCOMPtr<nsIFile> oldPathFile;
	nsCOMPtr<nsIAtom> folderRenameAtom;
	nsresult rv = GetFilePath(getter_AddRefs(oldPathFile));
	if (NS_FAILED(rv))
		return rv;
	nsCOMPtr<nsIMsgFolder> parentFolder;
	rv = GetParent(getter_AddRefs(parentFolder));
	if (!parentFolder)
		return NS_ERROR_FAILURE;
	nsCOMPtr<nsISupports> parentSupport = do_QueryInterface(parentFolder);
	nsCOMPtr<nsIFile> oldSummaryFile;
	rv = mailews::GetSummaryFileLocation(oldPathFile, getter_AddRefs(oldSummaryFile));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIFile> dirFile;
	int32_t count = mSubFolders.Count();

	if (count > 0)
		rv = CreateDirectoryForFolder(getter_AddRefs(dirFile));

	nsString aNewName;
	rv = GetName(aNewName);
	if (NS_FAILED(rv))
		return rv;
  
	nsCString savedFolderId;
	rv = this->GetStringProperty("folder_id", savedFolderId);
	NS_ENSURE_SUCCESS(rv, rv);
	
	nsCString change_key;
	rv = this->GetStringProperty("change_key", change_key);
	NS_ENSURE_SUCCESS(rv, rv);
	
	nsAutoString newDiskName(aNewName);
    mailews::NS_MsgHashIfNecessary(newDiskName);

	nsCOMPtr <nsIFile> parentPathFile;
	pNewParent->GetFilePath(getter_AddRefs(parentPathFile));
	NS_ENSURE_SUCCESS(rv,rv);
  
	bool isDirectory = false;
	parentPathFile->IsDirectory(&isDirectory);
	if (!isDirectory)
		AddDirectorySeparator(parentPathFile);

	rv = CheckIfFolderExists(aNewName, pNewParent, msgWindow);
	if (NS_FAILED(rv))
		return rv;

	ForceDBClosed();

	// Save of dir name before appending .msf
	nsAutoString newNameDirStr(newDiskName);

	if (! (mFlags & nsMsgFolderFlags::Virtual)) {
		bool exists = false;
		rv = oldPathFile->Exists(&exists);
		NS_ENSURE_SUCCESS(rv, rv);

		if (exists)
			rv = oldPathFile->CopyTo(parentPathFile, newDiskName);
	}
	if (NS_SUCCEEDED(rv))
	{
		newDiskName.AppendLiteral(SUMMARY_SUFFIX);
		oldSummaryFile->CopyTo(parentPathFile, newDiskName);
	}
	else
	{
		ThrowAlertMsg("folderMoveFailed", msgWindow);
		return rv;
	}

	if (NS_SUCCEEDED(rv) && count > 0)
	{
		// rename "*.sbd" directory
		newNameDirStr.AppendLiteral(".sbd");
		dirFile->CopyTo(parentPathFile, newNameDirStr);
	}

	nsCOMPtr<nsIMsgFolder> newFolder;
  
	rv = pNewParent->AddSubfolder(aNewName, getter_AddRefs(newFolder));
	if (newFolder)
	{
		newFolder->SetPrettyName(EmptyString());
		newFolder->SetPrettyName(aNewName);
		newFolder->SetFlags(mFlags);
		newFolder->SetStringProperty("folder_id", savedFolderId);
		newFolder->SetStringProperty("change_key", change_key);
		
		bool changed = false;
		MatchOrChangeFilterDestination(newFolder, true /*caseInsenstive*/, &changed);
		if (changed)
			AlertFilterChanged(msgWindow);

		if (count > 0)
			newFolder->RenameSubFolders(msgWindow, this);

		if (parentFolder)
		{
			nsCOMPtr<nsIMsgFolderNotificationService> notifier(do_GetService(NS_MSGNOTIFICATIONSERVICE_CONTRACTID));
			if (notifier)
				notifier->NotifyFolderMoveCopyCompleted(true, this, newFolder);
			
			SetParent(nullptr);
			
			parentFolder->PropagateDelete(this, true , msgWindow);
		}
		folderRenameAtom = mailews::MsgGetAtom("MoveCopyCompleted");
		newFolder->NotifyFolderEvent(folderRenameAtom);
		pNewParent->NotifyItemAdded(newFolder);
	}

	return rv;
}

NS_IMETHODIMP MailEwsMsgFolder::GetDeletable (bool *deletable)
{
	NS_ENSURE_ARG_POINTER(deletable);

	bool isServer;
	GetIsServer(&isServer);

	*deletable = !(isServer || (mFlags & nsMsgFolderFlags::SpecialUse));
	return NS_OK;
}

NS_IMETHODIMP MailEwsMsgFolder::RenameSubFolders(nsIMsgWindow *msgWindow, nsIMsgFolder *oldFolder)
{
	m_initialized = true;
	nsCOMPtr<nsISimpleEnumerator> enumerator;
	nsresult rv = oldFolder->GetSubFolders(getter_AddRefs(enumerator));
	NS_ENSURE_SUCCESS(rv, rv);

	bool hasMore;
	while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMore)) && hasMore)
	{
		nsCOMPtr<nsISupports> item;
		if (NS_FAILED(enumerator->GetNext(getter_AddRefs(item))))
			continue;

		nsCOMPtr<nsIMsgFolder> msgFolder(do_QueryInterface(item, &rv));
		if (NS_FAILED(rv))
			return rv;

		nsCOMPtr<nsIFile> oldPathFile;
		rv = msgFolder->GetFilePath(getter_AddRefs(oldPathFile));
		if (NS_FAILED(rv)) return rv;

		nsCOMPtr<nsIFile> newParentPathFile;
		rv = GetFilePath(getter_AddRefs(newParentPathFile));
		if (NS_FAILED(rv)) return rv;

		rv = AddDirectorySeparator(newParentPathFile);
		nsAutoCString oldLeafName;
		oldPathFile->GetNativeLeafName(oldLeafName);
		newParentPathFile->AppendNative(oldLeafName);

		nsCString newPathStr;
		newParentPathFile->GetNativePath(newPathStr);

		nsCOMPtr<nsIFile> newPathFile = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
		NS_ENSURE_SUCCESS(rv, rv);
		newPathFile->InitWithFile(newParentPathFile);

		nsCOMPtr<nsIFile> dbFilePath = newPathFile;

		nsCOMPtr<nsIMsgFolder> child;

		nsString folderName;
		rv = msgFolder->GetName(folderName);
		if (folderName.IsEmpty() || NS_FAILED(rv)) return rv;

		nsCString savedFolderId;
		rv = msgFolder->GetStringProperty("folder_id", savedFolderId);
		NS_ENSURE_SUCCESS(rv, rv);

		nsCString change_key;
		rv = msgFolder->GetStringProperty("change_key", change_key);
		NS_ENSURE_SUCCESS(rv, rv);

		nsCString utf7LeafName;
		rv = mailews::CopyUTF16toMUTF7(folderName, utf7LeafName);
		NS_ENSURE_SUCCESS(rv, rv);

		// XXX : Fix this non-sense by fixing AddSubfolderWithPath
		nsAutoString unicodeLeafName;
		CopyASCIItoUTF16(utf7LeafName, unicodeLeafName);

		rv = AddSubfolderWithPath(unicodeLeafName, dbFilePath, getter_AddRefs(child));
		if (!child || NS_FAILED(rv)) return rv;

		child->SetName(folderName);
		child->SetStringProperty("folder_id", savedFolderId);
		child->SetStringProperty("change_key", change_key);

		bool changed = false;
		msgFolder->MatchOrChangeFilterDestination(child, false /*caseInsensitive*/, &changed);
		if (changed)
			msgFolder->AlertFilterChanged(msgWindow);
		child->RenameSubFolders(msgWindow, msgFolder);
	}
	return rv;
}

NS_IMETHODIMP MailEwsMsgFolder::ClientRename(const nsAString& aNewName, nsIMsgWindow *msgWindow)
{
	nsCOMPtr<nsIFile> oldPathFile;
	nsCOMPtr<nsIAtom> folderRenameAtom;
	nsresult rv = GetFilePath(getter_AddRefs(oldPathFile));
	if (NS_FAILED(rv))
		return rv;
	nsCOMPtr<nsIMsgFolder> parentFolder;
	rv = GetParent(getter_AddRefs(parentFolder));
	if (!parentFolder)
		return NS_ERROR_FAILURE;
	nsCOMPtr<nsISupports> parentSupport = do_QueryInterface(parentFolder);
	nsCOMPtr<nsIFile> oldSummaryFile;
	rv = mailews::GetSummaryFileLocation(oldPathFile, getter_AddRefs(oldSummaryFile));
	NS_ENSURE_SUCCESS(rv, rv);
  
	nsCString savedFolderId;
	rv = GetStringProperty("folder_id", savedFolderId);
	NS_ENSURE_SUCCESS(rv, rv);

	nsCString change_key;
	rv = GetStringProperty("change_key", change_key);
	NS_ENSURE_SUCCESS(rv, rv);
  
	nsCOMPtr<nsIFile> dirFile;
	int32_t count = mSubFolders.Count();

	if (count > 0)
		rv = CreateDirectoryForFolder(getter_AddRefs(dirFile));

	nsAutoString newDiskName(aNewName);
    mailews::NS_MsgHashIfNecessary(newDiskName);

    if (mName.Equals(aNewName, nsCaseInsensitiveStringComparator()))
	{
		rv = ThrowAlertMsg("folderExists", msgWindow);
		return NS_MSG_FOLDER_EXISTS;
	}
	else
	{
		nsCOMPtr <nsIFile> parentPathFile;
		parentFolder->GetFilePath(getter_AddRefs(parentPathFile));
		NS_ENSURE_SUCCESS(rv,rv);
		bool isDirectory = false;
		parentPathFile->IsDirectory(&isDirectory);
		if (!isDirectory)
			AddDirectorySeparator(parentPathFile);

		rv = CheckIfFolderExists(aNewName, parentFolder, msgWindow);
		if (NS_FAILED(rv))
			return rv;
	}

	ForceDBClosed();

	// Save of dir name before appending .msf
	nsAutoString newNameDirStr(newDiskName);

	if (! (mFlags & nsMsgFolderFlags::Virtual)) {
		bool exists = false;
		rv = oldPathFile->Exists(&exists);
		NS_ENSURE_SUCCESS(rv, rv);

		if (exists)
			rv = oldPathFile->MoveTo(nullptr, newDiskName);
	}
  
	if (NS_SUCCEEDED(rv))
	{
		newDiskName.AppendLiteral(SUMMARY_SUFFIX);
		oldSummaryFile->MoveTo(nullptr, newDiskName);
	}
	else
	{
		ThrowAlertMsg("folderRenameFailed", msgWindow);
		return rv;
	}

	if (NS_SUCCEEDED(rv) && count > 0)
	{
		// rename "*.sbd" directory
		newNameDirStr.AppendLiteral(".sbd");
		dirFile->MoveTo(nullptr, newNameDirStr);
	}

	nsCOMPtr<nsIMsgFolder> newFolder;
	if (parentSupport)
	{
		rv = parentFolder->AddSubfolder(aNewName, getter_AddRefs(newFolder));
		if (newFolder)
		{
			newFolder->SetPrettyName(EmptyString());
			newFolder->SetPrettyName(aNewName);
			newFolder->SetFlags(mFlags);

			newFolder->SetStringProperty("folder_id", savedFolderId);
			newFolder->SetStringProperty("change_key", change_key);
      
			bool changed = false;

			MatchOrChangeFilterDestination(newFolder, true /*caseInsenstive*/, &changed);

			if (changed)
				AlertFilterChanged(msgWindow);

			if (count > 0)
				newFolder->RenameSubFolders(msgWindow, this);

			if (parentFolder)
			{
				nsCOMPtr<nsIMsgFolderNotificationService> notifier(do_GetService(NS_MSGNOTIFICATIONSERVICE_CONTRACTID));
				if (notifier)
					notifier->NotifyFolderRenamed(this, newFolder);   

				SetParent(nullptr);
				parentFolder->PropagateDelete(this, false, msgWindow);
				parentFolder->NotifyItemAdded(newFolder);
			}
			folderRenameAtom = mailews::MsgGetAtom("RenameCompleted");
			newFolder->NotifyFolderEvent(folderRenameAtom);
		}
	}
	return rv;
}

NS_IMETHODIMP MailEwsMsgFolder::Delete()
{
	nsresult rv;
	if (!mDatabase)
	{
		// Check if anyone has this db open. If so, do a force closed.
		nsCOMPtr<nsIMsgDBService> msgDBService = do_GetService(NS_MSGDB_SERVICE_CONTRACTID, &rv);
		NS_ENSURE_SUCCESS(rv, rv);
		msgDBService->CachedDBForFolder(this, getter_AddRefs(mDatabase));
	}
	if (mDatabase)
	{
		mDatabase->ForceClosed();
		mDatabase = nullptr;
	}

	nsCOMPtr<nsIFile> path;
	rv = GetFilePath(getter_AddRefs(path));
	if (NS_SUCCEEDED(rv))
	{
		nsCOMPtr<nsIFile> summaryLocation;
		rv = mailews::GetSummaryFileLocation(path, getter_AddRefs(summaryLocation));
		if (NS_SUCCEEDED(rv))
		{
			bool exists = false;
			rv = summaryLocation->Exists(&exists);
			if (NS_SUCCEEDED(rv) && exists)
			{
				rv = summaryLocation->Remove(false);
				if (NS_FAILED(rv))
					NS_WARNING("failed to remove ews summary file");
			}
		}
	}
	if (mPath)
		mPath->Remove(false);
	// should notify nsIMsgFolderListeners about the folder getting deleted...
	return rv;
}

NS_IMETHODIMP MailEwsMsgFolder::DeleteSubFolders(nsIArray* folders, nsIMsgWindow *msgWindow) {
	nsCOMPtr<nsIMsgFolder> curFolder;
	nsCOMPtr<nsIUrlListener> urlListener;
	nsCOMPtr<nsIMsgFolder> trashFolder;
	int32_t i;
	uint32_t folderCount = 0;
	nsresult rv;

	bool deleteNoTrash = TrashOrDescendentOfTrash(this);
	bool confirmed = false;
	bool confirmDeletion = true;

	nsCOMPtr<nsISupportsArray> foldersRemaining(do_CreateInstance(NS_SUPPORTSARRAY_CONTRACTID));
	folders->GetLength(&folderCount);

	for (i = folderCount - 1; i >= 0; i--)
	{
		curFolder = do_QueryElementAt(folders, i, &rv);
		if (NS_SUCCEEDED(rv))
		{
			uint32_t folderFlags;
			curFolder->GetFlags(&folderFlags);
			if (folderFlags & nsMsgFolderFlags::Virtual)
			{
				ClientRemoveSubFolder(curFolder);
				// since the folder pane only allows single selection, we can do this
				deleteNoTrash = confirmed = true;
				confirmDeletion = false;
			}
			else {
				foldersRemaining->InsertElementAt(curFolder, 0);
            }
		}
	}

	if (!deleteNoTrash)
	{
		nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
		NS_ENSURE_SUCCESS(rv, rv);
		prefBranch->GetBoolPref("mailnews.confirm.moveFoldersToTrash", &confirmDeletion);
	}
  
	if (!confirmed && (confirmDeletion || deleteNoTrash)) //let us alert the user if we are deleting folder immediately
	{
		NS_ENSURE_SUCCESS(rv, rv);
		if (!msgWindow)
			return NS_ERROR_NULL_POINTER;
		nsCOMPtr<nsIDocShell> docShell;
		msgWindow->GetRootDocShell(getter_AddRefs(docShell));
		nsCOMPtr<nsIPrompt> dialog;
		if (docShell)
			dialog = do_GetInterface(docShell);
		if (dialog)
		{
			int32_t buttonPressed = 0;
			// Default the dialog to "cancel".
			const uint32_t buttonFlags =
					(nsIPrompt::BUTTON_TITLE_IS_STRING * nsIPrompt::BUTTON_POS_0) +
					(nsIPrompt::BUTTON_TITLE_CANCEL * nsIPrompt::BUTTON_POS_1);

			bool dummyValue = false;
			rv = dialog->ConfirmEx(NS_LITERAL_STRING("Delete Folder").get(),
			                       NS_LITERAL_STRING("The folder will be deleted, confirm?").get(),
			                       buttonFlags,
			                       NS_LITERAL_STRING("Delete").get(),
			                       nullptr, nullptr, nullptr, &dummyValue,
			                       &buttonPressed);
			NS_ENSURE_SUCCESS(rv, rv);
			confirmed = !buttonPressed; // "ok" is in position 0
		}
	}
	else
		confirmed = true;

	if (confirmed) {
		nsCOMPtr<nsIMsgIncomingServer> server;
		rv = GetServer(getter_AddRefs(server));
		NS_ENSURE_SUCCESS(rv, rv);

		nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(server, &rv));
		NS_ENSURE_SUCCESS(rv, rv);

		nsCOMPtr<IMailEwsService> ewsService;
		rv = ewsServer->GetService(getter_AddRefs(ewsService));
		NS_ENSURE_SUCCESS(rv, rv);

		return ewsService->DeleteFolders(foldersRemaining,
		                                 deleteNoTrash,
		                                 nullptr);
	}

	return NS_OK;
}

NS_IMETHODIMP MailEwsMsgFolder::CreateSubfolder(const nsAString& folderName, nsIMsgWindow *msgWindow)
{
	if (folderName.IsEmpty())
		return NS_MSG_ERROR_INVALID_FOLDER_NAME;

	nsresult rv;
	if (mIsServer && folderName.LowerCaseEqualsLiteral("inbox"))  // Inbox, a special folder
	{
		ThrowAlertMsg("folderExists", msgWindow);
		return NS_MSG_FOLDER_EXISTS;
	}

	nsCOMPtr<nsIMsgIncomingServer> server;
	rv = GetServer(getter_AddRefs(server));

	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(server, &rv));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<IMailEwsService> ewsService;
	rv = ewsServer->GetService(getter_AddRefs(ewsService));
	NS_ENSURE_SUCCESS(rv, rv);

	rv = ewsService->CreateSubFolder(this,
	                                 folderName,
	                                 nullptr);
	return rv;
}

bool MailEwsMsgFolder::TrashOrDescendentOfTrash(nsIMsgFolder* folder) {
	NS_ENSURE_TRUE(folder, false);
	nsCOMPtr<nsIMsgFolder> parent;
	nsCOMPtr<nsIMsgFolder> curFolder = folder;
	nsresult rv;
	uint32_t flags = 0;
	do {
		rv = curFolder->GetFlags(&flags);
		if (NS_FAILED(rv)) return false;
		if (flags & nsMsgFolderFlags::Trash)
			return true;
		curFolder->GetParent(getter_AddRefs(parent));
		if (!parent) return false;
		curFolder = parent;
	} while (NS_SUCCEEDED(rv) && curFolder);
	return false;
}

NS_IMETHODIMP MailEwsMsgFolder::Rename(const nsAString& aNewName,
                                       nsIMsgWindow *msgWindow) {
	nsCOMPtr<nsIMsgIncomingServer> server;
	nsresult rv = GetServer(getter_AddRefs(server));

	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(server, &rv));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<IMailEwsService> ewsService;
	rv = ewsServer->GetService(getter_AddRefs(ewsService));
	NS_ENSURE_SUCCESS(rv, rv);

  
	return ewsService->RenameFolder(this, aNewName, msgWindow, nullptr);
}

NS_IMETHODIMP MailEwsMsgFolder::Alert(const nsACString & msg, nsIMsgWindow *msgWindow) {
    if (msgWindow) {
        nsCOMPtr<nsIPrompt> dialog;
        msgWindow->GetPromptDialog(getter_AddRefs(dialog));
        if (dialog) {
            nsString uMsg;
            NS_CStringToUTF16(msg, NS_CSTRING_ENCODING_UTF8, uMsg);
            dialog->Alert(nullptr, uMsg.get());
        }
    }
    return NS_OK;
}

/* attribute ACString folderId; */
NS_IMETHODIMP MailEwsMsgFolder::GetFolderId(nsACString & aFolderId)
{
    if (!m_FolderIdLoaded) {
        nsresult rv = GetStringProperty("folder_id", m_FolderId);

        NS_ENSURE_SUCCESS(rv, rv);
        m_FolderIdLoaded = true;
    }
    
    aFolderId = m_FolderId;
    return NS_OK;
}
NS_IMETHODIMP MailEwsMsgFolder::SetFolderId(const nsACString & aFolderId)
{
    m_FolderId = aFolderId;
    m_FolderIdLoaded = true;
	return SetStringProperty("folder_id", aFolderId);
}

/* attribute ACString changeKey; */
NS_IMETHODIMP MailEwsMsgFolder::GetChangeKey(nsACString & aChangeKey)
{
    if (!m_ChangeKeyLoaded) {
        nsresult rv = GetStringProperty("change_key", m_ChangeKey);
        NS_ENSURE_SUCCESS(rv, rv);
        m_ChangeKeyLoaded = true;
    }

    aChangeKey = m_ChangeKey;
    return NS_OK;
}

NS_IMETHODIMP MailEwsMsgFolder::SetChangeKey(const nsACString & aChangeKey)
{
    m_ChangeKey = aChangeKey;
    m_ChangeKeyLoaded = true;
	return SetStringProperty("change_key", aChangeKey);
}

/* attribute ACString folderId; */
NS_IMETHODIMP MailEwsMsgFolder::GetSyncState(nsACString & aSyncState)
{
    if (!m_SyncStateLoaded) {
        nsresult rv = GetStringProperty("sync_state", m_SyncState);
        NS_ENSURE_SUCCESS(rv, rv);
        m_SyncStateLoaded = true;
    }

    aSyncState = m_SyncState;
    return NS_OK;
}
NS_IMETHODIMP MailEwsMsgFolder::SetSyncState(const nsACString & aSyncState)
{
    m_SyncState = aSyncState;
    m_SyncStateLoaded = true;
	return SetStringProperty("sync_state", aSyncState);
}

/* nsIMsgFolder GetChildWithFolderId (in ACString folderId, in bool deep); */
NS_IMETHODIMP MailEwsMsgFolder::GetChildWithFolderId(const nsACString & folderId,
                                                     bool deep,
                                                     nsIMsgFolder * *_retval)
{
	//loop all sub folders
	nsCOMPtr<nsISimpleEnumerator> subFolders;
	nsresult rv = GetSubFolders(getter_AddRefs(subFolders));
	NS_ENSURE_SUCCESS(rv, rv);

	bool hasMore;
	while (NS_SUCCEEDED(subFolders->HasMoreElements(&hasMore)) && hasMore) {
		nsCOMPtr<nsISupports> item;

		rv = subFolders->GetNext(getter_AddRefs(item));

		nsCOMPtr<IMailEwsMsgFolder> msgFolder(do_QueryInterface(item));
		if (!msgFolder)
			continue;

		nsCString savedFolderId;
		rv = msgFolder->GetFolderId(savedFolderId);

		if (NS_SUCCEEDED(rv)) {
			if (savedFolderId.Equals(folderId)) {
				nsCOMPtr<nsIMsgFolder> _msgFolder(do_QueryInterface(msgFolder));
				
				_msgFolder.swap(*_retval);
				return NS_OK;
			}
		}

		if (deep) {
			rv = msgFolder->GetChildWithFolderId(folderId,
			                                     deep,
			                                     _retval);
			if (NS_SUCCEEDED(rv) && *_retval) {
				return NS_OK;
			}
		}
	}

	return NS_OK;
}

NS_IMETHODIMP MailEwsMsgFolder::GetSyncItemCallback(SyncItemCallback ** ppCallback) {
    *ppCallback = m_pSyncItemCallback.get();

    return NS_OK;
}

NS_IMETHODIMP MailEwsMsgFolder::GetMessages(nsISimpleEnumerator **result)
{
    NS_ENSURE_ARG_POINTER(result);
    if (!mDatabase)
        GetDatabase();
    if (mDatabase)
        return mDatabase->EnumerateMessages(result);
    return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
MailEwsMsgFolder::UpdateFolder(nsIMsgWindow * aMsgWindow)
{
	(void) RefreshSizeOnDisk();

	if (!PromptForMasterPasswordIfNecessary())
		return NS_ERROR_FAILURE;
	//Load cached folder_id, changed_key, sync_state
	nsCString str;
	if (NS_FAILED(GetFolderId(str))) {
		SetFolderId(nsCString(""));
		mailews_logger << "get sub folder, update folder id"
		               << std::endl;
	}
	if (NS_FAILED(GetChangeKey(str))) {
		SetChangeKey(nsCString(""));
		mailews_logger << "get sub folder, update change key"
		               << std::endl;
	}
	if (NS_FAILED(GetSyncState(str))) {
		SetSyncState(nsCString(""));
		NS_ASSERTION(false, "get sub folder, update sync state");
	}

	//Process Saved Item Ops
	m_pSyncItemCallback->ProcessSavedItemOps();

    //GetNewMessage
    GetNewMessages(aMsgWindow, nullptr);
    
	NotifyFolderEvent(mFolderLoadedAtom);
    return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP MailEwsMsgFolder::GetFolderURL(nsACString& aFolderURL)
{
    nsCOMPtr<nsIMsgFolder> rootFolder;
    nsresult rv = GetRootFolder(getter_AddRefs(rootFolder));
    NS_ENSURE_SUCCESS(rv, rv);
    rootFolder->GetURI(aFolderURL);
    if (rootFolder == this)
        return NS_OK;

    NS_ASSERTION(mURI.Length() > aFolderURL.Length(), "Should match with a folder name!");
    nsCString escapedName;
    mailews::MsgEscapeString(Substring(mURI, aFolderURL.Length()),
                    nsINetUtil::ESCAPE_URL_PATH,
                    escapedName);
    if (escapedName.IsEmpty())
        return NS_ERROR_OUT_OF_MEMORY;
    aFolderURL.Append(escapedName);
    return NS_OK;
}

nsresult MailEwsMsgFolder::CreateBaseMessageURI(const nsACString& aURI)
{
    nsAutoCString tailURI(aURI);

    mailews_logger << "CreateBaseMessageURI:"
              << nsCString(aURI).get()
              << std::endl;
    // chop off ews:/
    if (tailURI.Find(kEwsRootURI) == 0)
        tailURI.Cut(0, PL_strlen(kEwsRootURI));
    mBaseMessageURI = kEwsMessageRootURI;
    mBaseMessageURI += tailURI;
    return NS_OK;
}

NS_IMETHODIMP MailEwsMsgFolder::GetChildWithName(const nsAString & name,
                                                 bool deep,
                                                 nsIMsgFolder * *child)
{
    NS_ENSURE_ARG_POINTER(child);

    nsresult rv;

    nsAutoCString uri(mURI);
    uri.Append('/');

    // URI should use UTF-8
    // (see RFC2396 Uniform Resource Identifiers (URI): Generic Syntax)
    nsAutoCString escapedName;
    rv = mailews::NS_MsgEscapeEncodeURLPath(name, escapedName);
    NS_ENSURE_SUCCESS(rv, rv);

    // fix for #192780
    // if this is the root folder
    // make sure the the special folders
    // have the right uri.
    // on disk, host\INBOX should be a folder with the uri mailbox://user@host/Inbox"
    // as mailbox://user@host/Inbox != mailbox://user@host/INBOX
    nsCOMPtr<nsIMsgFolder> rootFolder;
    rv = GetRootFolder(getter_AddRefs(rootFolder));
    if (NS_SUCCEEDED(rv) && rootFolder && (rootFolder.get() == (nsIMsgFolder *)this))
    {
        if (MsgLowerCaseEqualsLiteral(escapedName, "inbox"))
            uri += "Inbox";
        else if (MsgLowerCaseEqualsLiteral(escapedName, "unsent%20messages"))
            uri += "Unsent%20Messages";
        else if (MsgLowerCaseEqualsLiteral(escapedName, "drafts"))
            uri += "Drafts";
        else if (MsgLowerCaseEqualsLiteral(escapedName, "trash"))
            uri += "Trash";
        else if (MsgLowerCaseEqualsLiteral(escapedName, "sent"))
            uri += "Sent";
        else if (MsgLowerCaseEqualsLiteral(escapedName, "templates"))
            uri +="Templates";
        else if (MsgLowerCaseEqualsLiteral(escapedName, "archives"))
            uri += "Archives";
        else
            uri += escapedName.get();
    }
    else
        uri += escapedName.get();

    return GetChildWithURI(uri, deep, true /*case Insensitive*/, child);
}

nsresult
MailEwsMsgFolder::GetTrashFolder(nsIMsgFolder** result)
{
    NS_ENSURE_ARG_POINTER(result);
    nsresult rv;
    nsCOMPtr<nsIMsgFolder> rootFolder;
    rv = GetRootFolder(getter_AddRefs(rootFolder));
    if (NS_SUCCEEDED(rv))
    {
        rootFolder->GetFolderWithFlags(nsMsgFolderFlags::Trash, result);
        if (!*result)
            rv = NS_ERROR_FAILURE;
    }
    return rv;
}

static
void
GetItemIdAndChangeKey(nsIMsgDBHdr * msgDBHdr,
                      nsTArray<nsCString> & itemIdsAndChangeKeys) {
    char * item_id = NULL;
    char * change_key = NULL;

    nsresult rv = msgDBHdr->GetStringProperty("item_id", &item_id);
    NS_FAILED_WARN(rv);
    if (NS_FAILED(rv) || item_id == NULL || strlen(item_id) == 0) {
        goto END;
    }
		
    rv = msgDBHdr->GetStringProperty("change_key", &change_key);
    NS_FAILED_WARN(rv);
    if (NS_FAILED(rv) || change_key == NULL || strlen(change_key) == 0) {
        goto END;
    }

    itemIdsAndChangeKeys.AppendElement(nsCString(item_id));
    itemIdsAndChangeKeys.AppendElement(nsCString(change_key));

END:
    if (item_id) NS_Free(item_id);
    if (change_key) NS_Free(change_key);
}

static
NS_IMETHODIMP
GetItemIdsAndChangeKeys(nsIArray * messages,
                        nsTArray<nsCString> & itemIdsAndChangeKeys) {
	nsCOMPtr<nsIMsgDBHdr> msgDBHdr;
    uint32_t messageCount;
    nsresult rv = messages->GetLength(&messageCount);
    NS_ENSURE_SUCCESS(rv, rv);

    for (uint32_t i = 0; i < messageCount; ++i)
	{
		msgDBHdr = do_QueryElementAt(messages, i, &rv);
		NS_FAILED_WARN(rv);
		if (NS_FAILED(rv))
			continue;

        GetItemIdAndChangeKey(msgDBHdr, itemIdsAndChangeKeys);
	}

    return NS_OK;
}


NS_IMETHODIMP
MailEwsMsgFolder::DeleteMessagesFromServer(nsIArray * messages) {
	nsTArray<nsCString> itemIdsAndChangeKeys;
	nsresult rv;

	rv = GetItemIdsAndChangeKeys(messages, itemIdsAndChangeKeys);
	NS_ENSURE_SUCCESS(rv, rv);
	
    if (itemIdsAndChangeKeys.Length() == 0) {
	    return NS_OK;
    }

    nsCOMPtr<nsIMsgIncomingServer> server;
    rv = GetServer(getter_AddRefs(server));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(server, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<IMailEwsService> ewsService;
    rv = ewsServer->GetService(getter_AddRefs(ewsService));
    NS_ENSURE_SUCCESS(rv, rv);

    bool isTrashFolder = mFlags & nsMsgFolderFlags::Trash;

    //Save Item Op
    for (PRUint32 i = 0; i < itemIdsAndChangeKeys.Length(); i+=2) {
	    nsCOMPtr<IMailEwsItemOp> itemOp;

	    rv = this->CreateItemOp(getter_AddRefs(itemOp));
	    if (NS_SUCCEEDED(rv)) {
		    itemOp->SetItemId(itemIdsAndChangeKeys[i]);
		    itemOp->SetChangeKey(itemIdsAndChangeKeys[i + 1]);
		    itemOp->SetRead(false);
		    if (isTrashFolder)
			    itemOp->SetOpType(IMailEwsItemOp::Saved_Item_Delete_Remote_Trash);
		    else
			    itemOp->SetOpType(IMailEwsItemOp::Saved_Item_Delete_Remote);
	    }
    }
    
    return ewsService->DeleteItems(this,
                                   &itemIdsAndChangeKeys,
                                   isTrashFolder,
                                   nullptr);
}

NS_IMETHODIMP
MailEwsMsgFolder::DeleteMessages(nsIArray *messages,
                                 nsIMsgWindow *msgWindow,
                                 bool deleteStorage,
                                 bool isMove,
                                 nsIMsgCopyServiceListener *listener,
                                 bool allowUndo)
{
    mailews_logger << "-------------------- Delete Messages" << std::endl;
    NS_ENSURE_ARG_POINTER(messages);

    uint32_t messageCount;
    nsresult rv = messages->GetLength(&messageCount);
    NS_ENSURE_SUCCESS(rv, rv);

    //Delete On Server
    DeleteMessagesFromServer(messages);

    bool isTrashFolder = mFlags & nsMsgFolderFlags::Trash;

    // notify on delete from trash and shift-delete
    if (!isMove && (deleteStorage || isTrashFolder))
    {
        nsCOMPtr<nsIMsgFolderNotificationService> notifier(do_GetService(NS_MSGNOTIFICATIONSERVICE_CONTRACTID));
        if (notifier)
            notifier->NotifyMsgsDeleted(messages);
    }

    if (!deleteStorage && !isTrashFolder)
    {
        nsCOMPtr<nsIMsgFolder> trashFolder;
        rv = GetTrashFolder(getter_AddRefs(trashFolder));
        if (NS_SUCCEEDED(rv))
        {
            nsCOMPtr<nsIMsgCopyService> copyService = do_GetService(NS_MSGCOPYSERVICE_CONTRACTID, &rv);
            NS_ENSURE_SUCCESS(rv, rv);
            return copyService->CopyMessages(this, messages, trashFolder,
                                             true, listener, msgWindow, allowUndo);
        }
    }
    else
    {
        GetDatabase();
        nsCOMPtr <nsIMsgDatabase> msgDB(mDatabase);
        if (NS_SUCCEEDED(rv))
        {
            nsCOMPtr<nsISupports> msgSupport;
            rv = EnableNotifications(allMessageCountNotifications, false, true /*dbBatching*/);
            if (NS_SUCCEEDED(rv))
            {
                nsCOMPtr<nsIMsgPluggableStore> msgStore;
                rv = GetMsgStore(getter_AddRefs(msgStore));
                if (NS_SUCCEEDED(rv))
                {
                    rv = msgStore->DeleteMessages(messages);
                    GetDatabase();
                    nsCOMPtr<nsIMsgDBHdr> msgDBHdr;
                    if (mDatabase)
                    {
                        for (uint32_t i = 0; i < messageCount; ++i)
                        {
                            msgDBHdr = do_QueryElementAt(messages, i, &rv);
                            rv = mDatabase->DeleteHeader(msgDBHdr, nullptr, false, true);
                        }
                    }
                }
            }
            else if (rv == NS_MSG_FOLDER_BUSY)
                ThrowAlertMsg("deletingMsgsFailed", msgWindow);

            // we are the source folder here for a move or shift delete
            //enable notifications because that will close the file stream
            // we've been caching, mark the db as valid, and commit it.
            EnableNotifications(allMessageCountNotifications, true, true /*dbBatching*/);
            if (!isMove)
                NotifyFolderEvent(NS_SUCCEEDED(rv) ? mDeleteOrMoveMsgCompletedAtom : mDeleteOrMoveMsgFailedAtom);
            if (msgWindow && !isMove)
                AutoCompact(msgWindow);
        }
    }
    return rv;
}

NS_IMETHODIMP
MailEwsMsgFolder::CopyMessages(nsIMsgFolder* srcFolder,
                               nsIArray *messages,
                               bool isMove,
                               nsIMsgWindow *window,
                               nsIMsgCopyServiceListener* listener,
                               bool isFolder,
                               bool allowUndo)
{
    nsCString srcFolderId, destFolderId;

    srcFolder->GetStringProperty("folder_id", srcFolderId);
    this->GetStringProperty("folder_id", destFolderId);
    
    mailews_logger << "-------------------- Copy Messages:"
              << "Source Folder:" << srcFolderId.get()
              << "Dest Folder:" << destFolderId.get()
              << std::endl;
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
MailEwsMsgFolder::CopyFolder(nsIMsgFolder* srcFolder,
                             bool isMoveFolder,
                             nsIMsgWindow *window,
                             nsIMsgCopyServiceListener* listener)
{
    NS_ASSERTION(false, "should be overridden by child class");
    return NS_ERROR_NOT_IMPLEMENTED;
}

// static
// nsresult
// AddKeywordsToMessages(nsIArray *aMessages, const nsACString& aKeywords)
// {
//     return ChangeKeywordForMessages(aMessages, aKeywords, true /* add */);
// }

// static
// nsresult
// ChangeKeywordForMessages(nsIArray *aMessages, const nsACString& aKeywords, bool add)
// {
//     nsresult rv = (add) ? MsgDBFolderBase::AddKeywordsToMessages(aMessages, aKeywords)
//             : MsgDBFolderBase::RemoveKeywordsFromMessages(aMessages, aKeywords);

//     NS_ENSURE_SUCCESS(rv, rv);
//     nsCOMPtr<nsIMsgPluggableStore> msgStore;
//     GetMsgStore(getter_AddRefs(msgStore));
//     NS_ENSURE_SUCCESS(rv, rv);
//     return msgStore->ChangeKeywords(aMessages, aKeywords, add);
// }

NS_IMETHODIMP
MailEwsMsgFolder::CopyFileMessage(nsIFile* aFile,
                                  nsIMsgDBHdr* messageToReplace,
                                  bool isDraftOrTemplate,
                                  uint32_t aNewMsgFlags,
                                  const nsACString &aNewMsgKeywords,
                                  nsIMsgWindow *window,
                                  nsIMsgCopyServiceListener* listener)
{
    mailews_logger << "--------------------^^^^^^^^^^^ Copy File Messages" << std::endl;
    nsCOMPtr<nsISupports> srcSupport = do_QueryInterface(aFile);

    nsCOMPtr<nsIMsgDBHdr> msgHdr;
    nsresult rv = CopyOrReplaceMessageToFolder(this,
                                               messageToReplace,
                                               aFile,
                                               getter_AddRefs(msgHdr));

    if (msgHdr) {
        nsMsgKey msgKey;
        msgHdr->GetMessageKey(&msgKey);
        listener->SetMessageKey(msgKey);

        char * msgId = nullptr;
        msgHdr->GetMessageId(&msgId);

        nsCString lmsgid;
        listener->GetMessageId(lmsgid);
        
        if (msgId) {
            mailews_logger << "copy file message, message id:"
                           << msgId
                           << ","
                           << lmsgid.get()
                           << std::endl;
        }
    }
    
    NS_FAILED_WARN(rv);

    nsresult rv1;
    nsCOMPtr<nsIMsgCopyService> copyService = do_GetService(NS_MSGCOPYSERVICE_CONTRACTID, &rv1);
    NS_ENSURE_SUCCESS(rv1, rv1);
    return copyService->NotifyCompletion(srcSupport, this, rv);
}


NS_IMETHODIMP
MailEwsMsgFolder::OnReadChanged(nsIDBChangeListener * aInstigator)
{
    /* do nothing.  if you care about this, override it.  see nsNewsFolder.cpp */
    mailews_logger << "OnReadChanged" << std::endl;
    return NS_OK;
}

NS_IMETHODIMP
MailEwsMsgFolder::GetMsgHdrsForItemIds(nsTArray<nsCString> * itemIds,
                                       nsIArray ** msgHdrs)
{
    nsCOMPtr <nsISimpleEnumerator> hdrs;

    GetDatabase();
    nsresult rv = mDatabase->ReverseEnumerateMessages(getter_AddRefs(hdrs));
    NS_ENSURE_SUCCESS(rv, rv);

    bool hasMore = false;
    nsCOMPtr<nsIMsgDBHdr> pHeader;
    std::map<std::string, int> itemIdsMap;

    for(size_t i = 0; i < itemIds->Length(); i++) {
        itemIdsMap[(*itemIds)[i].get()] = 0;
    }

	nsCOMPtr<nsIMutableArray> foundMsgHdr(do_CreateInstance(NS_ARRAY_CONTRACTID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    while(NS_SUCCEEDED(rv = hdrs->HasMoreElements(&hasMore))
          && hasMore
          && (itemIdsMap.size() > 0))
    {
        nsCOMPtr<nsISupports> supports;
        (void) hdrs->GetNext(getter_AddRefs(supports));
        pHeader = do_QueryInterface(supports);
        if (pHeader)
        {
            char * item_id = NULL;
            if (NS_SUCCEEDED(pHeader->GetStringProperty("item_id", &item_id)) &&
                item_id &&
                strlen(item_id) > 0) {
                std::map<std::string, int>::iterator it =
                        itemIdsMap.find(item_id);

                if (it != itemIdsMap.end()) {
                    foundMsgHdr->AppendElement(pHeader, false);
                    itemIdsMap.erase(it);
                }
            }

            if (item_id) NS_Free(item_id);
        }
    }

    if (msgHdrs)
        NS_IF_ADDREF(*msgHdrs = foundMsgHdr);

    return NS_OK;
}

NS_IMETHODIMP
MailEwsMsgFolder::GetMsgHdrsForMessageIds(nsTArray<nsCString> * msgIds,
                                       nsIArray ** msgHdrs)
{
    nsCOMPtr <nsISimpleEnumerator> hdrs;

    GetDatabase();
    nsresult rv = mDatabase->ReverseEnumerateMessages(getter_AddRefs(hdrs));
    NS_ENSURE_SUCCESS(rv, rv);

    bool hasMore = false;
    nsCOMPtr<nsIMsgDBHdr> pHeader;
    std::map<std::string, int> msgIdsMap;

    for(size_t i = 0; i < msgIds->Length(); i++) {
        std::string msgId((*msgIds)[i].get());

        if (msgId.size() > 2 &&
            *msgId.begin() == '<' &&
            *(msgId.end() - 1) == '>') {
            msgId = msgId.substr(1, msgId.size() - 2);
        }
        
        msgIdsMap[msgId] = 0;
    }

	nsCOMPtr<nsIMutableArray> foundMsgHdr(do_CreateInstance(NS_ARRAY_CONTRACTID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    while(NS_SUCCEEDED(rv = hdrs->HasMoreElements(&hasMore))
          && hasMore
          && (msgIdsMap.size() > 0))
    {
        nsCOMPtr<nsISupports> supports;
        (void) hdrs->GetNext(getter_AddRefs(supports));
        pHeader = do_QueryInterface(supports);
        if (pHeader)
        {
            char * messageId = nullptr;
            
            if (NS_SUCCEEDED(pHeader->GetMessageId(&messageId)) &&
                messageId &&
                strlen(messageId) > 0) {
                std::map<std::string, int>::iterator it =
                        msgIdsMap.find(messageId);
                if (it != msgIdsMap.end()) {
                    foundMsgHdr->AppendElement(pHeader, false);
                    msgIdsMap.erase(it);
                }
            }

            if (messageId) NS_Free(messageId);
        }
    }

    if (msgHdrs)
        NS_IF_ADDREF(*msgHdrs = foundMsgHdr);

    return NS_OK;
}

NS_IMETHODIMP
MailEwsMsgFolder::ClientDeleteMessages(nsIArray *messages)
{
    NS_ENSURE_ARG_POINTER(messages);

    uint32_t messageCount;
    nsresult rv = messages->GetLength(&messageCount);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIMsgFolderNotificationService> notifier(do_GetService(NS_MSGNOTIFICATIONSERVICE_CONTRACTID));
    if (notifier)
        notifier->NotifyMsgsDeleted(messages);

    GetDatabase();
    nsCOMPtr <nsIMsgDatabase> msgDB(mDatabase);
    if (NS_SUCCEEDED(rv))
    {
        nsCOMPtr<nsISupports> msgSupport;
        rv = EnableNotifications(allMessageCountNotifications, false, true /*dbBatching*/);
        if (NS_SUCCEEDED(rv))
        {
            nsCOMPtr<nsIMsgPluggableStore> msgStore;
            rv = GetMsgStore(getter_AddRefs(msgStore));
            if (NS_SUCCEEDED(rv))
            {
                rv = msgStore->DeleteMessages(messages);
                GetDatabase();
                nsCOMPtr<nsIMsgDBHdr> msgDBHdr;
                if (mDatabase)
                {
                    for (uint32_t i = 0; i < messageCount; ++i)
                    {
                        msgDBHdr = do_QueryElementAt(messages, i, &rv);
                        rv = mDatabase->DeleteHeader(msgDBHdr, nullptr, false, true);
                    }
                }
            }
        }

        // we are the source folder here for a move or shift delete
        //enable notifications because that will close the file stream
        // we've been caching, mark the db as valid, and commit it.
        EnableNotifications(allMessageCountNotifications, true, true /*dbBatching*/);

        NotifyFolderEvent(NS_SUCCEEDED(rv) ? mDeleteOrMoveMsgCompletedAtom : mDeleteOrMoveMsgFailedAtom);
    }
    return rv;
}

NS_IMETHODIMP
MailEwsMsgFolder::MarkMessagesRead(nsIArray *messages, bool markRead)
{
    nsresult rv = MsgDBFolderBase::MarkMessagesRead(messages, markRead);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetDatabase();

    if(NS_SUCCEEDED(rv))
        mDatabase->Commit(nsMsgDBCommitType::kLargeCommit);

    UpdateSummaryTotals(true);

    return MarkMessagesReadOnServer(messages, markRead);
}

NS_IMETHODIMP
MailEwsMsgFolder::MarkMessagesReadOnServer(nsIArray * msgHdrs, bool markRead)
{
    mailews_logger << "mark messages read!"
              << std::endl;
    
	nsTArray<nsCString> itemIdsAndChangeKeys;

	nsresult rv = GetItemIdsAndChangeKeys(msgHdrs, itemIdsAndChangeKeys);
	NS_ENSURE_SUCCESS(rv, rv);
	
    if (itemIdsAndChangeKeys.Length() == 0) {
	    return NS_OK;
    }

    nsCOMPtr<nsIMsgIncomingServer> server;
    rv = GetServer(getter_AddRefs(server));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(server, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<IMailEwsService> ewsService;
    rv = ewsServer->GetService(getter_AddRefs(ewsService));
    NS_ENSURE_SUCCESS(rv, rv);

    //Save Item Op
    for (PRUint32 i = 0; i < itemIdsAndChangeKeys.Length(); i+=2) {
	    nsCOMPtr<IMailEwsItemOp> itemOp;

	    rv = this->CreateItemOp(getter_AddRefs(itemOp));
	    if (NS_SUCCEEDED(rv)) {
		    itemOp->SetItemId((itemIdsAndChangeKeys)[i]);
		    itemOp->SetChangeKey((itemIdsAndChangeKeys)[i + 1]);
		    itemOp->SetRead(markRead);
		    itemOp->SetOpType(IMailEwsItemOp::Saved_Item_Read_Remote);
	    }
    }
    
    return ewsService->MarkItemsRead(this,
                                     &itemIdsAndChangeKeys,
                                     markRead,
                                     nullptr);
}

NS_IMETHODIMP MailEwsMsgFolder::CompactAll(nsIUrlListener *aListener,
                                           nsIMsgWindow *aMsgWindow,
                                           bool aCompactOfflineAlso)
{
    nsresult rv = NS_OK;
    nsCOMPtr<nsIMutableArray> folderArray;
    nsCOMPtr<nsIMsgFolder> rootFolder;
    nsCOMPtr<nsIArray> allDescendents;
    rv = GetRootFolder(getter_AddRefs(rootFolder));
    nsCOMPtr<nsIMsgPluggableStore> msgStore;
    GetMsgStore(getter_AddRefs(msgStore));
    bool storeSupportsCompaction;
    msgStore->GetSupportsCompaction(&storeSupportsCompaction);
    if (!storeSupportsCompaction)
        return NotifyCompactCompleted();

    if (NS_SUCCEEDED(rv) && rootFolder)
    {
        rv = rootFolder->GetDescendants(getter_AddRefs(allDescendents));
        NS_ENSURE_SUCCESS(rv, rv);
        uint32_t cnt = 0;
        rv = allDescendents->GetLength(&cnt);
        NS_ENSURE_SUCCESS(rv, rv);
        folderArray = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
        uint64_t expungedBytes = 0;
        for (uint32_t i = 0; i < cnt; i++)
        {
            nsCOMPtr<nsIMsgFolder> folder = do_QueryElementAt(allDescendents, i, &rv);
            NS_ENSURE_SUCCESS(rv, rv);

            expungedBytes = 0;
            if (folder)
                rv = folder->GetExpungedBytes((int64_t*)&expungedBytes);

            NS_ENSURE_SUCCESS(rv, rv);

            if (expungedBytes > 0)
                rv = folderArray->AppendElement(folder, false);
        }
        rv = folderArray->GetLength(&cnt);
        NS_ENSURE_SUCCESS(rv,rv);
        if (cnt == 0)
            return NotifyCompactCompleted();
    }
    nsCOMPtr <nsIMsgFolderCompactor> folderCompactor =  do_CreateInstance(NS_MSGLOCALFOLDERCOMPACTOR_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    return folderCompactor->CompactFolders(folderArray, nullptr,
                                           aListener, aMsgWindow);
}

NS_IMETHODIMP MailEwsMsgFolder::Compact(nsIUrlListener *aListener, nsIMsgWindow *aMsgWindow)
{
    nsCOMPtr<nsIMsgPluggableStore> msgStore;
    nsresult rv = GetMsgStore(getter_AddRefs(msgStore));
    NS_ENSURE_SUCCESS(rv, rv);
    bool supportsCompaction;

    nsCString uri;
    this->GetURI(uri);
    mailews_logger << "-----Src Folder uri:" << uri.get() << std::endl;
    msgStore->GetSupportsCompaction(&supportsCompaction);
    if (supportsCompaction)
        return msgStore->CompactFolder(this, aListener, aMsgWindow);
    return NS_OK;
}

NS_IMETHODIMP
MailEwsMsgFolder::NotifyCompactCompleted()
{
    mExpungedBytes = 0;
    m_newMsgs.Clear(); // if compacted, m_newMsgs probably aren't valid.
    // if compacted, processing flags probably also aren't valid.
    ClearProcessingFlags();
    (void) RefreshSizeOnDisk();
    (void) CloseDBIfFolderNotOpen();
    nsCOMPtr <nsIAtom> compactCompletedAtom;
    compactCompletedAtom = mailews::MsgGetAtom("CompactCompleted");
    NotifyFolderEvent(compactCompletedAtom);
    return NS_OK;
}

NS_IMETHODIMP MailEwsMsgFolder::RefreshSizeOnDisk()
{
  uint64_t oldFolderSize = mFolderSize;
  mFolderSize = 0; // we set this to 0 to force it to get recalculated from disk
  if (NS_SUCCEEDED(GetSizeOnDisk((int64_t*)&mFolderSize)))
    NotifyIntPropertyChanged(kFolderSizeAtom, oldFolderSize, mFolderSize);
  return NS_OK;
}

NS_IMETHODIMP MailEwsMsgFolder::GetSizeOnDisk(int64_t* aSize)
{
  NS_ENSURE_ARG_POINTER(aSize);
  nsresult rv = NS_OK;
  if (!mFolderSize)
  {
    nsCOMPtr <nsIFile> file;
    rv = GetFilePath(getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);
    int64_t folderSize;
    rv = file->GetFileSize(&folderSize);
    NS_ENSURE_SUCCESS(rv, rv);

    mFolderSize = folderSize;
  }
  *aSize = mFolderSize;
  return rv;
}

NS_IMETHODIMP MailEwsMsgFolder::CreateItemOp(IMailEwsItemOp ** ppItemOp) {
	nsresult rv = GetDatabase();
	if (NS_FAILED(rv))
		return rv;

	MailEwsMsgDatabaseHelper helper(mDatabase);
	return helper.CreateItemOp(ppItemOp);
}

NS_IMETHODIMP MailEwsMsgFolder::RemoveItemOp(const nsACString & itemId, int itemOp) {
	nsresult rv = GetDatabase();
	if (NS_FAILED(rv))
		return rv;

	MailEwsMsgDatabaseHelper helper(mDatabase);
	return helper.RemoveItemOp(itemId, itemOp);
}

NS_IMETHODIMP MailEwsMsgFolder::GetItemOps(nsIArray ** itemOps) {
	nsresult rv = GetDatabase();
	if (NS_FAILED(rv))
		return rv;

	MailEwsMsgDatabaseHelper helper(mDatabase);
	return helper.GetItemOps(itemOps);
}

NS_IMETHODIMP
MailEwsMsgFolder::MarkAllMessagesRead(nsIMsgWindow *aMsgWindow)
{
  nsresult rv = GetDatabase();
  m_newMsgs.Clear();

  if (NS_SUCCEEDED(rv))
  {
    EnableNotifications(allMessageCountNotifications, false, true /*dbBatching*/);
    nsMsgKey *thoseMarked;
    uint32_t numMarked;
    rv = mDatabase->MarkAllRead(&numMarked, &thoseMarked);
    EnableNotifications(allMessageCountNotifications, true, true /*dbBatching*/);
    NS_ENSURE_SUCCESS(rv, rv);

    // Setup a undo-state
    if (aMsgWindow && numMarked)
      rv = AddMarkAllReadUndoAction(aMsgWindow, thoseMarked, numMarked);

    nsCOMPtr<nsIMutableArray> msgHdrs;

    msgHdrs = do_CreateInstance(NS_ARRAY_CONTRACTID);
    
    for(uint32_t i = 0; i < numMarked; i++) {
        nsCOMPtr<nsIMsgDBHdr> msgHdr;
        rv = GetMessageHeader(thoseMarked[i], getter_AddRefs(msgHdr));

        if (NS_SUCCEEDED(rv) && msgHdr) {
            msgHdrs->AppendElement(msgHdr, false);
        }
    }
    
    MarkMessagesReadOnServer(msgHdrs, true);
    
    nsMemory::Free(thoseMarked);
  }

  SetHasNewMessages(false);
  return rv;
}

NS_IMETHODIMP
MailEwsMsgFolder::MarkThreadRead(nsIMsgThread *thread)
{
  nsresult rv = GetDatabase();
  if(NS_SUCCEEDED(rv))
  {
    nsMsgKey *keys;
    uint32_t numKeys;
    rv = mDatabase->MarkThreadRead(thread, nullptr, &numKeys, &keys);
    
    nsCOMPtr<nsIMutableArray> msgHdrs;

    msgHdrs = do_CreateInstance(NS_ARRAY_CONTRACTID);
    
    for(uint32_t i = 0; i < numKeys; i++) {
        nsCOMPtr<nsIMsgDBHdr> msgHdr;
        rv = GetMessageHeader(keys[i], getter_AddRefs(msgHdr));

        if (NS_SUCCEEDED(rv) && msgHdr) {
            msgHdrs->AppendElement(msgHdr, false);
        }
    }
    
    MarkMessagesReadOnServer(msgHdrs, true);
    
    nsMemory::Free(keys);
  }
  return rv;
}

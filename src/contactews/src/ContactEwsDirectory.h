#pragma once
#ifndef __CONTACT_EWS_DIRECTORY_H__
#define __CONTACT_EWS_DIRECTORY_H__

#include "nsIAbDirectory.h" /* include the interface we are going to support */
#include "nsIAbCard.h"
#include "nsCOMPtr.h"
#include "nsIAddrDatabase.h"
#include "nsStringGlue.h"
#include "nsIPrefBranch.h"
#include "nsIMutableArray.h"
#include "nsWeakReference.h"
#include "nsIAbDirectoryQuery.h"
#include "nsIAbDirectorySearch.h"
#include "nsIAbDirSearchListener.h"
#include "nsDataHashtable.h"
#include "nsInterfaceHashtable.h"
#include "nsIMutableArray.h"

#include "IContactEwsDirectory.h"

class IMailEwsService;
class ContactEwsDirectory: public nsIAbDirectory,
                           public nsSupportsWeakReference,
                           public nsIAbDirectoryQuery,
                           public nsIAbDirectorySearch,
                           public nsIAbDirSearchListener,
                           public nsIAbDirectoryQueryResultListener,
                           public IContactEwsDirectory
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
	NS_DECL_NSIABITEM
	NS_DECL_NSIABCOLLECTION
	NS_DECL_NSIABDIRECTORY
	NS_DECL_NSIABDIRSEARCHLISTENER
	NS_DECL_NSIABDIRECTORYQUERYRESULTLISTENER
	NS_DECL_NSIABDIRECTORYQUERY
	NS_DECL_NSIABDIRECTORYSEARCH
    NS_DECL_ICONTACTEWSDIRECTORY

	ContactEwsDirectory(void);

public:

protected:
	virtual ~ContactEwsDirectory(void);
    nsresult GetService(IMailEwsService ** s);
    nsresult GetAccountIdentity(nsCString & id);
protected:
	/**
	 * Initialise the directory prefs for this branch
	 */
	nsresult InitDirectoryPrefs();

	uint32_t m_LastModifiedDate;

	nsString m_ListDirName;
	nsString m_ListName;
	nsString m_ListNickName;
	nsString m_Description;
	bool     m_IsMailList;

	nsCString mURI;
	nsCString mQueryString;
	nsCString mURINoQuery;
	bool mIsValidURI;
	bool mIsQueryURI;

	nsCString m_DirPrefId;

	// Container for the query threads
	PRLock *mProtector;
	// Data for the search interfaces
	int32_t mSearchContext;
	nsCOMPtr<nsIPrefBranch> m_DirectoryPrefs;
	nsCOMPtr<nsIMutableArray> m_AddressList;
    int32_t mCurrentQueryId;
};

#endif //__CONTACT_EWS_DIRECTORY_H__

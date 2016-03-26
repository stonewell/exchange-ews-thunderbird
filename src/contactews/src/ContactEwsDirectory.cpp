#include "ContactEwsDirectory.h"

#include "nsAbBaseCID.h"
#include "nsMsgBaseCID.h"
#include "nsIAbCard.h"
#include "nsStringGlue.h"
#include "nsIAbBooleanExpression.h"
#include "nsIAbManager.h"
#include "nsIAbMDBDirectory.h"
#include "nsEnumeratorUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "prlog.h"
#include "prthread.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsCRTGlue.h"
#include "nsArrayUtils.h"
#include "nsArrayEnumerator.h"
#include "nsMsgUtils.h"
#include "nsIPrefLocalizedString.h"
#include "nsIIOService.h"
#include "prmem.h"
#include "mozilla/Services.h"
#include "nsIAbDirectoryQuery.h"
#include "nsIMsgAccountManager.h"
#include "nsIMsgIncomingServer.h"
#include "nsIMsgAccount.h"
#include "nsIMsgIdentity.h"

#include "ContactEwsCID.h"

#include "IMailEwsMsgIncomingServer.h"

#include <iostream>

ContactEwsDirectory::ContactEwsDirectory(void)
	: m_LastModifiedDate(0)
	, mIsValidURI(false)
	, mIsQueryURI(false)
	, mSearchContext(-1)
{
	m_IsMailList = false;
	mProtector = PR_NewLock() ;
}

ContactEwsDirectory::~ContactEwsDirectory(void)
{
	if (mProtector) { PR_DestroyLock(mProtector) ; }
}

NS_IMPL_ISUPPORTS(ContactEwsDirectory, nsIAbDirectory,
                  nsISupportsWeakReference,
                  nsIAbCollection, nsIAbItem,
                  nsIAbDirectoryQuery, nsIAbDirectorySearch,
                  nsIAbDirSearchListener,
                  IContactEwsDirectory)

NS_IMETHODIMP ContactEwsDirectory::Init(const char *aUri)
{
	nsAutoCString entry;
	nsAutoCString stub;

	mURINoQuery = aUri;
	mURI = aUri;
	mIsValidURI = true;

	int32_t searchCharLocation = mURINoQuery.FindChar('?');
	if (searchCharLocation >= 0)
	{
		mQueryString = Substring(mURINoQuery, searchCharLocation + 1);
		mURINoQuery.SetLength(searchCharLocation);
		mIsQueryURI = true;
	}

	return NS_OK;
}

// nsIAbDirectory methods

NS_IMETHODIMP ContactEwsDirectory::GetDirType(int32_t *aDirType)
{
	NS_ENSURE_ARG_POINTER(aDirType);
	*aDirType = 3;//LDAPDirectory;
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::GetURI(nsACString &aURI)
{
	if (mURI.IsEmpty())
		return NS_ERROR_NOT_INITIALIZED;

	aURI = mURI;
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::GetChildNodes(nsISimpleEnumerator **aNodes)
{
	return NS_NewEmptyEnumerator(aNodes);
}

NS_IMETHODIMP ContactEwsDirectory::GetChildCards(nsISimpleEnumerator ** result)
{
	NS_ENSURE_ARG_POINTER(result);

	nsresult rv;
    
	// when offline, we need to get the child cards for the local, replicated mdb directory 
	bool offline;
	nsCOMPtr <nsIIOService> ioService =
			mozilla::services::GetIOService();
	NS_ENSURE_TRUE(ioService, NS_ERROR_UNEXPECTED);
	rv = ioService->GetOffline(&offline);
	NS_ENSURE_SUCCESS(rv,rv);
    
	if (offline) {
		rv = NS_NewEmptyEnumerator(result);
		//TODO:Implement copy global address to local
		// nsCString fileName;
		// rv = GetReplicationFileName(fileName);
		// NS_ENSURE_SUCCESS(rv,rv);
      
		// // if there is no fileName, bail out now.
		// if (fileName.IsEmpty())
		// 	return NS_OK;

		// // perform the same query, but on the local directory
		// nsAutoCString localDirectoryURI(NS_LITERAL_CSTRING(kMDBDirectoryRoot));
		// localDirectoryURI.Append(fileName);
		// if (mIsQueryURI) 
		// {
		// 	localDirectoryURI.AppendLiteral("?");
		// 	localDirectoryURI.Append(mQueryString);
		// }

		// nsCOMPtr<nsIAbManager> abManager(do_GetService(NS_ABMANAGER_CONTRACTID,
		//                                                &rv));
		// NS_ENSURE_SUCCESS(rv, rv);

		// nsCOMPtr <nsIAbDirectory> directory;
		// rv = abManager->GetDirectory(localDirectoryURI,
		//                              getter_AddRefs(directory));
		// NS_ENSURE_SUCCESS(rv, rv);

		// rv = directory->GetChildCards(result);
	}
	else {
		// Start the search
		//m_CardCache = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
		NS_ENSURE_SUCCESS(rv, rv);

		rv = StartSearch();
		NS_ENSURE_SUCCESS(rv, rv);

		rv = NS_NewEmptyEnumerator(result);
	}

	NS_ENSURE_SUCCESS(rv,rv);
	return rv;
}

NS_IMETHODIMP ContactEwsDirectory::GetIsQuery(bool *aResult)
{
	NS_ENSURE_ARG_POINTER(aResult);
	*aResult = mIsQueryURI;
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::HasCard(nsIAbCard *aCard, bool *aHasCard)
{
	if (!aCard || !aHasCard)
		return NS_ERROR_NULL_POINTER;

	*aHasCard = false;//mCardList.Get(aCard, nullptr);
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::HasDirectory(nsIAbDirectory *aDirectory, bool *aHasDirectory)
{
	NS_ENSURE_ARG_POINTER(aDirectory);
	NS_ENSURE_ARG_POINTER(aHasDirectory);

	*aHasDirectory = false;

	uint32_t pos;
	if (m_AddressList && NS_SUCCEEDED(m_AddressList->IndexOf(0, aDirectory, &pos)))
		*aHasDirectory = true;

	return NS_OK;
}

static const char * PropertyNameSupported[] =
{
    kFirstNameProperty,
    kLastNameProperty,
    kDisplayNameProperty,
    kNicknameProperty,
    kPriEmailProperty,
};

static const uint32_t NbPropertyNameSupported =
        sizeof(PropertyNameSupported) /
        sizeof(const char *);

static bool IsPropertySupported(const char *aName) {
    uint32_t i = 0 ;
    
    for (i = 0 ; i < NbPropertyNameSupported ; ++ i) {
        if (strcmp(aName, PropertyNameSupported[i]) == 0) {
            return true;
        }
    }
    
    return false;
}

static bool IsValidCondition(nsIAbBooleanConditionString *aCondition,
                             nsCString & value)
{
    if (!aCondition) { return false; }
    nsAbBooleanConditionType conditionType = 0 ;
    nsresult retCode = NS_OK ;
    nsCString name;
    nsString uValue;
    
    retCode = aCondition->GetCondition(&conditionType) ;
    NS_ENSURE_SUCCESS(retCode, false) ;
    retCode = aCondition->GetName(getter_Copies(name)) ;
    NS_ENSURE_SUCCESS(retCode, false) ;
    retCode = aCondition->GetValue(getter_Copies(uValue)) ;
    NS_ENSURE_SUCCESS(retCode, false) ;
    CopyUTF16toUTF8(uValue, value);
    if (!IsPropertySupported(name.get())) {
        return false;
    }

    switch (conditionType) {
    case nsIAbBooleanConditionTypes::Exists :
    case nsIAbBooleanConditionTypes::Contains :
    case nsIAbBooleanConditionTypes::Is :
    case nsIAbBooleanConditionTypes::BeginsWith :
    case nsIAbBooleanConditionTypes::EndsWith :
        return true;
    default :
        return false;
    }

    return false;
}

static nsresult BuildResolveQueryValue(nsIAbBooleanExpression *aLevel,
                                       nsCString & value)
{
    if (!aLevel) { return NS_ERROR_NULL_POINTER ; }
    nsresult retCode = NS_OK ;
    nsAbBooleanOperationType operationType = 0 ;
    uint32_t nbExpressions = 0 ;
    nsCOMPtr<nsIArray> expressions;

    value.AssignLiteral("");
    
    retCode = aLevel->GetOperation(&operationType);
    NS_ENSURE_SUCCESS(retCode, retCode);
    retCode = aLevel->GetExpressions(getter_AddRefs(expressions));
    NS_ENSURE_SUCCESS(retCode, retCode);
    retCode = expressions->GetLength(&nbExpressions);
    NS_ENSURE_SUCCESS(retCode, retCode);
    if (nbExpressions == 0) { 
        return NS_OK ;
    }

    //NOT operation is not supported
    if (operationType == nsIAbBooleanOperationTypes::NOT)
        return NS_OK;

    uint32_t i = 0 ;

    nsCOMPtr<nsIAbBooleanConditionString> condition;
    nsCOMPtr<nsIAbBooleanExpression> subExpression;

    //Only use the first valid condition
    for (i = 0; i < nbExpressions; ++i) {
        condition = do_QueryElementAt(expressions, i, &retCode);

        if (NS_SUCCEEDED(retCode)) {
            nsCString v;
            if (IsValidCondition(condition, v)) {
                value = v;
                break;
            }
        }
        else { 
            subExpression = do_QueryElementAt(expressions, i, &retCode);

            if (NS_SUCCEEDED(retCode)) {
                nsCString v;
                retCode = BuildResolveQueryValue(subExpression,
                                                 v);

                if (NS_SUCCEEDED(retCode) && !v.IsEmpty()) {
                    value = v;
                    break;
                }
            }
        }
      
    }

    return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::DoQuery(nsIAbDirectory *aDirectory,
                                           nsIAbDirectoryQueryArguments * aArguments,
                                           nsIAbDirSearchListener *aListener,
                                           int32_t aResultLimit,
                                           int32_t aTimeout,
                                           int32_t *aReturnValue)
{
	if (!aArguments || !aListener || !aReturnValue)  { 
		return NS_ERROR_NULL_POINTER;
	}
    
    *aReturnValue = -1;
    nsCOMPtr<nsIAbBooleanExpression> expression ;
    nsISupports * supports;

    nsresult retCode = aArguments->GetExpression(&supports);
	NS_ENSURE_SUCCESS(retCode, retCode) ;

    expression = do_QueryInterface(supports, &retCode);
	NS_ENSURE_SUCCESS(retCode, retCode) ;
    
    nsCString value;
    retCode = BuildResolveQueryValue(expression,
                                     value);
    
	NS_ENSURE_SUCCESS(retCode, retCode) ;

    nsCOMPtr<IMailEwsService> ewsService;
    retCode = GetService(getter_AddRefs(ewsService));
	NS_ENSURE_SUCCESS(retCode, retCode);

    if (!ewsService) {
        return NS_ERROR_FAILURE;
    }

    nsCString uuid;
    GetUuid(uuid);
    
    PR_Lock(mProtector);
    *aReturnValue = ++mCurrentQueryId;
    PR_Unlock(mProtector);
    
    return ewsService->ResolveNames(value,
                                    aListener,
                                    aDirectory,
                                    uuid,
                                    aResultLimit,
                                    false, /* sync call */
                                    *aReturnValue,
                                    nullptr);
}

NS_IMETHODIMP ContactEwsDirectory::StopQuery(int32_t aContext)
{
    return NS_OK;
}

// nsIAbDirectorySearch methods
NS_IMETHODIMP ContactEwsDirectory::StartSearch(void)
{
	if (!mIsQueryURI) { return NS_ERROR_NOT_IMPLEMENTED ; }
	nsresult retCode = NS_OK ;
    
	retCode = StopSearch() ;
	NS_ENSURE_SUCCESS(retCode, retCode) ;
	//mCardList.Clear();

	nsCOMPtr<nsIAbBooleanExpression> expression ;

	nsCOMPtr<nsIAbManager> abManager(do_GetService(NS_ABMANAGER_CONTRACTID, &retCode));
	NS_ENSURE_SUCCESS(retCode, retCode);
    
	retCode = abManager->ConvertQueryStringToExpression(mQueryString,
                                                        getter_AddRefs(expression)) ;
	NS_ENSURE_SUCCESS(retCode, retCode) ;

    nsCOMPtr<nsIAbDirectoryQueryArguments> arguments = do_CreateInstance(NS_ABDIRECTORYQUERYARGUMENTS_CONTRACTID,&retCode);
    NS_ENSURE_SUCCESS(retCode, retCode);

    retCode = arguments->SetExpression(expression) ;
    NS_ENSURE_SUCCESS(retCode, retCode) ;

	return DoQuery(this, arguments, this, -1, 0, &mSearchContext);
}

NS_IMETHODIMP ContactEwsDirectory::StopSearch(void) 
{
	if (!mIsQueryURI) { return NS_ERROR_NOT_IMPLEMENTED ; }
	return StopQuery(mSearchContext) ;
}

// nsIAbDirSearchListener
NS_IMETHODIMP ContactEwsDirectory::OnSearchFinished(int32_t aResult,
                                                    const nsAString &aErrorMsg)
{
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::OnSearchFoundCard(nsIAbCard *aCard) 
{
	//mCardList.Put(aCard, aCard);
    // if (m_CardCache)
    //     m_CardCache->AppendElement(aCard, false);

    bool sync = false;
    aCard->GetPropertyAsBool("EwsSyncCall", &sync);
    if (!sync) {
        /* do not notify, when we do sync call, 
           the cards will return to client after function calll */
        nsresult rv;
        nsCOMPtr<nsIAbManager> abManager(do_GetService(NS_ABMANAGER_CONTRACTID, &rv));
        if (NS_SUCCEEDED(rv))
            rv = abManager->NotifyDirectoryItemAdded(this, aCard);
    }
	return NS_OK;
}

nsresult ContactEwsDirectory::GetService(IMailEwsService ** aService) {
    //Find username and host name
    nsCString userName;

    userName = Substring(mURINoQuery,
                         strlen(CONTACT_EWS_DIRECTORY_URI_PREFIX));
    
	int32_t searchCharLocation = userName.RFindChar('@');
	nsCString host;

    host = Substring(userName, searchCharLocation + 1);
	userName.SetLength(searchCharLocation);

    nsresult retCode = NS_OK;
    
    nsCOMPtr<nsIMsgAccountManager> accountMgr = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &retCode);
    NS_ENSURE_SUCCESS(retCode, retCode);

    nsCOMPtr<nsIMsgIncomingServer> incomingServer;
    retCode = accountMgr->FindServer(userName, host, nsCString("ews"),
                                     getter_AddRefs(incomingServer));
    NS_ENSURE_SUCCESS(retCode, retCode);

	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(incomingServer, &retCode));
    NS_ENSURE_SUCCESS(retCode, retCode);

	nsCOMPtr<IMailEwsService> ewsService;

	nsresult rv3 = ewsServer->GetService(getter_AddRefs(ewsService));
	NS_ENSURE_SUCCESS(rv3, rv3);

	ewsService.swap(*aService);
	return NS_OK;
}

nsresult ContactEwsDirectory::GetAccountIdentity(nsCString & id) {
    //Find username and host name
    nsCString userName;

    userName = Substring(mURINoQuery,
                         strlen(CONTACT_EWS_DIRECTORY_URI_PREFIX));
    
	int32_t searchCharLocation = userName.RFindChar('@');
	nsCString host;

    host = Substring(userName, searchCharLocation + 1);
	userName.SetLength(searchCharLocation);

    nsresult retCode = NS_OK;
    
    nsCOMPtr<nsIMsgAccountManager> accountMgr = do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &retCode);
    NS_ENSURE_SUCCESS(retCode, retCode);

    nsCOMPtr<nsIMsgIncomingServer> incomingServer;
    retCode = accountMgr->FindServer(userName, host, nsCString("ews"),
                                     getter_AddRefs(incomingServer));
    NS_ENSURE_SUCCESS(retCode, retCode);

    nsCOMPtr<nsIMsgAccount> account;
    retCode = accountMgr->FindAccountForServer(incomingServer,
                                               getter_AddRefs(account));
    NS_ENSURE_SUCCESS(retCode, retCode);

    nsCOMPtr<nsIMsgIdentity> identity;
    retCode = account->GetDefaultIdentity(getter_AddRefs(identity));
    NS_ENSURE_SUCCESS(retCode, retCode);
    
	return identity->GetKey(id);
}

#define PREF_MAIL_ADDR_BOOK_LASTNAMEFIRST "mail.addr_book.lastnamefirst"

NS_IMETHODIMP ContactEwsDirectory::OnQueryFoundCard(nsIAbCard *aCard)
{
	return OnSearchFoundCard(aCard);
}

NS_IMETHODIMP ContactEwsDirectory::OnQueryResult(int32_t aResult,
                                                 int32_t aErrorCode)
{
	return OnSearchFinished(aResult, EmptyString());
}

NS_IMETHODIMP ContactEwsDirectory::UseForAutocomplete(const nsACString &aIdentityKey, bool *aResult)
{
	NS_ENSURE_ARG_POINTER(aResult);

    /* Will do autocomplete in ContactEwsAutoCompleteSearch 
    nsCString key;
    GetAccountIdentity(key);
    
    *aResult = fkey.Equals(aIdentityKey);
    */
    *aResult = false;
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::GetUuid(nsACString &uuid)
{
	// XXX: not all directories have a dirPrefId...
	nsresult rv = GetDirPrefId(uuid);
	NS_ENSURE_SUCCESS(rv, rv);
	uuid.Append('&');
	nsString dirName;
	GetDirName(dirName);
	uuid.Append(NS_ConvertUTF16toUTF8(dirName));
	return rv;
}

NS_IMETHODIMP ContactEwsDirectory::GenerateName(int32_t aGenerateFormat,
                                                nsIStringBundle *aBundle,
                                                nsAString &name)
{
	return GetDirName(name);
}

NS_IMETHODIMP ContactEwsDirectory::GetPropertiesChromeURI(nsACString &aResult)
{
	aResult.AssignLiteral("chrome://messenger/content/addressbook/abAddressBookNameDialog.xul");
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::GetDirName(nsAString &aDirName)
{
	if (m_DirPrefId.IsEmpty())
	{
		aDirName = m_ListDirName;
		return NS_OK;
	}

	nsCString dirName;
	nsresult rv = GetLocalizedStringValue("description", EmptyCString(), dirName);
	NS_ENSURE_SUCCESS(rv, rv);

	// In TB 2 only some prefs had chrome:// URIs. We had code in place that would
	// only get the localized string pref for the particular address books that
	// were built-in.
	// Additionally, nsIPrefBranch::getComplexValue will only get a non-user-set,
	// non-locked pref value if it is a chrome:// URI and will get the string
	// value at that chrome URI. This breaks extensions/autoconfig that want to
	// set default pref values and allow users to change directory names.
	//
	// Now we have to support this, and so if for whatever reason we fail to get
	// the localized version, then we try and get the non-localized version
	// instead. If the string value is empty, then we'll just get the empty value
	// back here.
	if (dirName.IsEmpty())
	{
		rv = GetStringValue("description", EmptyCString(), dirName);
		NS_ENSURE_SUCCESS(rv, rv);
	}

	CopyUTF8toUTF16(dirName, aDirName);

	return NS_OK;
}

// XXX Although mailing lists could use the NotifyItemPropertyChanged
// mechanism here, it requires some rework on how we write/save data
// relating to mailing lists, so we're just using the old method of a
// local variable to store the mailing list name.
NS_IMETHODIMP ContactEwsDirectory::SetDirName(const nsAString &aDirName)
{
	if (m_DirPrefId.IsEmpty())
	{
		m_ListDirName = aDirName;
		return NS_OK;
	}

	// Store the old value.
	nsString oldDirName;
	nsresult rv = GetDirName(oldDirName);
	NS_ENSURE_SUCCESS(rv, rv);

	// Save the new value
	rv = SetLocalizedStringValue("description", NS_ConvertUTF16toUTF8(aDirName));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIAbManager> abManager = do_GetService(NS_ABMANAGER_CONTRACTID, &rv);

	if (NS_SUCCEEDED(rv))
		// We inherit from nsIAbDirectory, so this static cast should be safe.
		abManager->NotifyItemPropertyChanged(static_cast<nsIAbDirectory*>(this),
		                                     "DirName", oldDirName.get(),
		                                     nsString(aDirName).get());

	return NS_OK;
}


NS_IMETHODIMP ContactEwsDirectory::GetFileName(nsACString &aFileName)
{
	return GetStringValue("filename", EmptyCString(), aFileName);
}


NS_IMETHODIMP ContactEwsDirectory::GetLastModifiedDate(uint32_t *aLastModifiedDate)
{
	NS_ENSURE_ARG_POINTER(aLastModifiedDate);
	*aLastModifiedDate = m_LastModifiedDate;
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::SetLastModifiedDate(uint32_t aLastModifiedDate)
{
	if (aLastModifiedDate)
	{
		m_LastModifiedDate = aLastModifiedDate;
	}
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::GetListNickName(nsAString &aListNickName)
{
	aListNickName = m_ListNickName;
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::SetListNickName(const nsAString &aListNickName)
{
	m_ListNickName = aListNickName;
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::GetDescription(nsAString &aDescription)
{
	aDescription = m_Description;
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::SetDescription(const nsAString &aDescription)
{
	m_Description = aDescription;
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::GetIsMailList(bool *aIsMailList)
{
	*aIsMailList = m_IsMailList;
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::SetIsMailList(bool aIsMailList)
{
	m_IsMailList = aIsMailList;
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::GetAddressLists(nsIMutableArray * *aAddressLists)
{
	if (!m_AddressList) 
	{
		nsresult rv;
		m_AddressList = do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
		NS_ENSURE_SUCCESS(rv, rv);
	}

	*aAddressLists = m_AddressList;
	NS_ADDREF(*aAddressLists);
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::SetAddressLists(nsIMutableArray * aAddressLists)
{
	m_AddressList = aAddressLists;
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::CopyMailList(nsIAbDirectory* srcList)
{
	SetIsMailList(true);

	nsString str;
	srcList->GetDirName(str);
	SetDirName(str);
	srcList->GetListNickName(str);
	SetListNickName(str);
	srcList->GetDescription(str);
	SetDescription(str);

	nsCOMPtr<nsIMutableArray> pAddressLists;
	srcList->GetAddressLists(getter_AddRefs(pAddressLists));
	SetAddressLists(pAddressLists);
	return NS_OK;
}

NS_IMETHODIMP
ContactEwsDirectory::DeleteDirectory(nsIAbDirectory *directory)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP
ContactEwsDirectory::CreateNewDirectory(const nsAString &aDirName,
                                        const nsACString &aURI,
                                        uint32_t aType,
                                        const nsACString &aPrefName,
                                        nsACString &aResult)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP
ContactEwsDirectory::CreateDirectoryByURI(const nsAString &aDisplayName,
                                          const nsACString &aURI)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP ContactEwsDirectory::AddMailList(nsIAbDirectory *list, nsIAbDirectory **addedList)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP ContactEwsDirectory::EditMailListToDatabase(nsIAbCard *listCard)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP ContactEwsDirectory::AddCard(nsIAbCard *childCard, nsIAbCard **addedCard)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP ContactEwsDirectory::ModifyCard(nsIAbCard *aModifiedCard)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP ContactEwsDirectory::DeleteCards(nsIArray *cards)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP ContactEwsDirectory::DropCard(nsIAbCard *childCard, bool needToCopyCard)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP ContactEwsDirectory::CardForEmailAddress(const nsACString &aEmailAddress,
                                                       nsIAbCard ** aAbCard)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP ContactEwsDirectory::GetCardFromProperty(const char *aProperty,
                                                       const nsACString &aValue,
                                                       bool caseSensitive,
                                                       nsIAbCard **result)
{ return NS_ERROR_NOT_IMPLEMENTED; }

NS_IMETHODIMP ContactEwsDirectory::GetCardsFromProperty(const char *aProperty,
                                                        const nsACString &aValue,
                                                        bool caseSensitive,
                                                        nsISimpleEnumerator **result)
{ return NS_ERROR_NOT_IMPLEMENTED; }


NS_IMETHODIMP ContactEwsDirectory::GetSupportsMailingLists(bool *aSupportsMailingsLists)
{
	NS_ENSURE_ARG_POINTER(aSupportsMailingsLists);
	*aSupportsMailingsLists = false;
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::GetReadOnly(bool *aReadOnly)
{
	NS_ENSURE_ARG_POINTER(aReadOnly);
	// Default is that we are writable. Any implementation that is read-only must
	// override this method.
	*aReadOnly = true;
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::GetIsRemote(bool *aIsRemote)
{
	NS_ENSURE_ARG_POINTER(aIsRemote);
	*aIsRemote = true;
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::GetIsSecure(bool *aIsSecure)
{
	NS_ENSURE_ARG_POINTER(aIsSecure);
	*aIsSecure = true;
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::GetDirPrefId(nsACString &aDirPrefId)
{
	aDirPrefId = m_DirPrefId;
	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::SetDirPrefId(const nsACString &aDirPrefId)
{
	if (!m_DirPrefId.Equals(aDirPrefId))
	{
		m_DirPrefId.Assign(aDirPrefId);
		// Clear the directory pref branch so that it is re-initialized next
		// time its required.
		m_DirectoryPrefs = nullptr;
	}
	return NS_OK;
}

nsresult ContactEwsDirectory::InitDirectoryPrefs()
{
	if (m_DirPrefId.IsEmpty())
		return NS_ERROR_NOT_INITIALIZED;
    
	nsresult rv;
	nsCOMPtr<nsIPrefService> prefService(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCString realPrefId(m_DirPrefId);
	realPrefId.Append('.');

	return prefService->GetBranch(realPrefId.get(), getter_AddRefs(m_DirectoryPrefs));
}

NS_IMETHODIMP ContactEwsDirectory::GetIntValue(const char *aName,
                                               int32_t aDefaultValue,
                                               int32_t *aResult)
{
	NS_ENSURE_ARG_POINTER(aResult);

	if (!m_DirectoryPrefs && NS_FAILED(InitDirectoryPrefs()))
		return NS_ERROR_NOT_INITIALIZED;

	if (NS_FAILED(m_DirectoryPrefs->GetIntPref(aName, aResult)))
		*aResult = aDefaultValue;

	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::GetBoolValue(const char *aName,
                                                bool aDefaultValue,
                                                bool *aResult)
{
	NS_ENSURE_ARG_POINTER(aResult);

	if (!m_DirectoryPrefs && NS_FAILED(InitDirectoryPrefs()))
		return NS_ERROR_NOT_INITIALIZED;

	if (NS_FAILED(m_DirectoryPrefs->GetBoolPref(aName, aResult)))
		*aResult = aDefaultValue;

	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::GetStringValue(const char *aName,
                                                  const nsACString &aDefaultValue, 
                                                  nsACString &aResult)
{
	if (!m_DirectoryPrefs && NS_FAILED(InitDirectoryPrefs()))
		return NS_ERROR_NOT_INITIALIZED;

	nsCString value;

	/* unfortunately, there may be some prefs out there which look like (null) */
	if (NS_SUCCEEDED(m_DirectoryPrefs->GetCharPref(aName, getter_Copies(value))) &&
	    !value.EqualsLiteral("(null"))
		aResult = value;
	else
		aResult = aDefaultValue;

	return NS_OK;
}
/*
 * Get localized unicode string pref from properties file, convert into an
 * UTF8 string since address book prefs store as UTF8 strings. So far there
 * are 2 default prefs stored in addressbook.properties.
 * "ldap_2.servers.pab.description"
 * "ldap_2.servers.history.description"
 */
NS_IMETHODIMP ContactEwsDirectory::GetLocalizedStringValue(const char *aName,
                                                           const nsACString &aDefaultValue, 
                                                           nsACString &aResult)
{
	if (!m_DirectoryPrefs && NS_FAILED(InitDirectoryPrefs()))
		return NS_ERROR_NOT_INITIALIZED;

	nsString wvalue;
	nsCOMPtr<nsIPrefLocalizedString> locStr;

	nsresult rv = m_DirectoryPrefs->GetComplexValue(aName,
	                                                NS_GET_IID(nsIPrefLocalizedString),
	                                                getter_AddRefs(locStr));
	if (NS_SUCCEEDED(rv))
	{
		rv = locStr->ToString(getter_Copies(wvalue));
		NS_ENSURE_SUCCESS(rv, rv);
	}

	if (wvalue.IsEmpty())
		aResult = aDefaultValue;
	else
		CopyUTF16toUTF8(wvalue, aResult);

	return NS_OK;
}

NS_IMETHODIMP ContactEwsDirectory::SetIntValue(const char *aName,
                                               int32_t aValue)
{
	if (!m_DirectoryPrefs && NS_FAILED(InitDirectoryPrefs()))
		return NS_ERROR_NOT_INITIALIZED;

	return m_DirectoryPrefs->SetIntPref(aName, aValue);
}

NS_IMETHODIMP ContactEwsDirectory::SetBoolValue(const char *aName,
                                                bool aValue)
{
	if (!m_DirectoryPrefs && NS_FAILED(InitDirectoryPrefs()))
		return NS_ERROR_NOT_INITIALIZED;

	return m_DirectoryPrefs->SetBoolPref(aName, aValue);
}

NS_IMETHODIMP ContactEwsDirectory::SetStringValue(const char *aName,
                                                  const nsACString &aValue)
{
	if (!m_DirectoryPrefs && NS_FAILED(InitDirectoryPrefs()))
		return NS_ERROR_NOT_INITIALIZED;

	return m_DirectoryPrefs->SetCharPref(aName, nsCString(aValue).get());
}

NS_IMETHODIMP ContactEwsDirectory::SetLocalizedStringValue(const char *aName,
                                                           const nsACString &aValue)
{
	if (!m_DirectoryPrefs && NS_FAILED(InitDirectoryPrefs()))
		return NS_ERROR_NOT_INITIALIZED;

	nsresult rv;
	nsCOMPtr<nsIPrefLocalizedString> locStr(
	    do_CreateInstance(NS_PREFLOCALIZEDSTRING_CONTRACTID, &rv));
	NS_ENSURE_SUCCESS(rv, rv);

	rv = locStr->SetData(NS_ConvertUTF8toUTF16(aValue).get());
	NS_ENSURE_SUCCESS(rv, rv);

	return m_DirectoryPrefs->SetComplexValue(aName,
	                                         NS_GET_IID(nsIPrefLocalizedString),
	                                         locStr);
}

#define kDefaultPosition 1
NS_IMETHODIMP ContactEwsDirectory::GetPosition(int32_t *aPosition)
{
	return GetIntValue("position", kDefaultPosition, aPosition);
}

NS_IMETHODIMP ContactEwsDirectory::ShouldUseForAutoComplete(const nsACString & aIdKey, bool * aResult)
{
	NS_ENSURE_ARG_POINTER(aResult);

    nsCString key;
    GetAccountIdentity(key);
    
    *aResult = key.Equals(aIdKey);

    return NS_OK;
}

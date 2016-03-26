/* -*- mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
#include "ContactEwsAutoCompleteSearch.h"

#include "nsThreadUtils.h"
#include "nsIProxyInfo.h"
#include "nsProxyRelease.h"
#include "nsDebug.h"
#include "nsIMsgWindow.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgMailSession.h"
#include "nsThreadUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsXPCOMCIDInternal.h"
#include "nsMsgFolderFlags.h"
#include "nsIAbManager.h"
#include "nsAbBaseCID.h"
#include "nsIAbDirectory.h"
#include "nsNetUtil.h"
#include "nsIMsgIdentity.h"
#include "nsIMsgAccountManager.h"
#include "nsISimpleEnumerator.h"
#include "nsIAbDirectorySearch.h"
#include "nsIAbDirectoryQuery.h"
#include "nsIAbBooleanExpression.h"

#include "IContactEwsDirectory.h"

#include "ContactEwsAutoCompleteResult.h"

NS_IMPL_ISUPPORTS(ContactEwsAutoCompleteSearch, nsIAutoCompleteSearch, nsIAbDirSearchListener)

ContactEwsAutoCompleteSearch::ContactEwsAutoCompleteSearch()
: m_Result(NULL)
    , m_Listener(NULL)
    , m_SearchContext(-1)
{

}

ContactEwsAutoCompleteSearch::~ContactEwsAutoCompleteSearch()
{

}

extern
nsresult
parse_params(const nsCString & jsonData,
             nsCString & idKey,
             nsCString & accountKey,
             nsCString & type);

/* void startSearch (in AString searchString, in AString searchParam, in nsIAutoCompleteResult previousResult, in nsIAutoCompleteObserver listener); */
NS_IMETHODIMP ContactEwsAutoCompleteSearch::StartSearch(const nsAString & searchString,
                                                        const nsAString & searchParam,
                                                        nsIAutoCompleteResult *previousResult,
                                                        nsIAutoCompleteObserver *listener)
{
  //Generate query string
  nsresult rv = NS_OK;

  nsCString encSearchString;
  nsCString cSearchParam = NS_ConvertUTF16toUTF8(searchParam);
  nsCString idKey, accountKey, type;
  
  rv = parse_params(cSearchParam,
                    idKey,
                    accountKey,
                    type);

  m_Listener = listener;
  m_Result = new ContactEwsAutoCompleteResult(searchString);
  
  nsCOMPtr<nsINetUtil> nu = do_GetService(NS_NETUTIL_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = nu->EscapeURL(NS_ConvertUTF16toUTF8(searchString),
                     nsINetUtil::ESCAPE_URL_QUERY,
                     encSearchString);
	NS_ENSURE_SUCCESS(rv, rv);

  nsCString query("");
  /*
    var query = "(or(PrimaryEmail,c,"+searchString+")(DisplayName,c,"+searchString+")(FirstName,c,"+searchString+")(LastName,c,"+searchString+"))";
  */
  query.AppendLiteral("(or(PrimaryEmail,c,");
  query.Append(encSearchString);
  query.AppendLiteral(")(DisplayName,c,");
  query.Append(encSearchString);
  query.AppendLiteral(")(FirstName,c,");
  query.Append(encSearchString);
  query.AppendLiteral(")(LastName,c,");
  query.Append(encSearchString);
  query.AppendLiteral("))");
  
	nsCOMPtr<nsIAbManager> abManager(do_GetService(NS_ABMANAGER_CONTRACTID,
	                                               &rv));
	NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISimpleEnumerator> enumerator;
  rv = abManager->GetDirectories(getter_AddRefs(enumerator));
	NS_ENSURE_SUCCESS(rv, rv);

  bool hasMore = false;
  while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMore)) &&
         hasMore) {
    nsCOMPtr<nsISupports> ele;
    enumerator->GetNext(getter_AddRefs(ele));

    if (!ele) {
      continue;
    }

    nsCOMPtr<nsIAbDirectory> dir =
        do_QueryInterface(ele);

    if (!dir)
      continue;
    
    nsCOMPtr<nsIAbDirectoryQuery> query_dir =
        do_QueryInterface(dir, &rv);

    if (NS_FAILED(rv))
      continue;

    nsCOMPtr<IContactEwsDirectory> ews_dir =
        do_QueryInterface(query_dir, &rv);

    if (NS_FAILED(rv))
      continue;

    if (!query_dir || !ews_dir)
      continue;

    bool useForAutoComplete = false;
    
    rv = ews_dir->ShouldUseForAutoComplete(idKey, &useForAutoComplete);

    if (NS_FAILED(rv) || !useForAutoComplete)
      continue;
    
    nsCOMPtr<nsIAbBooleanExpression> expression ;

    rv = abManager->ConvertQueryStringToExpression(query,
                                                   getter_AddRefs(expression)) ;
    NS_ENSURE_SUCCESS(rv, rv) ;

    nsCOMPtr<nsIAbDirectoryQueryArguments> arguments = do_CreateInstance(NS_ABDIRECTORYQUERYARGUMENTS_CONTRACTID,&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = arguments->SetExpression(expression) ;
    NS_ENSURE_SUCCESS(rv, rv) ;

    rv = query_dir->DoQuery(dir,
                            arguments,
                            this,
                            -1,
                            0,
                            &m_SearchContext);
    NS_ENSURE_SUCCESS(rv, rv) ;
    break;
  }
  
  nsString msg;
  msg.AppendInt(m_SearchContext, 10);
  msg.Append(NS_LITERAL_STRING(":Query Complete"));

  m_SearchFinishMsg = msg;
  
  return NS_OK;
}

/* void stopSearch (); */
NS_IMETHODIMP ContactEwsAutoCompleteSearch::StopSearch()
{
  m_Listener = NULL;
  m_Result = NULL;
  m_SearchContext = -1;
  m_SearchFinishMsg.Assign(NS_LITERAL_STRING(""));
  return NS_OK;
}

// nsIAbDirSearchListener
NS_IMETHODIMP ContactEwsAutoCompleteSearch::OnSearchFinished(int32_t aResult,
                                                             const nsAString &aErrorMsg)
{
  if (!m_Listener || !m_SearchFinishMsg.Equals(aErrorMsg))
    return NS_OK;

  nsCOMPtr<IContactEwsAutoCompleteResult> ewsResult(do_QueryInterface(m_Result));

  if (!ewsResult)
    return NS_OK;
  
  if (aResult == nsIAbDirectoryQueryResultListener::queryResultComplete) {
    ewsResult->SetSearchResult(nsIAutoCompleteResult::RESULT_SUCCESS);
  } else if (aResult == nsIAbDirectoryQueryResultListener::queryResultError) {
    ewsResult->SetSearchResult(nsIAutoCompleteResult::RESULT_FAILURE);
  }

  m_Listener->OnSearchResult(this, m_Result);

  StopSearch();
	return NS_OK;
}

NS_IMETHODIMP ContactEwsAutoCompleteSearch::OnSearchFoundCard(nsIAbCard *aCard) 
{
  if (!m_Listener)
    return NS_OK;

  int32_t searchContext;
  aCard->GetPropertyAsUint32("EwsSearchContext", (uint32_t*)&searchContext);

  if (m_SearchContext != searchContext)
    return NS_OK;
  
  nsCOMPtr<IContactEwsAutoCompleteResult> ewsResult(do_QueryInterface(m_Result));

  if (!ewsResult)
    return NS_OK;

  ewsResult->AddCard(aCard);
  
	return NS_OK;
}

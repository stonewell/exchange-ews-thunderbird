#include "nsXPCOMCIDInternal.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIMsgIncomingServer.h"
#include "nsArrayUtils.h"

#include "libews.h"

#include "MailEwsResolveNamesTask.h"

#include "IMailEwsTaskCallback.h"
#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"

#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsAbBaseCID.h"
#include "nsIAbDirSearchListener.h"
#include "nsIAbDirectory.h"
#include "nsIAbDirectoryQuery.h"

#include "MailEwsLog.h"

static
nsresult
EwsContactToCard(ews_contact_item * item,
                 nsIAbCard ** newCard);

class ResolveNamesDoneTask : public MailEwsRunnable
{
public:
	ResolveNamesDoneTask(nsIRunnable * runnable,
	                     int result,
	                     ews_item ** items,
	                     int item_count,
	                     int32_t aResultLimit,
	                     const nsACString & uuid,
                         bool syncCall,
	                     nsIAbDirSearchListener * aListener,
                         nsIAbDirectory * aParent,
                         int32_t searchContext,
	                     nsISupportsArray * ewsTaskCallbacks)
		: MailEwsRunnable(ewsTaskCallbacks)
		, m_Runnable(runnable)
		, m_Result(result)
		, m_pListener(aListener)
		, m_items(items)
		, m_item_count(item_count)
		, m_ResultLimit(aResultLimit)
		, m_Uuid(uuid)
        , m_pParent(aParent)
        , m_SyncCall(syncCall)
        , m_SearchContext(searchContext)
	{
	}

	NS_IMETHOD Run() {
		if (m_Result == EWS_SUCCESS) {
			uint32_t nbResults = m_item_count;

			if (m_ResultLimit > 0 && nbResults > static_cast<uint32_t>(m_ResultLimit)) { 
				nbResults = static_cast<uint32_t>(m_ResultLimit) ; 
			}

			nsCOMPtr<nsIAbCard> card;

			for (uint32_t i = 0 ; i < nbResults ; ++ i) {
				ews_contact_item * p = (ews_contact_item*)m_items[i];
				nsresult retCode = EwsContactToCard(p, getter_AddRefs(card));
				if (NS_FAILED(retCode))
					continue;
                bool isMailList = false;
                card->GetIsMailList(&isMailList);
                if (isMailList) {
                    nsCString uri;
                    m_pParent->GetURI(uri);
                    uri.AppendLiteral("/");
                    uri.AppendLiteral(p->item.item_id);
                    card->SetMailListURI(uri.get());
                }
				card->SetDirectoryId(m_Uuid);

                card->SetPropertyAsBool("EwsSyncCall", m_SyncCall);
	            card->SetPropertyAsUint32("EwsSearchContext", m_SearchContext);
				m_pListener->OnSearchFoundCard(card);
			}
		}

        nsString msg;
        msg.AppendInt(m_SearchContext, 10);
        msg.Append(NS_LITERAL_STRING(":Query Complete"));
        
        m_pListener->OnSearchFinished(
            nsIAbDirectoryQueryResultListener::queryResultComplete,
            msg);
		if (m_items) ews_free_items(m_items, m_item_count);

        //Resolve names never report fail
		NotifyEnd(m_Runnable, EWS_SUCCESS);

		return NS_OK;
	}

private:
	nsCOMPtr<nsIRunnable> m_Runnable;
	int m_Result;
	nsCOMPtr<nsIAbDirSearchListener> m_pListener;
	ews_item ** m_items;
	int m_item_count;
	int32_t m_ResultLimit;
	nsCString m_Uuid;
    nsCOMPtr<nsIAbDirectory> m_pParent;
    bool m_SyncCall;
    int32_t m_SearchContext;
};


ResolveNamesTask::ResolveNamesTask(const nsACString & unresolvedEntry,
                                   nsIAbDirSearchListener * aListener,
                                   nsIAbDirectory * aParent,
                                   const nsACString & uuid,
                                   int32_t aResultLimit,
                                   bool doSyncCall,
                                   nsIMsgIncomingServer * pIncomingServer,
                                   int32_t searchContext,
                                   nsISupportsArray * ewsTaskCallbacks)
	: MailEwsRunnable(ewsTaskCallbacks)
	, m_pIncomingServer(pIncomingServer)
	, m_UnresolvedEntry(unresolvedEntry)
	, m_pListener(aListener)
	, m_ResultLimit(aResultLimit)
	, m_Uuid(uuid)
    , m_DoSyncCall(doSyncCall)
    , m_pParent(aParent)
    , m_SearchContext(searchContext) {
}

ResolveNamesTask::~ResolveNamesTask() {
}
	
NS_IMETHODIMP ResolveNamesTask::Run() {
	int ret = EWS_FAIL;
	char * err_msg = NULL;
	nsresult rv;

	if (EWS_FAIL == NotifyBegin(this)) {
		return NS_OK;
	}

    m_DoSyncCall =  (m_DoSyncCall && NS_IsMainThread());
	
	ews_session * session = NULL;
	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(m_pIncomingServer, &rv));
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<IMailEwsService> ewsService;
	rv = ewsServer->GetService(getter_AddRefs(ewsService));
	NS_ENSURE_SUCCESS(rv, rv);

	nsresult rv1 = NS_OK;

	rv1 = ewsService->GetNewSession(&session);

    ews_item ** pItems = NULL;
    int item_count = 0;

    //Resolve names never report error, if fail, just fail.
	if (NS_SUCCEEDED(rv)
	    && NS_SUCCEEDED(rv1)
	    && session) {
		
		ret = ews_resolve_names(session,
		                        m_UnresolvedEntry.get(),
		                        &pItems,
		                        item_count,
		                        &err_msg);
		NotifyError(this, EWS_SUCCESS, err_msg);
	} else {
		NotifyError(this, EWS_SUCCESS, err_msg);
	}

	if (err_msg) free(err_msg);

	nsCOMPtr<nsIRunnable> resultrunnable =
			new ResolveNamesDoneTask(this,
			                         ret,
			                         pItems,
			                         item_count,
			                         m_ResultLimit,
			                         m_Uuid,
                                     m_DoSyncCall,
			                         m_pListener,
                                     m_pParent,
                                     m_SearchContext,
			                         m_pEwsTaskCallbacks);

    if (m_DoSyncCall) {
        resultrunnable->Run();
    } else {
        NS_DispatchToMainThread(resultrunnable);
    }

    ewsService->ReleaseSession(session);
    
	return NS_OK;
}

static
nsresult
EwsContactToCard(ews_contact_item * item,
                 nsIAbCard ** newCard) {
  nsresult rv;
  nsCOMPtr<nsIAbCard> card = do_CreateInstance(NS_ABCARDPROPERTY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString v;
#define ASSIGN_VALUE(p, vv) \
  if (vv) \
      v.AssignLiteral(vv); \
  else \
      v.AssignLiteral(""); \
  card->Set##p(v);

  ASSIGN_VALUE(FirstName, item->complete_name.first_name);
  ASSIGN_VALUE(LastName, item->complete_name.last_name);
  ASSIGN_VALUE(DisplayName, item->display_name);
  ASSIGN_VALUE(PrimaryEmail, item->email_address);
  
#define ASSIGN_VALUE_P(p, vv) \
  if (vv) \
      v.AssignLiteral(vv); \
  else \
      v.AssignLiteral(""); \
  card->SetPropertyAsAString(k##p##Property, v);

  ASSIGN_VALUE_P(Nickname, item->nick_name);

#define ASSIGN_LIST_VALUE(p, l, i)            \
  if (l##_count > i)                          \
      card->SetPropertyAsAUTF8String(k##p##Property, nsCString(l[i]));

  ASSIGN_LIST_VALUE(WorkPhone, item->phone_numbers, 2);
  ASSIGN_LIST_VALUE(HomePhone, item->phone_numbers, 8);
  ASSIGN_LIST_VALUE(Fax, item->phone_numbers, 1);
  ASSIGN_LIST_VALUE(Pager, item->phone_numbers, 14);
  ASSIGN_LIST_VALUE(Cellular, item->phone_numbers, 11);

  ASSIGN_LIST_VALUE(Company, item->companies, 0);

  ASSIGN_VALUE_P(JobTitle, item->job_title);
  ASSIGN_VALUE_P(Department, item->department);

  card->SetIsMailList(!!item->is_maillist);

  // card->SetPropertyAsAString(kHomeCityProperty, unichars[index_HomeCity]);
  // card->SetPropertyAsAString(kHomeStateProperty, unichars[index_HomeState]);
  // card->SetPropertyAsAString(kHomeZipCodeProperty, unichars[index_HomeZip]);
  // card->SetPropertyAsAString(kHomeCountryProperty, unichars[index_HomeCountry]);
  // card->SetPropertyAsAString(kWorkCityProperty, unichars[index_WorkCity]);
  // card->SetPropertyAsAString(kWorkStateProperty, unichars[index_WorkState]);
  // card->SetPropertyAsAString(kWorkZipCodeProperty, unichars[index_WorkZip]);
  // card->SetPropertyAsAString(kWorkCountryProperty, unichars[index_WorkCountry]);
  // card->SetPropertyAsAString(kWorkWebPageProperty, unichars[index_WorkWebPage]);
  // card->SetPropertyAsAString(kHomeWebPageProperty, unichars[index_HomeWebPage]);
  // card->SetPropertyAsAString(kNotesProperty, unichars[index_Comments]);

  card.swap(*newCard);
  return NS_OK;
}


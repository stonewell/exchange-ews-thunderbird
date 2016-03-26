#include "CalEwsCalendarACLEntry.h"

#include "nsIMsgIncomingServer.h"
#include "nsIURI.h"
#include "nsIMsgIdentity.h"
#include "nsStringAPI.h"
#include "nsIMsgAccountManager.h"
#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsIMsgAccountManager.h"
#include "nsIArray.h"
#include "nsMsgBaseCID.h"

/* Implementation file */
NS_IMPL_ISUPPORTS(CalEwsCalendarACLEntry, calICalendarACLEntry)

CalEwsCalendarACLEntry::CalEwsCalendarACLEntry(calICalendar * calendar)
: m_Calendar(calendar)
{
}

CalEwsCalendarACLEntry::~CalEwsCalendarACLEntry()
{
}

/* readonly attribute calICalendarACLManager aclManager; */
NS_IMETHODIMP CalEwsCalendarACLEntry::GetAclManager(calICalendarACLManager * *aAclManager)
{
    return m_Calendar->GetAclManager(aAclManager);
}

/* readonly attribute boolean hasAccessControl; */
NS_IMETHODIMP CalEwsCalendarACLEntry::GetHasAccessControl(bool *aHasAccessControl)
{
    *aHasAccessControl = false;
    return NS_OK;
}

/* readonly attribute boolean userIsOwner; */
NS_IMETHODIMP CalEwsCalendarACLEntry::GetUserIsOwner(bool *aUserIsOwner)
{
    *aUserIsOwner = true;
    return NS_OK;
}

/* readonly attribute boolean userCanAddItems; */
NS_IMETHODIMP CalEwsCalendarACLEntry::GetUserCanAddItems(bool *aUserCanAddItems)
{
    *aUserCanAddItems = true;
    return NS_OK;
}

/* readonly attribute boolean userCanDeleteItems; */
NS_IMETHODIMP CalEwsCalendarACLEntry::GetUserCanDeleteItems(bool *aUserCanDeleteItems)
{
    *aUserCanDeleteItems = true;
    return NS_OK;
}

const char16_t * user_address[] = {
    (const char16_t *)L"User address",
};

/* void getUserAddresses (out uint32_t aCount, [array, size_is (aCount), retval] out wstring aAddresses); */
NS_IMETHODIMP CalEwsCalendarACLEntry::GetUserAddresses(uint32_t *aCount, char16_t * **aAddresses)
{
    *aCount = 0;
    *aAddresses = new char16_t *[0];
    return NS_OK;
}

/* void getUserIdentities (out uint32_t aCount, [array, size_is (aCount), retval] out nsIMsgIdentity aIdentities); */
NS_IMETHODIMP CalEwsCalendarACLEntry::GetUserIdentities(uint32_t *aCount, nsIMsgIdentity * **aIdentities)
{
    if (!m_Identity) {
        nsresult rv = GetIdentity();
        NS_ENSURE_SUCCESS(rv, rv);
    }

    *aCount = m_Identity ? 1 : 0;

    *aIdentities = nullptr;
    
    if (*aCount > 0) {
        *aIdentities = (nsIMsgIdentity**)malloc((*aCount) * sizeof(nsIMsgIdentity*));
        (*aIdentities)[0] = m_Identity;
    }
    
    return NS_OK;
}

/* void getOwnerIdentities (out uint32_t aCount, [array, size_is (aCount), retval] out nsIMsgIdentity aIdentities); */
NS_IMETHODIMP CalEwsCalendarACLEntry::GetOwnerIdentities(uint32_t *aCount, nsIMsgIdentity * **aIdentities)
{
    if (!m_Identity) {
        nsresult rv = GetIdentity();
        NS_ENSURE_SUCCESS(rv, rv);
    }

    *aCount = m_Identity ? 1 : 0;

    *aIdentities = nullptr;
    
    if (*aCount > 0) {
        *aIdentities = (nsIMsgIdentity**)malloc((*aCount) * sizeof(nsIMsgIdentity*));
        (*aIdentities)[0] = m_Identity;
    }
    
    return NS_OK;
}

/* void refresh (); */
NS_IMETHODIMP CalEwsCalendarACLEntry::Refresh()
{
    return NS_OK;
}

NS_IMETHODIMP
CalEwsCalendarACLEntry::GetIdentity() {
    //Find username and host name
    nsCString userName;
    nsCString spec;

    nsCOMPtr<nsIURI> m_Uri;

    nsresult rv = m_Calendar->GetUri(getter_AddRefs(m_Uri));
    NS_ENSURE_SUCCESS(rv, rv);
    
    m_Uri->GetSpec(spec);
    
    userName = Substring(spec,
                         strlen("calendarews://"));
    
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

    retCode = accountMgr->GetFirstIdentityForServer(incomingServer,
                                                 getter_AddRefs(m_Identity));
    return retCode;
}

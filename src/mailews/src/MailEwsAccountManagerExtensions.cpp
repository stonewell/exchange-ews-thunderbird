#include "nsIMsgIncomingServer.h"
#include "nsIServiceManager.h"
#include "nsIDirectoryService.h"
#include "nsIProperties.h"
#include "nsIFile.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include "nsEmbedString.h"
#include "MailEwsAccountManagerExtensions.h"

NS_IMPL_ISUPPORTS(MailEwsServerExt, nsIMsgAccountManagerExtension)

MailEwsServerExt::MailEwsServerExt()
{
}

MailEwsServerExt::~MailEwsServerExt()
{
}

NS_IMETHODIMP MailEwsServerExt::GetName(nsACString & aName)
{
  aName.AssignLiteral("ewsserver");
  return NS_OK;
}

/* boolean showPanel (in nsIMsgIncomingServer server); */
NS_IMETHODIMP MailEwsServerExt::ShowPanel(nsIMsgIncomingServer *server, bool *_retval)
{
  nsCString type;

  *_retval = false;

  if (server->GetType(type) == NS_OK) {
    *_retval = (type == "ews");
  }

  return NS_OK;
}

/* readonly attribute ACString chromePackageName; */
NS_IMETHODIMP MailEwsServerExt::GetChromePackageName(nsACString & aChromePackageName)
{
  aChromePackageName.AssignLiteral("mailews");
return NS_OK;
}

NS_IMPL_ISUPPORTS(MailEwsAuthExt, nsIMsgAccountManagerExtension)

MailEwsAuthExt::MailEwsAuthExt()
{
}

MailEwsAuthExt::~MailEwsAuthExt()
{
}

NS_IMETHODIMP MailEwsAuthExt::GetName(nsACString & aName)
{
  aName.AssignLiteral("ewsauth");
  return NS_OK;
}

/* boolean showPanel (in nsIMsgIncomingAuth server); */
NS_IMETHODIMP MailEwsAuthExt::ShowPanel(nsIMsgIncomingServer *server, bool *_retval)
{
  nsCString type;

  *_retval = false;

  if (server->GetType(type) == NS_OK) {
    *_retval = (type == "ews");
  }

  return NS_OK;
}

/* readonly attribute ACString chromePackageName; */
NS_IMETHODIMP MailEwsAuthExt::GetChromePackageName(nsACString & aChromePackageName)
{
  aChromePackageName.AssignLiteral("mailews");
    return NS_OK;
}


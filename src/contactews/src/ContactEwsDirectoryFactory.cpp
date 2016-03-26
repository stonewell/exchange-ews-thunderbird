#include "ContactEwsDirectoryFactory.h"

#include "nsServiceManagerUtils.h"
#include "nsIAbManager.h"
#include "nsIAbDirectory.h"

#include "nsEnumeratorUtils.h"
#include "nsAbBaseCID.h"

#include "ContactEwsCID.h"

NS_IMPL_ISUPPORTS(ContactEwsDirectoryFactory, nsIAbDirFactory)

ContactEwsDirectoryFactory::ContactEwsDirectoryFactory()
{
}

ContactEwsDirectoryFactory::~ContactEwsDirectoryFactory()
{
}

NS_IMETHODIMP
ContactEwsDirectoryFactory::GetDirectories(const nsAString &aDirName,
                                   const nsACString &aURI,
                                   const nsACString &aPrefName,
                                   nsISimpleEnumerator **aDirectories)
{
  NS_ENSURE_ARG_POINTER(aDirectories);

  nsresult rv;
  nsCOMPtr<nsIAbManager> abManager(do_GetService(NS_ABMANAGER_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIAbDirectory> directory;
  rv = abManager->GetDirectory(aURI, getter_AddRefs(directory));
  NS_ENSURE_SUCCESS(rv, rv);

  directory->SetDirPrefId(aPrefName);
  
  return NS_NewSingletonEnumerator(aDirectories, directory);
}

/* void deleteDirectory (in nsIAbDirectory directory); */
NS_IMETHODIMP
ContactEwsDirectoryFactory::DeleteDirectory(nsIAbDirectory *directory)
{
  // No actual deletion - as the LDAP Address Book is not physically
  // created in the corresponding CreateDirectory() unlike the Personal
  // Address Books. But we still need to return NS_OK from here.
  return NS_OK;
}

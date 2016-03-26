#include "mozilla/ModuleUtils.h"

#include "nsServiceManagerUtils.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsILocalFile.h"
#include "nsComponentManagerUtils.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include "ContactEwsCID.h"
#include "ContactEwsDirectory.h"
#include "ContactEwsDirectoryFactory.h"
#include "ContactEwsAutoCompleteSearch.h"

NS_DEFINE_NAMED_CID(CONTACT_EWS_DIRECTORY_CID);
NS_DEFINE_NAMED_CID(CONTACT_EWS_DIRECTORY_AUTO_COMPLETE_CID);
NS_DEFINE_NAMED_CID(CONTACT_EWS_DIRECTORY_FACTORY_CID);
NS_DEFINE_NAMED_CID(CONTACT_EWS_AUTO_COMPLETE_SEARCH_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(ContactEwsDirectory);
NS_GENERIC_FACTORY_CONSTRUCTOR(ContactEwsDirectoryFactory);
NS_GENERIC_FACTORY_CONSTRUCTOR(ContactEwsAutoCompleteSearch);

static const mozilla::Module::CIDEntry kContactEwsCIDs[] = {
  { &kCONTACT_EWS_DIRECTORY_CID, false, nullptr, ContactEwsDirectoryConstructor },
  { &kCONTACT_EWS_DIRECTORY_FACTORY_CID, false, nullptr, ContactEwsDirectoryFactoryConstructor },
  { &kCONTACT_EWS_DIRECTORY_AUTO_COMPLETE_CID, false, nullptr, ContactEwsDirectoryConstructor },
  { &kCONTACT_EWS_AUTO_COMPLETE_SEARCH_CID, false, nullptr, ContactEwsAutoCompleteSearchConstructor },
  { nullptr }
};

static const mozilla::Module::ContractIDEntry kContactEwsContracts[] = {
  { CONTACT_EWS_DIRECTORY_CONTRACTID, &kCONTACT_EWS_DIRECTORY_CID },
  { CONTACT_EWS_DIRECTORY_FACTORY_CONTRACTID, &kCONTACT_EWS_DIRECTORY_FACTORY_CID },
  { CONTACT_EWS_DIRECTORY_AUTO_COMPLETE_CONTRACTID, &kCONTACT_EWS_DIRECTORY_AUTO_COMPLETE_CID },
  { CONTACT_EWS_DIRECTORY_AUTO_COMPLETE_FACTORY_CONTRACTID, &kCONTACT_EWS_DIRECTORY_FACTORY_CID },
  { CONTACT_EWS_AUTO_COMPLETE_SEARCH_CONTRACTID, &kCONTACT_EWS_AUTO_COMPLETE_SEARCH_CID },
  { nullptr }
};

static nsresult ModuleLoadFunc() {
	nsresult rv = NS_OK;
	return rv;
}

static const mozilla::Module kContactEwsModule = {
  mozilla::Module::kVersion,
  kContactEwsCIDs,
  kContactEwsContracts,
  nullptr,
  nullptr,
  ModuleLoadFunc,
  nullptr
};

NSMODULE_DEFN(nsContactEwsModule) = &kContactEwsModule;

NS_IMPL_MOZILLA192_NSGETMODULE(&kContactEwsModule)


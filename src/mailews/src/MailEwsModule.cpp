#include <stdio.h>

#include "mozilla/ModuleUtils.h"
#include "MailEwsCID.h"
#include "MailEwsMsgProtocolInfo.h"
#include "MailEwsIncomingServer.h"
#include "MailEwsMsgFolder.h"
#include "MailEwsAccountManagerExtensions.h"
#include "MailEwsService.h"
#include "MailEwsMsgMessageService.h"
#include "MailEwsMsgUrl.h"
#include "MailEwsMsgProtocol.h"
#include "MailEwsItemsCallback.h"

#include "nsServiceManagerUtils.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsILocalFile.h"
#include "nsComponentManagerUtils.h"

#ifdef _WIN32
#include <windows.h>
#endif

NS_DEFINE_NAMED_CID(IMAILEWSMSGFOLDER_IID);
NS_DEFINE_NAMED_CID(IMAILEWSSERVICE_IID);
NS_DEFINE_NAMED_CID(IMAILEWSMSGINCOMINGSERVER_IID);
NS_DEFINE_NAMED_CID(MAIL_EWS_SERVER_EXT_CID);
NS_DEFINE_NAMED_CID(MAIL_EWS_MSG_PROTOCOL_INFO_CID);
NS_DEFINE_NAMED_CID(MAIL_EWS_AUTH_EXT_CID);
NS_DEFINE_NAMED_CID(MAIL_EWS_MSG_MESSAGE_SERVICE_CID);
NS_DEFINE_NAMED_CID(MAIL_EWS_MSG_URL_CID);
NS_DEFINE_NAMED_CID(IMAILEWSITEMSCALLBACK_IID);

NS_GENERIC_FACTORY_CONSTRUCTOR(MailEwsMsgProtocolInfo);
NS_GENERIC_FACTORY_CONSTRUCTOR(MailEwsMsgIncomingServer);
NS_GENERIC_FACTORY_CONSTRUCTOR(MailEwsMsgFolder);
NS_GENERIC_FACTORY_CONSTRUCTOR(MailEwsServerExt);
NS_GENERIC_FACTORY_CONSTRUCTOR(MailEwsAuthExt);
NS_GENERIC_FACTORY_CONSTRUCTOR(MailEwsService);
NS_GENERIC_FACTORY_CONSTRUCTOR(MailEwsMsgMessageService);
NS_GENERIC_FACTORY_CONSTRUCTOR(MailEwsMsgUrl);
NS_GENERIC_FACTORY_CONSTRUCTOR(MailEwsItemsCallback);

static const mozilla::Module::CIDEntry kMailEwsCIDs[] = {
  { &kMAIL_EWS_MSG_PROTOCOL_INFO_CID, false, nullptr, MailEwsMsgProtocolInfoConstructor },
  { &kIMAILEWSMSGINCOMINGSERVER_IID, false, nullptr, MailEwsMsgIncomingServerConstructor },
  { &kIMAILEWSMSGFOLDER_IID, false, nullptr, MailEwsMsgFolderConstructor },
  { &kMAIL_EWS_AUTH_EXT_CID, false, nullptr, MailEwsAuthExtConstructor },
  { &kMAIL_EWS_SERVER_EXT_CID, false, nullptr, MailEwsServerExtConstructor },
  { &kIMAILEWSSERVICE_IID, false, nullptr, MailEwsServiceConstructor },
  { &kMAIL_EWS_MSG_MESSAGE_SERVICE_CID, false, nullptr, MailEwsMsgMessageServiceConstructor },
  { &kMAIL_EWS_MSG_URL_CID, false, nullptr, MailEwsMsgUrlConstructor },
  { &kIMAILEWSITEMSCALLBACK_IID, false, nullptr, MailEwsItemsCallbackConstructor },
  { nullptr }
};

static const mozilla::Module::ContractIDEntry kMailEwsContracts[] = {
  { MAIL_EWS_MSG_PROTOCOL_INFO_CONTRACTID, &kMAIL_EWS_MSG_PROTOCOL_INFO_CID },
  { MAIL_EWS_MSG_INCOMING_SERVER_CONTRACTID, &kIMAILEWSMSGINCOMINGSERVER_IID },
  { MAIL_EWS_MSG_FOLDER_CONTRACTID, &kIMAILEWSMSGFOLDER_IID },
  //  { MAIL_EWS_MSG_DATABASE_CONTRACTID, &kMAIL_EWS_MSG_DATABASE_CID },
  { MAIL_EWS_AUTH_EXT_CONTRACTID, &kMAIL_EWS_AUTH_EXT_CID },
  { MAIL_EWS_SERVER_EXT_CONTRACTID, &kMAIL_EWS_SERVER_EXT_CID },
  { MAIL_EWS_SERVICE_CONTRACTID, &kIMAILEWSSERVICE_IID },
  { MAIL_EWS_MSG_MESSAGE_SERVICE_CONTRACTID, &kMAIL_EWS_MSG_MESSAGE_SERVICE_CID },
  { MAIL_EWS_MSG_MESSAGE_SERVICE_CONTRACTID2, &kMAIL_EWS_MSG_MESSAGE_SERVICE_CID },
  { MAIL_EWS_MSG_MESSAGE_SERVICE_CONTRACTID3, &kMAIL_EWS_MSG_MESSAGE_SERVICE_CID },
  { MAIL_EWS_MSG_MESSAGE_SERVICE_CONTRACTID4, &kMAIL_EWS_MSG_MESSAGE_SERVICE_CID },
  { MAIL_EWS_MSG_URL_CONTRACTID, &kMAIL_EWS_MSG_URL_CID },
  { MAIL_EWS_ITEMS_CALLBACK_CONTRACTID, &kIMAILEWSITEMSCALLBACK_IID },
  //  { MAIL_EWS_MSGDB_SERVICE_CONTRACTID, &kMAIL_EWS_MSGDB_SERVICE_CID },
  { nullptr }
};

static nsresult ModuleLoadFunc() {
	nsresult rv = NS_OK;
#ifdef _WIN32    
    HMODULE hMailEws = GetModuleHandle("mailews.dll");
    if (!hMailEws)
        return NS_ERROR_FAILURE;

    char cpath[256] = {0};

    if (!GetModuleFileName(hMailEws, cpath, 255)) {
        return NS_ERROR_FAILURE;
    }

	nsCOMPtr<nsILocalFile> mailEwsPath;

    mailEwsPath = do_CreateInstance("@mozilla.org/file/local;1", &rv);
	NS_ENSURE_SUCCESS(rv, rv);

    nsString path;
    path.AssignLiteral(cpath);
    mailEwsPath->InitWithPath(path);

	nsCOMPtr<nsIFile> mailEwsParentPath;
	nsCOMPtr<nsIFile> ewsPath;

    rv = mailEwsPath->GetParent(getter_AddRefs(mailEwsParentPath));
	NS_ENSURE_SUCCESS(rv, rv);

    rv = mailEwsParentPath->GetParent(getter_AddRefs(ewsPath));
	NS_ENSURE_SUCCESS(rv, rv);
    
	ewsPath->Append(NS_LITERAL_STRING("chrome"));
	ewsPath->Append(NS_LITERAL_STRING("mailews"));
	ewsPath->Append(NS_LITERAL_STRING("lib"));
	ewsPath->Append(NS_LITERAL_STRING("ews.dll"));

	nsCOMPtr<nsILocalFile> localPath(do_QueryInterface(ewsPath));

    PRLibrary *lib;
	localPath->Load(&lib);
#endif    
	return rv;
}

static const mozilla::Module kMailEwsModule = {
  mozilla::Module::kVersion,
  kMailEwsCIDs,
  kMailEwsContracts,
  nullptr,
  nullptr,
  ModuleLoadFunc,
  nullptr
};

NSMODULE_DEFN(nsMailEwsModule) = &kMailEwsModule;

NS_IMPL_MOZILLA192_NSGETMODULE(&kMailEwsModule)


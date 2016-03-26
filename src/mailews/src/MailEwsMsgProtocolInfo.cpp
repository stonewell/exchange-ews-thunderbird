#include "nsIMsgIncomingServer.h"
#include "nsIServiceManager.h"
#include "nsIDirectoryService.h"
#include "nsIProperties.h"
#include "nsIFile.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include "MailEwsMsgProtocolInfo.h"
#include "nsEmbedString.h"

/* Implementation file */
NS_IMPL_ISUPPORTS(MailEwsMsgProtocolInfo, nsIMsgProtocolInfo)

MailEwsMsgProtocolInfo::MailEwsMsgProtocolInfo()
{
  /* member initializers and constructor code */
}

MailEwsMsgProtocolInfo::~MailEwsMsgProtocolInfo()
{
  /* destructor code */
}

/* attribute nsIFile defaultLocalPath; */
NS_IMETHODIMP MailEwsMsgProtocolInfo::GetDefaultLocalPath(nsIFile * *aDefaultLocalPath)
{
  //TODO: load the root folder from preference
  nsCOMPtr<nsIServiceManager> servMan;
  nsresult rv = NS_GetServiceManager(getter_AddRefs(servMan));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIProperties> properties;
  rv = servMan->GetServiceByContractID("@mozilla.org/file/directory_service;1",
                                       NS_GET_IID(nsIProperties),
                                       getter_AddRefs(properties));

  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIFile> file;

  rv = properties->Get("ProfD", NS_GET_IID(nsIFile), getter_AddRefs(file));

  if (NS_FAILED(rv))
    return rv;

  nsEmbedString data;
  data.AppendASCII("MailEws");

  rv = file->Append(data);

  if (NS_FAILED(rv))
    return rv;

  bool exists = false;

  rv = file->Exists(&exists);

  if (NS_FAILED(rv))
    return rv;

  if (!exists) {
    rv = file->Create(nsIFile::DIRECTORY_TYPE, 0775);
    
    if (NS_FAILED(rv))
      return rv;
  }

  *aDefaultLocalPath = file.get();

  NS_IF_ADDREF(*aDefaultLocalPath);
  return rv;
}

NS_IMETHODIMP MailEwsMsgProtocolInfo::SetDefaultLocalPath(nsIFile *aDefaultLocalPath)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsIIDPtr serverIID; */
NS_IMETHODIMP MailEwsMsgProtocolInfo::GetServerIID(nsIID **aServerIID)
{
  *aServerIID = const_cast<nsIID*>(&NS_GET_IID(nsIMsgIncomingServer));
  return NS_OK;
}

/* readonly attribute boolean requiresUsername; */
NS_IMETHODIMP MailEwsMsgProtocolInfo::GetRequiresUsername(bool *aRequiresUsername)
{
  *aRequiresUsername = true;
  return NS_OK;
}

/* readonly attribute boolean preflightPrettyNameWithEmailAddress; */
NS_IMETHODIMP MailEwsMsgProtocolInfo::GetPreflightPrettyNameWithEmailAddress(bool *aPreflightPrettyNameWithEmailAddress)
{
  *aPreflightPrettyNameWithEmailAddress = true;
  return NS_OK;
}

/* readonly attribute boolean canDelete; */
NS_IMETHODIMP MailEwsMsgProtocolInfo::GetCanDelete(bool *aCanDelete)
{
  *aCanDelete = true;
  return NS_OK;
}

/* readonly attribute boolean canLoginAtStartUp; */
NS_IMETHODIMP MailEwsMsgProtocolInfo::GetCanLoginAtStartUp(bool *aCanLoginAtStartUp)
{
  *aCanLoginAtStartUp = true;
  return NS_OK;
}

/* readonly attribute boolean canDuplicate; */
NS_IMETHODIMP MailEwsMsgProtocolInfo::GetCanDuplicate(bool *aCanDuplicate)
{
  *aCanDuplicate = true;
  return NS_OK;
}

/* long getDefaultServerPort (in boolean isSecure); */
NS_IMETHODIMP MailEwsMsgProtocolInfo::GetDefaultServerPort(bool isSecure, int32_t *_retval)
{
  *_retval = true;
  return NS_OK;
}

/* readonly attribute boolean canGetMessages; */
NS_IMETHODIMP MailEwsMsgProtocolInfo::GetCanGetMessages(bool *aCanGetMessages)
{
  *aCanGetMessages = true;
  return NS_OK;
}

/* readonly attribute boolean canGetIncomingMessages; */
NS_IMETHODIMP MailEwsMsgProtocolInfo::GetCanGetIncomingMessages(bool *aCanGetIncomingMessages)
{
  *aCanGetIncomingMessages = true;
  return NS_OK;
}

/* readonly attribute boolean defaultDoBiff; */
NS_IMETHODIMP MailEwsMsgProtocolInfo::GetDefaultDoBiff(bool *aDefaultDoBiff)
{
  *aDefaultDoBiff = true;
  return NS_OK;
}

/* readonly attribute boolean showComposeMsgLink; */
NS_IMETHODIMP MailEwsMsgProtocolInfo::GetShowComposeMsgLink(bool *aShowComposeMsgLink)
{
  *aShowComposeMsgLink = true;
  return NS_OK;
}

/* readonly attribute boolean foldersCreatedAsync; */
NS_IMETHODIMP MailEwsMsgProtocolInfo::GetFoldersCreatedAsync(bool *aFoldersCreatedAsync)
{
  *aFoldersCreatedAsync = false;
  return NS_OK;
}


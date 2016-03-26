/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "msgCore.h"    // precompiled header...

#include "nsIURI.h"
#include "nsIMailboxUrl.h"
#include "MailEwsMsgUrl.h"

#include "nsStringGlue.h"
#include "nsIMsgDatabase.h"
#include "nsMsgDBCID.h"
#include "nsMsgBaseCID.h"
#include "nsIMsgHdr.h"

#include "nsIMsgFolder.h"
#include "prprf.h"
#include "prmem.h"
#include "nsIMsgMailSession.h"
#include "nsNetUtil.h"
#include "nsIFileURL.h"
#include "nsMsgUtils.h"

// this is totally lame and MUST be removed by M6
// the real fix is to attach the URI to the URL as it runs through netlib
// then grab it and use it on the other side
#include "nsCOMPtr.h"
#include "nsMsgBaseCID.h"
#include "nsIMsgAccountManager.h"
#include "MsgUtils.h"
#include "mozilla/Services.h"

#include "MailEwsLog.h"
#include "MailEwsMsgUtils.h"

// helper function for parsing the search field of a url
char * extractAttributeValue(const char * searchString, const char * attributeName);

MailEwsMsgUrl::MailEwsMsgUrl()
{
    m_mailboxAction = nsIMailboxUrl::ActionFetchMessage;
  m_messageID = nullptr;
  m_messageKey = nsMsgKey_None;
  m_messageSize = 0;
  m_messageFile = nullptr;
  m_addDummyEnvelope = false;
  m_canonicalLineEnding = false;
  m_curMsgIndex = 0;
}

MailEwsMsgUrl::~MailEwsMsgUrl()
{
    PR_Free(m_messageID);
}

NS_IMPL_ADDREF_INHERITED(MailEwsMsgUrl, MsgMailNewsUrlBase)
NS_IMPL_RELEASE_INHERITED(MailEwsMsgUrl, MsgMailNewsUrlBase)

NS_INTERFACE_MAP_BEGIN(MailEwsMsgUrl)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIMailboxUrl)
   NS_INTERFACE_MAP_ENTRY(nsIMailboxUrl)
   NS_INTERFACE_MAP_ENTRY(nsIMsgMessageUrl)
   NS_INTERFACE_MAP_ENTRY(nsIMsgI18NUrl)
NS_INTERFACE_MAP_END_INHERITING(MsgMailNewsUrlBase)

////////////////////////////////////////////////////////////////////////////////////
// Begin nsIMailboxUrl specific support
////////////////////////////////////////////////////////////////////////////////////
nsresult MailEwsMsgUrl::SetMailboxParser(nsIStreamListener * aMailboxParser)
{
  if (aMailboxParser)
    m_mailboxParser = aMailboxParser;
  return NS_OK;
}

nsresult MailEwsMsgUrl::GetMailboxParser(nsIStreamListener ** aConsumer)
{
  NS_ENSURE_ARG_POINTER(aConsumer);

  NS_IF_ADDREF(*aConsumer = m_mailboxParser);
  return  NS_OK;
}

nsresult MailEwsMsgUrl::SetMailboxCopyHandler(nsIStreamListener * aMailboxCopyHandler)
{
  if (aMailboxCopyHandler)
    m_mailboxCopyHandler = aMailboxCopyHandler;
  return NS_OK;
}

nsresult MailEwsMsgUrl::GetMailboxCopyHandler(nsIStreamListener ** aMailboxCopyHandler)
{
  NS_ENSURE_ARG_POINTER(aMailboxCopyHandler);

  if (aMailboxCopyHandler)
  {
    *aMailboxCopyHandler = m_mailboxCopyHandler;
    NS_IF_ADDREF(*aMailboxCopyHandler);
  }

  return  NS_OK;
}

nsresult MailEwsMsgUrl::GetMessageKey(nsMsgKey* aMessageKey)
{
  *aMessageKey = m_messageKey;
  return NS_OK;
}

NS_IMETHODIMP MailEwsMsgUrl::GetMessageSize(uint32_t * aMessageSize)
{
  if (aMessageSize)
  {
    *aMessageSize = m_messageSize;
    return NS_OK;
  }
  else
    return NS_ERROR_NULL_POINTER;
}

nsresult MailEwsMsgUrl::SetMessageSize(uint32_t aMessageSize)
{
  m_messageSize = aMessageSize;
  return NS_OK;
}

NS_IMETHODIMP MailEwsMsgUrl::SetUri(const char * aURI)
{
  mURI= aURI;
  return NS_OK;
}

NS_IMETHODIMP MailEwsMsgUrl::Clone(nsIURI **_retval)
{
  nsresult rv = MsgMailNewsUrlBase::Clone(_retval);
  NS_ENSURE_SUCCESS(rv, rv);
  // also clone the mURI member, because GetUri below won't work if
  // mURI isn't set due to nsIFile fun.
  nsCOMPtr <nsIMsgMessageUrl> clonedUrl = do_QueryInterface(*_retval);
  if (clonedUrl)
    clonedUrl->SetUri(mURI.get());
  return rv;
}

NS_IMETHODIMP MailEwsMsgUrl::GetUri(char ** aURI)
{
  // if we have been given a uri to associate with this url, then use it
  // otherwise try to reconstruct a URI on the fly....

  if (!mURI.IsEmpty())
    *aURI = ToNewCString(mURI);
  else
  {
      nsCString hostName;
      nsCString userName;
      nsCString path;
      GetHost(hostName);
      GetUsername(userName);
      GetFilePath(path);

      nsCString uriStr("ews-message://");
      uriStr.Append(userName);
      uriStr.Append('@');
      uriStr.Append(hostName);
      uriStr.Append(path);
      uriStr.Append('#');
      uriStr.AppendInt(m_messageKey);
      
      *aURI = ToNewCString(uriStr);

      mailews_logger << "GetUri:"
                     << (*aURI)
                     << std::endl;
  }

  return NS_OK;
}

nsresult MailEwsMsgUrl::GetMsgHdrForKey(nsMsgKey  msgKey, nsIMsgDBHdr ** aMsgHdr)
{
  nsresult rv = NS_OK;
  // if (aMsgHdr && m_filePath)
  // {
  //   nsCOMPtr<nsIMsgDatabase> mailDBFactory;
  //   nsCOMPtr<nsIMsgDatabase> mailDB;
  //   nsCOMPtr<nsIMsgDBService> msgDBService = do_GetService(MAIL_EWS_MSGDB_SERVICE_CONTRACTID, &rv);

  //   if (msgDBService)
  //     rv = msgDBService->OpenMailDBFromFile(m_filePath, nullptr, false,
  //                                           false, getter_AddRefs(mailDB));
  //   if (NS_SUCCEEDED(rv) && mailDB) // did we get a db back?
  //     rv = mailDB->GetMsgHdrForKey(msgKey, aMsgHdr);
  //   else
  //   {
  //     nsCOMPtr<nsIMsgWindow> msgWindow(do_QueryReferent(m_msgWindowWeak));
  //     if (!msgWindow)
  //     {
  //       nsCOMPtr<nsIMsgMailSession> mailSession = do_GetService(NS_MSGMAILSESSION_CONTRACTID, &rv);
  //       NS_ENSURE_SUCCESS(rv, rv);
  //       mailSession->GetTopmostMsgWindow(getter_AddRefs(msgWindow));
  //     }

  //     // maybe this is .eml file we're trying to read. See if we can get a header from the header sink.
  //     if (msgWindow)
  //     {
  //       nsCOMPtr<nsIMsgHeaderSink> headerSink;
  //       msgWindow->GetMsgHeaderSink(getter_AddRefs(headerSink));
  //       if (headerSink)
  //       {
  //         rv = headerSink->GetDummyMsgHeader(aMsgHdr);
  //         if (NS_SUCCEEDED(rv))
  //         {
  //           int64_t fileSize = 0;
  //           m_filePath->GetFileSize(&fileSize);
  //           (*aMsgHdr)->SetMessageSize(fileSize);
  //         }
  //       }
  //     }
  //   }
  // }
  // else
    rv = NS_ERROR_NULL_POINTER;

  return rv;
}

NS_IMETHODIMP MailEwsMsgUrl::GetMessageHeader(nsIMsgDBHdr ** aMsgHdr)
{
  nsCString uri;
  nsresult rv = GetUri(getter_Copies(uri));
  NS_ENSURE_SUCCESS(rv, rv);
  return mailews::GetMsgDBHdrFromURI(uri.get(), aMsgHdr);
}

NS_IMETHODIMP MailEwsMsgUrl::SetMessageHeader(nsIMsgDBHdr *aMsgHdr)
{
  m_dummyHdr = aMsgHdr;
  return NS_OK;
}

NS_IMPL_GETSET(MailEwsMsgUrl, AddDummyEnvelope, bool, m_addDummyEnvelope)
NS_IMPL_GETSET(MailEwsMsgUrl, CanonicalLineEnding, bool, m_canonicalLineEnding)

NS_IMETHODIMP
MailEwsMsgUrl::GetOriginalSpec(char **aSpec)
{
  if (!aSpec || m_originalSpec.IsEmpty())
    return NS_ERROR_NULL_POINTER;
  *aSpec = ToNewCString(m_originalSpec);
  return NS_OK;
}

NS_IMETHODIMP
MailEwsMsgUrl::SetOriginalSpec(const char *aSpec)
{
  m_originalSpec = aSpec;
  return NS_OK;
}

NS_IMETHODIMP MailEwsMsgUrl::SetMessageFile(nsIFile * aFile)
{
  m_messageFile = aFile;
  return NS_OK;
}

NS_IMETHODIMP MailEwsMsgUrl::GetMessageFile(nsIFile ** aFile)
{
  // why don't we return an error for null aFile?
  if (aFile)
    NS_IF_ADDREF(*aFile = m_messageFile);
  return NS_OK;
}

NS_IMETHODIMP MailEwsMsgUrl::IsUrlType(uint32_t type, bool *isType)
{
  NS_ENSURE_ARG(isType);

  switch(type)
  {
    case nsIMsgMailNewsUrl::eCopy:
      *isType = (m_mailboxAction == nsIMailboxUrl::ActionCopyMessage);
      break;
    case nsIMsgMailNewsUrl::eMove:
      *isType = (m_mailboxAction == nsIMailboxUrl::ActionMoveMessage);
      break;
    case nsIMsgMailNewsUrl::eDisplay:
      *isType = (m_mailboxAction == nsIMailboxUrl::ActionFetchMessage ||
                 m_mailboxAction == nsIMailboxUrl::ActionFetchPart);
      break;
    default:
      *isType = false;
  };

  return NS_OK;

}

////////////////////////////////////////////////////////////////////////////////////
// End nsIMailboxUrl specific support
////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// possible search part phrases include: MessageID=id&number=MessageKey

nsresult MailEwsMsgUrl::ParseSearchPart()
{
  nsAutoCString searchPart;
  nsresult rv = GetQuery(searchPart);
  // add code to this function to decompose everything past the '?'.....
  if (NS_SUCCEEDED(rv) && !searchPart.IsEmpty())
  {
    // the action for this mailbox must be a display message...
    char * msgPart = extractAttributeValue(searchPart.get(), "part=");
    if (msgPart)  // if we have a part in the url then we must be fetching just the part.
      m_mailboxAction = nsIMailboxUrl::ActionFetchPart;
    else
      m_mailboxAction = nsIMailboxUrl::ActionFetchMessage;

    char * messageKey = extractAttributeValue(searchPart.get(), "number=");
    m_messageID = extractAttributeValue(searchPart.get(),"messageid=");
    if (messageKey)
      m_messageKey = (nsMsgKey) mailews::ParseUint64Str(messageKey); // convert to a uint32_t...

    PR_Free(msgPart);
    PR_Free(messageKey);
  }
  else
      m_mailboxAction = nsIMailboxUrl::ActionFetchMessage;

  return rv;
}

// warning: don't assume when parsing the url that the protocol part is "news"...
nsresult MailEwsMsgUrl::ParseUrl()
{
  GetFilePath(m_file);

  ParseSearchPart();

  GetPath(m_file);

  return NS_OK;
}

NS_IMETHODIMP MailEwsMsgUrl::SetSpec(const nsACString &aSpec)
{
  nsresult rv = MsgMailNewsUrlBase::SetSpec(aSpec);
  if (NS_SUCCEEDED(rv))
    rv = ParseUrl();
  return rv;
}

NS_IMETHODIMP MailEwsMsgUrl::SetQuery(const nsACString &aQuery)
{
  nsresult rv = MsgMailNewsUrlBase::SetQuery(aQuery);
  if (NS_SUCCEEDED(rv))
    rv = ParseUrl();
  return rv;
}

// takes a string like ?messageID=fooo&number=MsgKey and returns a new string
// containing just the attribute value. i.e you could pass in this string with
// an attribute name of messageID and I'll return fooo. Use PR_Free to delete
// this string...

// Assumption: attribute pairs in the string are separated by '&'.
char * extractAttributeValue(const char * searchString, const char * attributeName)
{
  char * attributeValue = nullptr;

  if (searchString && attributeName)
  {
    // search the string for attributeName
    uint32_t attributeNameSize = PL_strlen(attributeName);
    char * startOfAttribute = PL_strcasestr(searchString, attributeName);
    if (startOfAttribute)
    {
      startOfAttribute += attributeNameSize; // skip over the attributeName
      if (startOfAttribute) // is there something after the attribute name
      {
        char * endOfAttribute = startOfAttribute ? PL_strchr(startOfAttribute, '&') : nullptr;
        nsDependentCString attributeValueStr;
        if (startOfAttribute && endOfAttribute) // is there text after attribute value
          attributeValueStr.Assign(startOfAttribute, endOfAttribute - startOfAttribute);
        else // there is nothing left so eat up rest of line.
          attributeValueStr.Assign(startOfAttribute);

        // now unescape the string...
        nsCString unescapedValue;
        mailews::MsgUnescapeString(attributeValueStr, 0, unescapedValue);
        attributeValue = PL_strdup(unescapedValue.get());
      } // if we have a attribute value

    } // if we have a attribute name
  } // if we got non-null search string and attribute name values

  return attributeValue;
}

// nsIMsgI18NUrl support

nsresult MailEwsMsgUrl::GetFolder(nsIMsgFolder **msgFolder)
{
  // if we have a RDF URI, then try to get the folder for that URI and then ask the folder
  // for it's charset....
  nsCString uri;
  GetUri(getter_Copies(uri));
  NS_ENSURE_TRUE(!uri.IsEmpty(), NS_ERROR_FAILURE);
  nsCOMPtr<nsIMsgDBHdr> msg;
  mailews::GetMsgDBHdrFromURI(uri.get(), getter_AddRefs(msg));
  if (!msg)
    return NS_ERROR_FAILURE;
  return msg->GetFolder(msgFolder);
}

NS_IMETHODIMP MailEwsMsgUrl::GetFolderCharset(char ** aCharacterSet)
{
  NS_ENSURE_ARG_POINTER(aCharacterSet);
  nsCOMPtr<nsIMsgFolder> folder;
  nsresult rv = GetFolder(getter_AddRefs(folder));
  
  // In cases where a file is not associated with a folder, for
  // example standalone .eml files, failure is normal.
  if (NS_FAILED(rv))
    return rv;
  nsCString tmpStr;
  folder->GetCharset(tmpStr);
  *aCharacterSet = ToNewCString(tmpStr);
  return NS_OK;
}

NS_IMETHODIMP MailEwsMsgUrl::GetFolderCharsetOverride(bool * aCharacterSetOverride)
{
  nsCOMPtr<nsIMsgFolder> folder;
  nsresult rv = GetFolder(getter_AddRefs(folder));
  NS_ENSURE_SUCCESS(rv,rv);
  NS_ENSURE_TRUE(folder, NS_ERROR_FAILURE);
  folder->GetCharsetOverride(aCharacterSetOverride);

  return NS_OK;
}

NS_IMETHODIMP MailEwsMsgUrl::GetCharsetOverRide(char ** aCharacterSet)
{
  if (!mCharsetOverride.IsEmpty())
    *aCharacterSet = ToNewCString(mCharsetOverride);
  else
    *aCharacterSet = nullptr;
  return NS_OK;
}

NS_IMETHODIMP MailEwsMsgUrl::SetCharsetOverRide(const char * aCharacterSet)
{
  mCharsetOverride = aCharacterSet;
  return NS_OK;
}

/* void setMoveCopyMsgKeys (out nsMsgKey keysToFlag, in long numKeys); */
NS_IMETHODIMP MailEwsMsgUrl::SetMoveCopyMsgKeys(nsMsgKey *keysToFlag, int32_t numKeys)
{
  m_keys.ReplaceElementsAt(0, m_keys.Length(), keysToFlag, numKeys);
  if (!m_keys.IsEmpty() && m_messageKey == nsMsgKey_None)
    m_messageKey = m_keys[0];
  return NS_OK;
}

NS_IMETHODIMP MailEwsMsgUrl::GetMoveCopyMsgHdrForIndex(uint32_t msgIndex, nsIMsgDBHdr **msgHdr)
{
  NS_ENSURE_ARG(msgHdr);
  if (msgIndex < m_keys.Length())
  {
    nsMsgKey nextKey = m_keys[msgIndex];
    return GetMsgHdrForKey(nextKey, msgHdr);
  }
  return NS_MSG_MESSAGE_NOT_FOUND;
}

NS_IMETHODIMP MailEwsMsgUrl::GetNumMoveCopyMsgs(uint32_t *numMsgs)
{
  NS_ENSURE_ARG(numMsgs);
  *numMsgs = m_keys.Length();
  return NS_OK;
}

NS_IMETHODIMP
MailEwsMsgUrl::GetCurMoveCopyMsgIndex(uint32_t *result)
{
  NS_ENSURE_ARG_POINTER(result);
  *result = m_curMsgIndex;
  return NS_OK;
}

NS_IMETHODIMP
MailEwsMsgUrl::SetCurMoveCopyMsgIndex(uint32_t aIndex)
{
  m_curMsgIndex = aIndex;
  return NS_OK;
}

NS_IMETHODIMP
MailEwsMsgUrl::GetMailboxAction(int32_t *result)
{
  NS_ENSURE_ARG_POINTER(result);
  *result = m_mailboxAction;
  return NS_OK;
}

NS_IMETHODIMP
MailEwsMsgUrl::SetMailboxAction(int32_t aIndex)
{
  m_mailboxAction = aIndex;
  return NS_OK;
}

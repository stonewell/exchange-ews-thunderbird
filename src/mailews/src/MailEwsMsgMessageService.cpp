/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
#include "msgCore.h"    // precompiled header...

#include "netCore.h"

#include "nsIServiceManager.h"
#include "nsIComponentManager.h"

#include "nsCOMPtr.h"
#include "nsIMsgFolder.h"
#include "nsIDocShell.h"
#include "nsIDocShellLoadInfo.h"
#include "nsIRDFService.h"
#include "nsRDFCID.h"
#include "nsIMsgStatusFeedback.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsILoadGroup.h"
#include "nsIMsgAccountManager.h"
#include "nsMsgBaseCID.h"
#include "nsMsgFolderFlags.h"
#include "nsISubscribableServer.h"
#include "nsIDirectoryService.h"
#include "nsMailDirServiceDefs.h"
#include "nsIWebNavigation.h"
#include "plbase64.h"
#include "nsIMsgHdr.h"
#include "MsgUtils.h"
#include "nsICacheService.h"
#include "nsIStreamListenerTee.h"
#include "nsNetCID.h"
#include "MsgI18N.h"
#include "nsIOutputStream.h"
#include "nsIInputStream.h"
#include "nsISeekableStream.h"
#include "nsICopyMsgStreamListener.h"
#include "nsIMsgParseMailMsgState.h"
#include "nsMsgLocalCID.h"
#include "nsIOutputStream.h"
#include "nsIDocShell.h"
#include "nsIDocShellLoadInfo.h"
#include "nsIMessengerWindowService.h"
#include "nsIWindowMediator.h"
#include "nsIPrompt.h"
#include "nsIWindowWatcher.h"
#include "nsIMsgMailSession.h"
#include "nsIStreamConverterService.h"
#include "nsIAutoSyncManager.h"
#include "nsThreadUtils.h"
#include "nsNetUtil.h"
#include "nsMsgMessageFlags.h"
#include "nsIMsgPluggableStore.h"
#include "nsIMsgMailNewsUrl.h"
#include "nsIProtocolHandler.h"
#include "nsIMsgDatabase.h"
#include "nsIMsgContentPolicy.h"
#include "nsIContentPolicy.h"
#include "nsCategoryCache.h"
#include "nsContentPolicyUtils.h"

#include "MailEwsCID.h"
#include "MailEwsMsgMessageService.h"
#include "MailEwsMsgProtocol.h"

#include "MailEwsLog.h"

static bool gInitialized = false;
//static int32_t gMIMEOnDemandThreshold = 15000;
//static bool gMIMEOnDemand = false;
static nsresult ParseMessageURI(const char* uri, nsCString& folderURI, uint32_t *key, char **part);
// static nsresult DecomposeURI(const nsACString &aMessageURI,
//                              nsIMsgFolder **aFolder,
//                              nsACString &aMsgKey);
static nsresult DecomposeURI(const nsACString &aMessageURI,
                             nsIMsgFolder **aFolder,
                             nsMsgKey *aMsgKey);
static nsresult PrepareMessageUrl(const char * aSrcMsgMailboxURI,
                                  nsIUrlListener * aUrlListener,
                                  nsMailboxAction aMailboxAction,
                                  nsIMailboxUrl ** aMailboxUrl,
                                  nsIMsgWindow *msgWindow);
static nsresult FetchMessage(const char* aMessageURI,
                             nsISupports * aDisplayConsumer,
                             nsIMsgWindow * aMsgWindow,
                             nsIUrlListener * aUrlListener,
                             const char * aFileName, /* only used by open attachment... */
                             nsMailboxAction mailboxAction,
                             const char * aCharsetOverride,
                             nsIURI ** aURL);
static nsresult RunMailboxUrl(nsIURI * aMailboxUrl, nsISupports * aDisplayConsumer);

static const nsCString SCHEMA("ews");

NS_IMPL_ISUPPORTS(MailEwsMsgMessageService,
                  nsIMsgMessageService,
                  nsIProtocolHandler,
                  nsIMsgMessageFetchPartService)

MailEwsMsgMessageService::MailEwsMsgMessageService()
: mPrintingOperation(false)
{
  if (!gInitialized)
  {
    nsresult rv;
    // initialize auto-sync service
    nsCOMPtr<nsIAutoSyncManager> autoSyncMgr = do_GetService(NS_AUTOSYNCMANAGER_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv) && autoSyncMgr) 
    {
      // auto-sync manager initialization goes here
      // assign new strategy objects here... 
    }
    NS_ASSERTION(autoSyncMgr != nullptr, "*** Cannot initialize nsAutoSyncManager service.");

    //Add schema to nsMsgContentPolicy to show embeded images
    nsCategoryCache<nsIContentPolicy> mPolicies(NS_CONTENTPOLICY_CATEGORY);
    nsCOMArray<nsIContentPolicy> entries;
    mPolicies.GetEntries(entries);
    int32_t count = entries.Count();
    for (int32_t i = 0; i < count; i++) {
      nsCOMPtr<nsIMsgContentPolicy> msgContentPolicy(do_QueryInterface(entries[i], &rv));

      if (NS_SUCCEEDED(rv) && msgContentPolicy) {
        msgContentPolicy->AddExposedProtocol(SCHEMA);
      }
    }
    
    gInitialized = true;
  }
}

MailEwsMsgMessageService::~MailEwsMsgMessageService()
{
}

NS_IMETHODIMP MailEwsMsgMessageService::GetUrlForUri(const char *aMessageURI, 
                                                     nsIURI **aURL, 
                                                     nsIMsgWindow *aMsgWindow) 
{
  nsAutoCString messageURI(aMessageURI);

  if (messageURI.Find(NS_LITERAL_CSTRING("&type=application/x-message-display")) != kNotFound)
    return NS_NewURI(aURL, aMessageURI);

  if (!strncmp(aMessageURI, "ews:", 4))
    return NS_NewURI(aURL, aMessageURI);

  nsCOMPtr<nsIMailboxUrl> mailboxurl;
  nsresult rv = PrepareMessageUrl(aMessageURI,
                                  nullptr,
                                  nsIMailboxUrl::ActionFetchMessage,
                                  getter_AddRefs(mailboxurl),
                                  aMsgWindow);
  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(mailboxurl, aURL);
}

NS_IMETHODIMP MailEwsMsgMessageService::OpenAttachment(const char *aContentType, 
                                                       const char *aFileName,
                                                       const char *aUrl, 
                                                       const char *aMessageUri, 
                                                       nsISupports *aDisplayConsumer, 
                                                       nsIMsgWindow *aMsgWindow, 
                                                       nsIUrlListener *aUrlListener)
{
  nsCOMPtr <nsIURI> URL;
  nsresult rv;

  nsAutoCString urlString(aUrl);
  urlString += "&type=";
  urlString += aContentType;
  urlString += "&filename=";
  urlString += aFileName;

  nsCOMPtr<nsIMailboxUrl> aMailboxUrl;
  
	URL = do_CreateInstance(MAIL_EWS_MSG_URL_CONTRACTID, &rv);
	NS_ENSURE_SUCCESS(rv, rv);

  URL->SetSpec(urlString);
  
  // try to run the url in the docshell...
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aDisplayConsumer, &rv));
  // if we were given a docShell, run the url in the docshell..otherwise just run it normally.
  if (NS_SUCCEEDED(rv) && docShell)
  {
    nsCOMPtr<nsIDocShellLoadInfo> loadInfo;
    // DIRTY LITTLE HACK --> since we are opening an attachment we want the docshell to
    // treat this load as if it were a user click event. Then the dispatching stuff will be much
    // happier.
    docShell->CreateLoadInfo(getter_AddRefs(loadInfo));
    loadInfo->SetLoadType(nsIDocShellLoadInfo::loadLink);
    return docShell->LoadURI(URL, loadInfo, nsIWebNavigation::LOAD_FLAGS_NONE, false);
  }
  return RunMailboxUrl(URL, aDisplayConsumer);
}

NS_IMETHODIMP MailEwsMsgMessageService::DisplayMessage(const char *aMessageURI,
                                                       nsISupports *aDisplayConsumer,  
                                                       nsIMsgWindow *aMsgWindow,
                                                       nsIUrlListener *aUrlListener,
                                                       const char *aCharsetOverride,
                                                       nsIURI **aURL)
{
  mailews_logger << "Display Message:"
            << aMessageURI
            << ","
            << (aCharsetOverride ? aCharsetOverride : "")
            << aDisplayConsumer
            << std::endl;

  return FetchMessage(aMessageURI, aDisplayConsumer, aMsgWindow,
                      aUrlListener,
                      nullptr, nsIMailboxUrl::ActionFetchMessage,
                      aCharsetOverride, aURL);
}

//
// rhp: Right now, this is the same as simple DisplayMessage, but it will change
// to support print rendering.
//
NS_IMETHODIMP MailEwsMsgMessageService::DisplayMessageForPrinting(const char *aMessageURI,
                                                                  nsISupports *aDisplayConsumer,  
                                                                  nsIMsgWindow *aMsgWindow,
                                                                  nsIUrlListener *aUrlListener,
                                                                  nsIURI **aURL) 
{
  mPrintingOperation = true;
  nsresult rv = DisplayMessage(aMessageURI, aDisplayConsumer, aMsgWindow, aUrlListener, nullptr, aURL);
  mPrintingOperation = false;
  return rv;
}

NS_IMETHODIMP MailEwsMsgMessageService::CopyMessage(const char *aSrcMailboxURI, 
                                                    nsIStreamListener *aMailboxCopy, 
                                                    bool moveMessage,
                                                    nsIUrlListener *aUrlListener, 
                                                    nsIMsgWindow *aMsgWindow, 
                                                    nsIURI **aURL)
{
  NS_ENSURE_ARG_POINTER(aSrcMailboxURI);
  NS_ENSURE_ARG_POINTER(aMailboxCopy);

  mailews_logger << "Copy Messages+++++++++++++++++++" << std::endl;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP MailEwsMsgMessageService::CopyMessages(uint32_t aNumKeys,
                                                     nsMsgKey*aMsgKeys,
                                                     nsIMsgFolder *srcFolder,
                                                     nsIStreamListener *aMailboxCopyHandler,
                                                     bool moveMessage,
                                                     nsIUrlListener *aUrlListener,
                                                     nsIMsgWindow *aMsgWindow, 
                                                     nsIURI **aURL)
{
  NS_ENSURE_ARG_POINTER(aMailboxCopyHandler);
  NS_ENSURE_ARG_POINTER(aMsgKeys);

  nsresult rv = NS_OK;
  NS_ENSURE_ARG(srcFolder);
  NS_ENSURE_ARG(aMsgKeys);
  nsCOMPtr<nsIMailboxUrl> mailboxurl;

  mailews_logger << "Message Service Copy Messages-------------" << std::endl;

  nsMailboxAction actionToUse = nsIMailboxUrl::ActionMoveMessage;
  if (!moveMessage)
     actionToUse = nsIMailboxUrl::ActionCopyMessage;

  nsCOMPtr <nsIMsgDBHdr> msgHdr;
  nsCOMPtr <nsIMsgDatabase> db;
  srcFolder->GetMsgDatabase(getter_AddRefs(db));
  if (db)
  {
    db->GetMsgHdrForKey(aMsgKeys[0], getter_AddRefs(msgHdr));
    if (msgHdr)
    {
      nsCString uri;
      srcFolder->GetURI(uri);
      mailews_logger << "Src Folder uri:" << uri.get() << std::endl;
      srcFolder->GetUriForMsg(msgHdr, uri);
      mailews_logger << "Msg uri:" << uri.get() << std::endl;
      rv = PrepareMessageUrl(uri.get(), aUrlListener, actionToUse , getter_AddRefs(mailboxurl), aMsgWindow);

      if (NS_SUCCEEDED(rv))
      {
        nsCOMPtr<nsIURI> url = do_QueryInterface(mailboxurl);
        nsCOMPtr<nsIMsgMailNewsUrl> msgUrl (do_QueryInterface(url));
        nsCOMPtr<nsIMailboxUrl> mailboxUrl (do_QueryInterface(url));
        msgUrl->SetMsgWindow(aMsgWindow);

        mailboxUrl->SetMoveCopyMsgKeys(aMsgKeys, aNumKeys);
        rv = RunMailboxUrl(url, aMailboxCopyHandler);
      }
    }
  }
  if (aURL && mailboxurl)
    CallQueryInterface(mailboxurl, aURL);

  return rv;
}

NS_IMETHODIMP MailEwsMsgMessageService::Search(nsIMsgSearchSession *aSearchSession, 
                                               nsIMsgWindow *aMsgWindow, 
                                               nsIMsgFolder *aMsgFolder, 
                                               const char *aSearchUri)
{
  NS_ENSURE_ARG_POINTER(aSearchUri);
  NS_ENSURE_ARG_POINTER(aMsgFolder);

  mailews_logger << "Search Messages ------------------" << std::endl;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP MailEwsMsgMessageService::SaveMessageToDisk(const char *aMessageURI,
                                                          nsIFile *aFile,
                                                          bool aAddDummyEnvelope,
                                                          nsIUrlListener *aUrlListener,
                                                          nsIURI **aURL,
                                                          bool canonicalLineEnding,
                                                          nsIMsgWindow *aMsgWindow)
{
  mailews_logger << "Save message to disk"
            << aMessageURI
            << aFile
            << std::endl;
  nsresult rv = NS_OK;
  nsCOMPtr<nsIMailboxUrl> mailboxurl;

  rv = PrepareMessageUrl(aMessageURI, aUrlListener, nsIMailboxUrl::ActionSaveMessageToDisk, getter_AddRefs(mailboxurl), aMsgWindow);

  if (NS_SUCCEEDED(rv))
  {
    nsCOMPtr<nsIMsgMessageUrl> msgUrl = do_QueryInterface(mailboxurl);
    if (msgUrl)
    {
      msgUrl->SetMessageFile(aFile);
      msgUrl->SetAddDummyEnvelope(aAddDummyEnvelope);
      msgUrl->SetCanonicalLineEnding(canonicalLineEnding);
    }

    nsCOMPtr<nsIURI> url = do_QueryInterface(mailboxurl);
    rv = RunMailboxUrl(url, nullptr);
  }

  if (aURL && mailboxurl)
    CallQueryInterface(mailboxurl, aURL);

  return rv;
}

// this method streams a message to the passed in consumer, with an optional stream converter
// and additional header (e.g., "header=filter")
NS_IMETHODIMP MailEwsMsgMessageService::StreamMessage(const char *aMessageURI, 
                                                      nsISupports *aConsumer, 
                                                      nsIMsgWindow *aMsgWindow,
                                                      nsIUrlListener *aUrlListener, 
                                                      bool aConvertData,
                                                      const nsACString &aAdditionalHeader,
                                                      bool aLocalOnly,
                                                      nsIURI **aURL)
{
  NS_ENSURE_ARG_POINTER(aMessageURI);

  mailews_logger << "Stream Message"
            << std::endl;

  nsAutoCString aURIString(aMessageURI);
  if (!aAdditionalHeader.IsEmpty())
  {
    aURIString.FindChar('?') == -1 ? aURIString += "?" : aURIString += "&";
    aURIString += "header=";
    aURIString += aAdditionalHeader;
  }

  return FetchMessage(aURIString.get(),
                      aConsumer,
                      aMsgWindow,
                      aUrlListener,
                      nullptr,
                      nsIMailboxUrl::ActionFetchMessage,
                      nullptr,
                      aURL);
}

// this method streams a message's headers to the passed in consumer.
NS_IMETHODIMP MailEwsMsgMessageService::StreamHeaders(const char *aMessageURI,
                                                      nsIStreamListener *aConsumer,
                                                      nsIUrlListener *aUrlListener,
                                                      bool aLocalOnly,
                                                      nsIURI **aURL)
{
  NS_ENSURE_ARG_POINTER(aMessageURI);
  NS_ENSURE_ARG_POINTER(aConsumer);

  mailews_logger << "Stream Headers"
            << std::endl;
  nsAutoCString folderURI;
  nsMsgKey msgKey;
  nsCOMPtr<nsIMsgFolder> folder;
  nsresult rv = DecomposeURI(nsDependentCString(aMessageURI),
                             getter_AddRefs(folder),
                             &msgKey);
  if (msgKey == nsMsgKey_None)
    return NS_MSG_MESSAGE_NOT_FOUND;

  nsCOMPtr<nsIInputStream> inputStream;
  int64_t messageOffset;
  uint32_t messageSize;
  rv = folder->GetOfflineFileStream(msgKey, &messageOffset, &messageSize, getter_AddRefs(inputStream));
  NS_ENSURE_SUCCESS(rv, rv);

  // GetOfflineFileStream returns NS_OK but null inputStream when there is an error getting the database
  if (!inputStream)
    return NS_ERROR_FAILURE;

  return mailews::MsgStreamMsgHeaders(inputStream, aConsumer);
}

NS_IMETHODIMP MailEwsMsgMessageService::IsMsgInMemCache(nsIURI *aUrl,
                                                        nsIMsgFolder *aMailFolder,
                                                        nsICacheEntryDescriptor **aCacheEntry,
                                                        bool *aResult)
{
  NS_ENSURE_ARG_POINTER(aUrl);
  NS_ENSURE_ARG_POINTER(aMailFolder);
  *aResult = false;

  return NS_OK;
}


NS_IMETHODIMP MailEwsMsgMessageService::MessageURIToMsgHdr(const char *uri, nsIMsgDBHdr **aRetVal)
{
  NS_ENSURE_ARG_POINTER(uri);
  NS_ENSURE_ARG_POINTER(aRetVal);

  mailews_logger << "MessageURIToMsgHdr:"
            << uri
            << std::endl;
  
  nsCOMPtr<nsIMsgFolder> folder;
  nsMsgKey msgKey;
  nsresult rv = DecomposeURI(nsDependentCString(uri),
                             getter_AddRefs(folder), &msgKey);
  NS_ENSURE_SUCCESS(rv,rv);
  rv = folder->GetMessageHeader(msgKey, aRetVal);
  NS_ENSURE_SUCCESS(rv,rv);

  return NS_OK;
}

static nsresult DecomposeURI(const nsACString &aMessageURI,
                             nsIMsgFolder **aFolder,
                             nsMsgKey *aMsgKey)
{
  NS_ENSURE_ARG_POINTER(aFolder);
  NS_ENSURE_ARG_POINTER(aMsgKey);

  nsAutoCString folderURI;
  nsresult rv = ParseMessageURI(PromiseFlatCString(aMessageURI).get(),
                                folderURI, aMsgKey, nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr <nsIRDFService> rdf = do_GetService("@mozilla.org/rdf/rdf-service;1",&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mailews_logger << "DecomposeURI:"
            << nsCString(aMessageURI).get()
            << ","
            << ", Folder URI:"
            << nsCString(folderURI).get()
            << std::endl;
  
  nsCOMPtr<nsIRDFResource> res;
  rv = rdf->GetResource(folderURI, getter_AddRefs(res));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMsgFolder> msgFolder = do_QueryInterface(res);
  NS_ENSURE_TRUE(msgFolder, NS_ERROR_FAILURE);

  msgFolder.swap(*aFolder);

  return NS_OK;
}

/* parses MessageURI */
static nsresult ParseMessageURI(const char* uri, nsCString& folderURI, uint32_t *key, char **part)
{
  if(!key)
    return NS_ERROR_NULL_POINTER;

  nsAutoCString uriStr(uri);
  int32_t folderEnd = -1;
  //
  if (StringBeginsWith(uriStr, NS_LITERAL_CSTRING("ews:-message"))) {
    uriStr.Replace(0, strlen("ews:-message"), nsCString("ews-message:"));
  }
  
  // imap-message uri's can have imap:// url strings tacked on the end,
  // e.g., when opening/saving attachments. We don't want to look for '#'
  // in that part of the uri, if the attachment name contains '#',
  // so check for that here.
  if (StringBeginsWith(uriStr, NS_LITERAL_CSTRING("ews-message")))
    folderEnd = uriStr.Find("ews://");

  int32_t keySeparator = MsgRFindChar(uriStr, '#', folderEnd);
  if(keySeparator != -1)
  {
    int32_t keyEndSeparator = mailews::MsgFindCharInSet(uriStr, "/?&", keySeparator);
    nsAutoString folderPath;
    folderURI = StringHead(uriStr, keySeparator);
    folderURI.Cut(3 /*strlen("ews")*/, 8 /*strlen("_message") */); // cut out the _message part of ews-message:
    // folder uri's don't have fully escaped usernames.
    int32_t atPos = folderURI.FindChar('@');
    if (atPos != -1)
    {
      nsCString unescapedName, escapedName;
      int32_t userNamePos = folderURI.Find("//") + 2;
      uint32_t origUserNameLen = atPos - userNamePos;
      if (NS_SUCCEEDED(mailews::MsgUnescapeString(Substring(folderURI, userNamePos,
                                                   origUserNameLen),
                                         0, unescapedName)))
      {
        // Re-escape the username, matching the way we do it in uris, not the
        // way necko escapes urls. See nsMsgIncomingServer::GetServerURI.
        mailews::MsgEscapeString(unescapedName, nsINetUtil::ESCAPE_XALPHAS, escapedName);
        folderURI.Replace(userNamePos, origUserNameLen, escapedName);
      }
    }
    nsAutoCString keyStr;
    if (keyEndSeparator != -1)
      keyStr = Substring(uriStr, keySeparator + 1, keyEndSeparator - (keySeparator + 1));
    else
      keyStr = Substring(uriStr, keySeparator + 1);

    *key = strtoul(keyStr.get(), nullptr, 10);

    if (part && keyEndSeparator != -1)
    {
      int32_t partPos = mailews::MsgFind(uriStr, "part=", false, keyEndSeparator);
      if (partPos != -1)
      {
        *part = ToNewCString(Substring(uriStr, keyEndSeparator));
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP MailEwsMsgMessageService::GetScheme(nsACString &aScheme)
{
  aScheme = "ews";
  return NS_OK;
}

NS_IMETHODIMP MailEwsMsgMessageService::GetDefaultPort(int32_t *aDefaultPort)
{
  NS_ENSURE_ARG_POINTER(aDefaultPort);
  *aDefaultPort = -1;  // mailbox doesn't use a port!!!!!
  return NS_OK;
}

NS_IMETHODIMP MailEwsMsgMessageService::AllowPort(int32_t port, const char *scheme, bool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  // don't override anything.
  *_retval = false;
  return NS_OK;
}

NS_IMETHODIMP MailEwsMsgMessageService::GetProtocolFlags(uint32_t *result)
{
  NS_ENSURE_ARG_POINTER(result);
  *result = URI_STD | URI_FORBIDS_AUTOMATIC_DOCUMENT_REPLACEMENT |
      URI_DANGEROUS_TO_LOAD | URI_FORBIDS_COOKIE_ACCESS
#ifdef IS_ORIGIN_IS_FULL_SPEC_DEFINED
      | ORIGIN_IS_FULL_SPEC
#endif
      ;
  return NS_OK;
}

NS_IMETHODIMP MailEwsMsgMessageService::NewURI(const nsACString &aSpec,
                                               const char *aOriginCharset,
                                               nsIURI *aBaseURI,
                                               nsIURI **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = 0;
  nsresult rv;
  nsCOMPtr<nsIURI> aMsgUri = do_CreateInstance(MAIL_EWS_MSG_URL_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aBaseURI)
  {
    nsAutoCString newSpec;
    rv = aBaseURI->Resolve(aSpec, newSpec);
    NS_ENSURE_SUCCESS(rv, rv);
    (void) aMsgUri->SetSpec(newSpec);
  }
  else
  {
    (void) aMsgUri->SetSpec(aSpec);
  }
  aMsgUri.swap(*_retval);

  return rv;
}

NS_IMETHODIMP MailEwsMsgMessageService::NewChannel(nsIURI *aURI, nsIChannel **_retval)
{
  return NewChannel2(aURI, nullptr, _retval);
}

NS_IMETHODIMP MailEwsMsgMessageService::NewChannel2(nsIURI *aURI,
                                            nsILoadInfo *aLoadInfo,
                                            nsIChannel **_retval)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv = NS_OK;
  nsAutoCString spec;
  aURI->GetSpec(spec);

  mailews_logger << "NewChannel2: "
                 << nsCString(spec).get()
                 << std::endl;
  
  nsRefPtr<MailEwsMsgProtocol> protocol = new MailEwsMsgProtocol(aURI);
  if (!protocol)
  {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  rv = protocol->Initialize(aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = protocol->SetLoadInfo(aLoadInfo);
  NS_ENSURE_SUCCESS(rv, rv);
                              
  return CallQueryInterface(protocol, _retval);
}

NS_IMETHODIMP MailEwsMsgMessageService::FetchMimePart(nsIURI *aURI, const char *aMessageURI, nsISupports *aDisplayConsumer, nsIMsgWindow *aMsgWindow, nsIUrlListener *aUrlListener, nsIURI **aURL)
{
  mailews_logger << "FetchMimePart --------------------------------------------"
            << std::endl;
  nsresult rv;
  nsCOMPtr<nsIMsgMailNewsUrl> msgUrl (do_QueryInterface(aURI, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  msgUrl->SetMsgWindow(aMsgWindow);

  // set up the url listener
  if (aUrlListener)
    msgUrl->RegisterListener(aUrlListener);

  return RunMailboxUrl(msgUrl, aDisplayConsumer);
}

static nsresult PrepareMessageUrl(const char * messageURI,
                                  nsIUrlListener * aUrlListener,
                                  nsMailboxAction aMailboxAction,
                                  nsIMailboxUrl ** aMailboxUrl,
                                  nsIMsgWindow * aMsgWindow) {
	nsAutoCString folderURI;
	nsMsgKey msgKey;
  char * part = NULL;
	
	nsresult rv = ParseMessageURI(PromiseFlatCString(messageURI).get(),
                                folderURI, &msgKey, &part);
	NS_ENSURE_SUCCESS(rv, rv);

	rv = CallCreateInstance(MAIL_EWS_MSG_URL_CONTRACTID, aMailboxUrl);
	NS_ENSURE_SUCCESS(rv, rv);

	nsCOMPtr<nsIMailboxUrl> mailboxurl(*aMailboxUrl);

	nsCString msgUrlSpec(folderURI);
	msgUrlSpec.AppendLiteral("?number=");
	msgUrlSpec.AppendInt(msgKey, 10);

  if (part) {
    nsCString strPart;
    strPart.Adopt(part);
    msgUrlSpec.AppendLiteral("&");
    msgUrlSpec.Append(strPart);
  }

	nsCOMPtr<nsIMsgMailNewsUrl> msgUrl;
	msgUrl = do_QueryInterface(mailboxurl);
        
	msgUrl->SetMsgWindow(aMsgWindow);
	msgUrl->SetSpec(msgUrlSpec);
  
	if (aUrlListener)
		msgUrl->RegisterListener(aUrlListener);
        
	nsCOMPtr<nsIMsgMessageUrl> url = do_QueryInterface(msgUrl);
	if (url)
	{
		url->SetOriginalSpec(messageURI);
		url->SetUri(messageURI);
	}

  //Update action the last step, since set spec will change the action
	mailboxurl->SetMailboxAction(aMailboxAction);

	return NS_OK;
}

// Takes a mailbox url, this method creates a protocol instance and loads the url
// into the protocol instance.
static nsresult RunMailboxUrl(nsIURI * aMailboxUrl, nsISupports * aDisplayConsumer)
{
  // create a protocol instance to run the url..
  nsresult rv = NS_OK;
  MailEwsMsgProtocol * protocol = new MailEwsMsgProtocol(aMailboxUrl);

  if (protocol)
  {
    rv = protocol->Initialize(aMailboxUrl);
    if (NS_FAILED(rv))
    {
      delete protocol;
      return rv;
    }
    NS_ADDREF(protocol);
    rv = protocol->LoadUrl(aMailboxUrl, aDisplayConsumer);
    NS_RELEASE(protocol); // after loading, someone else will have a ref cnt on the mailbox
  }

  return rv;
}

static nsresult FetchMessage(const char* aMessageURI,
                             nsISupports * aDisplayConsumer,
                             nsIMsgWindow * aMsgWindow,
                             nsIUrlListener * aUrlListener,
                             const char * aFileName, /* only used by open attachment... */
                             nsMailboxAction mailboxAction,
                             const char * aCharsetOverride,
                             nsIURI ** aURL) {
  nsresult rv;

  nsCOMPtr<nsIMsgFolder> folder;
  nsAutoCString mimePart;
  nsAutoCString folderURI;
  nsAutoCString messageURI(aMessageURI);
  nsCOMPtr <nsIURI> uri;
  nsCOMPtr<nsIMailboxUrl> mailboxurl;
  nsCOMPtr<nsIMsgMailNewsUrl> msgUrl;

  int32_t typeIndex = messageURI.Find("&type=application/x-message-display");
  if (typeIndex != kNotFound)
  {
    // This happens with forward inline of a message/rfc822 attachment opened in
    // a standalone msg window.
    // So, just cut to the chase and call AsyncOpen on a channel.
    messageURI.Cut(typeIndex,
                   sizeof("&type=application/x-message-display") - 1);
    rv = NS_NewURI(getter_AddRefs(uri), messageURI.get());
    mailboxurl = do_QueryInterface(uri);

    mailews_logger << "display message with uri 2:"
              << messageURI.get()
              << std::endl;
  } else {
    rv = PrepareMessageUrl(aMessageURI,
                           aUrlListener,
                           nsIMailboxUrl::ActionFetchMessage,
                           getter_AddRefs(mailboxurl),
                           aMsgWindow);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  uri = do_QueryInterface(mailboxurl);
  if (aURL)
    NS_IF_ADDREF(*aURL = uri);
  
  nsCOMPtr<nsIMsgI18NUrl> i18nurl(do_QueryInterface(msgUrl));
  if (i18nurl)
    i18nurl->SetCharsetOverRide(aCharsetOverride);

  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aDisplayConsumer, &rv));
  
  if (NS_SUCCEEDED(rv) && docShell) {
    rv = docShell->LoadURI(uri,
                           nullptr,
                           nsIWebNavigation::LOAD_FLAGS_NONE,
                           false);
    NS_ENSURE_SUCCESS(rv, rv);
    mailews_logger << "uri loaded"
              << uri
              << std::endl;
  } else {
    mailews_logger << "unable to get doc shell"
              << std::endl;
    rv = RunMailboxUrl(uri, aDisplayConsumer);
  }
  return rv;
}

/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "msgCore.h"
#include "nsStringGlue.h"
#include "MsgProtocolBase.h"
#include "nsIMsgMailNewsUrl.h"
#include "nsIMsgMailSession.h"
#include "nsMsgBaseCID.h"
#include "nsIStreamTransportService.h"
#include "nsISocketTransportService.h"
#include "nsISocketTransport.h"
#include "nsILoadGroup.h"
#include "nsIIOService.h"
#include "nsNetUtil.h"
#include "nsIFileURL.h"
#include "nsIMsgWindow.h"
#include "nsIMsgStatusFeedback.h"
#include "nsIWebProgressListener.h"
#include "nsIPipe.h"
#include "nsIPrompt.h"
#include "prprf.h"
#include "plbase64.h"
#include "nsIStringBundle.h"
#include "nsIProtocolProxyService.h"
#include "nsIProxyInfo.h"
#include "nsThreadUtils.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsDirectoryServiceDefs.h"
#include "MsgUtils.h"
#include "nsILineInputStream.h"
#include "nsIAsyncInputStream.h"
#include "nsIMsgIncomingServer.h"
#include "nsMimeTypes.h"
#include "nsAlgorithm.h"
#include "Services.h"
#include "nsILoadInfo.h"

#include <algorithm>

#undef PostMessage // avoid to collision with WinUser.h

using namespace mozilla;

NS_IMPL_ISUPPORTS(MsgProtocolBase, nsIChannel, nsIStreamListener,
  nsIRequestObserver, nsIRequest, nsITransportEventSink)

static char16_t *FormatStringWithHostNameByID(int32_t stringID, nsIMsgMailNewsUrl *msgUri);


MsgProtocolBase::MsgProtocolBase(nsIURI * aURL)
{
  m_flags = 0;
  m_readCount = 0;
  mLoadFlags = 0;
  m_socketIsOpen = false;
  mContentLength = -1;
  m_ContentCharset.Truncate();

  mailews::GetSpecialDirectoryWithFileName(NS_OS_TEMP_DIR, "tempMessage.eml",
                                  getter_AddRefs(m_tempMsgFile));

  mSuppressListenerNotifications = false;
  InitFromURI(aURL);
}

nsresult MsgProtocolBase::InitFromURI(nsIURI *aUrl)
{
  m_url = aUrl;

  nsCOMPtr <nsIMsgMailNewsUrl> mailUrl = do_QueryInterface(aUrl);
  if (mailUrl)
  {
    mailUrl->GetLoadGroup(getter_AddRefs(m_loadGroup));
    nsCOMPtr<nsIMsgStatusFeedback> statusFeedback;
    mailUrl->GetStatusFeedback(getter_AddRefs(statusFeedback));
    mProgressEventSink = do_QueryInterface(statusFeedback);
  }
  return NS_OK;
}

MsgProtocolBase::~MsgProtocolBase()
{}


static bool gGotTimeoutPref;
static int32_t gSocketTimeout = 60;

nsresult
MsgProtocolBase::GetQoSBits(uint8_t *aQoSBits)
{
  NS_ENSURE_ARG_POINTER(aQoSBits);
  const char* protocol = GetType();

  if (!protocol)
    return NS_ERROR_NOT_IMPLEMENTED;

  nsAutoCString prefName("mail.");
  prefName.Append(protocol);
  prefName.Append(".qos");

  nsresult rv;
  nsCOMPtr<nsIPrefBranch> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  int32_t val;
  rv = prefBranch->GetIntPref(prefName.get(), &val);
  NS_ENSURE_SUCCESS(rv, rv);
  *aQoSBits = (uint8_t) clamped(val, 0, 0xff);
  return NS_OK;
}

nsresult
MsgProtocolBase::OpenNetworkSocketWithInfo(const char * aHostName,
                                         int32_t aGetPort,
                                         const char *connectionType,
                                         nsIProxyInfo *aProxyInfo,
                                         nsIInterfaceRequestor* callbacks)
{
  NS_ENSURE_ARG(aHostName);

  nsresult rv = NS_OK;
  nsCOMPtr<nsISocketTransportService> socketService (do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID));
  NS_ENSURE_TRUE(socketService, NS_ERROR_FAILURE);

  // with socket connections we want to read as much data as arrives
  m_readCount = -1;

  nsCOMPtr<nsISocketTransport> strans;
  rv = socketService->CreateTransport(&connectionType, connectionType != nullptr,
                                      nsDependentCString(aHostName),
                                      aGetPort, aProxyInfo,
                                      getter_AddRefs(strans));
  if (NS_FAILED(rv)) return rv;

  strans->SetSecurityCallbacks(callbacks);

  // creates cyclic reference!
  nsCOMPtr<nsIThread> currentThread(do_GetCurrentThread());
  strans->SetEventSink(this, currentThread);

  m_socketIsOpen = false;
  m_transport = strans;

  if (!gGotTimeoutPref)
  {
    nsCOMPtr<nsIPrefBranch> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (prefBranch)
    {
      prefBranch->GetIntPref("mailnews.tcptimeout", &gSocketTimeout);
      gGotTimeoutPref = true;
    }
  }
  strans->SetTimeout(nsISocketTransport::TIMEOUT_CONNECT, gSocketTimeout + 60);
  strans->SetTimeout(nsISocketTransport::TIMEOUT_READ_WRITE, gSocketTimeout);

  uint8_t qos;
  rv = GetQoSBits(&qos);
  if (NS_SUCCEEDED(rv))
    strans->SetQoSBits(qos);

  return SetupTransportState();
}

/*
// open a connection on this url
nsresult
MsgProtocolBase::OpenNetworkSocket(nsIURI * aURL, const char *connectionType,
                                 nsIInterfaceRequestor* callbacks)
{
  NS_ENSURE_ARG(aURL);

  nsAutoCString hostName;
  int32_t port = 0;

  aURL->GetPort(&port);
  aURL->GetAsciiHost(hostName);

  nsCOMPtr<nsIProxyInfo> proxyInfo;

  nsCOMPtr<nsIProtocolProxyService> pps =
      do_GetService("@mozilla.org/network/protocol-proxy-service;1");

  NS_ASSERTION(pps, "Couldn't get the protocol proxy service!");

  if (pps)
  {
      nsresult rv = NS_OK;

      // Yes, this is ugly. But necko needs to grap a protocol handler
      // to ask for flags, and smtp isn't registered as a handler, only
      // mailto.
      // Note that I cannot just clone, and call SetSpec, since Clone on
      // nsSmtpUrl calls nsStandardUrl's clone method, which fails
      // because smtp isn't a registered protocol.
      // So we cheat. Whilst creating a uri manually is valid here,
      // do _NOT_ copy this to use in your own code - bbaetz
      nsCOMPtr<nsIURI> proxyUri = aURL;
      bool isSMTP = false;
      if (NS_SUCCEEDED(aURL->SchemeIs("smtp", &isSMTP)) && isSMTP)
      {
          nsAutoCString spec;
          rv = aURL->GetSpec(spec);
          if (NS_SUCCEEDED(rv))
              proxyUri = do_CreateInstance(NS_STANDARDURL_CONTRACTID, &rv);

          if (NS_SUCCEEDED(rv))
              rv = proxyUri->SetSpec(spec);
          if (NS_SUCCEEDED(rv))
              rv = proxyUri->SetScheme(NS_LITERAL_CSTRING("mailto"));
      }
      //
      // XXX(darin): Consider using AsyncResolve instead to avoid blocking
      //             the calling thread in cases where PAC may call into
      //             our DNS resolver.
      //
      if (NS_SUCCEEDED(rv))
          rv = pps->DeprecatedBlockingResolve(proxyUri, 0, getter_AddRefs(proxyInfo));
      NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't successfully resolve a proxy");
      if (NS_FAILED(rv)) proxyInfo = nullptr;
  }

  return OpenNetworkSocketWithInfo(hostName.get(), port, connectionType,
                                   proxyInfo, callbacks);
}
*/

nsresult MsgProtocolBase::GetFileFromURL(nsIURI * aURL, nsIFile **aResult)
{
  NS_ENSURE_ARG_POINTER(aURL);
  NS_ENSURE_ARG_POINTER(aResult);
  // extract the file path from the uri...
  nsAutoCString urlSpec;
  aURL->GetPath(urlSpec);
  urlSpec.Insert(NS_LITERAL_CSTRING("file://"), 0);
  nsresult rv;

// dougt - there should be an easier way!
  nsCOMPtr<nsIURI> uri;
  if (NS_FAILED(rv = NS_NewURI(getter_AddRefs(uri), urlSpec.get())))
      return rv;

  nsCOMPtr<nsIFileURL>    fileURL = do_QueryInterface(uri);
  if (!fileURL)   return NS_ERROR_FAILURE;

  return fileURL->GetFile(aResult);
  // dougt
}

nsresult MsgProtocolBase::OpenFileSocket(nsIURI * aURL, uint32_t aStartPosition, int32_t aReadCount)
{
  // mscott - file needs to be encoded directly into aURL. I should be able to get
  // rid of this method completely.

  nsresult rv = NS_OK;
  m_readCount = aReadCount;
  nsCOMPtr <nsIFile> file;

  rv = GetFileFromURL(aURL, getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> stream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(stream), file);
  if (NS_FAILED(rv)) return rv;

  // create input stream transport
  nsCOMPtr<nsIStreamTransportService> sts =
      do_GetService(NS_STREAMTRANSPORTSERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  rv = sts->CreateInputTransport(stream, int64_t(aStartPosition),
                                 int64_t(aReadCount), true,
                                 getter_AddRefs(m_transport));

  m_socketIsOpen = false;
  return rv;
}

nsresult MsgProtocolBase::GetTopmostMsgWindow(nsIMsgWindow **aWindow)
{
  nsresult rv;
  nsCOMPtr<nsIMsgMailSession> mailSession(do_GetService(NS_MSGMAILSESSION_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  return mailSession->GetTopmostMsgWindow(aWindow);
}

nsresult MsgProtocolBase::SetupTransportState()
{
  if (!m_socketIsOpen && m_transport)
  {
    nsresult rv;

    // open buffered, blocking output stream
    rv = m_transport->OpenOutputStream(nsITransport::OPEN_BLOCKING, 0, 0, getter_AddRefs(m_outputStream));
    if (NS_FAILED(rv)) return rv;
    // we want to open the stream
  } // if m_transport

  return NS_OK;
}

nsresult MsgProtocolBase::CloseSocket()
{
  nsresult rv = NS_OK;
  // release all of our socket state
  m_socketIsOpen = false;
  m_inputStream = nullptr;
  m_outputStream = nullptr;
  if (m_transport) {
    nsCOMPtr<nsISocketTransport> strans = do_QueryInterface(m_transport);
    if (strans) {
      strans->SetEventSink(nullptr, nullptr); // break cyclic reference!
    }
  }
  // we need to call Cancel so that we remove the socket transport from the mActiveTransportList.  see bug #30648
  if (m_request) {
    rv = m_request->Cancel(NS_BINDING_ABORTED);
  }
  m_request = 0;
  if (m_transport) {
    m_transport->Close(NS_BINDING_ABORTED);
    m_transport = 0;
  }

  return rv;
}

/*
* Writes the data contained in dataBuffer into the current output stream. It also informs
* the transport layer that this data is now available for transmission.
* Returns a positive number for success, 0 for failure (not all the bytes were written to the
* stream, etc). We need to make another pass through this file to install an error system (mscott)
*
* No logging is done in the base implementation, so aSuppressLogging is ignored.
*/

nsresult MsgProtocolBase::SendData(const char * dataBuffer, bool aSuppressLogging)
{
  uint32_t writeCount = 0;

  if (dataBuffer && m_outputStream)
    return m_outputStream->Write(dataBuffer, PL_strlen(dataBuffer), &writeCount);
    // TODO make sure all the bytes in PL_strlen(dataBuffer) were written
  else
    return NS_ERROR_INVALID_ARG;
}

// Whenever data arrives from the connection, core netlib notifices the protocol by calling
// OnDataAvailable. We then read and process the incoming data from the input stream.
NS_IMETHODIMP MsgProtocolBase::OnDataAvailable(nsIRequest *request, nsISupports *ctxt, nsIInputStream *inStr, uint64_t sourceOffset, uint32_t count)
{
  // right now, this really just means turn around and churn through the state machine
  nsCOMPtr<nsIURI> uri = do_QueryInterface(ctxt);
  return ProcessProtocolState(uri, inStr, sourceOffset, count);
}

NS_IMETHODIMP MsgProtocolBase::OnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
  nsresult rv = NS_OK;
  nsCOMPtr <nsIMsgMailNewsUrl> aMsgUrl = do_QueryInterface(ctxt, &rv);
  if (NS_SUCCEEDED(rv) && aMsgUrl)
  {
    rv = aMsgUrl->SetUrlState(true, NS_OK);
    if (m_loadGroup)
      m_loadGroup->AddRequest(static_cast<nsIRequest *>(this), nullptr /* context isupports */);
  }

  // if we are set up as a channel, we should notify our channel listener that we are starting...
  // so pass in ourself as the channel and not the underlying socket or file channel the protocol
  // happens to be using
  if (!mSuppressListenerNotifications && m_channelListener)
  {
    if (!m_channelContext)
      m_channelContext = do_QueryInterface(ctxt);
    rv = m_channelListener->OnStartRequest(this, m_channelContext);
  }

  nsCOMPtr<nsISocketTransport> strans = do_QueryInterface(m_transport);

  if (strans)
    strans->SetTimeout(nsISocketTransport::TIMEOUT_READ_WRITE, gSocketTimeout);

  NS_ENSURE_SUCCESS(rv, rv);
  return rv;
}

// stop binding is a "notification" informing us that the stream associated with aURL is going away.
NS_IMETHODIMP MsgProtocolBase::OnStopRequest(nsIRequest *request, nsISupports *ctxt, nsresult aStatus)
{
  nsresult rv = NS_OK;

  // if we are set up as a channel, we should notify our channel listener that we are starting...
  // so pass in ourself as the channel and not the underlying socket or file channel the protocol
  // happens to be using
  if (!mSuppressListenerNotifications && m_channelListener)
    rv = m_channelListener->OnStopRequest(this, m_channelContext, aStatus);

  nsCOMPtr <nsIMsgMailNewsUrl> msgUrl = do_QueryInterface(ctxt, &rv);
  if (NS_SUCCEEDED(rv) && msgUrl)
  {
    rv = msgUrl->SetUrlState(false, aStatus);
    if (m_loadGroup)
      m_loadGroup->RemoveRequest(static_cast<nsIRequest *>(this), nullptr, aStatus);

    // !m_channelContext because if we're set up as a channel, then the remove
    // request above will handle alerting the user, so we don't need to.
    //
    // !NS_BINDING_ABORTED because we don't want to see an alert if the user
    // cancelled the operation.  also, we'll get here because we call Cancel()
    // to force removal of the nsSocketTransport.  see CloseSocket()
    // bugs #30775 and #30648 relate to this
    if (!m_channelContext && NS_FAILED(aStatus) &&
        (aStatus != NS_BINDING_ABORTED))
    {
      int32_t errorID;
      switch (aStatus)
      {
          case NS_ERROR_UNKNOWN_HOST:
          case NS_ERROR_UNKNOWN_PROXY_HOST:
             errorID = UNKNOWN_HOST_ERROR;
             break;
          case NS_ERROR_CONNECTION_REFUSED:
          case NS_ERROR_PROXY_CONNECTION_REFUSED:
             errorID = CONNECTION_REFUSED_ERROR;
             break;
          case NS_ERROR_NET_TIMEOUT:
             errorID = NET_TIMEOUT_ERROR;
             break;
          default:
             errorID = UNKNOWN_ERROR;
             break;
      }

      NS_ASSERTION(errorID != UNKNOWN_ERROR, "unknown error, but don't alert user.");
      if (errorID != UNKNOWN_ERROR)
      {
        nsString errorMsg;
        errorMsg.Adopt(FormatStringWithHostNameByID(errorID, msgUrl));
        if (errorMsg.IsEmpty())
        {
          errorMsg.Assign(NS_LITERAL_STRING("[StringID "));
          errorMsg.AppendInt(errorID);
          errorMsg.AppendLiteral("?]");
        }

        nsCOMPtr<nsIMsgMailSession> mailSession =
          do_GetService(NS_MSGMAILSESSION_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mailSession->AlertUser(errorMsg, msgUrl);
      }
    } // if we got an error code
  } // if we have a mailnews url.

  // Drop notification callbacks to prevent cycles.
  mCallbacks = 0;
  mProgressEventSink = 0;
  // Call CloseSocket(), in case we got here because the server dropped the
  // connection while reading, and we never get a chance to get back into
  // the protocol state machine via OnDataAvailable.
  if (m_socketIsOpen)
    CloseSocket();

  return rv;
}

nsresult MsgProtocolBase::GetPromptDialogFromUrl(nsIMsgMailNewsUrl * aMsgUrl, nsIPrompt ** aPromptDialog)
{
  // get the nsIPrompt interface from the message window associated wit this url.
  nsCOMPtr<nsIMsgWindow> msgWindow;
  aMsgUrl->GetMsgWindow(getter_AddRefs(msgWindow));
  NS_ENSURE_TRUE(msgWindow, NS_ERROR_FAILURE);

  msgWindow->GetPromptDialog(aPromptDialog);

  NS_ENSURE_TRUE(*aPromptDialog, NS_ERROR_FAILURE);

  return NS_OK;
}

nsresult MsgProtocolBase::LoadUrl(nsIURI * aURL, nsISupports * aConsumer)
{
  // okay now kick us off to the next state...
  // our first state is a process state so drive the state machine...
  nsresult rv = NS_OK;
  nsCOMPtr <nsIMsgMailNewsUrl> aMsgUrl = do_QueryInterface(aURL, &rv);

  if (NS_SUCCEEDED(rv) && aMsgUrl)
  {
    bool msgIsInLocalCache;
    aMsgUrl->GetMsgIsInLocalCache(&msgIsInLocalCache);

    rv = aMsgUrl->SetUrlState(true, NS_OK); // set the url as a url currently being run...

    // if the url is given a stream consumer then we should use it to forward calls to...
    if (!m_channelListener && aConsumer) // if we don't have a registered listener already
    {
      m_channelListener = do_QueryInterface(aConsumer);
      if (!m_channelContext)
        m_channelContext = do_QueryInterface(aURL);
    }

    if (!m_socketIsOpen)
    {
      nsCOMPtr<nsISupports> urlSupports = do_QueryInterface(aURL);
      if (m_transport)
      {
        // don't open the input stream more than once
        if (!m_inputStream)
        {
          // open buffered, asynchronous input stream
          rv = m_transport->OpenInputStream(0, 0, 0, getter_AddRefs(m_inputStream));
          if (NS_FAILED(rv)) return rv;
        }

        nsCOMPtr<nsIInputStreamPump> pump;
        rv = NS_NewInputStreamPump(getter_AddRefs(pump),
          m_inputStream, -1, m_readCount);
        if (NS_FAILED(rv)) return rv;

        m_request = pump; // keep a reference to the pump so we can cancel it

        // put us in a state where we are always notified of incoming data
        rv = pump->AsyncRead(this, urlSupports);
        NS_ASSERTION(NS_SUCCEEDED(rv), "AsyncRead failed");
        m_socketIsOpen = true; // mark the channel as open
      }
    } // if we got an event queue service
    else if (!msgIsInLocalCache) // the connection is already open so we should begin processing our new url...
      rv = ProcessProtocolState(aURL, nullptr, 0, 0);
  }

  return rv;
}

///////////////////////////////////////////////////////////////////////
// The rest of this file is mostly nsIChannel mumbo jumbo stuff
///////////////////////////////////////////////////////////////////////

nsresult MsgProtocolBase::SetUrl(nsIURI * aURL)
{
  m_url = aURL;
  return NS_OK;
}

NS_IMETHODIMP MsgProtocolBase::SetLoadGroup(nsILoadGroup * aLoadGroup)
{
  m_loadGroup = aLoadGroup;
  return NS_OK;
}

NS_IMETHODIMP MsgProtocolBase::GetOriginalURI(nsIURI* *aURI)
{
    *aURI = m_originalUrl ? m_originalUrl : m_url;
    NS_IF_ADDREF(*aURI);
    return NS_OK;
}

NS_IMETHODIMP MsgProtocolBase::SetOriginalURI(nsIURI* aURI)
{
    m_originalUrl = aURI;
    return NS_OK;
}

NS_IMETHODIMP MsgProtocolBase::GetURI(nsIURI* *aURI)
{
    *aURI = m_url;
    NS_IF_ADDREF(*aURI);
    return NS_OK;
}

NS_IMETHODIMP MsgProtocolBase::Open(nsIInputStream **_retval)
{
  return NS_ImplementChannelOpen(this, _retval);
}

NS_IMETHODIMP MsgProtocolBase::AsyncOpen(nsIStreamListener *listener, nsISupports *ctxt)
{
    int32_t port;
    nsresult rv = m_url->GetPort(&port);
    if (NS_FAILED(rv))
        return rv;

    nsAutoCString scheme;
    rv = m_url->GetScheme(scheme);
    if (NS_FAILED(rv))
        return rv;


    rv = NS_CheckPortSafety(port, scheme.get());
    if (NS_FAILED(rv))
        return rv;

    // set the stream listener and then load the url
    m_channelContext = ctxt;
    m_channelListener = listener;
    return LoadUrl(m_url, nullptr);
}

NS_IMETHODIMP MsgProtocolBase::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
    *aLoadFlags = mLoadFlags;
    return NS_OK;
}

NS_IMETHODIMP MsgProtocolBase::SetLoadFlags(nsLoadFlags aLoadFlags)
{
  mLoadFlags = aLoadFlags;
  return NS_OK;       // don't fail when trying to set this
}

NS_IMETHODIMP MsgProtocolBase::GetContentType(nsACString &aContentType)
{
  // as url dispatching matures, we'll be intelligent and actually start
  // opening the url before specifying the content type. This will allow
  // us to optimize the case where the message url actual refers to
  // a part in the message that has a content type that is not message/rfc822

  if (m_ContentType.IsEmpty())
    aContentType.AssignLiteral("message/rfc822");
  else
    aContentType = m_ContentType;
  return NS_OK;
}

NS_IMETHODIMP MsgProtocolBase::SetContentType(const nsACString &aContentType)
{
  nsAutoCString charset;
  nsresult rv = NS_ParseContentType(aContentType, m_ContentType, charset);
  if (NS_FAILED(rv) || m_ContentType.IsEmpty())
    m_ContentType.AssignLiteral(UNKNOWN_CONTENT_TYPE);
  return rv;
}

NS_IMETHODIMP MsgProtocolBase::GetContentCharset(nsACString &aContentCharset)
{
  aContentCharset = m_ContentCharset;
  return NS_OK;
}

NS_IMETHODIMP MsgProtocolBase::SetContentCharset(const nsACString &aContentCharset)
{
  m_ContentCharset = aContentCharset;
  return NS_OK;
}

NS_IMETHODIMP
MsgProtocolBase::GetContentDisposition(uint32_t *aContentDisposition)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
MsgProtocolBase::SetContentDisposition(uint32_t aContentDisposition)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
MsgProtocolBase::GetContentDispositionFilename(nsAString &aContentDispositionFilename)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
MsgProtocolBase::SetContentDispositionFilename(const nsAString &aContentDispositionFilename)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
MsgProtocolBase::GetContentDispositionHeader(nsACString &aContentDispositionHeader)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP MsgProtocolBase::GetContentLength(int64_t *aContentLength)
{
  *aContentLength = mContentLength;
  return NS_OK;
}

NS_IMETHODIMP MsgProtocolBase::SetContentLength(int64_t aContentLength)
{
  mContentLength = aContentLength;
  return NS_OK;
}

NS_IMETHODIMP MsgProtocolBase::GetSecurityInfo(nsISupports * *aSecurityInfo)
{
  *aSecurityInfo = nullptr;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP MsgProtocolBase::GetName(nsACString &result)
{
  if (m_url)
    return m_url->GetSpec(result);
  result.Truncate();
  return NS_OK;
}

NS_IMETHODIMP MsgProtocolBase::GetOwner(nsISupports * *aPrincipal)
{
  *aPrincipal = mOwner;
  NS_IF_ADDREF(*aPrincipal);
  return NS_OK;
}

NS_IMETHODIMP MsgProtocolBase::SetOwner(nsISupports * aPrincipal)
{
  mOwner = aPrincipal;
  return NS_OK;
}

NS_IMETHODIMP MsgProtocolBase::GetLoadGroup(nsILoadGroup * *aLoadGroup)
{
  *aLoadGroup = m_loadGroup;
  NS_IF_ADDREF(*aLoadGroup);
  return NS_OK;
}

NS_IMETHODIMP
MsgProtocolBase::GetNotificationCallbacks(nsIInterfaceRequestor* *aNotificationCallbacks)
{
  *aNotificationCallbacks = mCallbacks.get();
  NS_IF_ADDREF(*aNotificationCallbacks);
  return NS_OK;
}

NS_IMETHODIMP
MsgProtocolBase::SetNotificationCallbacks(nsIInterfaceRequestor* aNotificationCallbacks)
{
  mCallbacks = aNotificationCallbacks;
  return NS_OK;
}

NS_IMETHODIMP
MsgProtocolBase::OnTransportStatus(nsITransport *transport, nsresult status,
                                 int64_t progress, int64_t progressMax)
{
  if ((mLoadFlags & LOAD_BACKGROUND) || !m_url)
    return NS_OK;

  // these transport events should not generate any status messages
  if (status == NS_NET_STATUS_RECEIVING_FROM ||
      status == NS_NET_STATUS_SENDING_TO)
    return NS_OK;

  if (!mProgressEventSink)
  {
    NS_QueryNotificationCallbacks(mCallbacks, m_loadGroup, mProgressEventSink);
    if (!mProgressEventSink)
      return NS_OK;
  }

  nsAutoCString host;
  m_url->GetHost(host);

  nsCOMPtr<nsIMsgMailNewsUrl> mailnewsUrl = do_QueryInterface(m_url);
  if (mailnewsUrl)
  {
    nsCOMPtr<nsIMsgIncomingServer> server;
    mailnewsUrl->GetServer(getter_AddRefs(server));
    if (server)
      server->GetRealHostName(host);
  }
  mProgressEventSink->OnStatus(this, nullptr, status,
                               NS_ConvertUTF8toUTF16(host).get());

  return NS_OK;
}

////////////////////////////////////////////////////////////////////////////////
// From nsIRequest
////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP MsgProtocolBase::IsPending(bool *result)
{
    *result = m_channelListener != nullptr;
    return NS_OK;
}

NS_IMETHODIMP MsgProtocolBase::GetStatus(nsresult *status)
{
  if (m_request)
    return m_request->GetStatus(status);

  *status = NS_OK;
  return *status;
}

NS_IMETHODIMP MsgProtocolBase::Cancel(nsresult status)
{
  NS_ASSERTION(m_request,"no channel");
  if (!m_request)
    return NS_ERROR_FAILURE;

  return m_request->Cancel(status);
}

NS_IMETHODIMP MsgProtocolBase::Suspend()
{
  if (m_request)
    return m_request->Suspend();

  NS_WARNING("no request to suspend");
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP MsgProtocolBase::Resume()
{
  if (m_request)
    return m_request->Resume();

  NS_WARNING("no request to resume");
  return NS_ERROR_NOT_AVAILABLE;
}

nsresult MsgProtocolBase::PostMessage(nsIURI* url, nsIFile *postFile)
{
  if (!url || !postFile) return NS_ERROR_NULL_POINTER;

#define POST_DATA_BUFFER_SIZE 2048

  // mscott -- this function should be re-written to use the file url code
  // so it can be asynch
  nsCOMPtr<nsIInputStream> inputStream;
  nsresult rv = NS_NewLocalFileInputStream(getter_AddRefs(inputStream), postFile);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsILineInputStream> lineInputStream(do_QueryInterface(inputStream, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  bool more = true;
  nsCString line;
  nsCString outputBuffer;

  do
  {
    lineInputStream->ReadLine(line, &more);

    /* escape starting periods
     */
    if (line.CharAt(0) == '.')
      line.Insert('.', 0);
    line.Append(NS_LITERAL_CSTRING(CRLF));
    outputBuffer.Append(line);
    // test hack by mscott. If our buffer is almost full, then
    // send it off & reset ourselves
    // to make more room.
    if (outputBuffer.Length() > POST_DATA_BUFFER_SIZE || !more)
    {
      rv = SendData(outputBuffer.get());
      NS_ENSURE_SUCCESS(rv, rv);
      // does this keep the buffer around? That would be best.
      // Maybe SetLength(0) instead?
      outputBuffer.Truncate();
    }
  } while (more);

  return NS_OK;
}

nsresult MsgProtocolBase::DoGSSAPIStep1(const char *service, const char *username, nsCString &response)
{
    nsresult rv;
#ifdef DEBUG_BenB
    printf("GSSAPI step 1 for service %s, username %s\n", service, username);
#endif

    // if this fails, then it means that we cannot do GSSAPI SASL.
    m_authModule = do_CreateInstance(NS_AUTH_MODULE_CONTRACTID_PREFIX "sasl-gssapi", &rv);
    NS_ENSURE_SUCCESS(rv,rv);

    m_authModule->Init(service, nsIAuthModule::REQ_DEFAULT, nullptr, NS_ConvertUTF8toUTF16(username).get(), nullptr);

    void *outBuf;
    uint32_t outBufLen;
    rv = m_authModule->GetNextToken((void *)nullptr, 0, &outBuf, &outBufLen);
    if (NS_SUCCEEDED(rv) && outBuf)
    {
        char *base64Str = PL_Base64Encode((char *)outBuf, outBufLen, nullptr);
        if (base64Str)
            response.Adopt(base64Str);
        else
            rv = NS_ERROR_OUT_OF_MEMORY;
        nsMemory::Free(outBuf);
    }

#ifdef DEBUG_BenB
    printf("GSSAPI step 1 succeeded\n");
#endif
    return rv;
}

nsresult MsgProtocolBase::DoGSSAPIStep2(nsCString &commandResponse, nsCString &response)
{
#ifdef DEBUG_BenB
    printf("GSSAPI step 2\n");
#endif
    nsresult rv;
    void *inBuf, *outBuf;
    uint32_t inBufLen, outBufLen;
    uint32_t len = commandResponse.Length();

    // Cyrus SASL may send us zero length tokens (grrrr)
    if (len > 0) {
        // decode into the input secbuffer
        inBufLen = (len * 3)/4;      // sufficient size (see plbase64.h)
        inBuf = nsMemory::Alloc(inBufLen);
        if (!inBuf)
            return NS_ERROR_OUT_OF_MEMORY;

        // strip off any padding (see bug 230351)
        const char *challenge = commandResponse.get();
        while (challenge[len - 1] == '=')
            len--;

        // We need to know the exact length of the decoded string to give to
        // the GSSAPI libraries. But NSPR's base64 routine doesn't seem capable
        // of telling us that. So, we figure it out for ourselves.

        // For every 4 characters, add 3 to the destination
        // If there are 3 remaining, add 2
        // If there are 2 remaining, add 1
        // 1 remaining is an error
        inBufLen = (len / 4)*3 + ((len % 4 == 3)?2:0) + ((len % 4 == 2)?1:0);

        rv = (PL_Base64Decode(challenge, len, (char *)inBuf))
             ? m_authModule->GetNextToken(inBuf, inBufLen, &outBuf, &outBufLen)
             : NS_ERROR_FAILURE;

        nsMemory::Free(inBuf);
    }
    else
    {
        rv = m_authModule->GetNextToken(NULL, 0, &outBuf, &outBufLen);
    }
    if (NS_SUCCEEDED(rv))
    {
        // And in return, we may need to send Cyrus zero length tokens back
        if (outBuf)
        {
            char *base64Str = PL_Base64Encode((char *)outBuf, outBufLen, nullptr);
            if (base64Str)
                response.Adopt(base64Str);
            else
                rv = NS_ERROR_OUT_OF_MEMORY;
        }
        else
            response.Adopt((char *)nsMemory::Clone("",1));
    }

#ifdef DEBUG_BenB
    printf(NS_SUCCEEDED(rv) ? "GSSAPI step 2 succeeded\n" : "GSSAPI step 2 failed\n");
#endif
    return rv;
}

nsresult MsgProtocolBase::DoNtlmStep1(const char *username, const char *password, nsCString &response)
{
    nsresult rv;

    m_authModule = do_CreateInstance(NS_AUTH_MODULE_CONTRACTID_PREFIX "ntlm", &rv);
    // if this fails, then it means that we cannot do NTLM auth.
    if (NS_FAILED(rv) || !m_authModule)
        return rv;

    m_authModule->Init(nullptr, 0, nullptr, NS_ConvertUTF8toUTF16(username).get(),
                       NS_ConvertUTF8toUTF16(password).get());

    void *outBuf;
    uint32_t outBufLen;
    rv = m_authModule->GetNextToken((void *)nullptr, 0, &outBuf, &outBufLen);
    if (NS_SUCCEEDED(rv) && outBuf)
    {
        char *base64Str = PL_Base64Encode((char *)outBuf, outBufLen, nullptr);
        if (base64Str)
          response.Adopt(base64Str);
        else
          rv = NS_ERROR_OUT_OF_MEMORY;
        nsMemory::Free(outBuf);
    }

    return rv;
}

nsresult MsgProtocolBase::DoNtlmStep2(nsCString &commandResponse, nsCString &response)
{
    nsresult rv;
    void *inBuf, *outBuf;
    uint32_t inBufLen, outBufLen;
    uint32_t len = commandResponse.Length();

    // decode into the input secbuffer
    inBufLen = (len * 3)/4;      // sufficient size (see plbase64.h)
    inBuf = nsMemory::Alloc(inBufLen);
    if (!inBuf)
        return NS_ERROR_OUT_OF_MEMORY;

    // strip off any padding (see bug 230351)
    const char *challenge = commandResponse.get();
    while (challenge[len - 1] == '=')
        len--;

    rv = (PL_Base64Decode(challenge, len, (char *)inBuf))
         ? m_authModule->GetNextToken(inBuf, inBufLen, &outBuf, &outBufLen)
         : NS_ERROR_FAILURE;

    nsMemory::Free(inBuf);
    if (NS_SUCCEEDED(rv) && outBuf)
    {
        char *base64Str = PL_Base64Encode((char *)outBuf, outBufLen, nullptr);
        if (base64Str)
          response.Adopt(base64Str);
        else
          rv = NS_ERROR_OUT_OF_MEMORY;
    }

    if (NS_FAILED(rv))
        response = "*";

    return rv;
}

char16_t *FormatStringWithHostNameByID(int32_t stringID, nsIMsgMailNewsUrl *msgUri)
{
  if (!msgUri)
    return nullptr;

  nsresult rv;

  nsCOMPtr<nsIStringBundleService> sBundleService =
    mozilla::services::GetStringBundleService();
  NS_ENSURE_TRUE(sBundleService, nullptr);

  nsCOMPtr<nsIStringBundle> sBundle;
  rv = sBundleService->CreateBundle(MSGS_URL, getter_AddRefs(sBundle));
  NS_ENSURE_SUCCESS(rv, nullptr);

  char16_t *ptrv = nullptr;
  nsCOMPtr<nsIMsgIncomingServer> server;
  rv = msgUri->GetServer(getter_AddRefs(server));
  NS_ENSURE_SUCCESS(rv, nullptr);

  nsCString hostName;
  rv = server->GetRealHostName(hostName);
  NS_ENSURE_SUCCESS(rv, nullptr);

  NS_ConvertASCIItoUTF16 hostStr(hostName);
  const char16_t *params[] = { hostStr.get() };
  rv = sBundle->FormatStringFromID(stringID, params, 1, &ptrv);
  NS_ENSURE_SUCCESS(rv, nullptr);

  return ptrv;
}

NS_IMETHODIMP MsgProtocolBase::GetLoadInfo(nsILoadInfo **aLoadInfo)
{
  *aLoadInfo = m_loadInfo;
  NS_IF_ADDREF(*aLoadInfo);
  return NS_OK;
}

NS_IMETHODIMP MsgProtocolBase::SetLoadInfo(nsILoadInfo *aLoadInfo)
{
  m_loadInfo = aLoadInfo;
  return NS_OK;
}
// vim: ts=2 sw=2

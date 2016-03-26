/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MailEwsMsgUrl_h__
#define MailEwsMsgUrl_h__

#include "mozilla/Attributes.h"
#include "nsIMailboxUrl.h"
#include "MsgMailNewsUrlBase.h"
#include "nsIFile.h"
#include "nsCOMPtr.h"
#include "MailNewsTypes.h"
#include "nsTArray.h"

class MailEwsMsgUrl : public nsIMailboxUrl, public MsgMailNewsUrlBase, public nsIMsgMessageUrl, public nsIMsgI18NUrl
{
public:
  // nsIURI over-ride...
  NS_IMETHOD SetSpec(const nsACString &aSpec) override;
  NS_IMETHOD SetQuery(const nsACString &aQuery) override;

  NS_IMETHOD GetFolder(nsIMsgFolder **msgFolder) override;

  // nsIMsgMailNewsUrl override
  NS_IMETHOD Clone(nsIURI **_retval) override;

  // MailEwsMsgUrl
  MailEwsMsgUrl();
  NS_DECL_NSIMSGMESSAGEURL
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIMSGI18NURL
  NS_DECL_NSIMAILBOXURL
  
  NS_IMETHOD IsUrlType(uint32_t type, bool *isType) override;

protected:
  virtual ~MailEwsMsgUrl();
  // protocol specific code to parse a url...
  virtual nsresult ParseUrl();
  nsresult GetMsgHdrForKey(nsMsgKey  msgKey, nsIMsgDBHdr ** aMsgHdr);

  // mailboxurl specific state
  nsCOMPtr<nsIStreamListener> m_mailboxParser;
  nsCOMPtr<nsIStreamListener> m_mailboxCopyHandler;

  nsMailboxAction m_mailboxAction; // the action this url represents...parse mailbox, display messages, etc.
  char *m_messageID;
  uint32_t m_messageSize;
  nsMsgKey m_messageKey;
  nsCString m_file;
  // This is currently only set when we're doing something with a .eml file.
  // If that changes, we should change the name of this var.
  nsCOMPtr<nsIMsgDBHdr> m_dummyHdr;

  // used by save message to disk
  nsCOMPtr<nsIFile> m_messageFile;
  bool                  m_addDummyEnvelope;
  bool                  m_canonicalLineEnding;
  nsresult ParseSearchPart();

  // for multiple msg move/copy
  nsTArray<nsMsgKey> m_keys;

  // truncated message support
  nsCString m_originalSpec;
  nsCString mURI; // the RDF URI associated with this url.
  nsCString mCharsetOverride; // used by nsIMsgI18NUrl...

  uint32_t m_curMsgIndex;
};

#endif // MailEwsMsgUrl_h__

/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
#pragma once
#ifndef __MAIL_EWS_MSG_MESSAGE_SERVICE_H__
#define __MAIL_EWS_MSG_MESSAGE_SERVICE_H__

#include "nsIMsgMessageService.h"
#include "nsCOMPtr.h"
#include "nsIFile.h"
#include "nsIURL.h"
#include "nsIUrlListener.h"
#include "nsIStreamListener.h"
#include "nsIProtocolHandler.h"
#include "nsIRDFService.h"

class nsCString;
class nsIMsgFolder;
class nsIMsgStatusFeedback;
class nsIMsgIncomingServer;

class MailEwsMsgMessageService : public nsIMsgMessageService,
    public nsIMsgMessageFetchPartService, public nsIProtocolHandler
{
public:
  MailEwsMsgMessageService();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIMSGMESSAGESERVICE
  NS_DECL_NSIMSGMESSAGEFETCHPARTSERVICE
  NS_DECL_NSIPROTOCOLHANDLER

protected:
  virtual ~MailEwsMsgMessageService();
  
  bool mPrintingOperation;
};

#endif //__MAIL_EWS_MSG_MESSAGE_SERVICE_H__

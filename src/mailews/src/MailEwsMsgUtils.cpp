/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nscore.h"
#include "prthread.h"
#include "nsStringGlue.h"
#include "MsgUtils.h"
#include "nsUnicharUtils.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIIOService.h"
#include "nsIURI.h"
#include "nsIOutputStream.h"
#include "nsThreadUtils.h"
#include "nsIMsgHdr.h"
#include "nsMsgBaseCID.h"
#include "nsMsgCompCID.h"
#include "nsIMsgDatabase.h"
#include "nsISeekableStream.h"
#include "nsDirectoryServiceDefs.h"

#include "nsIMsgCompose.h"
#include "nsIMsgCompFields.h"
#include "nsIMsgSend.h"
#include "nsIMsgAccountManager.h"
#include "nsIStringEnumerator.h"
#include "nsIPrefService.h"
#include "nsIMsgParseMailMsgState.h"
#include "nsMsgLocalCID.h"

#include "MsgI18N.h"

#include "nsMsgMessageFlags.h"
#include "nsNetCID.h"

#include "MailEwsMsgUtils.h"

#include "nsMimeTypes.h"
#include "nsCRT.h"
#include "nsNetUtil.h"
#include "nsAutoPtr.h"
#include "nsIMutableArray.h"

#include "nsIMsgFolder.h"
#include "nsIMsgPluggableStore.h"
#include "nsReadLine.h"

#include "plbase64.h"
#include "prmem.h"

#include <time.h>
#include "MailEwsLog.h"
#include <string>

#include "IMailEwsService.h"
#include "IMailEwsMsgIncomingServer.h"

static NS_DEFINE_CID(kMsgSendCID, NS_MSGSEND_CID);
static NS_DEFINE_CID(kMsgCompFieldsCID, NS_MSGCOMPFIELDS_CID);

#define IMPORT_LOG0(x)          {mailews_logger << "IMPORT LOG0:" << (x) << std::endl;}
#define IMPORT_LOG1(x, y)                 {mailews_logger << "IMPORT LOG1:" << (x) << "," << (y) << std::endl;}
#define IMPORT_LOG2(x, y, z)              {mailews_logger << "IMPORT LOG2:" << (x) << "," << (y) << "," << (z) <<  std::endl;}
#define IMPORT_LOG3(a, b, c, d)           {mailews_logger << "IMPORT LOG3:" << (a) << "," << (b) << "," << (c) << "," << (d) << std::endl;}

// We need to do some calculations to set these numbers to something reasonable!
// Unless of course, CreateAndSendMessage will NEVER EVER leave us in the lurch
#define kHungCount 100000
#define kHungAbortCount 1000

// Define maximum possible length for content type sanity check
#define kContentTypeLengthSanityCheck 32

#define kWhitespace "\b\t\r\n "

nsIMsgIdentity * MailEwsMsgCompose::s_pIdentity = nullptr;

NS_IMPL_ISUPPORTS(MailEwsSendListener, nsIMsgSendListener)

/* void OnStopSending (in string aMsgID, in nsresult aStatus, in wstring aMsg, in nsIFile returnFile); */
NS_IMETHODIMP MailEwsSendListener::OnStopSending(const char *aMsgID,
                                                 nsresult aStatus,
                                                 const char16_t *aMsg,
                                                 nsIFile *returnFile) {
    IMPORT_LOG0("Send end!");
    m_done = true;
    m_location = returnFile;
    nsString path;
    returnFile->GetPath(path);
    NS_LossyConvertUTF16toASCII_external convertString(path);

    mailews_logger << "msg file:"
              << convertString.get()
              << std::endl;
    return NS_OK;
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////



MailEwsMsgCompose::MailEwsMsgCompose()
{
    m_pAttachments = nullptr;
    m_pMsgFields = nullptr;
}


MailEwsMsgCompose::~MailEwsMsgCompose()
{
    NS_IF_RELEASE(m_pMsgFields);
}

nsresult MailEwsMsgCompose::CreateIdentity(void)
{
    if (s_pIdentity)
        return NS_OK;

    // Should only create identity from main thread
    NS_ENSURE_TRUE(NS_IsMainThread(), NS_ERROR_FAILURE);
    nsresult rv;
    nsCOMPtr<nsIMsgAccountManager> accMgr(do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = accMgr->CreateIdentity(&s_pIdentity);
    nsString name(NS_LITERAL_STRING("Import Identity"));
    if (s_pIdentity) {
        s_pIdentity->SetFullName(name);
        s_pIdentity->SetIdentityName(name);
        s_pIdentity->SetEmail(NS_LITERAL_CSTRING("import@import.service"));

        // SetDoFcc to false to save time when CreateAndSendMessage operates.
        // Profiling revealed that GetFolderURIFromUserPrefs was taking up a significant chunk
        // of time during the operation of CreateAndSendMessage. By calling SetDoFcc(false),
        // we skip Fcc handling code inside of InitCompositionFields (called indirectly during
        // CreateAndSendMessage operation). There's no point in any Fcc code firing since the
        // message will never actually be sent anyway.
        s_pIdentity->SetDoFcc(false);
    }
    return rv;
}

void MailEwsMsgCompose::ReleaseIdentity(void)
{
    if (s_pIdentity) {
        nsresult rv = s_pIdentity->ClearAllValues();
        NS_ASSERTION(NS_SUCCEEDED(rv),"failed to clear values");
        if (NS_FAILED(rv)) return;

        NS_RELEASE(s_pIdentity);
    }
}


nsresult MailEwsMsgCompose::CreateComponents(void)
{
    nsresult  rv = NS_OK;

    if (!m_pIOService) {
        IMPORT_LOG0("Creating nsIOService\n");
    
        m_pIOService = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    NS_IF_RELEASE(m_pMsgFields);

    if (NS_SUCCEEDED(rv)) {
        rv = CallCreateInstance(kMsgCompFieldsCID, &m_pMsgFields);
        if (NS_SUCCEEDED(rv) && m_pMsgFields) {
            IMPORT_LOG0("nsOutlookCompose - CreateComponents succeeded\n");
            m_pMsgFields->SetForcePlainText(false);
            return NS_OK;
        }
    }

    return NS_ERROR_FAILURE;
}

nsresult MailEwsMsgCompose::GetLocalAttachments(nsIArray **aArray)
{
    /*
      nsIURI      *url = nullptr;
    */
    nsresult rv;
    nsCOMPtr<nsIMutableArray> attachments (do_CreateInstance(NS_ARRAY_CONTRACTID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_IF_ADDREF(*aArray = attachments);
    int32_t count = 0;
    if (m_pAttachments)
        count = m_pAttachments->Count();
    if (!count)
        return NS_OK;

    nsCString urlStr;
    ImportAttachment * pAttach;

    for (int32_t i = 0; i < count; i++) {
        nsCOMPtr<nsIMsgAttachedFile> a(do_CreateInstance(NS_MSGATTACHEDFILE_CONTRACTID, &rv));
        NS_ENSURE_SUCCESS(rv, rv);
        // nsMsgNewURL(&url, "file://C:/boxster.jpg");
        // a[i].orig_url = url;

        pAttach = (ImportAttachment *) m_pAttachments->ElementAt(i);
        nsCOMPtr<nsIFile> tmpFile = do_QueryInterface(pAttach->pAttachment);
        a->SetTmpFile(tmpFile);
        urlStr.Adopt(0);

        nsCOMPtr <nsIURI> uri;
        nsresult rv = NS_NewFileURI(getter_AddRefs(uri), pAttach->pAttachment);
        NS_ENSURE_SUCCESS(rv, rv);
        uri->GetSpec(urlStr);
        if (urlStr.IsEmpty())
            return NS_ERROR_FAILURE;

        nsCOMPtr<nsIURI> origUrl;
        rv = m_pIOService->NewURI(urlStr, nullptr, nullptr, getter_AddRefs(origUrl));
        NS_ENSURE_SUCCESS(rv, rv);
        a->SetOrigUrl(origUrl);
        a->SetType(nsDependentCString(pAttach->mimeType));
        a->SetRealName(nsDependentCString(pAttach->description));
        a->SetEncoding(NS_LITERAL_CSTRING(ENCODING_BINARY));
        attachments->AppendElement(a, false);
    }
    return NS_OK;
}

void RecipientToStdString(ews_recipient * r, int c, std::string & v) {
	v.assign("");

	for(int i=0;i < c;i++) {
		if (!r[i].mailbox.email) continue;

		if (!v.empty())
			v.append(", ");

		if (r[i].mailbox.name) {
			v.append(r[i].mailbox.name);
			v.append(" <");
		}

		v.append(r[i].mailbox.email);

		if (r[i].mailbox.name) {
			v.append(">");
		}
	}
}

void RecipientToString2(ews_recipient * r, int c, nsString & v) {
    std::string vv("");

    RecipientToStdString(r, c, vv);

    v.AssignLiteral(vv.c_str());
}

void RecipientToString(ews_recipient * r, nsString & v) {
	RecipientToString2(r, 1, v);
}

nsresult MailEwsMsgCompose::SendTheMessage(ews_msg_item * msg_item,
                                           nsIMsgSendListener *pListener)
{
    nsresult rv = CreateComponents();
    if (NS_FAILED(rv))
        return rv;

    IMPORT_LOG0("Outlook Compose created necessary components\n");
    nsString bodyType;
    nsString charSet;
    nsString headerVal;

    RecipientToString(&msg_item->from, headerVal);
    if (!headerVal.IsEmpty())
        m_pMsgFields->SetFrom(headerVal);
    else {
        RecipientToString(&msg_item->sender, headerVal);
        if (!headerVal.IsEmpty())
            m_pMsgFields->SetFrom(headerVal);
    }
  
    RecipientToString2(msg_item->to_recipients,
                       msg_item->to_recipients_count,
                       headerVal);
    if (!headerVal.IsEmpty())
        m_pMsgFields->SetTo(headerVal);

    if (msg_item->item.subject) {
        char * key = msg_item->item.subject;
        uint32_t l = strlen(key);

        /* strip "Re: " */
        nsCString modifiedSubject;
        if (mailews::NS_MsgStripRE((const char **) &key,
                          &l,
                          modifiedSubject)) {
            mailews::NS_GetLocalizedUnicharPreferenceWithDefault(nullptr,
                                                        "mailnews.localizedRe",
                                                        EmptyString(),
                                                        headerVal);
            if (headerVal.IsEmpty()) {
                headerVal.AppendLiteral("Re");
            }
            
            headerVal.AppendLiteral(": ");
        } else {
            headerVal.AssignLiteral("");
        }

        if (modifiedSubject.IsEmpty()) {
            headerVal.AppendLiteral(key);
        } else {
            headerVal.AppendLiteral(modifiedSubject.get());
        }
        
        m_pMsgFields->SetSubject(headerVal);
    }
  
    if (msg_item->item.body_type == EWS_BODY_TEXT)
        bodyType.AssignLiteral("text/plain");
    else
        bodyType.AssignLiteral("text/html");
    headerVal.AssignLiteral("UTF-8");

    // Use platform charset as default if the msg doesn't specify one
    // (ie, no 'charset' param in the Content-Type: header). As the last
    // resort we'll use the mail default charset.
    // (ie, no 'charset' param in the Content-Type: header) or if the
    // charset parameter fails a length sanity check.
    // As the last resort we'll use the mail default charset.
    if (headerVal.IsEmpty() || (headerVal.Length() > kContentTypeLengthSanityCheck))
    {
        headerVal.AssignASCII(mailews::MsgI18NFileSystemCharset());
        if (headerVal.IsEmpty())
        { // last resort
            if (m_defCharset.IsEmpty())
            {
                nsString defaultCharset;
                mailews::NS_GetLocalizedUnicharPreferenceWithDefault(nullptr, "mailnews.view_default_charset",
                                                            NS_LITERAL_STRING("ISO-8859-1"), defaultCharset);
                m_defCharset = defaultCharset;
            }
            headerVal = m_defCharset;
        }
    }
    m_pMsgFields->SetCharacterSet(NS_LossyConvertUTF16toASCII(headerVal).get());
    charSet = headerVal;

    RecipientToString2(msg_item->cc_recipients,
                       msg_item->cc_recipients_count,
                       headerVal);
    if (!headerVal.IsEmpty())
        m_pMsgFields->SetCc(headerVal);

    RecipientToString2(msg_item->bcc_recipients,
                       msg_item->bcc_recipients_count,
                       headerVal);
    if (!headerVal.IsEmpty())
        m_pMsgFields->SetBcc(headerVal);

    if (msg_item->internet_message_id)
        m_pMsgFields->SetMessageId(msg_item->internet_message_id);
  
    RecipientToString2(msg_item->reply_to,
                       msg_item->reply_to_count,
                       headerVal);
    if (!headerVal.IsEmpty())
        m_pMsgFields->SetReplyTo(headerVal);

    if (msg_item->references)
        m_pMsgFields->SetReferences(msg_item->references);
  
    // what about all of the other headers?!?!?!?!?!?!
    char *pMimeType;
    if (!bodyType.IsEmpty())
        pMimeType = ToNewCString(NS_LossyConvertUTF16toASCII(bodyType));
    else
        pMimeType = strdup("text/plain");

    nsCOMPtr<nsIArray> pAttach;
    GetLocalAttachments(getter_AddRefs(pAttach));

    nsString uniBody;
    NS_CopyNativeToUnicode(nsDependentCString(msg_item->item.body), uniBody);

    /*
      l10n - I have the body of the message in the system charset,
      I need to "encode" it to be the charset for the message
      *UNLESS* of course, I don't know what the charset of the message
      should be?  How do I determine what the charset should
      be if it doesn't exist?

    */

    nsCString body;

    rv = mailews::MsgI18NConvertFromUnicode(NS_LossyConvertUTF16toASCII(charSet).get(),
                                     uniBody, body);
    if (NS_FAILED(rv) && !charSet.Equals(m_defCharset)) {
        // in this case, if we did not use the default compose
        // charset, then try that.
        body.Truncate();
        rv = mailews::MsgI18NConvertFromUnicode(NS_LossyConvertUTF16toASCII(charSet).get(),
                                         uniBody, body);
    }
    uniBody.Truncate();


    // See if it's a draft msg (ie, no From: or no To: AND no Cc: AND no Bcc:).
    // MailEws saves sent and draft msgs in Out folder (ie, mixed) and it does
    // store Bcc: header in the msg itself.
    nsAutoString from, to, cc, bcc;
    rv = m_pMsgFields->GetFrom(from);
    rv = m_pMsgFields->GetTo(to);
    rv = m_pMsgFields->GetCc(cc);
    rv = m_pMsgFields->GetBcc(bcc);
    bool createAsDraft = from.IsEmpty() || (to.IsEmpty() && cc.IsEmpty() && bcc.IsEmpty());

    nsCOMPtr<nsIImportService> impService(do_GetService(NS_IMPORTSERVICE_CONTRACTID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = impService->CreateRFC822Message(
        s_pIdentity,                  // dummy identity
        m_pMsgFields,                 // message fields
        pMimeType,                    // body type
        body,                         // body pointer
        createAsDraft,
        pAttach,                      // local attachments
        nullptr, //embededObjects
        pListener);                 // listener

    if (NS_FAILED(rv)) {
        IMPORT_LOG1("*** Error, CreateAndSendMessage FAILED: 0x%lx\n", (int)rv);
    }

    if (pMimeType)
        NS_Free(pMimeType);
    return rv;
}

nsresult UpdateMsgHdr(ews_msg_item * msg_item,
                      nsIMsgDBHdr * msgHdr) {
    std::string v;
    RecipientToStdString(&msg_item->sender, 1, v);
    if (v.empty()) {
        RecipientToStdString(&msg_item->from, 1, v);

        if (!v.empty()) {
            msgHdr->SetAuthor(v.c_str());
        }
    } else {
        msgHdr->SetAuthor(v.c_str());
    }

    RecipientToStdString(msg_item->to_recipients,
                         msg_item->to_recipients_count, v);

    if (!v.empty()) {
        msgHdr->SetRecipients(v.c_str());
    }

    RecipientToStdString(msg_item->cc_recipients,
                         msg_item->cc_recipients_count, v);

    if (!v.empty()) {
        msgHdr->SetCcList(v.c_str());
    }

    RecipientToStdString(msg_item->bcc_recipients,
                         msg_item->bcc_recipients_count, v);

    if (!v.empty()) {
        msgHdr->SetBccList(v.c_str());
    }

    if (msg_item->references)
        msgHdr->SetReferences(msg_item->references);

    if (msg_item->item.subject) {
        uint32_t flags;
        uint32_t L = strlen(msg_item->item.subject);
        char * key = msg_item->item.subject;

        if (L > 0) {
            (void)msgHdr->GetFlags(&flags);
            /* strip "Re: " */
            nsCString modifiedSubject;
            if (mailews::NS_MsgStripRE((const char **) &key, &L, modifiedSubject))
                flags |= nsMsgMessageFlags::HasRe;
            else
                flags &= ~nsMsgMessageFlags::HasRe;
            msgHdr->SetFlags(flags); // this *does not* update the mozilla-status header in the local folder
            if (modifiedSubject.IsEmpty()) {
                msgHdr->SetSubject(key);
            } else {
                msgHdr->SetSubject(modifiedSubject.get());
            }
        } else {
            msgHdr->SetSubject(msg_item->item.subject);
        }
    }
    
    if (msg_item->internet_message_id)
        msgHdr->SetMessageId(msg_item->internet_message_id);

    PRTime t;
    mailews::Seconds2PRTime(msg_item->item.received_time, &t);
    msgHdr->SetDate(t);

    msgHdr->MarkRead(!!msg_item->read);
    msgHdr->MarkHasAttachments(msg_item->item.has_attachments);

    msgHdr->SetStringProperty("item_id", msg_item->item.item_id);
    msgHdr->SetStringProperty("change_key", msg_item->item.change_key);
    return NS_OK;
}

static
void
CopyHdrPropertiesWithSkipList(nsIMsgDBHdr *destHdr,
                              nsIMsgDBHdr *srcHdr,
                              const nsCString &skipList)
{
    nsCOMPtr<nsIUTF8StringEnumerator> propertyEnumerator;
    nsresult rv = srcHdr->GetPropertyEnumerator(getter_AddRefs(propertyEnumerator));
    NS_ENSURE_SUCCESS_VOID(rv);

    // We'll add spaces at beginning and end so we can search for space-name-space
    nsCString dontPreserveEx(NS_LITERAL_CSTRING(" "));
    dontPreserveEx.Append(skipList);
    dontPreserveEx.AppendLiteral(" ");

    nsAutoCString property;
    nsCString sourceString;
    bool hasMore;
    while (NS_SUCCEEDED(propertyEnumerator->HasMore(&hasMore)) && hasMore)
    {
        propertyEnumerator->GetNext(property);
        nsAutoCString propertyEx(NS_LITERAL_CSTRING(" "));
        propertyEx.Append(property);
        propertyEx.AppendLiteral(" ");
        if (dontPreserveEx.Find(propertyEx) != -1) // -1 is not found
            continue;

        srcHdr->GetStringProperty(property.get(), getter_Copies(sourceString));
        destHdr->SetStringProperty(property.get(), sourceString.get());
    }

    nsMsgLabelValue label = 0;
    srcHdr->GetLabel(&label);
    destHdr->SetLabel(label);
}

static
void
CopyPropertiesToMsgHdr(nsIMsgDBHdr *destHdr,
                       nsIMsgDBHdr *srcHdr,
                       bool aIsMove)
{
    nsresult rv;
    nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
    NS_ENSURE_SUCCESS_VOID(rv);

    nsCString dontPreserve;

    // These preferences exist so that extensions can control which properties
    // are preserved in the database when a message is moved or copied. All
    // properties are preserved except those listed in these preferences
    if (aIsMove)
        prefBranch->GetCharPref("mailnews.database.summary.dontPreserveOnMove",
                                getter_Copies(dontPreserve));
    else
        prefBranch->GetCharPref("mailnews.database.summary.dontPreserveOnCopy",
                                getter_Copies(dontPreserve));

    CopyHdrPropertiesWithSkipList(destHdr, srcHdr, dontPreserve);
}

static nsresult __AddNewMessageToFolder(nsIMsgFolder * dstFolder,
                                        ews_msg_item * msg_item,
                                        nsIMsgDBHdr* messageToReplace,
                                        nsIFile * pSrc,
                                        nsIMsgDBHdr ** _msgHdr) {
    nsCOMPtr<nsIOutputStream> outputStream;
    nsCOMPtr<nsIMsgPluggableStore> msgStore;
    nsresult rv;
    
    nsCOMPtr<nsIMsgDatabase> destDB;
    rv = dstFolder->GetMsgDatabase(getter_AddRefs(destDB));
    
    rv = dstFolder->GetMsgStore(getter_AddRefs(msgStore));
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<nsIMsgDBHdr> msgHdr;
    bool reusable;

    rv = msgStore->GetNewMsgOutputStream(dstFolder,
                                         getter_AddRefs(msgHdr),
                                         &reusable,
                                         getter_AddRefs(outputStream));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr <nsIInputStream> pInputStream;
    rv = NS_NewLocalFileInputStream(getter_AddRefs(pInputStream), pSrc);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<nsLineBuffer<char> > lineBuffer(new nsLineBuffer<char>);
    NS_ENSURE_TRUE(lineBuffer, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr <nsIMsgParseMailMsgState> msgParser = do_CreateInstance(NS_PARSEMAILMSGSTATE_CONTRACTID, &rv);
    msgParser->SetMailDB(destDB);
    msgParser->SetState(nsIMsgParseMailMsgState::ParseHeadersState);
    msgParser->SetNewMsgHdr(msgHdr);
    
    nsCString line;
    bool more = false;
    uint32_t write_count = 0;
    uint32_t msg_size = 0;

    //Need to write Local message header
    //From
    nsAutoCString result;
    time_t now = time ((time_t*) 0);
    char *ct = ctime(&now);
    ct[24] = 0;
    result = "From - ";
    result += ct;
    result += MSG_LINEBREAK;

    outputStream->Write(result.get(),
                        result.Length(),
                        &write_count);
    msg_size += write_count;
    
    //Status
    NS_NAMED_LITERAL_CSTRING(MozillaStatus,
                             "X-Mozilla-Status: 0001" MSG_LINEBREAK);
    outputStream->Write(MozillaStatus.get(),
                        MozillaStatus.Length(),
                        &write_count);
    msg_size += write_count;

    //Status2
    NS_NAMED_LITERAL_CSTRING(MozillaStatus2,
                             "X-Mozilla-Status2: 00000000" MSG_LINEBREAK);
    outputStream->Write(MozillaStatus2.get(),
                        MozillaStatus2.Length(),
                        &write_count);
    msg_size += write_count;

    do {
        rv = NS_ReadLine(pInputStream.get(),
                         lineBuffer.get(),
                         line,
                         &more);
        if (NS_FAILED(rv))
            break;

        nsCString newLine(line);
        newLine.AppendLiteral("\x0D\x0A");
        msgParser->ParseAFolderLine(newLine.get(), newLine.Length());

        //Need to replace Date: Header with received time
        if (!strncmp(line.get(), "Date:", strlen("Date:")) &&
            msg_item &&
            msg_item->item.received_time > 0) {
            char buf[256] = {0};
            size_t len = strftime(buf, 255, "%a, %d %b %Y %H:%M:%S %z", localtime(&msg_item->item.received_time));

            outputStream->Write("Date: ", strlen("Date: "), &write_count);
            msg_size += write_count;
            outputStream->Write(buf, len, &write_count);
            msg_size += write_count;
            outputStream->Write("\x0D\x0A", 2, &write_count);
            msg_size += write_count;
        } else if (!strncmp(line.get(), "Subject:", strlen("Subject:"))) {
            const char * key = line.get() + strlen("Subject:");
            uint32_t l = strlen(key);
            
            nsCString newSubject("Subject: ");

            /* strip "Re: " */
            nsCString modifiedSubject;
            if (mailews::NS_MsgStripRE((const char **) &key,
                              &l,
                              modifiedSubject)) {
                nsString utf16LocalizedRe;
                mailews::NS_GetLocalizedUnicharPreferenceWithDefault(nullptr,
                                                            "mailnews.localizedRe",
                                                            EmptyString(),
                                                            utf16LocalizedRe);
                NS_ConvertUTF16toUTF8 localizedRe(utf16LocalizedRe);

                if (localizedRe.IsEmpty()) {
                    newSubject.Append("Re");
                } else {
                    newSubject.Append(localizedRe);
                }
                newSubject.Append(": ");
            }

            if (!modifiedSubject.IsEmpty()) {
                newSubject.Append(modifiedSubject);
            } else {
                newSubject.AppendLiteral(key);
            }
            outputStream->Write(newSubject.get(), newSubject.Length(), &write_count);
            msg_size += write_count;
            outputStream->Write("\x0D\x0A", 2, &write_count);
            msg_size += write_count;
        } else {
            outputStream->Write(line.get(), line.Length(), &write_count);
            msg_size += write_count;
            outputStream->Write("\x0D\x0A", 2, &write_count);
            msg_size += write_count;
        }
    } while(more);

    pInputStream->Close();

    msgParser->FinishHeader();

    msgParser->GetNewMsgHdr(getter_AddRefs(msgHdr));
    
    if (NS_SUCCEEDED(rv)) {
        if (msg_item)
            UpdateMsgHdr(msg_item, msgHdr);
        else if (messageToReplace) {
            CopyPropertiesToMsgHdr(msgHdr, messageToReplace, true);
        }

        msgHdr->SetMessageSize(msg_size);
        
        destDB->AddNewHdrToDB(msgHdr, true /* notify */);
        msgStore->FinishNewMessage(outputStream, msgHdr);
        dstFolder->UpdateSummaryTotals(true);
    }
    else {
        msgStore->DiscardNewMessage(outputStream, msgHdr);
        IMPORT_LOG0( "*** Error importing message\n");
    }

    if (!reusable)
        outputStream->Close();

    destDB->Commit(nsMsgDBCommitType::kLargeCommit);

    if (_msgHdr)
        msgHdr.swap(*_msgHdr);
    return rv;
}

nsresult AddNewMessageToFolder(nsIMsgFolder * dstFolder,
                               ews_msg_item * msg_item,
                               nsIFile * pSrc) {
    return __AddNewMessageToFolder(dstFolder,
                                   msg_item,
                                   nullptr,
                                   pSrc,
                                   nullptr);
}

nsresult CopyOrReplaceMessageToFolder(nsIMsgFolder * dstFolder,
                                      nsIMsgDBHdr* messageToReplace,
                                      nsIFile * pSrc,
                                      nsIMsgDBHdr ** _msgHdr) {
    return __AddNewMessageToFolder(dstFolder,
                                   nullptr,
                                   messageToReplace,
                                   pSrc,
                                   _msgHdr);
}


nsresult
MailEwsMsgCreateTempFile(const char *tFileName, nsIFile **tFile)
{
    if ((!tFileName) || (!*tFileName))
        tFileName = "nsmail.tmp";

    nsresult rv = mailews::GetSpecialDirectoryWithFileName(NS_OS_TEMP_DIR,
                                                  tFileName,
                                                  tFile);

    NS_ENSURE_SUCCESS(rv, rv);

    rv = (*tFile)->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 00600);
    if (NS_FAILED(rv))
        NS_RELEASE(*tFile);

    return rv;
}

// //TEST code
// {
//     nsCOMPtr<nsIMsgDatabase> destDB;
//     nsresult rv = this->GetMsgDatabase(getter_AddRefs(destDB));
        
//     nsCOMPtr <nsIOutputStream> offlineStore;
//     nsCOMPtr<nsIMsgPluggableStore> msgStore;
//     nsCOMPtr<nsIMsgIncomingServer> dstServer;
//     nsCOMPtr<nsIMsgDBHdr> newMsgHdr;
//     bool aReusable;

//     this->GetServer(getter_AddRefs(dstServer));
//     rv = dstServer->GetMsgStore(getter_AddRefs(msgStore));
//     NS_ENSURE_SUCCESS(rv, rv);

//     rv = msgStore->GetNewMsgOutputStream(this,
//                                          getter_AddRefs(newMsgHdr),
//                                          &aReusable,
//                                          getter_AddRefs(offlineStore));
        
//   if (NS_SUCCEEDED(rv) && offlineStore)
//   {
//     int64_t curOfflineStorePos = 0;
//     nsCOMPtr <nsISeekableStream> seekable = do_QueryInterface(offlineStore);
//     if (seekable)
//       seekable->Tell(&curOfflineStorePos);
//     else
//     {
//       NS_ERROR("needs to be a random store!");
//       return NS_ERROR_FAILURE;
//     }

//     //Write a test line to store
//     uint32_t bytesWritten = 0;
//     std::string home(getenv("HOME"));
//     home.append("/1.eml");
//     FILE * fp = fopen(home.c_str(), "rb");
//     char buf[4096] = {0};
//     size_t read_count = 0;
//     while ((read_count = fread(buf, 1, 4096, fp)) > 0) {
//         printf("bytes read:%d\n", read_count);
//         while(read_count > 0) {
//             uint32_t write_count = 0;
//             char * pbuf = buf;
//             offlineStore->Write(buf, read_count, &write_count);
//             bytesWritten += write_count;
//             read_count -= write_count;
//             pbuf += write_count;
//         }
//     }
//     fclose(fp);

//     // const char * buf = "this is a test body!\r\r\r";
//     // offlineStore->Write(buf, strlen(buf), &bytesWritten);
//     printf("bytes written:%d\n", bytesWritten);
        
//     newMsgHdr->SetMessageOffset(curOfflineStorePos);
//     newMsgHdr->SetMessageSize(bytesWritten);
//     newMsgHdr->SetOfflineMessageSize(bytesWritten);
        
//     uint32_t origFlags;
//     uint32_t flags = 1;
//     (void)newMsgHdr->GetFlags(&origFlags);
//     if (origFlags & nsMsgMessageFlags::HasRe)
//         flags |= nsMsgMessageFlags::HasRe;
//     else
//         flags &= ~nsMsgMessageFlags::HasRe;

//     flags &= ~nsMsgMessageFlags::Offline; // don't keep nsMsgMessageFlags::Offline for local msgs
//     newMsgHdr->SetFlags(flags);
 
//     //set Headers
//     newMsgHdr->SetAuthor("jingnan.si@gmail.com");
//     newMsgHdr->SetRecipients("stone_si@symantec.com");
//     newMsgHdr->SetSubject("hello222, test subject");
//     newMsgHdr->SetLineCount(1);
        
//     destDB->UpdatePendingAttributes(newMsgHdr);

//     destDB->AddNewHdrToDB(newMsgHdr, true /* notify */);
//     this->SetFlag(nsMsgFolderFlags::OfflineEvents);
//     if (msgStore)
//         msgStore->FinishNewMessage(offlineStore, newMsgHdr);
// 	UpdateSummaryTotals(true);
//     destDB->Commit(nsMsgDBCommitType::kLargeCommit);
//   }
// }

static int do_send_mail(ews_session * session,
                        const char * content,
                        int len_content,
                        char ** pp_err_msg) {
    char * encoded_content = PL_Base64Encode(content,
                                             len_content,
                                             nullptr);

    ews_msg_item msg_item;
    memset(&msg_item, 0, sizeof(ews_msg_item));
    msg_item.item.item_type = EWS_Item_Message;
        
    msg_item.item.mime_content = encoded_content;
		
    int ret = ews_send_item(session,
                        &msg_item,
                        pp_err_msg);

    PR_Free(encoded_content);

    return ret;
}    

int
msg_server_send_mail(nsIMsgIncomingServer * pIncomingServer,
                     const nsCString & mimeContent,
                     nsCString & err_msg) {
    nsresult rv = NS_OK;
    
	ews_session * session = NULL;
	nsCOMPtr<IMailEwsMsgIncomingServer> ewsServer(do_QueryInterface(pIncomingServer, &rv));
	NS_ENSURE_SUCCESS(rv, EWS_FAIL);

	nsCOMPtr<IMailEwsService> ewsService;
	rv = ewsServer->GetService(getter_AddRefs(ewsService));
	NS_ENSURE_SUCCESS(rv, EWS_FAIL);

	nsresult rv1 = ewsService->GetNewSession(&session);
    int ret = EWS_FAIL;

	if (NS_SUCCEEDED(rv)
	    && NS_SUCCEEDED(rv1)
	    && session) {
        ret = do_send_mail(session,
                           mimeContent.get(),
                           mimeContent.Length(),
                           getter_Copies(err_msg));
	} else {
        err_msg.AssignLiteral("");
		ret = (session ? EWS_FAIL : 401);
	}

    if (session)
        ewsService->ReleaseSession(session);
    
    return ret;
}

#ifndef MailEwsCompose_h__
#define MailEwsCompose_h__

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include "MsgUtils.h"
#include "nsIFile.h"
#include "nsIInputStream.h"
#include "nsVoidArray.h"
#include "nsIImportService.h"
#include "nsIMsgIncomingServer.h"

#include "libews.h"

#ifdef MOZILLA_INTERNAL_API
#include "nsNativeCharsetUtils.h"
#else
#include "MsgI18N.h"
#define NS_CopyNativeToUnicode(source, dest)                            \
	mailews::MsgI18NConvertToUnicode(mailews::MsgI18NFileSystemCharset(), source, dest)
#endif

class nsIMsgSend;
class nsIMsgCompFields;
class nsIMsgIdentity;
class nsIMsgSendListener;
class nsIIOService;

#include "nsIMsgSend.h"

nsresult AddNewMessageToFolder(nsIMsgFolder * dstFolder,
                               ews_msg_item * msg_item,
                               nsIFile * pSrc);
nsresult
MailEwsMsgCreateTempFile(const char *tFileName, nsIFile **tFile);
nsresult CopyOrReplaceMessageToFolder(nsIMsgFolder * dstFolder,
                                      nsIMsgDBHdr* messageToReplace,
                                      nsIFile * pSrc,
                                      nsIMsgDBHdr ** _msgHdr);

typedef class {
public:
	nsCOMPtr <nsIFile>  pAttachment;
	char *      mimeType;
	char *      description;
} ImportAttachment;

// First off, a listener
class MailEwsSendListener : public nsIMsgSendListener
{
public:
    MailEwsSendListener() {
        m_done = false;
    }

    // nsISupports interface
    NS_DECL_THREADSAFE_ISUPPORTS

    /* void OnStartSending (in string aMsgID, in uint32_t aMsgSize); */
    NS_IMETHOD OnStartSending(const char *aMsgID, uint32_t aMsgSize) override {return NS_OK;}

    /* void OnProgress (in string aMsgID, in uint32_t aProgress, in uint32_t aProgressMax); */
    NS_IMETHOD OnProgress(const char *aMsgID, uint32_t aProgress, uint32_t aProgressMax) override {return NS_OK;}

    /* void OnStatus (in string aMsgID, in wstring aMsg); */
    NS_IMETHOD OnStatus(const char *aMsgID, const char16_t *aMsg) override {return NS_OK;}

    /* void OnStopSending (in string aMsgID, in nsresult aStatus, in wstring aMsg, in nsIFile returnFile); */
    NS_IMETHOD OnStopSending(const char *aMsgID,
                             nsresult aStatus,
                             const char16_t *aMsg,
                             nsIFile *returnFile) override;

    /* void OnSendNotPerformed */
    NS_IMETHOD OnSendNotPerformed(const char *aMsgID, nsresult aStatus) override {return NS_OK;}

    /* void OnGetDraftFolderURI (); */
    NS_IMETHOD OnGetDraftFolderURI(const char *aFolderURI)  override {return NS_OK;}

    void Reset() { m_done = false;  m_location = nullptr;}

public:
    bool m_done;
    nsCOMPtr <nsIFile> m_location;

protected:
    virtual ~MailEwsSendListener() {}
};

class MailEwsMsgCompose {
public:
	MailEwsMsgCompose();
	~MailEwsMsgCompose();

	nsresult  SendTheMessage(ews_msg_item * msg_item,
                             nsIMsgSendListener *pListener);

	void    SetAttachments(nsVoidArray *pAttachments) { m_pAttachments = pAttachments;}

	static nsresult CreateIdentity(void);
	static void    ReleaseIdentity(void);

private:
	nsresult  CreateComponents(void);

	nsresult GetLocalAttachments(nsIArray **aArray);

private:
	static nsIMsgIdentity *    s_pIdentity;

	nsVoidArray *      m_pAttachments;
	nsIMsgCompFields *    m_pMsgFields;
	nsCOMPtr<nsIIOService> m_pIOService;
	nsString        m_defCharset;
	nsCOMPtr<nsIImportService>  m_pImportService;
};

int
msg_server_send_mail(nsIMsgIncomingServer * pIncomingServer,
                     const nsCString & mimeContent,
                     nsCString &_err_msg);

#endif /* MailEwsMsgUtils_h__ */

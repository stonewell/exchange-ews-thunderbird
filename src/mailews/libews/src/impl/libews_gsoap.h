/*
 * libews_gsoap.h
 *
 *  Created on: Apr 2, 2014
 *      Author: stone
 */

#ifndef LIBEWS_GSOAP_H_
#define LIBEWS_GSOAP_H_

#include "ewsExchangeServiceBindingProxy.h"
#include "libews.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include "../utils/curl_help.h"
#include "libews_gsoap_utils.h"
#include "libews_impl_macros.h"
#include <memory>

namespace ews {

class DLL_LOCAL CEWSSessionGsoapImpl: public CEWSSession {
public:
	CEWSSessionGsoapImpl(EWSConnectionInfo * connInfo,
	                     EWSAccountInfo * accountInfo,
	                     EWSProxyInfo * proxyInfo);
	virtual ~CEWSSessionGsoapImpl();

public:
	ExchangeServiceBindingProxy * GetProxy() {
		return &m_Proxy;
	}

	virtual CEWSFolderOperation * CreateFolderOperation();
	virtual CEWSItemOperation * CreateItemOperation();
	virtual CEWSAttachmentOperation * CreateAttachmentOperation();
	virtual CEWSSubscriptionOperation * CreateSubscriptionOperation();
	virtual CEWSContactOperation * CreateContactOperation();
	virtual CEWSCalendarOperation * CreateCalendarOperation();
	virtual CEWSTaskOperation * CreateTaskOperation();

	bool Initialize(CEWSError * pError = NULL);
	void InitForEachRequest();
	void CleanupForEachRequest();
	virtual CEWSString GetErrorMsg();
	virtual void AutoDiscover(CEWSError * pError);

	virtual EWSConnectionInfo * GetConnectionInfo() {
		return &m_ConnInfo;
	}
    
	virtual EWSAccountInfo * GetAccountInfo() {
		return &m_AccountInfo;
	}
    
	virtual EWSProxyInfo * GetProxyInfo() {
		return &m_ProxyInfo;
	}
    
private:
	EWSConnectionInfo m_ConnInfo;
	EWSAccountInfo m_AccountInfo;
	EWSProxyInfo m_ProxyInfo;

	ExchangeServiceBindingProxy m_Proxy;
	CUrlHelper * m_pCUrlHelper;
    std::string * m_TimeZoneId;
	static bool m_envInitialized;
};

class DLL_LOCAL CEWSSessionRequestGuard {
public:
	CEWSSessionRequestGuard(CEWSSessionGsoapImpl * pSessionImpl)
			: m_pSessionImpl(pSessionImpl) {
		m_pSessionImpl->InitForEachRequest();
	}

	~CEWSSessionRequestGuard() {
		m_pSessionImpl->CleanupForEachRequest();
	}

	CEWSSessionGsoapImpl * m_pSessionImpl;
};

class DLL_LOCAL CEWSFolderGsoapImpl: public CEWSFolder {
	friend class CEWSFolderOperationGsoapImpl;
public:
	static CEWSFolderGsoapImpl * CreateInstance(EWSFolderTypeEnum type =
	                                            CEWSFolder::Folder_Mail, ews2__FolderType * pFolder = NULL);

	CEWSFolderGsoapImpl() :
			m_pParentFolder(NULL), m_DisplayName(""), m_FolderId(""), m_ParentFolderId(
			    ""), m_FolderType(CEWSFolder::Folder_Mail), m_ChangeKey(""), m_TotalCount(
			        0), m_UnreadCount(0), m_Visible(true) {

	}

	CEWSFolderGsoapImpl(EWSFolderTypeEnum type, const CEWSString & displayName) :
			m_pParentFolder(NULL), m_DisplayName(displayName), m_FolderId(""), m_ParentFolderId(
			    ""), m_FolderType(type), m_ChangeKey(""), m_TotalCount(0), m_UnreadCount(
			        0), m_Visible(true) {

	}

	virtual ~CEWSFolderGsoapImpl() {

	}

	virtual CEWSFolder * GetParentFolder() const {
		return m_pParentFolder;
	}

	virtual const CEWSString & GetDisplayName() const {
		return m_DisplayName;
	}
	virtual const CEWSString & GetFolderId() const {
		return m_FolderId;
	}
	virtual EWSFolderTypeEnum GetFolderType() const {
		return m_FolderType;
	}

	virtual const CEWSString & GetChangeKey() const {
		return m_ChangeKey;
	}

	virtual void SetParentFolder(CEWSFolder * parentFolder) {
		m_pParentFolder = parentFolder;
	}

	virtual void SetDisplayName(const CEWSString & name) {
		m_DisplayName = name;
	}

	virtual void SetFolderType(EWSFolderTypeEnum type) {
		m_FolderType = type;
	}

	virtual void SetFolderId(const CEWSString & id) {
		m_FolderId = id;
	}

	virtual void SetParentFolderId(const CEWSString & parentFolderId) {
		m_ParentFolderId = parentFolderId;
	}

	virtual const CEWSString & GetParentFolderId() const {
		if (m_pParentFolder)
			return m_pParentFolder->GetFolderId();

		return m_ParentFolderId;
	}

	virtual void SetChangeKey(const CEWSString & changeKey) {
		m_ChangeKey = changeKey;
	}

	virtual int GetTotalCount() const {
		return m_TotalCount;
	}

	virtual int GetUnreadCount() const {
		return m_UnreadCount;
	}

	virtual void SetTotalCount(int totalCount) {
		m_TotalCount = totalCount;
	}

	virtual void SetUnreadCount(int unreadCount) {
		m_UnreadCount = unreadCount;
	}

	virtual bool IsVisible() const {
		return m_Visible;
	}
	virtual void SetVisible(bool v) {
		m_Visible = v;
	}
private:
	CEWSFolder * m_pParentFolder;
	CEWSString m_DisplayName;
	CEWSString m_FolderId;
	CEWSString m_ParentFolderId;
	EWSFolderTypeEnum m_FolderType;
	CEWSString m_ChangeKey;
	int m_TotalCount;
	int m_UnreadCount;
	bool m_Visible;
};

class DLL_LOCAL CEWSFolderOperationGsoapCallback : public CEWSFolderOperationCallback{
public:
	CEWSFolderOperationGsoapCallback();
	virtual ~CEWSFolderOperationGsoapCallback();

public:
	virtual void NewFolder(std::auto_ptr<CEWSFolderGsoapImpl> * pFolder) = 0;
	virtual void UpdateFolder(std::auto_ptr<CEWSFolderGsoapImpl> * pFolder) = 0;
	virtual void DeleteFolder(std::auto_ptr<CEWSFolderGsoapImpl> * pFolder) = 0;
};

class DLL_LOCAL CEWSFolderOperationGsoapImpl: public CEWSFolderOperation {
public:
	CEWSFolderOperationGsoapImpl(CEWSSessionGsoapImpl * pSession);
	virtual ~CEWSFolderOperationGsoapImpl();

public:
	CEWSFolderList * List(CEWSFolder::EWSFolderTypeEnum type,
	                      const CEWSFolder * pParentFolder = NULL,
	                      CEWSError * pError = NULL);
	CEWSFolderList * List(CEWSFolder::EWSFolderTypeEnum type,
	                      CEWSFolder::EWSDistinguishedFolderIdNameEnum distinguishedFolderId,
	                      CEWSError * pError = NULL);
	bool SyncFolders(CEWSFolderOperationCallback * pFolderCallback,
	                 CEWSError * pError = NULL);
	bool Delete(CEWSFolder * pFolder, bool hardDelete, CEWSError * pError = NULL);
	bool Update(CEWSFolder * pFolder, CEWSError * pError = NULL);
	bool Create(CEWSFolder * pFolder, const CEWSFolder * pParentFolder = NULL,
	            CEWSError * pError = NULL);
	CEWSFolder * GetFolder(CEWSFolder::EWSDistinguishedFolderIdNameEnum distinguishedFolderId,
	                       CEWSError * pError = NULL);
	CEWSFolder * GetFolder(const CEWSString & folderId,
	                       CEWSError * pError = NULL);
	CEWSFolder * GetFolder(const CEWSFolder * pFolder,
	                       CEWSError * pError = NULL);
	virtual CEWSFolderList * GetFolders(int * distinguishedFolderId,
	                                    int count,
	                                    CEWSError * pError);
	virtual CEWSFolderList * GetFolders(const CEWSStringList & folderId,
	                                    CEWSError * pError);
private:
	bool __SyncFolders(CEWSFolderOperationGsoapCallback * pFolderCallback,
	                   ews2__TargetFolderIdType * targetFolder,
	                   CEWSError * pError = NULL);
	CEWSFolderList * __GetFolder(__ews2__union_NonEmptyArrayOfBaseFolderIdsType * union_NonEmptyArrayOfBaseFolderIdsType,
	                             int count,
	                             bool idOnly = false,
	                             CEWSError * pError = NULL);
	bool __FindFolder(
	    CEWSFolderOperationGsoapCallback * pFolderCallback,
	    __ews2__union_NonEmptyArrayOfBaseFolderIdsType * parentFolder,
	    CEWSError * pError);
private:
	CEWSSessionGsoapImpl * m_pSession;
};

class DLL_LOCAL ItemTypeBuilder {
public:
	ItemTypeBuilder(soap * soap,
	                const CEWSItem * pEWSItem,
	                CEWSObjectPool * pObjPool);
	virtual ~ItemTypeBuilder();
	
	virtual ews2__ItemType * Build();
	virtual ews2__ItemType * CreateItemType();
protected:
	soap * m_pSoap;
	const CEWSItem * m_pEWSItem;
	CEWSObjectPool * m_pObjPool;
};

class DLL_LOCAL CEWSInternetMessageHeaderGsoapImpl : public CEWSInternetMessageHeader {
public:
	CEWSInternetMessageHeaderGsoapImpl()
			: INIT_PROPERTY(Value, "")
			, INIT_PROPERTY(HeaderName, "") {
	}
    
	virtual ~CEWSInternetMessageHeaderGsoapImpl() {
	}

public:
	IMPL_PROPERTY_2(CEWSString, Value);
	IMPL_PROPERTY_2(CEWSString, HeaderName);
};

class DLL_LOCAL CEWSItemGsoapImpl: public virtual CEWSItem {
public:
	static CEWSItemGsoapImpl * CreateInstance(EWSItemType itemType,
	                                          ews2__ItemType * pItem);

	CEWSItemGsoapImpl() 
			:INIT_PROPERTY(ItemId, "")
			,INIT_PROPERTY(BodyType, CEWSItem::BODY_HTML)
			,INIT_PROPERTY(Body, "")
			,INIT_PROPERTY(MimeContent, "")
			,INIT_PROPERTY(CreateTime, 0)
			,INIT_PROPERTY(ReceivedTime, 0)
			,INIT_PROPERTY(SentTime, 0)
			,INIT_PROPERTY(DisplayCc, "")
			,INIT_PROPERTY(DisplayTo, "")
			,INIT_PROPERTY(InReplyTo, "")
			,INIT_PROPERTY(Subject, "")
			,INIT_PROPERTY_NOT_OWN_PT(Attachments, new CEWSAttachmentList())
			,INIT_PROPERTY(ItemType, CEWSItem::Item_Message)
			,INIT_PROPERTY_B_2(Attachments, false)
			,INIT_PROPERTY(ChangeKey, "")
			,INIT_PROPERTY(ParentFolderId, "")
			,INIT_PROPERTY(ParentFolderChangeKey, "")
			,INIT_PROPERTY(ItemClass, "")
			,INIT_PROPERTY(Sensitivity, CEWSItem::Normal)
			,INIT_PROPERTY(Size, 0)
			,INIT_PROPERTY(Categories, NULL)
			,INIT_PROPERTY(Importance, CEWSItem::Norm)
			,INIT_PROPERTY(Submitted, false)
			,INIT_PROPERTY(Draft, false)
			,INIT_PROPERTY(FromMe, false)
			,INIT_PROPERTY(Resend, false)
			,INIT_PROPERTY(Unmodified, false)
			,INIT_PROPERTY(InternetMessageHeaders, NULL)
			,INIT_PROPERTY(ReminderDueBy, 0)
			,INIT_PROPERTY(ReminderIsSet, false)
			,INIT_PROPERTY(ReminderMinutesBeforeStart, "")
			,INIT_PROPERTY(LastModifiedName, "")
			,INIT_PROPERTY(LastModifiedTime, 0) {

	}

	virtual ~CEWSItemGsoapImpl() {
		RELEASE_PROPERTY_NOT_OWN_PT(Attachments);
		if (m_Categories) delete m_Categories;
		if (m_InternetMessageHeaders) delete m_InternetMessageHeaders;
	}
public:
	IMPL_PROPERTY_2(CEWSString, ItemId);
	IMPL_PROPERTY(EWSBodyType, BodyType);
	IMPL_PROPERTY_2(CEWSString, Body);
	IMPL_PROPERTY_2(CEWSString, MimeContent);
	IMPL_PROPERTY(time_t, CreateTime);
	IMPL_PROPERTY(time_t, ReceivedTime);
	IMPL_PROPERTY(time_t, SentTime);
	IMPL_PROPERTY_2(CEWSString, DisplayCc);
	IMPL_PROPERTY_2(CEWSString, DisplayTo);
	IMPL_PROPERTY_2(CEWSString, InReplyTo);
	IMPL_PROPERTY_2(CEWSString, Subject);
	IMPL_PROPERTY_NOT_OWN_PT(CEWSAttachmentList *, Attachments);
	IMPL_PROPERTY(EWSItemType, ItemType);
	IMPL_PROPERTY_B_2(bool, Attachments);
	IMPL_PROPERTY_2(CEWSString,ChangeKey);
	IMPL_PROPERTY_2(CEWSString, ParentFolderId);
	IMPL_PROPERTY_2(CEWSString, ParentFolderChangeKey);
	IMPL_PROPERTY_2(CEWSString, ItemClass);
	IMPL_PROPERTY(enum EWSSensitivityTypeEnum, Sensitivity);
	IMPL_PROPERTY(int, Size);
	IMPL_PROPERTY(CEWSStringList *, Categories);
	IMPL_PROPERTY(enum EWSImportanceTypeEnum, Importance);
	IMPL_PROPERTY_B(bool, Submitted);
	IMPL_PROPERTY_B(bool, Draft);
	IMPL_PROPERTY_B(bool, FromMe);
	IMPL_PROPERTY_B(bool, Resend);
	IMPL_PROPERTY_B(bool, Unmodified);
	IMPL_PROPERTY(CEWSInternetMessageHeaderList *, InternetMessageHeaders);
	IMPL_PROPERTY(time_t, ReminderDueBy);
	IMPL_PROPERTY(bool, ReminderIsSet);
	IMPL_PROPERTY_2(CEWSString, ReminderMinutesBeforeStart);
	IMPL_PROPERTY_2(CEWSString, LastModifiedName);
	IMPL_PROPERTY(time_t, LastModifiedTime);

	virtual void AddAttachment(CEWSAttachment * pAttachment);
	virtual void RemoveAttachment(CEWSAttachment * pAttachment);
	virtual void ClearAttachments();

	virtual void FromItemType(ews2__ItemType * pItem);
	virtual void UpdateAttachments(
	    ews2__NonEmptyArrayOfAttachmentsType * pAttachments);
};

class DLL_LOCAL MessageItemTypeBuilder : public ItemTypeBuilder {
public:
	MessageItemTypeBuilder(soap * soap,
	                       const CEWSMessageItem * pEWSItem,
	                       CEWSObjectPool * pObjPool);
	virtual ~MessageItemTypeBuilder();

	virtual ews2__ItemType * Build();
	virtual ews2__ItemType * CreateItemType();
};

class DLL_LOCAL CEWSMessageItemGsoapImpl: public virtual CEWSItemGsoapImpl,
                                          public virtual CEWSMessageItem {
public:
	CEWSMessageItemGsoapImpl() :
			INIT_PROPERTY(Sender, NULL)
			, INIT_PROPERTY_NOT_OWN_PT(ToRecipients, new CEWSRecipientList())
			, INIT_PROPERTY_NOT_OWN_PT(CcRecipients,new CEWSRecipientList())
			, INIT_PROPERTY_NOT_OWN_PT(BccRecipients,new CEWSRecipientList())
			, INIT_PROPERTY(ReadReceiptRequested, false)
			, INIT_PROPERTY(DeliveryReceiptRequested,false)
			, INIT_PROPERTY(ConversationIndex, "")
			, INIT_PROPERTY(ConversationTopic, "")
			, INIT_PROPERTY(From, NULL)
			, INIT_PROPERTY(InternetMessageId, "")
			, INIT_PROPERTY(Read, false)
			, INIT_PROPERTY(ResponseRequested, false)
			, INIT_PROPERTY(References, "")
			, INIT_PROPERTY_NOT_OWN_PT(ReplyTo, new CEWSRecipientList())
			, INIT_PROPERTY(ReceivedBy, NULL)
			, INIT_PROPERTY(ReceivedRepresenting, NULL)
	{
		SetItemType(CEWSItem::Item_Message);
	}

	virtual ~CEWSMessageItemGsoapImpl() {
		RELEASE_PROPERTY_NOT_OWN_PT(ToRecipients);
		RELEASE_PROPERTY_NOT_OWN_PT(CcRecipients);
		RELEASE_PROPERTY_NOT_OWN_PT(BccRecipients);
		RELEASE_PROPERTY_NOT_OWN_PT(ReplyTo);

		if (m_Sender)
			delete m_Sender;
		if (m_From)
			delete m_From;
		if (m_ReceivedBy)
			delete m_ReceivedBy;
		if (m_ReceivedRepresenting)
			delete m_ReceivedRepresenting;
	}

protected:
	virtual void FromMessageType(ews2__MessageType * pItem);
	virtual void FromArrayOfRecipientsType(CEWSRecipientList * pList,
	                                       ews2__ArrayOfRecipientsType * pItem);

public:
	virtual void FromItemType(ews2__ItemType * pItem);

	IMPL_PROPERTY(CEWSRecipient *,Sender)
			;IMPL_PROPERTY_NOT_OWN_PT(CEWSRecipientList *,ToRecipients)
					 ;IMPL_PROPERTY_NOT_OWN_PT(CEWSRecipientList *,CcRecipients)
							  ;IMPL_PROPERTY_NOT_OWN_PT(CEWSRecipientList *,BccRecipients)
									   ;IMPL_PROPERTY_B(bool ,ReadReceiptRequested)
											    ;IMPL_PROPERTY_B(bool ,DeliveryReceiptRequested)
													     ;IMPL_PROPERTY_2(CEWSString,ConversationIndex)
															      ;IMPL_PROPERTY_2(CEWSString,ConversationTopic)
																	       ;IMPL_PROPERTY(CEWSRecipient *,From)
																			        ;IMPL_PROPERTY_2(CEWSString,InternetMessageId)
																					         ;IMPL_PROPERTY_B(bool ,Read)
																							          ;IMPL_PROPERTY_B(bool ,ResponseRequested)
																									           ;IMPL_PROPERTY_2(CEWSString,References)
																											            ;IMPL_PROPERTY_NOT_OWN_PT(CEWSRecipientList *,ReplyTo)
																													             ;IMPL_PROPERTY(CEWSRecipient *,ReceivedBy)
																															              ;IMPL_PROPERTY(CEWSRecipient *,ReceivedRepresenting)
																																	               ;

private:
};

class DLL_LOCAL CEWSPhysicalAddressGsoapImpl : public virtual CEWSPhysicalAddress{
public:
	CEWSPhysicalAddressGsoapImpl()
			: INIT_PROPERTY(Street, "")
			, INIT_PROPERTY(City, "")
			, INIT_PROPERTY(State, "")
			, INIT_PROPERTY(CountryOrRegion, "")
			, INIT_PROPERTY(PostalCode, "")	{
	}
	
	virtual ~CEWSPhysicalAddressGsoapImpl() {
	}
	
	IMPL_PROPERTY_2(CEWSString, Street);
	IMPL_PROPERTY_2(CEWSString, City);
	IMPL_PROPERTY_2(CEWSString, State);
	IMPL_PROPERTY_2(CEWSString, CountryOrRegion);
	IMPL_PROPERTY_2(CEWSString, PostalCode);
};

class DLL_LOCAL CEWSCompleteNameGsoapImpl
		: public virtual CEWSCompleteName {
public:
	CEWSCompleteNameGsoapImpl()
			: INIT_PROPERTY(Title, "")
			, INIT_PROPERTY(FirstName, "")
			, INIT_PROPERTY(MiddleName, "")
			, INIT_PROPERTY(LastName, "")
			, INIT_PROPERTY(Suffix, "")
			, INIT_PROPERTY(Initials, "")
			, INIT_PROPERTY(FullName, "")
			, INIT_PROPERTY(Nickname, "")
			, INIT_PROPERTY(YomiFirstName, "")
			, INIT_PROPERTY(YomiLastName, "") {
	}

	virtual ~CEWSCompleteNameGsoapImpl() {
	}

	IMPL_PROPERTY_2(CEWSString, Title);
	IMPL_PROPERTY_2(CEWSString, FirstName);
	IMPL_PROPERTY_2(CEWSString, MiddleName);
	IMPL_PROPERTY_2(CEWSString, LastName);
	IMPL_PROPERTY_2(CEWSString, Suffix);
	IMPL_PROPERTY_2(CEWSString, Initials);
	IMPL_PROPERTY_2(CEWSString, FullName);
	IMPL_PROPERTY_2(CEWSString, Nickname);
	IMPL_PROPERTY_2(CEWSString, YomiFirstName);
	IMPL_PROPERTY_2(CEWSString, YomiLastName);
};

class DLL_LOCAL CEWSContactItemGsoapImpl
		: public virtual CEWSItemGsoapImpl
		, public virtual CEWSContactItem {
public:
	CEWSContactItemGsoapImpl()
			: INIT_PROPERTY(Name, "")
			, INIT_PROPERTY(EmailAddress, "")
			, INIT_PROPERTY(RoutingType, "")
			, INIT_PROPERTY(MailboxType, CEWSEmailAddress::MailboxType_Mailbox)
			, INIT_PROPERTY(DisplayName, "")
			, INIT_PROPERTY(GivenName, "")
			, INIT_PROPERTY(Initials, "")
			, INIT_PROPERTY(MiddleName, "")
			, INIT_PROPERTY(Nickname, "")
			, INIT_PROPERTY(CompleteName, NULL)
			, INIT_PROPERTY(CompanyName, "")
			, INIT_PROPERTY_NOT_OWN_PT(EmailAddresses, new CEWSStringList)
			, INIT_PROPERTY_NOT_OWN_PT(PhysicalAddresses, new CEWSPhysicalAddressList)
			, INIT_PROPERTY_NOT_OWN_PT(PhoneNumbers, new CEWSStringList)
			, INIT_PROPERTY(AssistantName, "")
			, INIT_PROPERTY(Birthday, 0)
			, INIT_PROPERTY(BusinessHomePage, "")
			, INIT_PROPERTY_NOT_OWN_PT(Children, new CEWSStringList)
			, INIT_PROPERTY_NOT_OWN_PT(Companies, new CEWSStringList)
			, INIT_PROPERTY(ContactSource, CEWSContactItem::ContactSource_ActiveDirectory)
			, INIT_PROPERTY(Department, "")
			, INIT_PROPERTY(Generation, "")
			, INIT_PROPERTY_NOT_OWN_PT(ImAddresses, new CEWSStringList)
			, INIT_PROPERTY(JobTitle, "")
			, INIT_PROPERTY(Manager, "")
			, INIT_PROPERTY(Mileage, "")
			, INIT_PROPERTY(OfficeLocation, "")
			, INIT_PROPERTY_NOT_OWN_PT(PostalAddress, NULL)
			, INIT_PROPERTY(Profession, "")
			, INIT_PROPERTY(SpouseName, "")
			, INIT_PROPERTY(Surname, "")
			, INIT_PROPERTY(WeddingAnniversary, 0)
			, INIT_PROPERTY(MailList, false) {
		SetItemType(CEWSItem::Item_Contact);
	}
			
	virtual ~CEWSContactItemGsoapImpl() {
		if (m_CompleteName) delete m_CompleteName;

		RELEASE_PROPERTY_NOT_OWN_PT(EmailAddresses);
		RELEASE_PROPERTY_NOT_OWN_PT(PhysicalAddresses);
		RELEASE_PROPERTY_NOT_OWN_PT(PhoneNumbers);
		RELEASE_PROPERTY_NOT_OWN_PT(Children);
		RELEASE_PROPERTY_NOT_OWN_PT(Companies);
		RELEASE_PROPERTY_NOT_OWN_PT(ImAddresses);
		RELEASE_PROPERTY_NOT_OWN_PT(PostalAddress);
	}
	
public:
	virtual void FromItemType(ews2__ItemType * pItem);

	IMPL_PROPERTY_2(CEWSString, Name);
	IMPL_PROPERTY_2(CEWSString, EmailAddress);
	IMPL_PROPERTY_2(CEWSString, RoutingType);
	IMPL_PROPERTY(CEWSAttendee::EWSMailboxTypeEnum, MailboxType);
	IMPL_PROPERTY_2(CEWSString, DisplayName);
	IMPL_PROPERTY_2(CEWSString, GivenName);
	IMPL_PROPERTY_2(CEWSString, Initials);
	IMPL_PROPERTY_2(CEWSString, MiddleName);
	IMPL_PROPERTY_2(CEWSString, Nickname);
	IMPL_PROPERTY(CEWSCompleteName *, CompleteName);
	IMPL_PROPERTY_2(CEWSString, CompanyName);
	IMPL_PROPERTY_NOT_OWN_PT(CEWSStringList *, EmailAddresses);
	IMPL_PROPERTY_NOT_OWN_PT(CEWSPhysicalAddressList *, PhysicalAddresses);
	IMPL_PROPERTY_NOT_OWN_PT(CEWSStringList *, PhoneNumbers);
	IMPL_PROPERTY_2(CEWSString, AssistantName);
	IMPL_PROPERTY(time_t, Birthday);
	IMPL_PROPERTY_2(CEWSString, BusinessHomePage);
	IMPL_PROPERTY_NOT_OWN_PT(CEWSStringList *, Children);
	IMPL_PROPERTY_NOT_OWN_PT(CEWSStringList *, Companies);
	IMPL_PROPERTY(EWSContactSourceEnum, ContactSource);
	IMPL_PROPERTY_2(CEWSString, Department);
	IMPL_PROPERTY_2(CEWSString, Generation);
	IMPL_PROPERTY_NOT_OWN_PT(CEWSStringList *, ImAddresses);
	IMPL_PROPERTY_2(CEWSString, JobTitle);
	IMPL_PROPERTY_2(CEWSString, Manager);
	IMPL_PROPERTY_2(CEWSString, Mileage);
	IMPL_PROPERTY_2(CEWSString, OfficeLocation);
	IMPL_PROPERTY_NOT_OWN_PT(CEWSPhysicalAddress *, PostalAddress);
	IMPL_PROPERTY_2(CEWSString, Profession);
	IMPL_PROPERTY_2(CEWSString, SpouseName);
	IMPL_PROPERTY_2(CEWSString, Surname);
	IMPL_PROPERTY(time_t, WeddingAnniversary);
	IMPL_PROPERTY_B(bool, MailList);
};

class DLL_LOCAL CEWSContactOperationGsoapImpl: public CEWSContactOperation {
public:
	CEWSContactOperationGsoapImpl(CEWSSessionGsoapImpl * pSession);
	virtual ~CEWSContactOperationGsoapImpl();

public:
	virtual CEWSItemList * ResolveNames(const CEWSString & unresolvedEntry, CEWSError * pError);
private:
	CEWSSessionGsoapImpl * m_pSession;
};

class DLL_LOCAL CEWSItemOperationGsoapImpl: public CEWSItemOperation {
public:
	CEWSItemOperationGsoapImpl(CEWSSessionGsoapImpl * pSession);
	virtual ~CEWSItemOperationGsoapImpl();
public:
	virtual bool SyncItems(const CEWSString & folderId,
	                       CEWSItemOperationCallback * callback,
	                       CEWSError * pError = NULL);
	virtual bool SyncItems(const CEWSFolder * pFolder,
	                       CEWSItemOperationCallback * callback,
	                       CEWSError * pError = NULL);
	virtual bool SyncItems(CEWSFolder::EWSDistinguishedFolderIdNameEnum distinguishedFolderId,
	                       CEWSItemOperationCallback * callback,
	                       CEWSError * pError = NULL);
	virtual bool GetUnreadItems(const CEWSString & folderId,
	                            CEWSItemOperationCallback * callback,
	                            CEWSError * pError);
	virtual CEWSItem * GetItem(const CEWSString & itemId,
	                           int flags,
	                           CEWSError * pError = NULL);
	virtual bool GetItems(const CEWSStringList & itemIds,
	                      CEWSItemList * pItemList,
	                      int flags,
	                      CEWSError * pError = NULL);
	virtual bool CreateItem(CEWSItem * pItem,
	                        const CEWSFolder * pSaveFolder,
	                        CEWSError * pError = NULL);
	virtual bool CreateItem(CEWSItem * pItem,
	                        CEWSFolder::EWSDistinguishedFolderIdNameEnum distinguishedFolderId,
	                        CEWSError * pError = NULL);
	virtual bool SendItem(CEWSMessageItem * pMessageItem,
	                      CEWSError * pError = NULL);
	virtual bool DeleteItem(CEWSItem * pItem,
	                        bool moveToDeleted,
	                        CEWSError * pError = NULL);
	virtual bool DeleteItems(CEWSItemList * pItemList,
	                         bool moveToDeleted,
	                         CEWSError * pError);
	virtual bool MarkItemsRead(CEWSItemList * pItemList,
	                           bool read,
	                           CEWSError * pError);

private:
	friend class CEWSCalendarOperationGsoapImpl;
	friend class CEWSTaskOperationGsoapImpl;
    
	bool CreateItem(CEWSItem * pItem,
	                const CEWSFolder * pSaveFolder,
	                ews2__MessageDispositionType messageDispositionType,
	                CEWSError * pError = NULL);
	bool CreateItem(CEWSItem * pItem,
	                CEWSFolder::EWSDistinguishedFolderIdNameEnum distinguishedFolderId,
	                ews2__MessageDispositionType messageDispositionType,
	                CEWSError * pError = NULL);
	bool CreateItem(CEWSItem * pItem,
	                ews1__CreateItemType * pCreateItemType,
	                CEWSError * pError);
	bool __SyncItems(ews2__TargetFolderIdType * targetFolderIdType,
	                 ews2__ItemResponseShapeType * pItemShape,
	                 CEWSItemOperationCallback * callback,
	                 CEWSError * pError);
private:
	CEWSSessionGsoapImpl * m_pSession;
};

class DLL_LOCAL CEWSCalendarOperationGsoapImpl: public CEWSCalendarOperation {
public:
	CEWSCalendarOperationGsoapImpl(CEWSSessionGsoapImpl * pSession);
	virtual ~CEWSCalendarOperationGsoapImpl();

public:
	virtual bool SyncCalendar(CEWSItemOperationCallback * callback,
	                          CEWSError * pError);
	virtual bool GetRecurrenceMasterItemId(const CEWSString & itemId,
	                                       CEWSString & masterItemId,
	                                       CEWSString & masterUId,
	                                       CEWSError * pError);
	virtual bool AcceptEvent(CEWSCalendarItem * pEwsItem,
	                         CEWSError * pError);
	virtual bool TentativelyAcceptEvent(CEWSCalendarItem * pEwsItem,
	                                    CEWSError * pError);
	virtual bool DeclineEvent(CEWSCalendarItem * pEwsItem,
	                          CEWSError * pError);
	virtual bool DeleteOccurrence(const CEWSString & masterItemId,
	                              int occurrenceInstance,
	                              CEWSError * pError);
	virtual bool UpdateEvent(CEWSCalendarItem * pEwsItem,
	                         int flags,
	                         CEWSError * pError);
	virtual bool UpdateEventOccurrence(CEWSCalendarItem * pEwsItem,
	                                   int instanceIndex,
	                                   int flags,
	                                   CEWSError * pError);
private:
	bool __ResponseToEvent(int responseType,
	                       ews2__WellKnownResponseObjectType * itemType,
	                       CEWSCalendarItem * pEwsItem,
	                       CEWSError * pError);    
	bool __UpdateEvent(CEWSCalendarItem * pItem,
	                   int falgs,
	                   ews2__ItemChangeType * ItemChange,
	                   CEWSError * pError);
    
	CEWSSessionGsoapImpl * m_pSession;
	CEWSItemOperationGsoapImpl m_ItemOp;
};

class DLL_LOCAL CEWSEmailAddressImpl
		: public virtual CEWSEmailAddress {
public:
	friend class CEWSRecipientGsoapImpl;
    
	CEWSEmailAddressImpl()
			: INIT_PROPERTY(Name, "")
			, INIT_PROPERTY(EmailAddress, "")
			, INIT_PROPERTY(ItemId, "") 
			, INIT_PROPERTY(ChangeKey, "") 
			, INIT_PROPERTY(RoutingType, "") 
			, INIT_PROPERTY(MailboxType, CEWSEmailAddress::MailboxType_Mailbox) {

	}

	virtual ~CEWSEmailAddressImpl() {

	}

public:
	IMPL_PROPERTY_2(CEWSString, Name);
	IMPL_PROPERTY_2(CEWSString, EmailAddress);
	IMPL_PROPERTY_2(CEWSString, ItemId);
	IMPL_PROPERTY_2(CEWSString, ChangeKey);
	IMPL_PROPERTY_2(CEWSString, RoutingType);
	IMPL_PROPERTY(enum EWSMailboxTypeEnum, MailboxType);
};

class DLL_LOCAL CEWSRecipientGsoapImpl
		: public virtual CEWSEmailAddressImpl
		, public virtual CEWSRecipient {
public:
	static CEWSRecipientGsoapImpl * CreateInstance(
	    ews2__SingleRecipientType * pItem);
	static CEWSRecipientGsoapImpl * CreateInstance(
	    ews2__EmailAddressType * pItem);

	CEWSRecipientGsoapImpl() {

	}

	virtual ~CEWSRecipientGsoapImpl() {

	}

public:
	virtual bool IsSame(CEWSRecipient * pAttachment);
};

class DLL_LOCAL CEWSAttachmentGsoapImpl: public virtual CEWSAttachment {
public:
	static CEWSAttachmentGsoapImpl * CreateInstance(EWSAttachmentType type,
	                                                ews2__AttachmentType * pAttachment);

	CEWSAttachmentGsoapImpl() :
			INIT_PROPERTY(AttachmentType, CEWSAttachment::Attachment_File),
			INIT_PROPERTY(AttachmentId, ""),
			INIT_PROPERTY(ContentId, ""),
			INIT_PROPERTY(ContentLocation, ""),
			INIT_PROPERTY(ContentType, ""),
			INIT_PROPERTY(Name, "") {

	}
	virtual ~CEWSAttachmentGsoapImpl() {

	}
public:
	IMPL_PROPERTY(EWSAttachmentType, AttachmentType)
			;IMPL_PROPERTY_2(CEWSString, AttachmentId)
					 ;IMPL_PROPERTY_2(CEWSString, ContentId)
							  ;IMPL_PROPERTY_2(CEWSString, ContentLocation)
									   ;IMPL_PROPERTY_2(CEWSString, ContentType)
											    ;IMPL_PROPERTY_2(CEWSString, Name)
													     ;

	virtual bool IsSame(CEWSAttachment * pAttachment);
};

class DLL_LOCAL CEWSItemAttachmentGsoapImpl: public virtual CEWSAttachmentGsoapImpl,
                                             public virtual CEWSItemAttachment {
public:
	CEWSItemAttachmentGsoapImpl() :
			INIT_PROPERTY(ItemId, ""),
			INIT_PROPERTY(ItemType, CEWSItem::Item_Message) {
		SetAttachmentType(CEWSAttachment::Attachment_Item);
	}
	virtual ~CEWSItemAttachmentGsoapImpl() {

	}
public:
	IMPL_PROPERTY_2(CEWSString, ItemId)
			;IMPL_PROPERTY(CEWSItem::EWSItemType, ItemType)
					 ;
};

class DLL_LOCAL CEWSFileAttachmentGsoapImpl: public virtual CEWSAttachmentGsoapImpl,
                                             public virtual CEWSFileAttachment {
public:
	CEWSFileAttachmentGsoapImpl() :
			INIT_PROPERTY(Content, "") {
		SetAttachmentType(CEWSAttachment::Attachment_File);
	}
	virtual ~CEWSFileAttachmentGsoapImpl() {

	}

public:
	IMPL_PROPERTY_2(CEWSString, Content)
			;
};

class DLL_LOCAL CEWSAttachmentOperationGsoapImpl: public CEWSAttachmentOperation {
public:
	CEWSAttachmentOperationGsoapImpl(CEWSSessionGsoapImpl * pSession);
	virtual ~CEWSAttachmentOperationGsoapImpl();
public:
	virtual CEWSAttachment * GetAttachment(const CEWSString & attachmentId,
	                                       CEWSError * pError = NULL);
	virtual bool CreateAttachment(CEWSAttachment * pAttachment,
	                              CEWSItem * parentItem, CEWSError * pError = NULL);
	virtual bool CreateAttachments(const CEWSAttachmentList * pAttachmentList,
	                               CEWSItem * parentItem, CEWSError * pError);
private:
	CEWSSessionGsoapImpl * m_pSession;
};

class DLL_LOCAL CEWSSubscriptionGsoapImpl : public CEWSSubscription {
public:
	CEWSSubscriptionGsoapImpl() :
			INIT_PROPERTY(SubscriptionId, ""),
			INIT_PROPERTY(WaterMark, "") {

	}
	virtual ~CEWSSubscriptionGsoapImpl() {

	}

public:
	IMPL_PROPERTY_2(CEWSString, SubscriptionId);
	IMPL_PROPERTY_2(CEWSString, WaterMark);
};

class DLL_LOCAL CEWSSubscriptionOperationGsoapImpl : public CEWSSubscriptionOperation {
public:
	CEWSSubscriptionOperationGsoapImpl(CEWSSessionGsoapImpl * pSession);
	virtual ~CEWSSubscriptionOperationGsoapImpl();

public:
	virtual CEWSSubscription * SubscribeWithPull(const CEWSStringList & folderIdList,
                                                 
	                                             const CEWSIntList & distinguishedIdNameList, int notifyTypeFlags,
	                                             int timeout,
	                                             CEWSError * pError = NULL);
	virtual CEWSSubscription * SubscribeWithPush(const CEWSStringList & folderIdList,
	                                             const CEWSIntList & distinguishedIdNameList, int notifyTypeFlags,
	                                             int timeout,
	                                             const CEWSString & url,
	                                             CEWSSubscriptionCallback * callback,
	                                             CEWSError * pError = NULL);
	virtual bool Unsubscribe(const CEWSSubscription * subscription, CEWSError * pError = NULL);
	virtual bool GetEvents(const CEWSSubscription * subscription,
	                       CEWSSubscriptionCallback * callback,
	                       bool & moreEvents,
	                       CEWSError * pError = NULL) ;

	virtual bool Unsubscribe(const CEWSString & subscriptionId,
	                         const CEWSString & waterMark,
	                         CEWSError * pError = NULL);
	virtual bool GetEvents(const CEWSString & subscriptionId,
	                       const CEWSString & waterMark,
	                       CEWSSubscriptionCallback * callback,
	                       bool & moreEvents,
	                       CEWSError * pError = NULL);
private:
	CEWSSessionGsoapImpl * m_pSession;
	CEWSSubscription * __Subscribe(const CEWSStringList & folderIdList,
	                               const CEWSIntList & distinguishedIdNameList,
	                               int notifyTypeFlags,
	                               int timeout,
	                               const CEWSString & url,
	                               CEWSSubscriptionCallback * callback,
	                               bool pullSubscription,
	                               CEWSError * pError = NULL);

};

class DLL_LOCAL CEWSRecurrenceRangeGsoapImpl
		: public virtual CEWSRecurrenceRange {
public:
	CEWSRecurrenceRangeGsoapImpl()
			: INIT_PROPERTY(RecurrenceRangeType, CEWSRecurrenceRange::NoEnd)
			, INIT_PROPERTY(StartDate, "") {
	}
    
	virtual ~CEWSRecurrenceRangeGsoapImpl() {
	}

public:
	IMPL_PROPERTY(enum EWSRecurrenceRangeTypeEnum, RecurrenceRangeType);
	IMPL_PROPERTY_2(CEWSString, StartDate);
};

class DLL_LOCAL CEWSNoEndRecurrenceRangeGsoapImpl
		: public virtual CEWSRecurrenceRangeGsoapImpl
		, public virtual CEWSNoEndRecurrenceRange{
public:
	CEWSNoEndRecurrenceRangeGsoapImpl() {
		SetRecurrenceRangeType(CEWSRecurrenceRange::NoEnd);
	}
    
	virtual ~CEWSNoEndRecurrenceRangeGsoapImpl() {
	}
};

class DLL_LOCAL CEWSEndDateRecurrenceRangeGsoapImpl
		: public virtual CEWSRecurrenceRangeGsoapImpl
		, public virtual CEWSEndDateRecurrenceRange {
public:
	CEWSEndDateRecurrenceRangeGsoapImpl()
			: INIT_PROPERTY(EndDate, "") {
		SetRecurrenceRangeType(CEWSRecurrenceRange::EndDate);
	}
            
	virtual ~CEWSEndDateRecurrenceRangeGsoapImpl() {
	}

public:
	IMPL_PROPERTY_2(CEWSString, EndDate);
};

class DLL_LOCAL CEWSNumberedRecurrenceRangeGsoapImpl
		: public virtual CEWSRecurrenceRangeGsoapImpl
		, public virtual CEWSNumberedRecurrenceRange {
public:
	CEWSNumberedRecurrenceRangeGsoapImpl()
			: INIT_PROPERTY(NumberOfOccurrences, 0) {
		SetRecurrenceRangeType(CEWSRecurrenceRange::Numbered);
	}
    
	virtual ~CEWSNumberedRecurrenceRangeGsoapImpl() {
	}

public:
	IMPL_PROPERTY(int, NumberOfOccurrences);
};

class DLL_LOCAL CEWSRecurrencePatternGsoapImpl : public virtual CEWSRecurrencePattern {
public:
	CEWSRecurrencePatternGsoapImpl()
			: INIT_PROPERTY(RecurrenceType, CEWSRecurrencePattern::Daily) {
	}
	virtual ~CEWSRecurrencePatternGsoapImpl() {
	}

public:
	IMPL_PROPERTY(enum EWSRecurrenceTypeEnum, RecurrenceType);
};

class DLL_LOCAL CEWSRecurrencePatternIntervalGsoapImpl
		: public virtual CEWSRecurrencePatternGsoapImpl
		, public virtual CEWSRecurrencePatternInterval {
public:
	CEWSRecurrencePatternIntervalGsoapImpl()
			: INIT_PROPERTY(Interval, 1) {
	}
	virtual ~CEWSRecurrencePatternIntervalGsoapImpl() {
	}
public:
	IMPL_PROPERTY(int, Interval);
};

class DLL_LOCAL CEWSRecurrencePatternRelativeYearlyGsoapImpl
		: public virtual CEWSRecurrencePatternGsoapImpl
		, public virtual CEWSRecurrencePatternRelativeYearly {
public:
	CEWSRecurrencePatternRelativeYearlyGsoapImpl()
			: INIT_PROPERTY(DaysOfWeek, CEWSRecurrencePattern::Day)
			, INIT_PROPERTY(DayOfWeekIndex, CEWSRecurrencePattern::First)
			, INIT_PROPERTY(Month, CEWSRecurrencePattern::January) {
		SetRecurrenceType(CEWSRecurrencePattern::RelativeYearly);
	}   
	virtual ~CEWSRecurrencePatternRelativeYearlyGsoapImpl() {
	}
public:
	IMPL_PROPERTY(enum CEWSRecurrencePattern::EWSDayOfWeekEnum, DaysOfWeek);
	IMPL_PROPERTY(enum CEWSRecurrencePattern::EWSDayOfWeekIndexEnum, DayOfWeekIndex);
	IMPL_PROPERTY(enum CEWSRecurrencePattern::EWSMonthNamesEnum, Month);
};

class DLL_LOCAL CEWSRecurrencePatternAbsoluteYearlyGsoapImpl
		: public virtual CEWSRecurrencePatternGsoapImpl
		, public virtual CEWSRecurrencePatternAbsoluteYearly {
public:
	CEWSRecurrencePatternAbsoluteYearlyGsoapImpl()
			: INIT_PROPERTY(DayOfMonth, 0)
			, INIT_PROPERTY(Month, CEWSRecurrencePattern::January) {
		SetRecurrenceType(CEWSRecurrencePattern::AbsoluteYearly);
	}
    
	virtual ~CEWSRecurrencePatternAbsoluteYearlyGsoapImpl() {
	}
public:
	IMPL_PROPERTY(int, DayOfMonth);
	IMPL_PROPERTY(enum CEWSRecurrencePattern::EWSMonthNamesEnum, Month);
};

class DLL_LOCAL CEWSRecurrencePatternRelativeMonthlyGsoapImpl
		: public virtual CEWSRecurrencePatternIntervalGsoapImpl
		, public virtual CEWSRecurrencePatternRelativeMonthly {
public:
	CEWSRecurrencePatternRelativeMonthlyGsoapImpl()
			: INIT_PROPERTY(DaysOfWeek, CEWSRecurrencePattern::Day)
			, INIT_PROPERTY(DayOfWeekIndex, CEWSRecurrencePattern::First) {
		SetRecurrenceType(CEWSRecurrencePattern::RelativeMonthly);
	}

	virtual ~CEWSRecurrencePatternRelativeMonthlyGsoapImpl() {
	}
public:
	IMPL_PROPERTY(enum CEWSRecurrencePattern::EWSDayOfWeekEnum, DaysOfWeek);
	IMPL_PROPERTY(enum CEWSRecurrencePattern::EWSDayOfWeekIndexEnum, DayOfWeekIndex);
};

class DLL_LOCAL CEWSRecurrencePatternAbsoluteMonthlyGsoapImpl
		: public virtual CEWSRecurrencePatternIntervalGsoapImpl
		, public virtual CEWSRecurrencePatternAbsoluteMonthly {
public:
	CEWSRecurrencePatternAbsoluteMonthlyGsoapImpl()
			: INIT_PROPERTY(DayOfMonth, 0) {
		SetRecurrenceType(CEWSRecurrencePattern::AbsoluteMonthly);
	}
	virtual ~CEWSRecurrencePatternAbsoluteMonthlyGsoapImpl() {
	}
public:
	IMPL_PROPERTY(int, DayOfMonth);
};

class DLL_LOCAL CEWSRecurrencePatternWeeklyGsoapImpl
		: public virtual CEWSRecurrencePatternIntervalGsoapImpl
		, public virtual CEWSRecurrencePatternWeekly {
public:
	CEWSRecurrencePatternWeeklyGsoapImpl()
			: INIT_PROPERTY(DaysOfWeek, CEWSRecurrencePattern::Day) {
		SetRecurrenceType(CEWSRecurrencePattern::Weekly);
	}
    
	virtual ~CEWSRecurrencePatternWeeklyGsoapImpl() {
	}
public:
	IMPL_PROPERTY(enum CEWSRecurrencePattern::EWSDayOfWeekEnum, DaysOfWeek);
};

class DLL_LOCAL CEWSRecurrencePatternDailyGsoapImpl
		: public virtual CEWSRecurrencePatternIntervalGsoapImpl
		, public virtual CEWSRecurrencePatternDaily {
public:
	CEWSRecurrencePatternDailyGsoapImpl() {
		SetRecurrenceType(CEWSRecurrencePattern::Daily);
	}
	virtual ~CEWSRecurrencePatternDailyGsoapImpl() {
	}
};

class DLL_LOCAL CEWSRecurrenceGsoapImpl
		: public virtual CEWSRecurrence {
public:
	CEWSRecurrenceGsoapImpl()
			: INIT_PROPERTY(RecurrenceRange, NULL)
			, INIT_PROPERTY(RecurrencePattern, NULL) {
	}
	virtual ~CEWSRecurrenceGsoapImpl() {
		if (m_RecurrenceRange) delete m_RecurrenceRange;
		if (m_RecurrencePattern) delete m_RecurrencePattern;
	}

public:
	IMPL_PROPERTY(CEWSRecurrenceRange *, RecurrenceRange);
	IMPL_PROPERTY(CEWSRecurrencePattern *, RecurrencePattern);
};

class DLL_LOCAL CEWSOccurrenceInfoGsoapImpl
		: public virtual CEWSOccurrenceInfo {
public:
	CEWSOccurrenceInfoGsoapImpl()
			: INIT_PROPERTY(ItemId, "")
			, INIT_PROPERTY(ChangeKey, "")
			, INIT_PROPERTY(Start, 0)
			, INIT_PROPERTY(End, 0)
			, INIT_PROPERTY(OriginalStart, 0) {
	}
	virtual ~CEWSOccurrenceInfoGsoapImpl() {
	}

public:
	IMPL_PROPERTY_2(CEWSString, ItemId);
	IMPL_PROPERTY_2(CEWSString, ChangeKey);
	IMPL_PROPERTY(time_t, Start);
	IMPL_PROPERTY(time_t, End);
	IMPL_PROPERTY(time_t, OriginalStart);
};

class DLL_LOCAL CEWSDeletedOccurrenceInfoGsoapImpl
		: public virtual CEWSDeletedOccurrenceInfo{
public:
	CEWSDeletedOccurrenceInfoGsoapImpl()
			: INIT_PROPERTY(Start, 0) {
	}

	virtual ~CEWSDeletedOccurrenceInfoGsoapImpl() {
	}

public:
	IMPL_PROPERTY(time_t, Start);
};

class DLL_LOCAL CEWSAttendeeGsoapImpl
		: public virtual CEWSEmailAddressImpl
		, public virtual CEWSAttendee {
public:
	CEWSAttendeeGsoapImpl()
			: INIT_PROPERTY(ResponseType, CEWSAttendee::Unknown)
			, INIT_PROPERTY(LastResponseTime, 0) {
	}
	virtual ~CEWSAttendeeGsoapImpl() {
	}

public:
	IMPL_PROPERTY(enum EWSResponseTypeEnum, ResponseType);
	IMPL_PROPERTY(time_t, LastResponseTime);
};

class DLL_LOCAL CEWSTimeChangeSequenceGsoapImpl
		: public virtual CEWSTimeChangeSequence{
public:
	CEWSTimeChangeSequenceGsoapImpl()
			: INIT_PROPERTY(TimeChangeSequenceType, CEWSTimeChangeSequence::RelativeYearly) {
	}
	virtual ~CEWSTimeChangeSequenceGsoapImpl() {
	}

public:
	IMPL_PROPERTY(enum EWSTimeChangeSequenceTypeEnum, TimeChangeSequenceType);
};

class DLL_LOCAL CEWSTimeChangeSequenceRelativeYearlyGsoapImpl
		: public virtual CEWSTimeChangeSequenceGsoapImpl
		, public virtual CEWSTimeChangeSequenceRelativeYearly {
public:
	CEWSTimeChangeSequenceRelativeYearlyGsoapImpl()
			: INIT_PROPERTY(RelativeYearly,
			                new CEWSRecurrencePatternRelativeYearlyGsoapImpl) {
		SetTimeChangeSequenceType(CEWSTimeChangeSequence::RelativeYearly);
	}
    
	virtual ~CEWSTimeChangeSequenceRelativeYearlyGsoapImpl() {
		if (m_RelativeYearly) delete m_RelativeYearly;
	}

public:
	IMPL_PROPERTY(CEWSRecurrencePatternRelativeYearly *, RelativeYearly);
};

class DLL_LOCAL CEWSTimeChangeSequenceAbsoluteDateGsoapImpl
		: public virtual CEWSTimeChangeSequenceGsoapImpl
		, public virtual CEWSTimeChangeSequenceAbsoluteDate{
public:
	CEWSTimeChangeSequenceAbsoluteDateGsoapImpl()
			: INIT_PROPERTY(AbsoluteDate, "") {
		SetTimeChangeSequenceType(CEWSTimeChangeSequence::AbsoluteDate);
	}
	virtual ~CEWSTimeChangeSequenceAbsoluteDateGsoapImpl() {
	}

public:
	IMPL_PROPERTY_2(CEWSString, AbsoluteDate);
};

class DLL_LOCAL CEWSTimeChangeGsoapImpl
		: public virtual CEWSTimeChange {
public:
	CEWSTimeChangeGsoapImpl()
			: INIT_PROPERTY(Offset, 0)
			, INIT_PROPERTY(TimeChangeSequence, NULL)
			, INIT_PROPERTY(Time, "")
			, INIT_PROPERTY(TimeZoneName, "") {
	}
	virtual ~CEWSTimeChangeGsoapImpl() {
		if (m_TimeChangeSequence) delete m_TimeChangeSequence;
	}

public:
	IMPL_PROPERTY(long long, Offset);
	IMPL_PROPERTY(CEWSTimeChangeSequence *, TimeChangeSequence);
	IMPL_PROPERTY_2(CEWSString, Time);
	IMPL_PROPERTY_2(CEWSString, TimeZoneName);
};

class DLL_LOCAL CEWSTimeZoneGsoapImpl
		: public virtual CEWSTimeZone{
public:
	CEWSTimeZoneGsoapImpl()
			: INIT_PROPERTY(TimeZoneName, "")
			, INIT_PROPERTY(BaseOffset, 0)
			, INIT_PROPERTY(Standard, NULL)
			, INIT_PROPERTY(Daylight, NULL) {
	}
    
	virtual ~CEWSTimeZoneGsoapImpl() {
		if (m_Standard) delete m_Standard;
		if (m_Daylight) delete m_Daylight;
	}

public:
	IMPL_PROPERTY_2(CEWSString, TimeZoneName);
	IMPL_PROPERTY(long long, BaseOffset);
	IMPL_PROPERTY(CEWSTimeChange *, Standard);
	IMPL_PROPERTY(CEWSTimeChange *, Daylight);
};

class DLL_LOCAL CalendarItemTypeBuilder : public ItemTypeBuilder {
public:
	CalendarItemTypeBuilder(soap * soap,
	                        const CEWSCalendarItem * pEWSItem,
	                        CEWSObjectPool * pObjPool);
	virtual ~CalendarItemTypeBuilder();

	virtual ews2__ItemType * Build();
	virtual ews2__ItemType * CreateItemType();
};

class DLL_LOCAL CEWSCalendarItemGsoapImpl
		: public virtual CEWSItemGsoapImpl
		, public virtual CEWSCalendarItem {
public:
	CEWSCalendarItemGsoapImpl()
			: INIT_PROPERTY(UID, "")
			, INIT_PROPERTY(RecurrenceId, 0)
			, INIT_PROPERTY(DateTimeStamp, 0)
			, INIT_PROPERTY(Start, 0)
			, INIT_PROPERTY(End, 0)
			, INIT_PROPERTY(OriginalStart, 0)
			, INIT_PROPERTY(AllDayEvent, false)
			, INIT_PROPERTY(LegacyFreeBusyStatus, CEWSCalendarItem::Free)
			, INIT_PROPERTY(Location, "")
			, INIT_PROPERTY(When, "")
			, INIT_PROPERTY(Meeting, false)
			, INIT_PROPERTY(Cancelled, false)
			, INIT_PROPERTY(Recurring, false)
			, INIT_PROPERTY(MeetingRequestWasSent, false)
			, INIT_PROPERTY(ResponseRequested, false)
			, INIT_PROPERTY(CalendarItemType, CEWSCalendarItem::Single)
			, INIT_PROPERTY(MyResponseType, CEWSAttendee::Unknown)
			, INIT_PROPERTY(Organizer, NULL)
			, INIT_PROPERTY_NOT_OWN_PT(RequiredAttendees, new CEWSAttendeeList)
			, INIT_PROPERTY_NOT_OWN_PT(OptionalAttendees, new CEWSAttendeeList)
			, INIT_PROPERTY_NOT_OWN_PT(Resources, new CEWSAttendeeList)
			, INIT_PROPERTY(ConflictingMeetingCount, 0)
			, INIT_PROPERTY(AdjacentMeetingCount, 0)
			, INIT_PROPERTY_NOT_OWN_PT(ConflictingMeetings, new CEWSItemList)
			, INIT_PROPERTY_NOT_OWN_PT(AdjacentMeetings, new CEWSItemList)
			, INIT_PROPERTY(Duration, "")
			, INIT_PROPERTY(TimeZone, "")
			, INIT_PROPERTY(AppointmentReplyTime, 0)
			, INIT_PROPERTY(AppointmentSequenceNumber, 0)
			, INIT_PROPERTY(AppointmentState, 0)
			, INIT_PROPERTY(Recurrence, NULL)
			, INIT_PROPERTY(FirstOccurrence, NULL)
			, INIT_PROPERTY(LastOccurrence, NULL)
			, INIT_PROPERTY_NOT_OWN_PT(ModifiedOccurrences, new CEWSOccurrenceInfoList)
			, INIT_PROPERTY_NOT_OWN_PT(DeletedOccurrences, new CEWSDeletedOccurrenceInfoList)
			, INIT_PROPERTY(MeetingTimeZone, NULL)
			, INIT_PROPERTY(ConferenceType, 0)
			, INIT_PROPERTY(AllowNewTimeProposal, false)
			, INIT_PROPERTY(OnlineMeeting, false)
			, INIT_PROPERTY(MeetingWorkspaceUrl, "")
			, INIT_PROPERTY(NetShowUrl, "")    {
		SetItemType(CEWSItem::Item_Calendar);
	}

	virtual ~CEWSCalendarItemGsoapImpl() {
		if (m_Organizer) delete m_Organizer;
		if (m_Recurrence) delete m_Recurrence;
		if (m_FirstOccurrence) delete m_FirstOccurrence;
		if (m_LastOccurrence) delete m_LastOccurrence;
		if (m_MeetingTimeZone) delete m_MeetingTimeZone;
        
		RELEASE_PROPERTY_NOT_OWN_PT(RequiredAttendees);
		RELEASE_PROPERTY_NOT_OWN_PT(OptionalAttendees);
		RELEASE_PROPERTY_NOT_OWN_PT(Resources);
		RELEASE_PROPERTY_NOT_OWN_PT(ConflictingMeetings);
		RELEASE_PROPERTY_NOT_OWN_PT(AdjacentMeetings);
		RELEASE_PROPERTY_NOT_OWN_PT(ModifiedOccurrences);
		RELEASE_PROPERTY_NOT_OWN_PT(DeletedOccurrences);
	}

public:
	virtual void FromItemType(ews2__ItemType * pItem);
    
	IMPL_PROPERTY_2(CEWSString, UID);
	IMPL_PROPERTY(time_t, RecurrenceId);
	IMPL_PROPERTY(time_t, DateTimeStamp);
	IMPL_PROPERTY(time_t, Start);
	IMPL_PROPERTY(time_t, End);
	IMPL_PROPERTY(time_t, OriginalStart);
	IMPL_PROPERTY_B(bool, AllDayEvent);
	IMPL_PROPERTY(enum EWSLegacyFreeBusyTypeEnum, LegacyFreeBusyStatus);
	IMPL_PROPERTY_2(CEWSString, Location);
	IMPL_PROPERTY_2(CEWSString, When);
	IMPL_PROPERTY_B(bool, Meeting);
	IMPL_PROPERTY_B(bool, Cancelled);
	IMPL_PROPERTY_B(bool, Recurring);
	IMPL_PROPERTY(bool, MeetingRequestWasSent);
	IMPL_PROPERTY_B(bool, ResponseRequested);
	IMPL_PROPERTY(enum EWSCalendarItemTypeEnum, CalendarItemType);
	IMPL_PROPERTY(enum CEWSAttendee::EWSResponseTypeEnum, MyResponseType);
	IMPL_PROPERTY(CEWSRecipient *, Organizer);
	IMPL_PROPERTY_NOT_OWN_PT(CEWSAttendeeList *, RequiredAttendees);
	IMPL_PROPERTY_NOT_OWN_PT(CEWSAttendeeList *, OptionalAttendees);
	IMPL_PROPERTY_NOT_OWN_PT(CEWSAttendeeList *, Resources);
	IMPL_PROPERTY(int, ConflictingMeetingCount);
	IMPL_PROPERTY(int, AdjacentMeetingCount);
	IMPL_PROPERTY_NOT_OWN_PT(CEWSItemList *, ConflictingMeetings);
	IMPL_PROPERTY_NOT_OWN_PT(CEWSItemList *, AdjacentMeetings);
	IMPL_PROPERTY_2(CEWSString, Duration);
	IMPL_PROPERTY_2(CEWSString, TimeZone);
	IMPL_PROPERTY(time_t, AppointmentReplyTime);
	IMPL_PROPERTY(int, AppointmentSequenceNumber);
	IMPL_PROPERTY(int, AppointmentState);
	IMPL_PROPERTY(CEWSRecurrence *, Recurrence);
	IMPL_PROPERTY(CEWSOccurrenceInfo *, FirstOccurrence);
	IMPL_PROPERTY(CEWSOccurrenceInfo *, LastOccurrence);
	IMPL_PROPERTY_NOT_OWN_PT(CEWSOccurrenceInfoList *, ModifiedOccurrences);
	IMPL_PROPERTY_NOT_OWN_PT(CEWSDeletedOccurrenceInfoList *, DeletedOccurrences);
	IMPL_PROPERTY(CEWSTimeZone *, MeetingTimeZone);
	IMPL_PROPERTY(int, ConferenceType);
	IMPL_PROPERTY_B(bool, AllowNewTimeProposal);
	IMPL_PROPERTY_B(bool, OnlineMeeting);
	IMPL_PROPERTY_2(CEWSString, MeetingWorkspaceUrl);
	IMPL_PROPERTY_2(CEWSString, NetShowUrl);
};

class DLL_LOCAL CEWSTaskItemGsoapImpl
		: public virtual CEWSItemGsoapImpl
		, public virtual CEWSTaskItem {
public:
	CEWSTaskItemGsoapImpl()
			:INIT_PROPERTY(ActualWork, 0)
			,INIT_PROPERTY(AssignedTime, 0)
			,INIT_PROPERTY(BillingInformation, "")
			,INIT_PROPERTY(ChangeCount, 0)
			,INIT_PROPERTY(Companies, 0)
			,INIT_PROPERTY(CompleteDate, 0)
			,INIT_PROPERTY(Contacts, 0)
			,INIT_PROPERTY(DelegationState, CEWSTaskItem::NoMatch)
			,INIT_PROPERTY(Delegator, "")
			,INIT_PROPERTY(DueDate, 0)
			,INIT_PROPERTY(IsAssignmentEditable, 0)
			,INIT_PROPERTY(Complete, false)
			,INIT_PROPERTY(Recurring, false)
			,INIT_PROPERTY(TeamTask, false)
			,INIT_PROPERTY(Mileage, "")
			,INIT_PROPERTY(Owner, "")
			,INIT_PROPERTY(PercentComplete, 0)
			,INIT_PROPERTY(Recurrence, 0)
			,INIT_PROPERTY(StartDate, 0)
			,INIT_PROPERTY(Status, CEWSTaskItem::NotStarted)
			,INIT_PROPERTY(StatusDescription, "")
			,INIT_PROPERTY(TotalWork, 0) {
	}
            
	virtual ~CEWSTaskItemGsoapImpl() {
		if (m_Recurrence) delete m_Recurrence;
		if (m_Companies) delete m_Companies;
		if (m_Contacts) delete m_Contacts;
	}

public:
	IMPL_PROPERTY(int, ActualWork);
	IMPL_PROPERTY(time_t, AssignedTime);
	IMPL_PROPERTY_2(CEWSString, BillingInformation);
	IMPL_PROPERTY(int, ChangeCount);
	IMPL_PROPERTY(CEWSStringList *, Companies);
	IMPL_PROPERTY(time_t, CompleteDate);
	IMPL_PROPERTY(CEWSStringList *, Contacts);
	IMPL_PROPERTY(enum DelegateStateTypeEnum, DelegationState);
	IMPL_PROPERTY_2(CEWSString, Delegator);
	IMPL_PROPERTY(time_t, DueDate);
	IMPL_PROPERTY(int, IsAssignmentEditable);
	IMPL_PROPERTY_B(bool, Complete);
	IMPL_PROPERTY_B(bool, Recurring);
	IMPL_PROPERTY_B(bool, TeamTask);
	IMPL_PROPERTY_2(CEWSString, Mileage);
	IMPL_PROPERTY_2(CEWSString, Owner);
	IMPL_PROPERTY(double, PercentComplete);
	IMPL_PROPERTY(CEWSRecurrence *, Recurrence);
	IMPL_PROPERTY(time_t, StartDate);
	IMPL_PROPERTY(enum StatusTypeEnum, Status);
	IMPL_PROPERTY_2(CEWSString, StatusDescription);
	IMPL_PROPERTY(int, TotalWork);
    
public:
	virtual void FromItemType(ews2__ItemType * pItem);
};

class DLL_LOCAL CEWSTaskOperationGsoapImpl: public CEWSTaskOperation {
public:
	CEWSTaskOperationGsoapImpl(CEWSSessionGsoapImpl * pSession);
	virtual ~CEWSTaskOperationGsoapImpl();

public:
	virtual bool SyncTask(CEWSItemOperationCallback * callback,
	                      CEWSError * pError);
	virtual bool UpdateTask(CEWSTaskItem * pEwsItem,
	                        int flags,
	                        CEWSError * pError);
	virtual bool UpdateTaskOccurrence(CEWSTaskItem * pEwsItem,
	                                  int instanceIndex,
	                                  int flags,
	                                  CEWSError * pError);
private:
	bool __UpdateTask(CEWSTaskItem * pItem,
	                  int flags,
	                  ews2__ItemChangeType * ItemChange,
	                  CEWSError * pError);

	CEWSSessionGsoapImpl * m_pSession;
	CEWSItemOperationGsoapImpl m_ItemOp;
};

class DLL_LOCAL TaskItemTypeBuilder : public ItemTypeBuilder {
public:
	TaskItemTypeBuilder(soap * soap,
	                    const CEWSTaskItem * pEWSItem,
	                    CEWSObjectPool * pObjPool);
	virtual ~TaskItemTypeBuilder();

	virtual ews2__ItemType * Build();
	virtual ews2__ItemType * CreateItemType();
};

#include "libews_gsoap_defs.h"
}
#endif /* LIBEWS_GSOAP_H_ */

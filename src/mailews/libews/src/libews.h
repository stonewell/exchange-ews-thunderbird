/*
 * libews.h
 *
 *  Created on: Mar 28, 2014
 *      Author: stone
 */

#ifndef LIBEWS_H_
#define LIBEWS_H_

#include "libews_defs.h"

#ifdef __cplusplus

#include "utils/ews_string_stl.h"
#include "utils/ews_list_stl.h"
#include "libews_macros.h"

namespace ews {

class CEWSFolderOperation;
class CEWSItemOperation;
class CEWSAttachmentOperation;
class CEWSSubscriptionOperation;
class CEWSContactOperation;
class CEWSCalendarOperation;
class CEWSTaskOperation;

class DLL_PUBLIC CEWSError {
public:
	CEWSError();
	CEWSError(const CEWSString & msg);
	CEWSError(int errorCode, const CEWSString & msg);
	virtual ~CEWSError();

	const CEWSString & GetErrorMessage() const;
	void SetErrorMessage(const CEWSString & msg);

	int GetErrorCode() const;
	void SetErrorCode(int errorCode);
private:
	CEWSString m_ErrorMsg;
	int m_ErrorCode;
};

class DLL_PUBLIC CEWSOperatorProvider {
public:
	CEWSOperatorProvider();
	virtual ~CEWSOperatorProvider();

public:
	virtual CEWSFolderOperation * CreateFolderOperation() = 0;
	virtual CEWSItemOperation * CreateItemOperation() = 0;
	virtual CEWSAttachmentOperation * CreateAttachmentOperation() = 0;
	virtual CEWSSubscriptionOperation * CreateSubscriptionOperation() = 0;
	virtual CEWSContactOperation * CreateContactOperation() = 0;
	virtual CEWSCalendarOperation * CreateCalendarOperation() = 0;
	virtual CEWSTaskOperation * CreateTaskOperation() = 0;
};

struct DLL_PUBLIC EWSProxyInfo {
	enum EWSProxyTypeEnum {
		HTTP = EWS_PROXY_HTTP,
		SOCKS4 = EWS_PROXY_SOCKS4,
		SOCKS5 = EWS_PROXY_SOCKS5
	};

	CEWSString Host;
	CEWSString User;
	CEWSString Password;
	EWSProxyTypeEnum ProxyType;
	long Port;
};

struct DLL_PUBLIC EWSConnectionInfo {
	enum EWSAuthMethodEnum {
		Auth_NTLM = EWS_AUTH_NTLM,
		Auth_Basic = EWS_AUTH_BASIC,
	};

	CEWSString Endpoint;
	EWSAuthMethodEnum AuthMethod;
    CEWSString TimezoneId;
};

struct DLL_PUBLIC EWSAccountInfo {
	CEWSString UserName;
	CEWSString Password;
	CEWSString Domain;
	CEWSString EmailAddress;
};

class DLL_PUBLIC CEWSSession: public CEWSOperatorProvider {
public:
    
	static CEWSSession * CreateSession(EWSConnectionInfo * connInfo,
	                                   EWSAccountInfo * accountInfo,
	                                   EWSProxyInfo * proxyInfo,
	                                   CEWSError * pError);

	virtual void AutoDiscover(CEWSError * pError) = 0;

	virtual EWSConnectionInfo * GetConnectionInfo() = 0;
	virtual EWSAccountInfo * GetAccountInfo() = 0;
	virtual EWSProxyInfo * GetProxyInfo() = 0;
protected:
	CEWSSession();
public:
	virtual ~CEWSSession();
};

class DLL_PUBLIC CEWSFolder {
public:
	enum EWSFolderTypeEnum {
		Folder_Mail = EWS_FOLDER_MAIL,
		//Folder_Calendar = 2, Not Support
		//Folder_Contacts = 4,

		Folder_All_Types = EWS_FOLDER_ALL_TYPES, // | Folder_Calendar | FolderContacts
	};

	enum EWSDistinguishedFolderIdNameEnum {
		calendar = EWSDistinguishedFolderIdName_calendar,
		contacts = EWSDistinguishedFolderIdName_contacts,
		deleteditems = EWSDistinguishedFolderIdName_deleteditems,
		drafts = EWSDistinguishedFolderIdName_drafts,
		inbox = EWSDistinguishedFolderIdName_inbox,
		journal = EWSDistinguishedFolderIdName_journal,
		notes = EWSDistinguishedFolderIdName_notes,
		outbox = EWSDistinguishedFolderIdName_outbox,
		sentitems = EWSDistinguishedFolderIdName_sentitems,
		tasks = EWSDistinguishedFolderIdName_tasks,
		msgfolderroot = EWSDistinguishedFolderIdName_msgfolderroot,
		publicfoldersroot = EWSDistinguishedFolderIdName_publicfoldersroot,
		root = EWSDistinguishedFolderIdName_root,
		junkemail = EWSDistinguishedFolderIdName_junkemail,
		searchfolders = EWSDistinguishedFolderIdName_searchfolders,
		voicemail = EWSDistinguishedFolderIdName_voicemail
	};
public:
	static CEWSFolder * CreateInstance(EWSFolderTypeEnum type =
	                                   CEWSFolder::Folder_Mail,
	                                   const CEWSString & displayName = "");

	CEWSFolder();
	virtual ~CEWSFolder();

	virtual CEWSFolder * GetParentFolder() const = 0;
	virtual const CEWSString & GetParentFolderId() const = 0;
	virtual const CEWSString & GetDisplayName() const = 0;
	virtual const CEWSString & GetFolderId() const = 0;
	virtual EWSFolderTypeEnum GetFolderType() const = 0;
	virtual const CEWSString & GetChangeKey() const = 0;
	virtual int GetTotalCount() const = 0;
	virtual int GetUnreadCount() const = 0;
	virtual bool IsVisible() const = 0;

	virtual void SetDisplayName(const CEWSString & name) = 0;
	virtual void SetFolderId(const CEWSString & id) = 0;
	virtual void SetChangeKey(const CEWSString & changeKey) = 0;
	virtual void SetTotalCount(int totalCount) = 0;
	virtual void SetUnreadCount(int unreadCount) =0;
	virtual void SetVisible(bool v) = 0;
};

DEFINE_LIST(CEWSFolder);

class DLL_PUBLIC CEWSEmailAddress {
public:
	CEWSEmailAddress();
	virtual ~CEWSEmailAddress();

public:
	enum EWSMailboxTypeEnum {
		MailboxType_Mailbox = EWS_MAILBOXTYPE_MAILBOX,
		MailboxType_PublicDL = EWS_MAILBOXTYPE_PUBLICDL,
		MailboxType_PrivateDL = EWS_MAILBOXTYPE_PRIVATEDL,
		MailboxType_Contact = EWS_MAILBOXTYPE_CONTACT,
		MailboxType_PublicFolder = EWS_MAILBOXTYPE_PUBLICFOLDER,
	};
	    
public:
	DECLARE_PROPERTY(const CEWSString &, Name);
	DECLARE_PROPERTY(const CEWSString &, EmailAddress);
	DECLARE_PROPERTY(const CEWSString &, ItemId);
	DECLARE_PROPERTY(const CEWSString &, ChangeKey);
	DECLARE_PROPERTY(const CEWSString &, RoutingType);
	DECLARE_PROPERTY(enum EWSMailboxTypeEnum, MailboxType);
};

class DLL_PUBLIC CEWSRecipient : public virtual CEWSEmailAddress {
public:
	static CEWSRecipient * CreateInstance();

	CEWSRecipient();
	virtual ~CEWSRecipient();

public:
	virtual bool IsSame(CEWSRecipient * pRecipient) = 0;
};

DEFINE_LIST(CEWSRecipient);

class CEWSItem;
class DLL_PUBLIC CEWSAttachment {
public:
	enum EWSAttachmentType {
		Attachment_Item = EWS_Attachment_Item,
		Attachment_File = EWS_Attachment_File
	};

	static CEWSAttachment * CreateInstance(EWSAttachmentType type);

	CEWSAttachment();
	virtual ~CEWSAttachment();

public:
	virtual bool IsSame(CEWSAttachment * pAttachment) = 0;

public:
	DECLARE_PROPERTY(EWSAttachmentType, AttachmentType);
	DECLARE_PROPERTY(const CEWSString &, AttachmentId);
	DECLARE_PROPERTY(const CEWSString &, ContentId);
	DECLARE_PROPERTY(const CEWSString &, ContentLocation);
	DECLARE_PROPERTY(const CEWSString &, ContentType);
	DECLARE_PROPERTY(const CEWSString &, Name);
};

DEFINE_LIST(CEWSAttachment);

class DLL_PUBLIC CEWSFileAttachment: public virtual CEWSAttachment {
public:
	CEWSFileAttachment();
	virtual ~CEWSFileAttachment();
	static CEWSFileAttachment * CreateInstance();
public:
	DECLARE_PROPERTY(const CEWSString &, Content);
};

class DLL_PUBLIC CEWSInternetMessageHeader {
public:
	CEWSInternetMessageHeader();
	virtual ~CEWSInternetMessageHeader();

	static CEWSInternetMessageHeader * CreateInstance();
public:
	DECLARE_PROPERTY(const CEWSString &, Value);
	DECLARE_PROPERTY(const CEWSString &, HeaderName);
};

DEFINE_LIST(CEWSInternetMessageHeader);

class DLL_PUBLIC CEWSItem {
public:
	enum EWSItemType {
		Item_Message = EWS_Item_Message,
		Item_Contact = EWS_Item_Contact,
		Item_Calendar = EWS_Item_Calendar,
		Item_Task = EWS_Item_Task,
	};

	enum EWSBodyType {
		BODY_HTML = EWS_BODY_HTML, BODY_TEXT = EWS_BODY_TEXT
	};

	enum EWSSensitivityTypeEnum {
		Normal = EWS_Sensitivity_Normal,
		Personal = EWS_Sensitivity_Personal,
		Private = EWS_Sensitivity_Private,
		Confidential = EWS_Sensitivity_Confidential,
	};

	enum EWSImportanceTypeEnum {
		Low = EWS_Importance_Low,
		Norm = EWS_Importance_Normal,
		High = EWS_Importance_High,
	};
    
	static CEWSItem * CreateInstance(EWSItemType itemType);

	CEWSItem();
	virtual ~CEWSItem();
public:
	DECLARE_PROPERTY(EWSItemType, ItemType);

	DECLARE_PROPERTY(const CEWSString &, MimeContent);

	DECLARE_PROPERTY(const CEWSString &, ItemId);
	DECLARE_PROPERTY(const CEWSString &, ChangeKey);
    
	DECLARE_PROPERTY(const CEWSString &, ParentFolderId);
	DECLARE_PROPERTY(const CEWSString &, ParentFolderChangeKey);

	DECLARE_PROPERTY(const CEWSString &, ItemClass);
	DECLARE_PROPERTY(const CEWSString &, Subject);

	DECLARE_PROPERTY(enum EWSSensitivityTypeEnum, Sensitivity);

	DECLARE_PROPERTY(EWSBodyType, BodyType);
	DECLARE_PROPERTY(const CEWSString &, Body);

	DECLARE_PROPERTY(CEWSAttachmentList *, Attachments);
	DECLARE_PROPERTY_B_2(bool , Attachments);

	DECLARE_PROPERTY(time_t, ReceivedTime);
	DECLARE_PROPERTY(int, Size);

	DECLARE_PROPERTY(CEWSStringList *, Categories);
	DECLARE_PROPERTY(enum EWSImportanceTypeEnum, Importance);
    
	DECLARE_PROPERTY(const CEWSString &, InReplyTo);

	DECLARE_PROPERTY_B(bool, Submitted);
	DECLARE_PROPERTY_B(bool, Draft);
	DECLARE_PROPERTY_B(bool, FromMe);
	DECLARE_PROPERTY_B(bool, Resend);
	DECLARE_PROPERTY_B(bool, Unmodified);

	DECLARE_PROPERTY(CEWSInternetMessageHeaderList *, InternetMessageHeaders);
    
	DECLARE_PROPERTY(time_t, SentTime);
	DECLARE_PROPERTY(time_t, CreateTime);

	DECLARE_PROPERTY(time_t, ReminderDueBy);
	DECLARE_PROPERTY(bool, ReminderIsSet);
	DECLARE_PROPERTY(const CEWSString &, ReminderMinutesBeforeStart);
    
	DECLARE_PROPERTY(const CEWSString &, DisplayCc);
	DECLARE_PROPERTY(const CEWSString &, DisplayTo);

	DECLARE_PROPERTY(const CEWSString &, LastModifiedName);
	DECLARE_PROPERTY(time_t, LastModifiedTime);
};

DEFINE_LIST(CEWSItem);

class DLL_PUBLIC CEWSMessageItem: public virtual CEWSItem {
public:
	CEWSMessageItem();
	virtual ~CEWSMessageItem();
public:
	static CEWSMessageItem * CreateInstance();

	DECLARE_PROPERTY_GETTER(CEWSRecipient *,Sender);
	DECLARE_PROPERTY(CEWSRecipientList *,ToRecipients);
	DECLARE_PROPERTY(CEWSRecipientList *,CcRecipients);
	DECLARE_PROPERTY(CEWSRecipientList *,BccRecipients);
	DECLARE_PROPERTY_B(bool ,ReadReceiptRequested);
	DECLARE_PROPERTY_B(bool ,DeliveryReceiptRequested);
	DECLARE_PROPERTY(const CEWSString &,ConversationIndex);
	DECLARE_PROPERTY(const CEWSString &,ConversationTopic);
	DECLARE_PROPERTY_GETTER(CEWSRecipient *,From);
	DECLARE_PROPERTY(const CEWSString &,InternetMessageId);
	DECLARE_PROPERTY_B(bool ,Read);
	DECLARE_PROPERTY_B(bool ,ResponseRequested);
	DECLARE_PROPERTY(const CEWSString &,References);
	DECLARE_PROPERTY(CEWSRecipientList *,ReplyTo);
	DECLARE_PROPERTY_GETTER(CEWSRecipient *,ReceivedBy);
	DECLARE_PROPERTY_GETTER(CEWSRecipient *,ReceivedRepresenting);
};

class DLL_PUBLIC CEWSPhysicalAddress {
public:
	CEWSPhysicalAddress();
	virtual ~CEWSPhysicalAddress();

	static CEWSPhysicalAddress * CreateInstance();
    
	DECLARE_PROPERTY(const CEWSString &, Street);
	DECLARE_PROPERTY(const CEWSString &, City);
	DECLARE_PROPERTY(const CEWSString &, State);
	DECLARE_PROPERTY(const CEWSString &, CountryOrRegion);
	DECLARE_PROPERTY(const CEWSString &, PostalCode);
};

DEFINE_LIST(CEWSPhysicalAddress)

class DLL_PUBLIC CEWSCompleteName {
public:
	CEWSCompleteName();
	virtual ~CEWSCompleteName();
	
	DECLARE_PROPERTY(const CEWSString &, Title);
	DECLARE_PROPERTY(const CEWSString &, FirstName);
	DECLARE_PROPERTY(const CEWSString &, MiddleName);
	DECLARE_PROPERTY(const CEWSString &, LastName);
	DECLARE_PROPERTY(const CEWSString &, Suffix);
	DECLARE_PROPERTY(const CEWSString &, Initials);
	DECLARE_PROPERTY(const CEWSString &, FullName);
	DECLARE_PROPERTY(const CEWSString &, Nickname);
	DECLARE_PROPERTY(const CEWSString &, YomiFirstName);
	DECLARE_PROPERTY(const CEWSString &, YomiLastName);
};

class DLL_PUBLIC CEWSContactItem: public virtual CEWSItem {
public:
	CEWSContactItem();
	virtual ~CEWSContactItem();
public:
	enum EWSContactSourceEnum {
		ContactSource_ActiveDirectory = EWS_CONTACT_SOURCE_ACTIVE_DIRECTORY,
		ContactSource_Store = EWS_CONTACT_SOURCE_STORE,
	};

	static CEWSContactItem * CreateInstance();

	DECLARE_PROPERTY(const CEWSString &, Name);
	DECLARE_PROPERTY(const CEWSString &, EmailAddress);
	DECLARE_PROPERTY(const CEWSString &, RoutingType);
	DECLARE_PROPERTY(CEWSEmailAddress::EWSMailboxTypeEnum, MailboxType);
	DECLARE_PROPERTY(const CEWSString &, DisplayName);
	DECLARE_PROPERTY(const CEWSString &, GivenName);
	DECLARE_PROPERTY(const CEWSString &, Initials);
	DECLARE_PROPERTY(const CEWSString &, MiddleName);
	DECLARE_PROPERTY(const CEWSString &, Nickname);
	DECLARE_PROPERTY(CEWSCompleteName *, CompleteName);
	DECLARE_PROPERTY(const CEWSString &, CompanyName);
	DECLARE_PROPERTY(CEWSStringList *, EmailAddresses);
	DECLARE_PROPERTY(CEWSPhysicalAddressList *, PhysicalAddresses);
	DECLARE_PROPERTY(CEWSStringList *, PhoneNumbers);
	DECLARE_PROPERTY(const CEWSString &, AssistantName);
	DECLARE_PROPERTY(time_t, Birthday);
	DECLARE_PROPERTY(const CEWSString &, BusinessHomePage);
	DECLARE_PROPERTY(CEWSStringList *, Children);
	DECLARE_PROPERTY(CEWSStringList *, Companies);
	DECLARE_PROPERTY(EWSContactSourceEnum, ContactSource);
	DECLARE_PROPERTY(const CEWSString &, Department);
	DECLARE_PROPERTY(const CEWSString &, Generation);
	DECLARE_PROPERTY(CEWSStringList *, ImAddresses);
	DECLARE_PROPERTY(const CEWSString &, JobTitle);
	DECLARE_PROPERTY(const CEWSString &, Manager);
	DECLARE_PROPERTY(const CEWSString &, Mileage);
	DECLARE_PROPERTY(const CEWSString &, OfficeLocation);
	DECLARE_PROPERTY(CEWSPhysicalAddress *, PostalAddress);
	DECLARE_PROPERTY(const CEWSString &, Profession);
	DECLARE_PROPERTY(const CEWSString &, SpouseName);
	DECLARE_PROPERTY(const CEWSString &, Surname);
	DECLARE_PROPERTY(time_t, WeddingAnniversary);
	DECLARE_PROPERTY_B(bool, MailList);
};
	
class DLL_PUBLIC CEWSContactOperation {
public:
	CEWSContactOperation();
	virtual ~CEWSContactOperation();

public:
	virtual CEWSItemList * ResolveNames(const CEWSString & unresolvedEntry, CEWSError * pError) = 0;
};

class DLL_PUBLIC CEWSRecurrenceRange {
public:
	CEWSRecurrenceRange();
	virtual ~CEWSRecurrenceRange();

public:
	enum EWSRecurrenceRangeTypeEnum {
		NoEnd = EWSRecurrenceRangeType_NoEnd,
		EndDate = EWSRecurrenceRangeType_EndDate,
		Numbered = EWSRecurrenceRangeType_Numbered,
	};
    
	static CEWSRecurrenceRange * CreateInstance(EWSRecurrenceRangeTypeEnum rangeType);
public:
	DECLARE_PROPERTY(enum EWSRecurrenceRangeTypeEnum, RecurrenceRangeType);
	DECLARE_PROPERTY(const CEWSString &, StartDate);
};

class DLL_PUBLIC CEWSNoEndRecurrenceRange : public virtual CEWSRecurrenceRange {
public:
	CEWSNoEndRecurrenceRange();
	virtual ~CEWSNoEndRecurrenceRange();

public:
};

class DLL_PUBLIC CEWSEndDateRecurrenceRange : public virtual CEWSRecurrenceRange {
public:
	CEWSEndDateRecurrenceRange();
	virtual ~CEWSEndDateRecurrenceRange();

public:
	DECLARE_PROPERTY(const CEWSString &, EndDate);
};

class DLL_PUBLIC CEWSNumberedRecurrenceRange : public virtual CEWSRecurrenceRange {
public:
	CEWSNumberedRecurrenceRange();
	virtual ~CEWSNumberedRecurrenceRange();

public:
	DECLARE_PROPERTY(int, NumberOfOccurrences);
};

class DLL_PUBLIC CEWSRecurrencePattern {
public:
	CEWSRecurrencePattern();
	virtual ~CEWSRecurrencePattern();

public:
	enum EWSRecurrenceTypeEnum {
		RelativeYearly = EWS_Recurrence_RelativeYearly,
		AbsoluteYearly = EWS_Recurrence_AbsoluteYearly,
		RelativeMonthly = EWS_Recurrence_RelativeMonthly,
		AbsoluteMonthly = EWS_Recurrence_AbsoluteMonthly,
		Weekly = EWS_Recurrence_Weekly,
		Daily = EWS_Recurrence_Daily,
	};

	static CEWSRecurrencePattern * CreateInstance(EWSRecurrenceTypeEnum patternType);

	enum EWSDayOfWeekEnum {
		Monday = EWS_DayOfWeek_Monday,
		Tuesday = EWS_DayOfWeek_Tuesday,
		Wednesday = EWS_DayOfWeek_Wednesday,
		Thursday = EWS_DayOfWeek_Thursday,
		Friday = EWS_DayOfWeek_Friday,
		Saturday = EWS_DayOfWeek_Saturday,
		Sunday = EWS_DayOfWeek_Sunday,
		Day = EWS_DayOfWeek_Day,
		Weekday = EWS_DayOfWeek_Weekday,
		WeekendDay = EWS_DayOfWeek_WeekendDay,
	};
	enum EWSDayOfWeekIndexEnum {
		First = EWS_DayOfWeekIndex_First,
		Second = EWS_DayOfWeekIndex_Second,
		Third = EWS_DayOfWeekIndex_Third,
		Fourth = EWS_DayOfWeekIndex_Fourth,
		Last = EWS_DayOfWeekIndex_Last,
	};
	enum EWSMonthNamesEnum {
		January = EWS_MonthNames_January,
		February = EWS_MonthNames_February,
		March = EWS_MonthNames_March,
		April = EWS_MonthNames_April,
		May = EWS_MonthNames_May,
		June = EWS_MonthNames_June,
		July = EWS_MonthNames_July,
		August = EWS_MonthNames_August,
		September = EWS_MonthNames_September,
		October = EWS_MonthNames_October,
		November = EWS_MonthNames_November,
		December = EWS_MonthNames_December,
	};
public:
	DECLARE_PROPERTY(enum EWSRecurrenceTypeEnum, RecurrenceType);
};

class DLL_PUBLIC CEWSRecurrencePatternInterval : public virtual CEWSRecurrencePattern {
public:
	CEWSRecurrencePatternInterval();
	virtual ~CEWSRecurrencePatternInterval();
public:
	DECLARE_PROPERTY(int, Interval);
};

class DLL_PUBLIC CEWSRecurrencePatternRelativeYearly : public virtual CEWSRecurrencePattern {
public:
	CEWSRecurrencePatternRelativeYearly();
	virtual ~CEWSRecurrencePatternRelativeYearly();
public:
	DECLARE_PROPERTY(enum CEWSRecurrencePattern::EWSDayOfWeekEnum, DaysOfWeek);
	DECLARE_PROPERTY(enum CEWSRecurrencePattern::EWSDayOfWeekIndexEnum, DayOfWeekIndex);
	DECLARE_PROPERTY(enum CEWSRecurrencePattern::EWSMonthNamesEnum, Month);
};

class DLL_PUBLIC CEWSRecurrencePatternAbsoluteYearly : public virtual CEWSRecurrencePattern {
public:
	CEWSRecurrencePatternAbsoluteYearly();
	virtual ~CEWSRecurrencePatternAbsoluteYearly();
public:
	DECLARE_PROPERTY(int, DayOfMonth);
	DECLARE_PROPERTY(enum CEWSRecurrencePattern::EWSMonthNamesEnum, Month);
};

class DLL_PUBLIC CEWSRecurrencePatternRelativeMonthly : public virtual CEWSRecurrencePatternInterval {
public:
	CEWSRecurrencePatternRelativeMonthly();
	virtual ~CEWSRecurrencePatternRelativeMonthly();
public:
	DECLARE_PROPERTY(enum CEWSRecurrencePattern::EWSDayOfWeekEnum, DaysOfWeek);
	DECLARE_PROPERTY(enum CEWSRecurrencePattern::EWSDayOfWeekIndexEnum, DayOfWeekIndex);
};

class DLL_PUBLIC CEWSRecurrencePatternAbsoluteMonthly : public virtual CEWSRecurrencePatternInterval {
public:
	CEWSRecurrencePatternAbsoluteMonthly();
	virtual ~CEWSRecurrencePatternAbsoluteMonthly();
public:
	DECLARE_PROPERTY(int, DayOfMonth);
};

class DLL_PUBLIC CEWSRecurrencePatternWeekly : public virtual CEWSRecurrencePatternInterval {
public:
	CEWSRecurrencePatternWeekly();
	virtual ~CEWSRecurrencePatternWeekly();
public:
	DECLARE_PROPERTY(enum CEWSRecurrencePattern::EWSDayOfWeekEnum, DaysOfWeek);
};

class DLL_PUBLIC CEWSRecurrencePatternDaily : public virtual CEWSRecurrencePatternInterval {
public:
	CEWSRecurrencePatternDaily();
	virtual ~CEWSRecurrencePatternDaily();
};

class DLL_PUBLIC CEWSRecurrence {
public:
	CEWSRecurrence();
	virtual ~CEWSRecurrence();

	static CEWSRecurrence * CreateInstance();
public:
	DECLARE_PROPERTY(CEWSRecurrenceRange *, RecurrenceRange);
	DECLARE_PROPERTY(CEWSRecurrencePattern *, RecurrencePattern);
};

class DLL_PUBLIC CEWSOccurrenceInfo {
public:
	CEWSOccurrenceInfo();
	virtual ~CEWSOccurrenceInfo();

public:
	static CEWSOccurrenceInfo * CreateInstance();
    
	DECLARE_PROPERTY(const CEWSString &, ItemId);
	DECLARE_PROPERTY(const CEWSString &, ChangeKey);
	DECLARE_PROPERTY(time_t, Start);
	DECLARE_PROPERTY(time_t, End);
	DECLARE_PROPERTY(time_t, OriginalStart);
};

DEFINE_LIST(CEWSOccurrenceInfo);

class DLL_PUBLIC CEWSDeletedOccurrenceInfo {
public:
	CEWSDeletedOccurrenceInfo();
	virtual ~CEWSDeletedOccurrenceInfo();

public:
	static CEWSDeletedOccurrenceInfo * CreateInstance();

	DECLARE_PROPERTY(time_t, Start);
};

DEFINE_LIST(CEWSDeletedOccurrenceInfo);

class DLL_PUBLIC CEWSAttendee : public virtual CEWSEmailAddress {
public:
	CEWSAttendee();
	virtual ~CEWSAttendee();
public:
	static CEWSAttendee * CreateInstance();
    
	enum EWSResponseTypeEnum {
		Unknown = EWS_ResponseType_Unknown,
		Organizer = EWS_ResponseType_Organizer,
		Tentative = EWS_ResponseType_Tentative,
		Accept = EWS_ResponseType_Accept,
		Decline = EWS_ResponseType_Decline,
		NoResponseReceived = EWS_ResponseType_NoResponseReceived,
	};

public:
	DECLARE_PROPERTY(enum EWSResponseTypeEnum, ResponseType);
	DECLARE_PROPERTY(time_t, LastResponseTime);
};

DEFINE_LIST(CEWSAttendee);

class DLL_PUBLIC CEWSTimeChangeSequence {
public:
	CEWSTimeChangeSequence();
	virtual ~CEWSTimeChangeSequence();

public:
	enum EWSTimeChangeSequenceTypeEnum {
		RelativeYearly = EWS_TimeChange_Sequence_RelativeYearly,
		AbsoluteDate = EWS_TimeChange_Sequence_AbsoluteDate,
	};

	static CEWSTimeChangeSequence * CreateInstance(EWSTimeChangeSequenceTypeEnum sequenceType);

public:
	DECLARE_PROPERTY(enum EWSTimeChangeSequenceTypeEnum, TimeChangeSequenceType);
};

class DLL_PUBLIC CEWSTimeChangeSequenceRelativeYearly : public virtual CEWSTimeChangeSequence {
public:
	CEWSTimeChangeSequenceRelativeYearly();
	virtual ~CEWSTimeChangeSequenceRelativeYearly();

public:
	DECLARE_PROPERTY(CEWSRecurrencePatternRelativeYearly *, RelativeYearly);
};

class DLL_PUBLIC CEWSTimeChangeSequenceAbsoluteDate : public virtual CEWSTimeChangeSequence {
public:
	CEWSTimeChangeSequenceAbsoluteDate();
	virtual ~CEWSTimeChangeSequenceAbsoluteDate();

public:
	DECLARE_PROPERTY(const CEWSString &, AbsoluteDate);
};

class DLL_PUBLIC CEWSTimeChange {
public:
	CEWSTimeChange();
	virtual ~CEWSTimeChange();

	static CEWSTimeChange * CreateInstance();
public:
	DECLARE_PROPERTY(long long, Offset);
	DECLARE_PROPERTY(CEWSTimeChangeSequence *, TimeChangeSequence);
	DECLARE_PROPERTY(const CEWSString &, Time);
	DECLARE_PROPERTY(const CEWSString &, TimeZoneName);
};

class DLL_PUBLIC CEWSTimeZone {
public:
	CEWSTimeZone();
	virtual ~CEWSTimeZone();

	static CEWSTimeZone * CreateInstance();
public:
	DECLARE_PROPERTY(const CEWSString &, TimeZoneName);
	DECLARE_PROPERTY(long long, BaseOffset);
	DECLARE_PROPERTY(CEWSTimeChange *, Standard);
	DECLARE_PROPERTY(CEWSTimeChange *, Daylight);
};

class DLL_PUBLIC CEWSCalendarItem : public virtual CEWSItem {
public:
	CEWSCalendarItem();
	virtual ~CEWSCalendarItem();

public:
	enum EWSLegacyFreeBusyTypeEnum {
		Free = EWS_LegacyFreeBusyType_Free,
		Tentative = EWS_LegacyFreeBusyType_Tentative,
		Busy = EWS_LegacyFreeBusyType_Busy,
		OOF = EWS_LegacyFreeBusyType_OOF,
		NoData = EWS_LegacyFreeBusyType_NoData,
	};

	enum EWSCalendarItemTypeEnum {
		Single = EWS_CalendarItemType_Single,
		Occurrence = EWS_CalendarItemType_Occurrence,
		Exception = EWS_CalendarItemType_Exception,
		RecurringMaster = EWS_CalendarItemType_RecurringMaster,
	};

public:
	DECLARE_PROPERTY(const CEWSString &, UID);
	DECLARE_PROPERTY(time_t, RecurrenceId);
	DECLARE_PROPERTY(time_t, DateTimeStamp);
	DECLARE_PROPERTY(time_t, Start);
	DECLARE_PROPERTY(time_t, End);
	DECLARE_PROPERTY(time_t, OriginalStart);
	DECLARE_PROPERTY_B(bool, AllDayEvent);
	DECLARE_PROPERTY(enum EWSLegacyFreeBusyTypeEnum, LegacyFreeBusyStatus);
	DECLARE_PROPERTY(const CEWSString &, Location);
	DECLARE_PROPERTY(const CEWSString &, When);
	DECLARE_PROPERTY_B(bool, Meeting);
	DECLARE_PROPERTY_B(bool, Cancelled);
	DECLARE_PROPERTY_B(bool, Recurring);
	DECLARE_PROPERTY(bool, MeetingRequestWasSent);
	DECLARE_PROPERTY_B(bool, ResponseRequested);
	DECLARE_PROPERTY(enum EWSCalendarItemTypeEnum, CalendarItemType);
	DECLARE_PROPERTY(enum CEWSAttendee::EWSResponseTypeEnum, MyResponseType);
	DECLARE_PROPERTY(CEWSRecipient *, Organizer);
	DECLARE_PROPERTY(CEWSAttendeeList *, RequiredAttendees);
	DECLARE_PROPERTY(CEWSAttendeeList *, OptionalAttendees);
	DECLARE_PROPERTY(CEWSAttendeeList *, Resources);
	DECLARE_PROPERTY(int, ConflictingMeetingCount);
	DECLARE_PROPERTY(int, AdjacentMeetingCount);
	DECLARE_PROPERTY(CEWSItemList *, ConflictingMeetings);
	DECLARE_PROPERTY(CEWSItemList *, AdjacentMeetings);
	DECLARE_PROPERTY(const CEWSString &, Duration);
	DECLARE_PROPERTY(const CEWSString &, TimeZone);
	DECLARE_PROPERTY(time_t, AppointmentReplyTime);
	DECLARE_PROPERTY(int, AppointmentSequenceNumber);
	DECLARE_PROPERTY(int, AppointmentState);
	DECLARE_PROPERTY(CEWSRecurrence *, Recurrence);
	DECLARE_PROPERTY(CEWSOccurrenceInfo *, FirstOccurrence);
	DECLARE_PROPERTY(CEWSOccurrenceInfo *, LastOccurrence);
	DECLARE_PROPERTY(CEWSOccurrenceInfoList *, ModifiedOccurrences);
	DECLARE_PROPERTY(CEWSDeletedOccurrenceInfoList *, DeletedOccurrences);
	DECLARE_PROPERTY(CEWSTimeZone *, MeetingTimeZone);
	DECLARE_PROPERTY(int, ConferenceType);
	DECLARE_PROPERTY_B(bool, AllowNewTimeProposal);
	DECLARE_PROPERTY_B(bool, OnlineMeeting);
	DECLARE_PROPERTY(const CEWSString &, MeetingWorkspaceUrl);
	DECLARE_PROPERTY(const CEWSString &, NetShowUrl);
    
};

class CEWSItemOperationCallback;
class DLL_PUBLIC CEWSCalendarOperation {
public:
	CEWSCalendarOperation();
	virtual ~CEWSCalendarOperation();

public:
	enum UpdateFlags {
		Body = EWS_UpdateCalendar_Flags_Body,
		Start = EWS_UpdateCalendar_Flags_Start,
		End = EWS_UpdateCalendar_Flags_End,
		RequiredAttendee = EWS_UpdateCalendar_Flags_Required_Attendee,
		OptionalAttendee = EWS_UpdateCalendar_Flags_Optional_Attendee,
		Location = EWS_UpdateCalendar_Flags_Location,
		Subject = EWS_UpdateCalendar_Flags_Subject,
	};
	virtual bool SyncCalendar(CEWSItemOperationCallback * callback,
	                          CEWSError * pError) = 0;
	virtual bool GetRecurrenceMasterItemId(const CEWSString & itemId,
	                                       CEWSString & masterItemId,
	                                       CEWSString & masterUId,
	                                       CEWSError * pError) = 0;
	virtual bool AcceptEvent(CEWSCalendarItem * pEwsItem,
	                         CEWSError * pError) = 0;
	virtual bool TentativelyAcceptEvent(CEWSCalendarItem * pEwsItem,
	                                    CEWSError * pError) = 0;
	virtual bool DeclineEvent(CEWSCalendarItem * pEwsItem,
	                          CEWSError * pError) = 0;
	virtual bool DeleteOccurrence(const CEWSString & masterItemId,
	                              int occurrenceInstance,
	                              CEWSError * pError) = 0;
	virtual bool UpdateEvent(CEWSCalendarItem * pEwsItem,
	                         int flags,
	                         CEWSError * pError) = 0;
	virtual bool UpdateEventOccurrence(CEWSCalendarItem * pEwsItem,
	                                   int instanceIndex,
	                                   int flags,
	                                   CEWSError * pError) = 0;
};

class DLL_PUBLIC CEWSTaskItem : public virtual CEWSItem {
public:
	CEWSTaskItem();
	virtual ~CEWSTaskItem();
public:
	enum DelegateStateTypeEnum {
		NoMatch = EWS__TaskDelegateStateType__NoMatch,
		OwnNew = EWS__TaskDelegateStateType__OwnNew,
		Owned = EWS__TaskDelegateStateType__Owned,
		Accepted = EWS__TaskDelegateStateType__Accepted,
		Declined = EWS__TaskDelegateStateType__Declined,
		Max = EWS__TaskDelegateStateType__Max,
	};

	enum StatusTypeEnum {
		NotStarted = EWS__TaskStatusType__NotStarted,
		InProgress = EWS__TaskStatusType__InProgress,
		Completed = EWS__TaskStatusType__Completed,
		WaitingOnOthers = EWS__TaskStatusType__WaitingOnOthers,
		Deferred = EWS__TaskStatusType__Deferred,
	};
    
	DECLARE_PROPERTY(int, ActualWork);
	DECLARE_PROPERTY(time_t, AssignedTime);
	DECLARE_PROPERTY(const CEWSString &, BillingInformation);
	DECLARE_PROPERTY(int, ChangeCount);
	DECLARE_PROPERTY(CEWSStringList *, Companies);
	DECLARE_PROPERTY(time_t, CompleteDate);
	DECLARE_PROPERTY(CEWSStringList *, Contacts);
	DECLARE_PROPERTY(enum DelegateStateTypeEnum, DelegationState);
	DECLARE_PROPERTY(const CEWSString &, Delegator);
	DECLARE_PROPERTY(time_t, DueDate);
	DECLARE_PROPERTY(int, IsAssignmentEditable);
	DECLARE_PROPERTY_B(bool, Complete);
	DECLARE_PROPERTY_B(bool, Recurring);
	DECLARE_PROPERTY_B(bool, TeamTask);
	DECLARE_PROPERTY(const CEWSString &, Mileage);
	DECLARE_PROPERTY(const CEWSString &, Owner);
	DECLARE_PROPERTY(double, PercentComplete);
	DECLARE_PROPERTY(CEWSRecurrence *, Recurrence);
	DECLARE_PROPERTY(time_t, StartDate);
	DECLARE_PROPERTY(enum StatusTypeEnum, Status);
	DECLARE_PROPERTY(const CEWSString &, StatusDescription);
	DECLARE_PROPERTY(int, TotalWork);
};

class DLL_PUBLIC CEWSTaskOperation {
public:
	CEWSTaskOperation();
	virtual ~CEWSTaskOperation();

public:
	enum UpdateFlags {
		Body = EWS_UpdateCalendar_Flags_Body,
		Start = EWS_UpdateCalendar_Flags_Start,
		End = EWS_UpdateCalendar_Flags_End,
		Subject = EWS_UpdateCalendar_Flags_Subject,
		Status = EWS_UpdateCalendar_Flags_Status,
		Percent = EWS_UpdateCalendar_Flags_Percent,
		Complete = EWS_UpdateCalendar_Flags_Complete,
	};

	virtual bool SyncTask(CEWSItemOperationCallback * callback,
	                      CEWSError * pError) = 0;
	virtual bool UpdateTask(CEWSTaskItem * pEwsItem,
	                        int flags,
	                        CEWSError * pError) = 0;
	virtual bool UpdateTaskOccurrence(CEWSTaskItem * pEwsItem,
	                                  int instanceIndex,
	                                  int flags,
	                                  CEWSError * pError) = 0;
};

class DLL_PUBLIC CEWSItemAttachment: public virtual CEWSAttachment {
public:
	static CEWSItemAttachment * CreateInstance();

	CEWSItemAttachment();
	virtual ~CEWSItemAttachment();
public:
	DECLARE_PROPERTY(const CEWSString &, ItemId);
	DECLARE_PROPERTY(CEWSItem::EWSItemType, ItemType);
};

class DLL_PUBLIC CEWSFolderOperationCallback {
public:
	CEWSFolderOperationCallback();
	virtual ~CEWSFolderOperationCallback();

public:
	virtual const CEWSString & GetSyncState() const = 0;
	virtual void SetSyncState(const CEWSString & syncState) = 0;

	virtual CEWSFolder::EWSFolderTypeEnum GetFolderType() const = 0;

	virtual void NewFolder(const CEWSFolder * pFolder) = 0;
	virtual void UpdateFolder(const CEWSFolder * pFolder) = 0;
	virtual void DeleteFolder(const CEWSFolder * pFolder) = 0;
};

class DLL_PUBLIC CEWSFolderOperation {
public:
	CEWSFolderOperation();
	virtual ~CEWSFolderOperation();

public:
	virtual CEWSFolderList * List(CEWSFolder::EWSFolderTypeEnum type,
	                              const CEWSFolder * pParentFolder,
	                              CEWSError * pError) = 0;
	virtual CEWSFolderList * List(CEWSFolder::EWSFolderTypeEnum type,
	                              CEWSFolder::EWSDistinguishedFolderIdNameEnum distinguishedFolderId,
	                              CEWSError * pError) = 0;
	virtual bool SyncFolders(CEWSFolderOperationCallback * pFolderCallback,
	                         CEWSError * pError) = 0;
	virtual bool Delete(CEWSFolder * pFolder, bool hardDelete, CEWSError * pError) = 0;
	virtual bool Update(CEWSFolder * pFolder, CEWSError * pError) = 0;
	virtual bool Create(CEWSFolder * pFolder,
	                    const CEWSFolder * pParentFolder,
	                    CEWSError * pError) = 0;
	virtual CEWSFolder * GetFolder(CEWSFolder::EWSDistinguishedFolderIdNameEnum distinguishedFolderId,
	                               CEWSError * pError) = 0;
	virtual CEWSFolder * GetFolder(const CEWSString & folderId,
	                               CEWSError * pError) = 0;
	virtual CEWSFolderList * GetFolders(int * distinguishedFolderId,
	                                    int count,
	                                    CEWSError * pError) = 0;
	virtual CEWSFolderList * GetFolders(const CEWSStringList & folderId,
	                                    CEWSError * pError) = 0;
};

class DLL_PUBLIC CEWSItemOperationCallback {
public:
	CEWSItemOperationCallback();
	virtual ~CEWSItemOperationCallback();
public:
	virtual const CEWSString & GetSyncState() const = 0;
	virtual int GetSyncMaxReturnItemCount() const = 0;
	virtual const CEWSStringList & GetIgnoreItemIdList() const = 0;

	virtual void SetSyncState(const CEWSString & syncState) = 0;
	virtual void NewItem(const CEWSItem * pItem) = 0;
	virtual void UpdateItem(const CEWSItem * pItem) = 0;
	virtual void DeleteItem(const CEWSItem * pItem) = 0;
	virtual void ReadItem(const CEWSItem * pItem, bool bRead) = 0;
};

class DLL_PUBLIC CEWSItemOperation {
public:
	CEWSItemOperation();
	virtual ~CEWSItemOperation();

	enum EWSGetItemFlags {
		None = EWS_GetItems_Flags_None,
		MimeContent = EWS_GetItems_Flags_MimeContent,
		MessageItem = EWS_GetItems_Flags_MessageItem,
		ContactItem = EWS_GetItems_Flags_ContactItem,
		CalendarItem = EWS_GetItems_Flags_CalendarItem,
		IdOnly = EWS_GetItems_Flags_IdOnly,
		AllProperties = EWS_GetItems_Flags_AllProperties,
		RecurringMasterItem = EWS_GetItems_Flags_RecurringMasterItem,
		OccurrenceItem = EWS_GetItems_Flags_OccurrenceItem,
		TaskItem = EWS_GetItems_Flags_TaskItem,
	};
public:
	virtual bool SyncItems(const CEWSString & folderId,
	                       CEWSItemOperationCallback * callback,
	                       CEWSError * pError) = 0;
	virtual bool SyncItems(const CEWSFolder * pFolder,
	                       CEWSItemOperationCallback * callback,
	                       CEWSError * pError) = 0;
	virtual bool SyncItems(CEWSFolder::EWSDistinguishedFolderIdNameEnum distinguishedFolderId,
	                       CEWSItemOperationCallback * callback,
	                       CEWSError * pError) = 0;
	virtual bool GetUnreadItems(const CEWSString & folderId,
	                            CEWSItemOperationCallback * callback,
	                            CEWSError * pError) = 0;
	virtual CEWSItem * GetItem(const CEWSString & itemId,
	                           int flags,
	                           CEWSError * pError) = 0;
	virtual bool GetItems(const CEWSStringList & itemIds,
	                      CEWSItemList * pItemList,
	                      int flags,
	                      CEWSError * pError) = 0;
	virtual bool CreateItem(CEWSItem * pItem,
	                        const CEWSFolder * pSaveFolder,
	                        CEWSError * pError) = 0;
	virtual bool CreateItem(CEWSItem * pItem,
	                        CEWSFolder::EWSDistinguishedFolderIdNameEnum distinguishedFolderId,
	                        CEWSError * pError) = 0;
	virtual bool SendItem(CEWSMessageItem * pMessageItem,
	                      CEWSError * pError) = 0;
	virtual bool DeleteItem(CEWSItem * pItem,
	                        bool moveToDeleted,
	                        CEWSError * pError) = 0;
	virtual bool DeleteItems(CEWSItemList * pItemList,
	                         bool moveToDeleted,
	                         CEWSError * pError) = 0;
	virtual bool MarkItemsRead(CEWSItemList * pItemList,
	                           bool read,
	                           CEWSError * pError) = 0;
};

class DLL_PUBLIC CEWSAttachmentOperation {
public:
	CEWSAttachmentOperation();
	virtual ~CEWSAttachmentOperation();
public:
	virtual CEWSAttachment * GetAttachment(const CEWSString & attachmentId,
	                                       CEWSError * pError) = 0;
	virtual bool CreateAttachment(CEWSAttachment * pAttachment,
	                              CEWSItem * parentItem,
	                              CEWSError * pError) = 0;
	virtual bool CreateAttachments(
	    const CEWSAttachmentList * pAttachmentList,
	    CEWSItem * parentItem,
	    CEWSError * pError) = 0;
};

class DLL_PUBLIC CEWSSubscription {
public:
	CEWSSubscription();
	virtual ~CEWSSubscription();

	enum EWSNotifyEventTypeEnum {
		COPY = EWS_NOTIFY_EVENT_TYPE_COPY,
		CREATE = EWS_NOTIFY_EVENT_TYPE_CREATE,
		DEL = EWS_NOTIFY_EVENT_TYPE_DELETE,
		MODIFY = EWS_NOTIFY_EVENT_TYPE_MODIFY,
		MOVE = EWS_NOTIFY_EVENT_TYPE_MOVE,
		NEW_MAIL = EWS_NOTIFY_EVENT_TYPE_NEWMAIL,
	};

public:
	DECLARE_PROPERTY(const CEWSString &, SubscriptionId);
	DECLARE_PROPERTY(const CEWSString &, WaterMark);
};

class DLL_PUBLIC CEWSSubscriptionCallback {
public:
	CEWSSubscriptionCallback();
	virtual ~CEWSSubscriptionCallback();

public:
	virtual void NewMail(const CEWSString & parentFolderId,
	                     const CEWSString & itemId) = 0;

	virtual void MoveItem(const CEWSString & oldParentFolderId,
	                      const CEWSString & parentFolderId,
	                      const CEWSString & oldItemId,
	                      const CEWSString & itemId) = 0;
	virtual void ModifyItem(const CEWSString & parentFolderId,
	                        const CEWSString & itemId,
	                        int unreadCount) = 0;
	virtual void CopyItem(const CEWSString & oldParentFolderId,
	                      const CEWSString & parentFolderId,
	                      const CEWSString & oldItemId,
	                      const CEWSString & itemId) = 0;
	virtual void DeleteItem(const CEWSString & parentFolderId,
	                        const CEWSString & itemId) = 0;
	virtual void CreateItem(const CEWSString & parentFolderId,
	                        const CEWSString & itemId) = 0;

	virtual void MoveFolder(const CEWSString & oldParentFolderId,
	                        const CEWSString & parentFolderId,
	                        const CEWSString & oldFolderId,
	                        const CEWSString & folderId) = 0;
	virtual void ModifyFolder(const CEWSString & parentFolderId,
	                          const CEWSString & folderId,
	                          int unreadCount) = 0;
	virtual void CopyFolder(const CEWSString & oldParentFolderId,
	                        const CEWSString & parentFolderId,
	                        const CEWSString & oldFolderId,
	                        const CEWSString & folderId) = 0;
	virtual void DeleteFolder(const CEWSString & parentFolderId,
	                          const CEWSString & folderId) = 0;
	virtual void CreateFolder(const CEWSString & parentFolderId,
	                          const CEWSString & folderId) = 0;
};

class DLL_PUBLIC CEWSSubscriptionOperation {
public:
	CEWSSubscriptionOperation();
	virtual ~CEWSSubscriptionOperation();

public:
	virtual CEWSSubscription * SubscribeWithPull(const CEWSStringList & folderIdList,
	                                             const CEWSIntList & distinguishedIdNameList,
	                                             int notifyTypeFlags,
	                                             int timeout,
	                                             CEWSError * pError) = 0;
	virtual CEWSSubscription * SubscribeWithPush(const CEWSStringList & folderIdList,
	                                             const CEWSIntList & distinguishedIdNameList,
	                                             int notifyTypeFlags,
	                                             int timeout,
	                                             const CEWSString & url,
	                                             CEWSSubscriptionCallback * callback,
	                                             CEWSError * pError) = 0;
	virtual bool Unsubscribe(const CEWSSubscription * subscription,
	                         CEWSError * pError) = 0;
	virtual bool Unsubscribe(const CEWSString & subscriptionId,
	                         const CEWSString & waterMark,
	                         CEWSError * pError) = 0;
	virtual bool GetEvents(const CEWSSubscription * subscription,
	                       CEWSSubscriptionCallback * callback,
	                       bool & moreEvents,
	                       CEWSError * pError) = 0;
	virtual bool GetEvents(const CEWSString & subscriptionId,
	                       const CEWSString & waterMark,
	                       CEWSSubscriptionCallback * callback,
	                       bool & moreEvents,
	                       CEWSError * pError) = 0;
};
} //namespace ews
;
#endif //C++

#ifndef __cplusplus
typedef int bool;
#endif //C

//functions for c program
#ifdef __cplusplus
extern "C" {
#endif
	typedef void ews_session;

	typedef struct __ews_session_params {
		const char * endpoint;

		const char * domain;
		const char * user;
		const char * password;
		const char * email;
		int auth_method;

		const char * proxy_host;
		const char * proxy_user;
		const char * proxy_password;
		int proxy_type;
		long proxy_port;

        const char * timezone_id;
	} ews_session_params;

	DLL_PUBLIC int ews_session_init(ews_session_params * params,
	                                ews_session ** pp_session,
	                                char ** pp_err_msg);
	DLL_PUBLIC void ews_session_set_timezone_id(ews_session * session,
                                                const char * tz_id);
	DLL_PUBLIC void ews_session_cleanup(ews_session * session);

	typedef struct __ews_folder {
		char * id;
		char * change_key;
		char * display_name;
		char * parent_folder_id;
		int folder_type;
		int total_count;
		int unread_count;
		bool is_visible;
	} ews_folder;

	typedef const char * (*f_get_sync_state)(void * user_data);
	typedef void (*f_set_sync_state)(const char * sync_state, void * user_data);

	typedef int (*f_get_folder_type)(void * user_data);

	typedef void (*f_new_folder)(ews_folder * folder, void * user_data);
	typedef void (*f_update_folder)(ews_folder * folder, void * user_data);
	typedef void (*f_delete_folder)(ews_folder * folder, void * user_data);

	typedef struct __ews_sync_folder_callback {
		f_get_sync_state get_sync_state;
		f_set_sync_state set_sync_state;
		f_get_folder_type get_folder_type;
		f_new_folder new_folder;
		f_update_folder update_folder;
		f_delete_folder delete_folder;
		void * user_data;
	} ews_sync_folder_callback;

	/* folder operation */
	DLL_PUBLIC int ews_sync_folder(ews_session * session,
	                               ews_sync_folder_callback * callback,
	                               char ** pp_err_msg);
	DLL_PUBLIC int ews_get_folders_by_id(ews_session * session,
	                                     const char ** folder_id,
	                                     int count,
	                                     ews_folder ** ppfolder,
	                                     char ** pp_err_msg);
	DLL_PUBLIC int ews_get_folders_by_distinguished_id_name(ews_session * session,
	                                                        int * distinguished_id_name,
	                                                        int count,
	                                                        ews_folder ** ppfolder,
	                                                        char ** pp_err_msg);
	DLL_PUBLIC int ews_create_folder(ews_session * session,
	                                 ews_folder * folder,
	                                 const char * parent_folder_id,
	                                 char ** pp_err_msg);
	DLL_PUBLIC int ews_update_folder(ews_session * session,
	                                 ews_folder * folder,
	                                 char ** pp_err_msg);
	DLL_PUBLIC int ews_delete_folder(ews_session * session,
	                                 ews_folder * folder,
	                                 bool hardDelete,
	                                 char ** pp_err_msg);
	DLL_PUBLIC int ews_delete_folder_by_id(ews_session * session,
	                                       const char * folder_id,
	                                       const char * folder_change_key,
	                                       bool hardDelete,
	                                       char ** pp_err_msg);
	DLL_PUBLIC void ews_free_folder(ews_folder * folder);
	DLL_PUBLIC void ews_free_folders(ews_folder * folder, int count);

	/* attachment operations */
	typedef struct __ews_attachment {
		int attachment_type;
		char * attachment_id;
		char * content_id;
		char * content_location;
		char * content_type;
		char * name;
	} ews_attachment;

	typedef struct __ews_item_attachment {
		struct __ews_attachment attachment;
		char * item_id;
		int item_type;
	} ews_item_attachment;

	typedef struct __ews_file_attachment {
		ews_attachment attachment;

		char * content;
	} ews_file_attachment;

	DLL_PUBLIC int ews_get_attachment(ews_session * session,
	                                  const char * attachment_id,
	                                  ews_attachment ** pp_attachment,
	                                  char ** pp_err_msg);
	DLL_PUBLIC int ews_create_attachments(ews_session * session,
	                                      const char * item_id,
	                                      ews_attachment * attachments,
	                                      int attachment_count,
	                                      char ** pp_err_msg);

	typedef struct __ews_emailaddress {
		char * item_id;
		char * name;
		char * email;
		char * routing_type;
		char * change_key;
		int mailbox_type;
	} ews_emailaddress;
    
	typedef struct __ews_recipient {
		ews_emailaddress mailbox;
	} ews_recipient;

	typedef struct __ews_internet_message_header {
		char * value;
		char * header_name;
	} ews_internet_message_header;
	
	/* item operations */
	typedef struct __ews_item {
		int item_type;
		
		int body_type;
		
		time_t create_time;
		time_t received_time;
		time_t sent_time;

		char * item_id;
		char * body;
		char * display_cc;
		char * display_to;
		char * inreply_to;
		char * subject;
		char * change_key;
		char * mime_content;

		ews_attachment ** attachments;
		int attachment_count;
		bool has_attachments;

		char * parent_folder_id;
		char * parent_folder_change_key;
		char * item_class;

		int sensitivity;
		int size;

		int categories_count;
		char ** categories;

		int importance;

		bool is_submitted;
		bool is_draft;
		bool is_from_me;
		bool is_resend;
		bool isunmodified;

		int internet_message_headers_count;
		ews_internet_message_header ** internet_message_headers;


		time_t reminder_due_by;
		bool reminder_is_set;
		char * reminder_minutes_before_start;

		char * last_modified_name;
		time_t last_modified_time;
	} ews_item;

	typedef struct __ews_msg_item {
		ews_item item;

		ews_recipient sender;
		ews_recipient from;
		ews_recipient received_by;
		ews_recipient received_representing;

		ews_recipient * to_recipients;
		int to_recipients_count;
		ews_recipient * cc_recipients;
		int cc_recipients_count;
		ews_recipient * bcc_recipients;
		int bcc_recipients_count;
		ews_recipient * reply_to;
		int reply_to_count;

		int read_receeipt_requested;
		int delivery_receipt_requested;
		int read;
		int response_requested;

		char * conversation_index;
		char * conversation_topic;
		char * internet_message_id;
		char * references;
	} ews_msg_item;

	typedef struct __ews_complete_name {
		char * title;
		char * first_name;
		char * middle_name;
		char * last_name;
		char * suffix;
		char * initials;
		char * full_name;
		char * yomi_first_name;
		char * yomi_last_name;
	} ews_complete_name;

	typedef struct __ews_physical_address {
		char * street;
		char * city;
		char * state;
		char * country_or_region;
		char * postal_code;
	} ews_physical_address;
        
	typedef struct __ews_contact_item {
		ews_item item;
        
		char * name;
		char * email_address;
		char * routing_type;
		int mailbox_type;
		char * display_name;
		char * given_name;
		char * initials;
		char * middle_name;
		char * nick_name;

		ews_complete_name complete_name;

		char * company_name;
        
		int email_addresses_count;
		char ** email_addresses;

		int physical_addresses_count;
		ews_physical_address ** physical_addresses;
        
		int phone_numbers_count;
		char ** phone_numbers;

		char * assistant_name;
		time_t birthday;
		char * business_home_page;
        
		int children_count;
		char ** children;
        
		int companies_count;
		char ** companies;

		int contact_source;
		char * department;
		char * generation;
        
		int im_addresses_count;
		char ** im_addresses;

		char * job_title;
		char * manager;
		char * mileage;
		char * office_location;

		ews_physical_address * postal_address;
		char * profession;
		char * spouse_name;
		char * sur_name;
		time_t wedding_anniversary;
		bool is_maillist;
	} ews_contact_item;

	typedef struct __ews_attendee {
		ews_emailaddress email_address;

		int response_type;
		time_t last_response_time;
	} ews_attendee;

	typedef struct __ews_recurrence_range {
		int range_type;
		char * start_date;
		char * end_date;
		int number_of_occurrences;
	} ews_recurrence_range;

	typedef struct __ews_recurrence_pattern {
		int pattern_type;

		int interval;
		int days_of_week;
		int day_of_week_index;
		int month;
		int day_of_month;
	} ews_recurrence_pattern;
    
	typedef struct __ews_recurrence {
		ews_recurrence_range range;
		ews_recurrence_pattern pattern;
	} ews_recurrence;

	typedef struct __ews_occurrence_info {
		char * item_id;
		char * change_key;
		time_t start;
		time_t end;
		time_t original_start;
	} ews_occurrence_info;

	typedef struct __ews_deleted_occurrence_info {
		time_t start;
	} ews_deleted_occurrence_info;

	typedef struct __ews_time_change_sequence {
		int sequence_type;
        
		int days_of_week;
		int day_of_week_index;
		int month;
        
		char * absolute_date;
	} ews_time_change_sequence;
    
	typedef struct __ews_time_change {
		long long offset;
		ews_time_change_sequence time_change_sequence;
		char * time;
		char * time_zone_name;
	} ews_time_change;
    
	typedef struct __ews_time_zone {
		char * time_zone_name;
		long long base_offset;
		ews_time_change * standard;
		ews_time_change * daylight;
	} ews_time_zone;
    
	typedef struct __ews_calendar_item {
		ews_item item;
		char * uid;
		time_t recurrence_id;
		time_t date_time_stamp;
		time_t start;
		time_t end;
		time_t original_start;
		bool all_day_event;
		int legacy_free_busy_status;
		char * location;
		char * when;
		bool is_meeting;
		bool is_cancelled;
		bool is_recurring;
		bool meeting_request_was_sent;
		bool is_response_requested;
		int calendar_item_type;
		int my_response_type;
		ews_recipient * organizer;

		int required_attendees_count;
		ews_attendee ** required_attendees;
		int optional_attendees_count;
		ews_attendee ** optional_attendees;
		int resources_count;
		ews_attendee ** resources;

		int conflicting_meetings_count;
		ews_item ** conflicting_meetings;
		int adjacent_meetings_count;
		ews_item ** adjacent_meetings;

		char * duration;
		char * time_zone;
		time_t appointment_reply_time;
		int appointment_sequence_number;
		int appointment_state;

		ews_recurrence * recurrence;
		ews_occurrence_info * first_occurrence;
		ews_occurrence_info * last_occurrence;

		int modified_occurrences_count;
		ews_occurrence_info ** modified_occurrences;

		int deleted_occurrences_count;
		ews_deleted_occurrence_info ** deleted_occurrences;

		ews_time_zone * meeting_time_zone;
		int conference_type;
		bool is_allow_new_time_proposal;
		bool is_online_meeting;

		char * meeting_workspace_url;
		char * net_show_url;

		char * recurring_master_id;
		char * recurring_master_uid;
	} ews_calendar_item;
    
	typedef struct __ews_task_item {
		ews_item item;
        
		int actual_work;
		time_t assigned_time;
		char * billing_information;
		int change_count;
		int companies_count;
		char ** companies;
		time_t complete_date;
		int contacts_count;
		char ** contacts;
		int delegation_state;
		char * delegator;
		time_t due_date;
		int is_assignment_editable;
		bool is_complete;
		bool is_recurring;
		bool is_team_task;
		char * mileage;
		char * owner;
		double percent_complete;
		ews_recurrence * recurrence;
		time_t start_date;
		int status;
		char * status_description;
		int total_work;
	} ews_task_item;
    
	typedef int (*f_get_max_return_item_count)(void * user_data);
	typedef char ** (*f_get_ignored_item_ids)(int * p_count, void * user_data);
	typedef void (*f_new_item)(const ews_item * item, void * user_data);
	typedef void (*f_update_item)(const ews_item * item, void * user_data);
	typedef void (*f_delete_item)(const ews_item * item, void * user_data);
	typedef void (*f_read_item)(const ews_item * item, int read, void * user_data);

	typedef struct __ews_sync_item_callback {
		f_get_sync_state get_sync_state;
		f_set_sync_state set_sync_state;
		f_get_max_return_item_count get_max_return_item_count;
		f_get_ignored_item_ids get_ignored_item_ids;
		f_new_item new_item;
		f_update_item update_item;
		f_delete_item delete_item;
		f_read_item read_item;
		void * user_data;
	} ews_sync_item_callback;

	DLL_PUBLIC int ews_sync_items(ews_session * session,
	                              const char * folder_id,
	                              ews_sync_item_callback * callback,
	                              char ** pp_err_msg);
	DLL_PUBLIC int ews_get_unread_items(ews_session * session,
	                                    const char * folder_id,
	                                    ews_sync_item_callback * callback,
	                                    char ** pp_err_msg);
	DLL_PUBLIC int ews_get_item(ews_session * session,
	                            const char * item_id,
	                            int flags,
	                            ews_item ** pp_item,
	                            char ** pp_error_msg);
	DLL_PUBLIC int ews_get_items(ews_session * session,
	                             const char ** item_id,
	                             int & item_count, /* in/out */
	                             int flags,
	                             ews_item *** pp_item,
	                             char ** pp_error_msg);
	DLL_PUBLIC int ews_create_item_by_id(ews_session * session,
	                                     const char * folder_id,
	                                     ews_item * item,
	                                     char ** pp_err_msg);
	DLL_PUBLIC int ews_create_item_by_distinguished_id_name(ews_session * session,
	                                                        int distinguished_id_name,
	                                                        ews_item * item,
	                                                        char ** pp_err_msg);
	DLL_PUBLIC int ews_delete_item(ews_session * session,
	                               const char * item_id,
	                               const char * change_key,
	                               int moveToDeleted,
	                               char ** pp_error_msg);
	DLL_PUBLIC int ews_send_item(ews_session * session,
	                             ews_msg_item * msg_item,
	                             char ** pp_error_msg);
	DLL_PUBLIC void ews_free_item(ews_item * item);
	DLL_PUBLIC void ews_free_items(ews_item ** item, int item_count);
	DLL_PUBLIC ews_msg_item * ews_msg_item_clone(ews_msg_item * v);
	DLL_PUBLIC int ews_delete_items(ews_session * session,
	                                ews_item * items,
	                                int item_count,
	                                int moveToDeleted,
	                                char ** pp_err_msg);
	DLL_PUBLIC int ews_mark_items_read(ews_session * session,
	                                   ews_item * items,
	                                   int item_count,
	                                   int read,
	                                   char ** pp_err_msg);
	DLL_PUBLIC int ews_resolve_names(ews_session * session,
	                                 const char * unresolvedEntry,
	                                 ews_item *** pp_items,
	                                 int & items_count,
	                                 char ** pp_err_msg);
    
	DLL_PUBLIC int ews_sync_calendar(ews_session * session,
	                                 ews_sync_item_callback * callback,
	                                 char ** pp_err_msg);
	DLL_PUBLIC int ews_cal_get_recurrence_master_id(ews_session * session,
	                                                const char * item_id,
	                                                char ** mast_item_id,
	                                                char ** mast_item_uid,
	                                                char ** pp_err_msg);
	DLL_PUBLIC int ews_calendar_accept_event(ews_session * session,
	                                         ews_calendar_item * item,
	                                         char ** pp_err_msg);
	DLL_PUBLIC int ews_calendar_decline_event(ews_session * session,
	                                          ews_calendar_item * item,
	                                          char ** pp_err_msg);
	DLL_PUBLIC int ews_calendar_tentatively_accept_event(ews_session * session,
	                                                     ews_calendar_item * item,
	                                                     char ** pp_err_msg);
	DLL_PUBLIC int ews_calendar_delete_occurrence(ews_session * session,
	                                              const char * master_id,
	                                              int occurrence_instance,
	                                              char ** pp_err_msg);
	DLL_PUBLIC int ews_calendar_update_event(ews_session * session,
	                                         ews_calendar_item * item,
	                                         int flags,
	                                         char ** pp_err_msg);
	DLL_PUBLIC int ews_calendar_update_event_occurrence(ews_session * session,
	                                                    ews_calendar_item * item,
	                                                    int instanceIndex,
	                                                    int flags,
	                                                    char ** pp_err_msg);
    
	DLL_PUBLIC int ews_sync_task(ews_session * session,
	                             ews_sync_item_callback * callback,
	                             char ** pp_err_msg);
	DLL_PUBLIC int ews_calendar_update_task(ews_session * session,
	                                        ews_task_item * item,
	                                        int flags,
	                                        char ** pp_err_msg);
	DLL_PUBLIC int ews_calendar_update_task_occurrence(ews_session * session,
	                                                   ews_task_item * item,
	                                                   int instanceIndex,
	                                                   int flags,
	                                                   char ** pp_err_msg);

	DLL_PUBLIC int ews_autodiscover(ews_session_params * params,
	                                char ** pp_endpoint,
	                                char ** pp_oab_url,
	                                int * auth_method,
	                                char ** pp_err_msg);

	DLL_PUBLIC int ews_autodiscover_free(ews_session_params * params,
		char * endpoint,
		char * oab_url,
		char * err_msg);

	typedef void ews_notification_server;
	
	typedef struct __ews_subscription {
		char * subscription_id;
		char * water_mark;
	} ews_subscription;

	typedef struct __ews_notification_server_params {
		char * ip;
		int port;
		int use_https;
	} ews_notification_server_params;
	
	typedef void (*f_move_or_copy_event)(const char * old_parent_folder_id,
	                                     const char * parent_folder_id,
	                                     const char * old_id,
	                                     const char * id,
	                                     bool is_item,
	                                     void * user_data);
	typedef void (*f_modify_event)(const char * parent_folder_id,
	                               const char * id,
	                               bool is_item,
	                               void * user_data);
	typedef void (*f_modify_event_2)(const char * parent_folder_id,
	                                 const char * id,
	                                 int unreadCount,
	                                 bool is_item,
	                                 void * user_data);

	typedef struct __ews_event_notification_callback {
		f_modify_event new_mail;

		f_modify_event_2 modify;
		f_modify_event remove;
		f_modify_event create;
		f_move_or_copy_event move;
		f_move_or_copy_event copy;
		void * user_data;
	} ews_event_notification_callback;

	DLL_PUBLIC ews_notification_server * ews_create_notification_server(ews_session *session,
	                                                                    ews_event_notification_callback * pcallback,
	                                                                    ews_notification_server_params * params);
	DLL_PUBLIC int ews_notification_server_bind(ews_notification_server * server);
	DLL_PUBLIC int ews_notification_server_get_url(ews_notification_server * server, char ** pp_url);
	DLL_PUBLIC int ews_notification_server_run(ews_notification_server * server);
	DLL_PUBLIC int ews_notification_server_stop(ews_notification_server * server);
	DLL_PUBLIC int ews_free_notification_server(ews_notification_server * server);

	DLL_PUBLIC int ews_subscribe_to_folders_with_pull(ews_session * session,
	                                                  const char ** folder_ids,
	                                                  int folder_id_count,
	                                                  int * distinguished_ids,
	                                                  int distinguished_id_count,
	                                                  int notifyFlags,
	                                                  int timeout,
	                                                  ews_subscription ** pp_subscription,
	                                                  char ** pp_error_msg);
	DLL_PUBLIC int ews_subscribe_to_folders_with_push(ews_session * session,
	                                                  const char ** folder_ids,
	                                                  int folder_id_count,
	                                                  int * distinguished_ids,
	                                                  int distinguished_id_count,
	                                                  int notifyFlags,
	                                                  int timeout,
	                                                  const char * url,
	                                                  ews_event_notification_callback * callback,
	                                                  ews_subscription ** pp_subscription,
	                                                  char ** pp_error_msg);
	DLL_PUBLIC int ews_unsubscribe(ews_session * session,
	                               ews_subscription * subscription,
	                               char ** pp_error_msg);
	DLL_PUBLIC int ews_get_events(ews_session * session,
	                              ews_subscription * subscription,
	                              ews_event_notification_callback * callback,
	                              bool * more_item,
	                              char ** pp_error_msg);
	DLL_PUBLIC void ews_free_subscription(ews_subscription * subscription);
	
#ifdef __cplusplus
}
#endif

#endif /* LIBEWS_H_ */

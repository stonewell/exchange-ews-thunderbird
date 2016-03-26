/*
 * libews_defs.h
 *
 *  Created on: Aug 27, 2014
 *      Author: stone
 */

#ifndef LIBEWS_DEFS_H_
#define LIBEWS_DEFS_H_

#include <stddef.h>
#include <time.h>
#include <string.h>

#ifdef _WIN32
#define strdup _strdup
#pragma warning(disable:4250)
#endif

#if defined _WIN32 || defined __CYGWIN__
  #ifdef BUILDING_DLL
    #ifdef __GNUC__
      #define DLL_PUBLIC __attribute__ ((dllexport))
    #else
      #define DLL_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #else
    #ifdef __GNUC__
      #define DLL_PUBLIC __attribute__ ((dllimport))
    #else
      #define DLL_PUBLIC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #endif
  #define DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define DLL_PUBLIC __attribute__ ((visibility ("default")))
    #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define DLL_PUBLIC
    #define DLL_LOCAL
  #endif
#endif

#define EWS_SUCCESS (0)
#define EWS_FAIL (-1)
#define EWS_INVALID_PARAM (-2)

//folder type
#define EWS_FOLDER_MAIL 1
#define EWS_FOLDER_ALL_TYPES (EWS_FOLDER_MAIL)

#define EWS_PROXY_HTTP (0)
#define EWS_PROXY_SOCKS4 (1)
#define EWS_PROXY_SOCKS5 (2)

#define EWSDistinguishedFolderIdName_calendar (0)
#define EWSDistinguishedFolderIdName_contacts (1)
#define EWSDistinguishedFolderIdName_deleteditems (2)
#define EWSDistinguishedFolderIdName_drafts (3)
#define EWSDistinguishedFolderIdName_inbox (4)
#define EWSDistinguishedFolderIdName_journal (5)
#define EWSDistinguishedFolderIdName_notes (6)
#define EWSDistinguishedFolderIdName_outbox (7)
#define EWSDistinguishedFolderIdName_sentitems (8)
#define EWSDistinguishedFolderIdName_tasks (9)
#define EWSDistinguishedFolderIdName_msgfolderroot (10)
#define EWSDistinguishedFolderIdName_publicfoldersroot (11)
#define EWSDistinguishedFolderIdName_root (12)
#define EWSDistinguishedFolderIdName_junkemail (13)
#define EWSDistinguishedFolderIdName_searchfolders (14)
#define EWSDistinguishedFolderIdName_voicemail (15)

#define EWS_Attachment_Item (0)
#define EWS_Attachment_File (1)

#define	EWS_Item_Message (0)
#define EWS_Item_Contact (1)
#define EWS_Item_Calendar (2)
#define EWS_Item_Task (3)
#define EWS_Item_Unknown (-1)

#define EWS_BODY_HTML (0)
#define EWS_BODY_TEXT (1)

#define EWS_NOTIFY_EVENT_TYPE_COPY (1L)
#define EWS_NOTIFY_EVENT_TYPE_CREATE (1L << 1)
#define EWS_NOTIFY_EVENT_TYPE_DELETE (1L << 2)
#define EWS_NOTIFY_EVENT_TYPE_MODIFY (1L << 3)
#define EWS_NOTIFY_EVENT_TYPE_MOVE (1L << 4)
#define EWS_NOTIFY_EVENT_TYPE_NEWMAIL (1L << 5)

//auth method
#define EWS_AUTH_NTLM (0)
#define EWS_AUTH_BASIC (1)

#define EWS_CONTACT_SOURCE_ACTIVE_DIRECTORY (0)
#define EWS_CONTACT_SOURCE_STORE (1)

#define EWS_MAILBOXTYPE_MAILBOX (0)
#define EWS_MAILBOXTYPE_PUBLICDL (1)
#define EWS_MAILBOXTYPE_PRIVATEDL (2)
#define EWS_MAILBOXTYPE_CONTACT (3)
#define EWS_MAILBOXTYPE_PUBLICFOLDER (4)

#define EWS_LegacyFreeBusyType_Free (0)
#define EWS_LegacyFreeBusyType_Tentative (1)
#define EWS_LegacyFreeBusyType_Busy (2)
#define EWS_LegacyFreeBusyType_OOF (3)
#define EWS_LegacyFreeBusyType_NoData (4)

#define EWS_CalendarItemType_Single (0)
#define EWS_CalendarItemType_Occurrence (1)
#define EWS_CalendarItemType_Exception (2)
#define EWS_CalendarItemType_RecurringMaster (3)

#define EWS_ResponseType_Unknown (0)
#define EWS_ResponseType_Organizer (1)
#define EWS_ResponseType_Tentative (2)
#define EWS_ResponseType_Accept (3)
#define EWS_ResponseType_Decline (4)
#define EWS_ResponseType_NoResponseReceived (5)

#define EWSRecurrenceRangeType_NoEnd (1)
#define EWSRecurrenceRangeType_EndDate (2)
#define EWSRecurrenceRangeType_Numbered (3)

#define EWS_Recurrence_RelativeYearly (1)
#define EWS_Recurrence_AbsoluteYearly (2)
#define EWS_Recurrence_RelativeMonthly (3)
#define EWS_Recurrence_AbsoluteMonthly (4)
#define EWS_Recurrence_Weekly (5)
#define EWS_Recurrence_Daily (6)

#define EWS_DayOfWeek_Sunday (0)
#define EWS_DayOfWeek_Monday (1)
#define EWS_DayOfWeek_Tuesday (2)
#define EWS_DayOfWeek_Wednesday (3)
#define EWS_DayOfWeek_Thursday (4)
#define EWS_DayOfWeek_Friday (5)
#define EWS_DayOfWeek_Saturday (6)
#define EWS_DayOfWeek_Day (7)
#define EWS_DayOfWeek_Weekday (8)
#define EWS_DayOfWeek_WeekendDay (9)

#define EWS_DayOfWeekIndex_First (0)
#define EWS_DayOfWeekIndex_Second (1)
#define EWS_DayOfWeekIndex_Third (2)
#define EWS_DayOfWeekIndex_Fourth (3)
#define EWS_DayOfWeekIndex_Last (4)

#define EWS_MonthNames_January (0)
#define EWS_MonthNames_February (1)
#define EWS_MonthNames_March (2)
#define EWS_MonthNames_April (3)
#define EWS_MonthNames_May (4)
#define EWS_MonthNames_June (5)
#define EWS_MonthNames_July (6)
#define EWS_MonthNames_August (7)
#define EWS_MonthNames_September (8)
#define EWS_MonthNames_October (9)
#define EWS_MonthNames_November (10)
#define EWS_MonthNames_December (11)

#define EWS_TimeChange_Sequence_RelativeYearly (1)
#define EWS_TimeChange_Sequence_AbsoluteDate (2)

#define EWS_GetItems_Flags_None (0)
#define EWS_GetItems_Flags_MimeContent (1)
#define EWS_GetItems_Flags_MessageItem (1 << 1)
#define EWS_GetItems_Flags_ContactItem (1 << 2)
#define EWS_GetItems_Flags_CalendarItem (1 << 3)
#define EWS_GetItems_Flags_IdOnly (1 << 4)
#define EWS_GetItems_Flags_AllProperties (1 << 5)
#define EWS_GetItems_Flags_RecurringMasterItem (1 << 6)
#define EWS_GetItems_Flags_OccurrenceItem (1 << 7)
#define EWS_GetItems_Flags_TaskItem (1 << 8)

#define EWS_Importance_Low (0)
#define EWS_Importance_Normal (1)
#define EWS_Importance_High (2)

#define EWS_Sensitivity_Normal (0)
#define EWS_Sensitivity_Personal (1)
#define EWS_Sensitivity_Private (2)
#define EWS_Sensitivity_Confidential (3)

#define EWS_UpdateCalendar_Flags_Body (1)
#define EWS_UpdateCalendar_Flags_Start (1 << 1)
#define EWS_UpdateCalendar_Flags_End (1 << 2)
#define EWS_UpdateCalendar_Flags_Required_Attendee (1 << 3)
#define EWS_UpdateCalendar_Flags_Optional_Attendee (1 << 4)
#define EWS_UpdateCalendar_Flags_Location (1 << 5)
#define EWS_UpdateCalendar_Flags_Subject (1 << 6)
#define EWS_UpdateCalendar_Flags_Status (1 << 7)
#define EWS_UpdateCalendar_Flags_Percent (1 << 8)
#define EWS_UpdateCalendar_Flags_Is_Complete (1 << 9)
#define EWS_UpdateCalendar_Flags_Complete (1 << 10)
        
#define EWS__TaskDelegateStateType__NoMatch (0)
#define EWS__TaskDelegateStateType__OwnNew (1)
#define EWS__TaskDelegateStateType__Owned (2)
#define EWS__TaskDelegateStateType__Accepted (3)
#define EWS__TaskDelegateStateType__Declined (4)
#define EWS__TaskDelegateStateType__Max (5)

#define EWS__TaskStatusType__NotStarted (0)
#define EWS__TaskStatusType__InProgress (1)
#define EWS__TaskStatusType__Completed (2)
#define EWS__TaskStatusType__WaitingOnOthers (3)
#define EWS__TaskStatusType__Deferred (4)

#endif /* LIBEWS_DEFS_H_ */

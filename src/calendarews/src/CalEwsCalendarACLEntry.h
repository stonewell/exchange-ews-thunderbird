#pragma once
#ifndef __CAL_EWS_CALENDAR_ACL_ENTRY_H__
#define __CAL_EWS_CALENDAR_ACL_ENTRY_H__

#include "calICalendarACLManager.h"
#include "calICalendar.h"

#include "nsCOMPtr.h"

class CalEwsCalendarACLEntry
        : public calICalendarACLEntry {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_CALICALENDARACLENTRY
    
    CalEwsCalendarACLEntry(calICalendar * calendar);

private:
    virtual ~CalEwsCalendarACLEntry();

    NS_IMETHOD GetIdentity();

    nsCOMPtr<calICalendar> m_Calendar;
    nsCOMPtr<nsIMsgIdentity> m_Identity;
};

#endif //__CAL_EWS_CALENDAR_ACL_ENTRY_H__

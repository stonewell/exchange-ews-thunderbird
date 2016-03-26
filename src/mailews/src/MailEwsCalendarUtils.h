#pragma once
#ifndef __MAIL_EWS_CALENDAR_UTILS_H__
#define __MAIL_EWS_CALENDAR_UTILS_H__

#define GET_DATE_TIME(cal, ews) \
	{ \
	nsCOMPtr<calIDateTime> dt; \
	\
	cal(getter_AddRefs(dt)); \
	ews = FromCalDateTimeToTime(dt, false);           \
	}

#define GET_DATE_TIME_3(cal, ews, notime)              \
	{ \
	nsCOMPtr<calIDateTime> dt; \
	\
	cal(getter_AddRefs(dt)); \
	ews = FromCalDateTimeToTime(dt, notime);           \
	}

#define UNIX_TIME_TO_PRTIME 1000000

time_t
FromCalDateTimeToTime(calIDateTime * date_time, bool notime);

#endif //__MAIL_EWS_CALENDAR_UTILS_H__

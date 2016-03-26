#include "rfc3339.h"
#include <cstdlib>
#include <cstdio>

#ifdef _WIN32
#define timegm _mkgmtime
#define snprintf _snprintf
#endif //_WIN32

namespace date
{

time_t Rfc3339::parse(const std::string& rfc3339Date) {
	bool utcOffset = false;
	// format validation
	if (false == validate(rfc3339Date, utcOffset)) return (time_t)-1;
	int year = atoi(rfc3339Date.substr(0, dateYearIndex).c_str());
	int month = atoi(rfc3339Date.substr(dateYearIndex + 1, dateMonthIndex - dateYearIndex - 1).c_str());
	int day = atoi(rfc3339Date.substr(dateMonthIndex + 1, dateTimeIndex - dateMonthIndex - 1).c_str());
	int hour = atoi(rfc3339Date.substr(dateTimeIndex + 1, timeHourIndex - dateTimeIndex - 1).c_str());
	int minute = atoi(rfc3339Date.substr(timeHourIndex + 1, timeMinuteIndex - timeHourIndex - 1).c_str());
	int second = atoi(rfc3339Date.substr(timeMinuteIndex + 1, timeSecondIndex - timeMinuteIndex - 1).c_str());
	// local time
	int numOffsetHour = 0;
	int numOffsetMinute = 0;
	if (false == utcOffset) {
		numOffsetHour = atoi(rfc3339Date.substr(timeSecondIndex, timeNumOffsetIndex - timeSecondIndex).c_str());
		numOffsetMinute = atoi(rfc3339Date.substr(timeNumOffsetIndex + 1).c_str());
	}
	// number validation
	if (false == validate(
		year,
		month,
		day,
		hour,
		minute,
		second,
		numOffsetHour,
		numOffsetMinute
	)) return (time_t)-1;
	tm gt;
	gt.tm_year = year - 1900;
	gt.tm_mon = month - 1,
	gt.tm_mday = day,
	gt.tm_hour = hour;
	gt.tm_min = minute;
	gt.tm_sec = second;
	// local time
	if (false == utcOffset) gt.tm_hour -= numOffsetHour;
	return timegm(&gt);
}
std::string Rfc3339::serialize(const time_t date, bool notime) {
	tm *gt;
	gt = gmtime(&date);
	// if (localTime)内部ではgt->tm_hourの値に問題がある tm構造体のメモリを共有している？
	int tmp = gt->tm_hour;
	if (localTime) {
		// local time
		tm *lt;
		lt = localtime(&date);
		char dateString[26];
        if (notime) {
            snprintf(dateString, 26, "%04d%c%02d%c%02d",
                     lt->tm_year + 1900,
                     dateSeparation,
                     lt->tm_mon + 1,
                     dateSeparation,
                     lt->tm_mday
                     );
        } else {
            snprintf(dateString, 26, "%04d%c%02d%c%02d%c%02d%c%02d%c%02d%+03d%c00",
                     lt->tm_year + 1900,
                     dateSeparation,
                     lt->tm_mon + 1,
                     dateSeparation,
                     lt->tm_mday,
                     dateTimeSeparation,
                     lt->tm_hour,
                     timeSeparation,
                     lt->tm_min,
                     timeSeparation,
                     lt->tm_sec,
                     (0 == lt->tm_hour) ? 24 - tmp : lt->tm_hour - tmp, // number offset
                     timeSeparation
                     );
        }
		return std::string(dateString);
	}
	// utc time
	char dateString[25];

    if (notime) {
        snprintf(dateString, 25, "%04d%c%02d%c%02d%c",
                 gt->tm_year + 1900,
                 dateSeparation,
                 gt->tm_mon + 1,
                 dateSeparation,
                 gt->tm_mday,
                 timeUtcOffset
                 );
    } else {
        snprintf(dateString, 25, "%04d%c%02d%c%02d%c%02d%c%02d%c%02d%c",
                 gt->tm_year + 1900,
                 dateSeparation,
                 gt->tm_mon + 1,
                 dateSeparation,
                 gt->tm_mday,
                 dateTimeSeparation,
                 gt->tm_hour,
                 timeSeparation,
                 gt->tm_min,
                 timeSeparation,
                 gt->tm_sec,
                 timeUtcOffset
                 );
    }
	return std::string(dateString);
}


Rfc3339::Rfc3339() {
	this->setSummerTime(false);
	this->setLocalTime(false);
	minYear = 0;
	maxYear = 9999;
	minMonth = 1;
	maxMonth = 12;
	minDay = 1;
	maxDay[0] = 31; // January
	maxDay[1] = 28; // February, normal year
	maxDay[2] = 31; // March
	maxDay[3] = 30; // April
	maxDay[4] = 31; // May
	maxDay[5] = 30; // June
	maxDay[6] = 31; // July
	maxDay[7] = 31; // August
	maxDay[8] = 30; // September
	maxDay[9] = 31; // October
	maxDay[10] = 30; // November
	maxDay[11] = 31; // December
	maxDayFebruaryLeapYear = 29; // February, leap year
	minHour = 0;
	maxHour = 23;
	minMinute = 0;
	maxMinute = 59;
	minSecond = 0;
	maxSecond = 59;
	minNumOffsetHour = -12;
	maxNumOffsetHour = 12;
	dateSeparation = '-';
	dateTimeSeparation = 'T';
	timeSeparation = ':';
	timeUtcOffset = 'Z';
	timeNumOffsetPlus = '+';
	timeNumOffsetMinus = '-';
	dateYearIndex = 4;
	dateMonthIndex = 7;
	dateTimeIndex = 10;
	timeHourIndex = 13;
	timeMinuteIndex = 16;
	timeSecondIndex = 19;
	timeNumOffsetIndex = 22;
}
Rfc3339::~Rfc3339() {
}
Rfc3339::Rfc3339(const Rfc3339& obj) {
}
Rfc3339& Rfc3339::operator=(const Rfc3339& obj) {
	return *this;
}
void Rfc3339::setSummerTime(bool summerTime) {
	this->summerTime = summerTime;
}
void Rfc3339::setLocalTime(bool localTime) {
	this->localTime = localTime;
}
bool Rfc3339::isLeapYear(int year) {
	return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}
bool Rfc3339::validate(const std::string& rfc3339Date, bool& utcOffset) {
	if (dateSeparation != rfc3339Date.at(dateYearIndex)) return false;
	if (dateSeparation != rfc3339Date.at(dateMonthIndex)) return false;
	if (dateTimeSeparation != rfc3339Date.at(dateTimeIndex)) return false;
	if (timeSeparation != rfc3339Date.at(timeHourIndex)) return false;
	if (timeSeparation != rfc3339Date.at(timeMinuteIndex)) return false;
	// check time offset
	if (timeUtcOffset == rfc3339Date.at(timeSecondIndex)) utcOffset = true;
	else if (timeNumOffsetPlus == rfc3339Date.at(timeSecondIndex)) utcOffset = false;
	else if (timeNumOffsetMinus == rfc3339Date.at(timeSecondIndex)) utcOffset = false;
	else return false;
	// localtime
	if (false == utcOffset) {
		if (timeSeparation != rfc3339Date.at(timeNumOffsetIndex)) return false;
	}
	return true;
}
bool Rfc3339::validate(
	const int year,
	const int month,
	const int day,
	const int hour,
	const int minute,
	const int second,
	const int numOffsetHour,
	const int numOffsetMinute
) {
	if ((year < minYear) || (maxYear < year)) return false;
	if ((month < minMonth) || (maxMonth < month)) return false;
	// leap year and February
	if (isLeapYear(year) && (2 == month))
		if ((day < minDay) || (maxDayFebruaryLeapYear < day)) return false;
	if ((day < minDay) || (maxDay[month - 1] == day)) return false;
	if ((hour < minHour) || (maxHour < hour)) return false;
	if ((minute < minMinute) || (maxMinute < minute)) return false;
	if ((second < minSecond) || (maxSecond < second)) return false;
	// RFC3339, numOffsetHour and is same hour?
	// but numOffsetHour is -12 ~ 12?
	if ((numOffsetHour < minNumOffsetHour) || (maxNumOffsetHour < numOffsetHour)) return false;
	// numOffsetMinute is 0?
	//if ((numOffsetMinute < minMinute) || (maxMinute < numOffsetMinute)) return false;
	if (0 != numOffsetMinute) return false;
	return true;
}

}; // namespace date

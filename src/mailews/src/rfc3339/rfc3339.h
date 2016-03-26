#pragma once

#include <ctime>
#include <string>

namespace date
{

class Rfc3339
{
public:
	// if return -1, parse fail
	time_t parse(const std::string& rfc3339Date);
	std::string serialize(const time_t date, bool notime = false);
	// true:on false:off
	void setSummerTime(bool summerTime);
	// true:create localtime string false:create UTC time string
	void setLocalTime(bool localTime);
	// true:leap year false:normal year
	bool isLeapYear(int year);
	Rfc3339();
	~Rfc3339();

private:
	Rfc3339(const Rfc3339& obj);
	Rfc3339& operator=(const Rfc3339& obj);
	//12345678910111213141516171819202122232425
	//YYYY-MM-D D T H H : M M : S S Z
	//YYYY-MM-D D T H H : M M : S S + H H : M M
	// [utcOffset]true:utc false:local [localOffset]true:plus false:minus
	bool validate(const std::string& rfc3339Date, bool& utcOffset);
	bool validate(
		const int year,
		const int month,
		const int day,
		const int hour,
		const int minute,
		const int second,
		const int numOffsetHour,
		const int numOffsetMinute
	);
	bool summerTime;
	bool localTime;
	int minYear;
	int maxYear;
	int minMonth;
	int maxMonth;
	int minDay;
	int maxDay[12]; // each month
	int maxDayFebruaryLeapYear; // February, leap year
	int minHour;
	int maxHour;
	int minMinute;
	int maxMinute;
	int minSecond;
	int maxSecond;
	int minNumOffsetHour;
	int maxNumOffsetHour;
	char dateSeparation;
	char dateTimeSeparation;
	char timeSeparation;
	char timeUtcOffset;
	char timeNumOffsetPlus;
	char timeNumOffsetMinus;
	std::string::size_type dateYearIndex;
	std::string::size_type dateMonthIndex;
	std::string::size_type dateTimeIndex;
	std::string::size_type timeHourIndex;
	std::string::size_type timeMinuteIndex;
	std::string::size_type timeSecondIndex;
	std::string::size_type timeNumOffsetIndex;
};

}; // namespace date

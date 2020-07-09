#include <ESP8266WiFi.h>
#include "Rtc.h"
#include "sntp.h"
#include "Debug.h"

RtcReboot Rtc::rtcReboot;
uint32_t Rtc::rtcRebootCrc = 0;
TIME_T Rtc::rtcTime;
uint32_t Rtc::utcTime;
uint8_t Rtc::operationFlag = 0;
static const uint8_t kDaysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; // API starts months from 1, this array starts from 0
static const char kMonthNamesEnglish[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

String Rtc::GetBuildDateAndTime()
{
    // "2017-03-07T11:08:02" - ISO8601:2004
    char bdt[21];
    char *p;
    char mdate[] = __DATE__; // "Mar  7 2017"
    char *smonth = mdate;
    int day = 0;
    int year = 0;

    // sscanf(mdate, "%s %d %d", bdt, &day, &year);  // Not implemented in 2.3.0 and probably too much code
    uint8_t i = 0;
    for (char *str = strtok_r(mdate, " ", &p); str && i < 3; str = strtok_r(NULL, " ", &p))
    {
        switch (i++)
        {
        case 0: // Month
            smonth = str;
            break;
        case 1: // Day
            day = atoi(str);
            break;
        case 2: // Year
            year = atoi(str);
        }
    }
    int month = (strstr(kMonthNamesEnglish, smonth) - kMonthNamesEnglish) / 3 + 1;
    snprintf_P(bdt, sizeof(bdt), PSTR("%d-%02d-%02d %s"), year, month, day, __TIME__);
    return String(bdt); // 2017-03-07T11:08:02
}

String Rtc::msToHumanString(uint32_t const msecs)
{
    uint32_t totalseconds = msecs / 1000;
    if (totalseconds == 0)
    {
        return F("0T00:00:00");
    }

    // Note: millis() can only count up to 45 days, so uint8_t is safe.
    uint8_t days = totalseconds / (60 * 60 * 24);
    uint8_t hours = (totalseconds / (60 * 60)) % 24;
    uint8_t minutes = (totalseconds / 60) % 60;
    uint8_t seconds = totalseconds % 60;

    char dt[16];
    snprintf_P(dt, sizeof(dt), PSTR("%dT%02d:%02d:%02d"), days, hours, minutes, seconds);
    return String(dt);
}

String Rtc::timeSince(uint32_t const start)
{
    if (start == 0)
        return F("Never");
    uint32_t diff = 0;
    uint32_t now = millis();
    if (start < now)
        diff = now - start;
    else
        diff = UINT32_MAX - start + now;
    return msToHumanString(diff) + F(" ago");
}

void Rtc::breakTime(uint32_t time_input, TIME_T &tm)
{
    // break the given time_input into time components
    // this is a more compact version of the C library localtime function
    // note that year is offset from 1970 !!!

    uint8_t year;
    uint8_t month;
    uint8_t month_length;
    uint32_t time;
    unsigned long days;

    time = time_input;
    tm.second = time % 60;
    time /= 60; // now it is minutes
    tm.minute = time % 60;
    time /= 60; // now it is hours
    tm.hour = time % 24;
    time /= 24; // now it is days
    tm.days = time;
    tm.day_of_week = ((time + 4) % 7) + 1; // Sunday is day 1

    year = 0;
    days = 0;
    while ((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time)
    {
        year++;
    }
    tm.year = year + 1970; // year is offset from 1970

    days -= LEAP_YEAR(year) ? 366 : 365;
    time -= days; // now it is days in this year, starting at 0
    tm.day_of_year = time;

    days = 0;
    month = 0;
    month_length = 0;
    for (month = 0; month < 12; month++)
    {
        if (1 == month)
        { // february
            if (LEAP_YEAR(year))
            {
                month_length = 29;
            }
            else
            {
                month_length = 28;
            }
        }
        else
        {
            month_length = kDaysInMonth[month];
        }

        if (time >= month_length)
        {
            time -= month_length;
        }
        else
        {
            break;
        }
    }
    tm.month = month + 1;                 // jan is month 1
    tm.day_of_month = time + 1;           // day of month
    tm.valid = (time_input > 1451602800); // 2016-01-01
}

void Rtc::loop()
{
    if (bitRead(operationFlag, 0))
    {
        bitClear(operationFlag, 0);
        getNtp();
    }
}

void Rtc::getNtp()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        uint32_t ntp_time = sntp_get_current_timestamp();
        if (ntp_time > 1451602800)
        {
            utcTime = ntp_time;
            breakTime(utcTime, rtcTime);
            Debug::AddInfo(PSTR("NTP: %04d-%02d-%02d %02d:%02d:%02d"), rtcTime.year, rtcTime.month, rtcTime.day_of_month, rtcTime.hour, rtcTime.minute, rtcTime.second);
        }
    }
}

void Rtc::perSecondDo()
{
    bool isAdd = false;
    if (utcTime == 0 || perSecond % 600 == 0)
    {
        bitSet(operationFlag, 0);
    }
    if (utcTime > 0)
    {
        utcTime += 1;
        breakTime(utcTime, rtcTime);
        //Debug::AddInfo(PSTR("Ticker: %04d-%02d-%02d %02d:%02d:%02d"), rtcTime.year, rtcTime.month, rtcTime.day_of_month, rtcTime.hour, rtcTime.minute, rtcTime.second);
    }
}

void Rtc::init()
{
    sntp_stop();
    if (globalConfig.wifi.ntp[0] != '\0')
    {
        Debug::AddInfo(PSTR("NTP Server: %s"), globalConfig.wifi.ntp);
        sntp_setservername(0, globalConfig.wifi.ntp);
    }
    else
    {
        Debug::AddInfo(PSTR("NTP Server: default"));
        sntp_setservername(0, (char *)"120.25.115.20");
        sntp_setservername(1, (char *)"203.107.6.88");
        sntp_setservername(2, (char *)"ntp3.aliyun.com");
    }
    sntp_set_timezone(8);
    sntp_init();
    utcTime = 0;
}

uint32_t Rtc::getRtcRebootCrc()
{
    uint32_t crc = 0;
    uint8_t *bytes = (uint8_t *)&rtcReboot;
    for (uint32_t i = 0; i < sizeof(RtcReboot); i++)
    {
        crc += bytes[i] * (i + 1);
    }
    return crc;
}

void Rtc::rtcRebootLoad()
{
    ESP.rtcUserMemoryRead(100 - sizeof(RtcReboot), (uint32_t *)&rtcReboot, sizeof(RtcReboot)); // 0x280
    if (rtcReboot.valid != RTC_MEM_VALID)
    {
        memset(&rtcReboot, 0, sizeof(RtcReboot));
        rtcReboot.valid = RTC_MEM_VALID;
        rtcReboot.fast_reboot_count = 0;
        rtcRebootSave();
    }
    rtcRebootCrc = getRtcRebootCrc();
}

void Rtc::rtcRebootSave()
{
    if (getRtcRebootCrc() != rtcRebootCrc)
    {
        rtcReboot.valid = RTC_MEM_VALID;
        ESP.rtcUserMemoryWrite(100 - sizeof(RtcReboot), (uint32_t *)&rtcReboot, sizeof(RtcReboot));
        rtcRebootCrc = getRtcRebootCrc();
    }
}

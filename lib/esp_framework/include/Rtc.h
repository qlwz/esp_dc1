// Ntp.h

#ifndef _NTP_h
#define _NTP_h

#include "Arduino.h"

#define LEAP_YEAR(Y) (((1970 + Y) > 0) && !((1970 + Y) % 4) && (((1970 + Y) % 100) || !((1970 + Y) % 400)))

typedef struct
{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day_of_week; // sunday is day 1
    uint8_t day_of_month;
    uint8_t month;
    uint16_t day_of_year;
    uint16_t year;
    unsigned long days;
    bool valid;
} TIME_T;

typedef struct _RtcReboot
{
    uint16_t valid;            // 280 (RTC memory offset 100 - sizeof(RTCRBT))
    uint8_t fast_reboot_count; // 282
    uint8_t free_003[1];       // 283
} RtcReboot;

const uint16_t RTC_MEM_VALID = 0xA55A;

class Rtc
{
protected:
    static uint32_t rtcRebootCrc;
    static uint8_t operationFlag;
    static void getNtp();

public:
    static void breakTime(uint32_t time_input, TIME_T &tm);
    static uint32_t utcTime;
    static RtcReboot rtcReboot;
    static TIME_T rtcTime;

    static String msToHumanString(uint32_t const msecs);
    static String timeSince(uint32_t const start);

    static String GetBuildDateAndTime();
    static void perSecondDo();
    static void init();
    static void loop();

    static uint32_t getRtcRebootCrc();
    static void rtcRebootLoad();
    static void rtcRebootSave();
};

#endif

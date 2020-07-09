// Debug.h

#ifndef _DEBUG_h
#define _DEBUG_h

#include <ESP8266WiFi.h>
#include "Config.h"

enum LoggingLevels
{
    LOG_LEVEL_NONE,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_DEBUG_MORE,
    LOG_LEVEL_ALL
};

class Debug
{
protected:
    static size_t strchrspn(const char *str1, int character);

public:
#ifdef WEB_LOG_SIZE
    static uint8_t webLogIndex;
    static char webLog[WEB_LOG_SIZE];
    static void GetLog(uint8_t idx, char **entry_pp, uint16_t *len_p);
#endif

#ifdef USE_SYSLOG
    static IPAddress ip;
    static void Syslog();
#endif
    static void AddLog(uint8_t loglevel);
    static void AddLog(uint8_t loglevel, PGM_P formatP, ...);

    static void AddInfo(PGM_P formatP, ...);
    static void AddDebug(PGM_P formatP, ...);
    static void AddError(PGM_P formatP, ...);
};

#endif

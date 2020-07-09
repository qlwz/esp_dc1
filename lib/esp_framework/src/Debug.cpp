#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "Config.h"
#include "Debug.h"
#include "Rtc.h"

#ifdef WEB_LOG_SIZE
uint8_t Debug::webLogIndex = 1;
char Debug::webLog[WEB_LOG_SIZE] = {'\0'};
#endif
#ifdef USE_SYSLOG
IPAddress Debug::ip;
#endif

WiFiUDP Udp;
size_t Debug::strchrspn(const char *str1, int character)
{
    size_t ret = 0;
    char *start = (char *)str1;
    char *end = strchr(str1, character);
    if (end)
        ret = end - start;
    return ret;
}

#ifdef WEB_LOG_SIZE
void Debug::GetLog(uint8_t idx, char **entry_pp, uint16_t *len_p)
{
    char *entry_p = NULL;
    size_t len = 0;

    if (idx)
    {
        char *it = webLog;
        do
        {
            uint8_t cur_idx = *it;
            it++;
            size_t tmp = strchrspn(it, '\1');
            tmp++; // Skip terminating '\1'
            if (cur_idx == idx)
            { // Found the requested entry
                len = tmp;
                entry_p = it;
                break;
            }
            it += tmp;
        } while (it < webLog + WEB_LOG_SIZE && *it != '\0');
    }
    *entry_pp = entry_p;
    *len_p = len;
}
#endif

#ifdef USE_SYSLOG
void Debug::Syslog()
{
    if ((2 & globalConfig.debug.type) != 2 || WiFi.status() != WL_CONNECTED || globalConfig.debug.server[0] == '\0' || globalConfig.debug.port == 0)
    {
        return;
    }

    if (!ip)
    {
        WiFi.hostByName(globalConfig.debug.server, ip);
    }
    if (Udp.beginPacket(ip, globalConfig.debug.port))
    {
        char syslog_preamble[64]; // Hostname + Id

        snprintf_P(syslog_preamble, sizeof(syslog_preamble), PSTR("%s "), UID);
        memmove(tmpData + strlen(syslog_preamble), tmpData, sizeof(tmpData) - strlen(syslog_preamble));
        tmpData[sizeof(tmpData) - 1] = '\0';
        memcpy(tmpData, syslog_preamble, strlen(syslog_preamble));
        Udp.write(tmpData, strlen(tmpData));
        Udp.endPacket();
        delay(1); // Add time for UDP handling (#5512)
    }
}
#endif

void Debug::AddLog(uint8_t loglevel)
{
    char mxtime[10]; // "13:45:21 "
    snprintf_P(mxtime, sizeof(mxtime), PSTR("%02d:%02d:%02d "), Rtc::rtcTime.hour, Rtc::rtcTime.minute, Rtc::rtcTime.second);

    if ((1 & globalConfig.debug.type) == 1)
    {
        Serial.printf(PSTR("%s%s\r\n"), mxtime, tmpData);
    }
    if ((8 & globalConfig.debug.type) == 8)
    {
        Serial1.printf(PSTR("%s%s\r\n"), mxtime, tmpData);
    }

#ifdef WEB_LOG_SIZE
    //if (Settings.webserver && (loglevel <= Settings.weblog_level))
    //{
    // Delimited, zero-terminated buffer of log lines.
    // Each entry has this format: [index][log data]['\1']
    if ((4 & globalConfig.debug.type) == 4 || loglevel == LOG_LEVEL_ERROR)
    {
        if (!webLogIndex)
            webLogIndex++;                                           // Index 0 is not allowed as it is the end of char string
        while (webLogIndex == webLog[0] ||                           // If log already holds the next index, remove it
               strlen(webLog) + strlen(tmpData) + 13 > WEB_LOG_SIZE) // 13 = web_log_index + mxtime + '\1' + '\0'
        {
            char *it = webLog;
            it++;                                              // Skip web_log_index
            it += strchrspn(it, '\1');                         // Skip log line
            it++;                                              // Skip delimiting "\1"
            memmove(webLog, it, WEB_LOG_SIZE - (it - webLog)); // Move buffer forward to remove oldest log line
        }
        snprintf_P(webLog, sizeof(webLog), PSTR("%s%c%s%s\1"), webLog, webLogIndex++, mxtime, tmpData);
        if (!webLogIndex)
            webLogIndex++; // Index 0 is not allowed as it is the end of char string
    }
#endif

#ifdef USE_SYSLOG
    Syslog();
#endif
}

void Debug::AddLog(uint8_t loglevel, PGM_P formatP, ...)
{
    va_list arg;
    va_start(arg, formatP);
    vsnprintf_P(tmpData, sizeof(tmpData), formatP, arg);
    va_end(arg);

    AddLog(loglevel);
}

void Debug::AddInfo(PGM_P formatP, ...)
{
    va_list arg;
    va_start(arg, formatP);
    vsnprintf_P(tmpData, sizeof(tmpData), formatP, arg);
    va_end(arg);

    AddLog(LOG_LEVEL_INFO);
}

void Debug::AddDebug(PGM_P formatP, ...)
{
    va_list arg;
    va_start(arg, formatP);
    vsnprintf_P(tmpData, sizeof(tmpData), formatP, arg);
    va_end(arg);

    AddLog(LOG_LEVEL_DEBUG);
}

void Debug::AddError(PGM_P formatP, ...)
{
    va_list arg;
    va_start(arg, formatP);
    vsnprintf_P(tmpData, sizeof(tmpData), formatP, arg);
    va_end(arg);

    AddLog(LOG_LEVEL_ERROR);
}

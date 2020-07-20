// Wifi.h

#ifndef _WIFI_h
#define _WIFI_h

#include <WiFiClient.h>
#include <DNSServer.h>
#include "Arduino.h"

//#define WIFI_CONNECT_TIMEOUT 300
#ifndef WIFI_PORTAL_TIMEOUT
#define WIFI_PORTAL_TIMEOUT 300
#endif
#define MinimumWifiSignalQuality 8

class Wifi
{
private:
    static bool connect;

    static String _ssid;
    static String _pass;

    static DNSServer *dnsServer;
#ifdef WIFI_CONNECT_TIMEOUT
    static unsigned long connectStart;
#endif

public:
    static unsigned long configPortalStart;
    static bool isDHCP;
    static WiFiEventHandler STAGotIP;
    //static WiFiEventHandler STADisconnected;
    static WiFiClient wifiClient;
    static void connectWifi();
    static void setupWifi();
    static void setupWifiManager(bool resetSettings);
    static bool isIp(String str);

    static uint8_t waitForConnectResult();
    static void tryConnect(String ssid, String pass);

    static void loop();
};

#endif

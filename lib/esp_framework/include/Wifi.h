// Wifi.h

#ifndef _WIFI_h
#define _WIFI_h

#include <WiFiClient.h>
#include <DNSServer.h>
#include "Arduino.h"

#define ConnectTimeOut 300
#define ConfigPortalTimeOut 120
#define MinimumWifiSignalQuality 8

class Wifi
{
private:
    static bool connect;

    static String _ssid;
    static String _pass;

    static DNSServer *dnsServer;
    static unsigned long connectStart;

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

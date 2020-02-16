// Http.h

#ifndef _HTTP_h
#define _HTTP_h

#include "Arduino.h"
#include <ESP8266WebServer.h>
#include <DNSServer.h>

class Http
{
private:
    static bool isBegin;
    static void handleRoot();
    static void handleMqtt();
    static void handledhcp();
    static void handleScanWifi();
    static void handleWifi();
    static void handleDiscovery();
    static void handleRestart();
    static void handleReset();
    static void handleNotFound();
    static void handleModuleSetting();
    static void handleOTA();
    static void handleGetStatus();
    static void handleUpdate();
    static bool checkAuth();
    static String updaterError;

public:
    static ESP8266WebServer *server;
    static void begin();
    static void stop();
    static void loop();
    static bool captivePortal();

    static void OTA(String url);
};

#endif

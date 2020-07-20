// Http.h

#ifndef _HTTP_h
#define _HTTP_h

#include "Arduino.h"
#include <ESP8266WebServer.h>

class Http
{
private:
    static bool isBegin;
    static void handleRoot();
#ifndef DISABLE_MQTT
    static void handleMqtt();
#ifndef DISABLE_MQTT_DISCOVERY
    static void handleDiscovery();
#endif
#endif
    static void handledhcp();
    static void handleScanWifi();
    static void handleWifi();
    static void handleOperate();
    static void handleNotFound();
    static void handleModuleSetting();
    static void handleOTA();
    static void handleGetStatus();
    static void handleUpdate();
    static bool checkAuth();

public:
    static ESP8266WebServer *server;
    static void begin();
    static void stop();
    static void loop();
    static bool captivePortal();

    static void OTA(String url);
};

#endif

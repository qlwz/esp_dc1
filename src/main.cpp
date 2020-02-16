#include <Arduino.h>
#include <ESP8266mDNS.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPUpdateServer.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>

#include "Framework.h"
#include "DC1.h"

void setup()
{
    Framework::one(115200);

    module = new DC1();

    Framework::setup();
}

void loop()
{
    Framework::loop();
}
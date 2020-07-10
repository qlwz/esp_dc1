#include <Arduino.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WiFi.h>

#include "Framework.h"
#include "DC1.h"

#ifdef USE_HOMEKIT
#include "homekit.h"
#endif

void setup()
{
    Framework::one(115200);

    module = new DC1();

    Framework::setup();

#ifdef USE_HOMEKIT
    homekit_init();
#endif
}

void loop()
{
#ifdef USE_HOMEKIT
    homekit_loop();
#else
    Framework::loop();
#endif
}
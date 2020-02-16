
// Framework.h

#ifndef _FRAMEWORK_h
#define _FRAMEWORK_h

#include "Arduino.h"

class Framework
{
    static uint16_t rebootCount;
    static void callback(char *topic, byte *payload, unsigned int length);
    static void connectedCallback();
    static void tickerPerSecondDo();

public:
    static void one(unsigned long baud);
    static void setup();
    static void loop();
};

#endif
#include <ESP8266WiFi.h>
#include "Led.h"
#include "Module.h"

Ticker *Led::ledTicker;
Ticker *Led::ledTicker2;
uint8_t Led::io = 99;
uint8_t Led::light;
uint8_t Led::ledType = 0;
bool Led::isOn = false;

void Led::init(uint8_t _io, uint8_t _light)
{
    io = _io;
    light = _light;
    pinMode(io, OUTPUT);

    Led::ledType = 0;
    ledTicker = new Ticker();
    ledTicker2 = new Ticker();

    off();
}

void Led::loop()
{
    if (io == 99)
    {
        return;
    }
    if (module && module->moduleLed())
    {
        if (ledTicker->active())
        {
            ledTicker->detach();
        }
        Led::ledType = 3;
    }
    else if (WiFi.status() != WL_CONNECTED)
    {
        if (Led::ledType != 0)
        {
            Led::ledType = 0;
            ledTicker->attach(0.2, []() { toggle(); });
        }
    }
    else if (!Mqtt::mqttClient.connected())
    {
        if (Led::ledType != 1)
        {
            Led::ledType = 1;
            ledTicker->attach(0.3, []() { toggle(); });
        }
    }
    else
    {
        if (Led::ledType != 2)
        {
            Led::ledType = 2;
            ledTicker->attach(5, led, 200);
        }
    }
}

void Led::on()
{
    if (io != 99 && !isOn)
    {
        isOn = true;
        digitalWrite(io, light);
    }
}

void Led::off()
{
    if (io != 99 && isOn)
    {
        isOn = false;
        digitalWrite(io, !light);
    }
}

void Led::toggle()
{
    isOn ? off() : on();
}

void Led::led(int ms)
{
    if (io != 99)
    {
        on();
        ledTicker2->once_ms(ms, []() { off(); });
    }
}

void Led::blinkLED(int duration, int n)
{
    if (io == 99)
    {
        return;
    }
    for (int i = 0; i < n; i++)
    {
        on();
        delay(duration);
        off();
        if (n != i + 1)
        {
            delay(duration);
        }
    }
}

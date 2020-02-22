#include <EEPROM.h>
#include "Framework.h"
#include "Module.h"
#include "Rtc.h"
#include "Http.h"
#include "Util.h"

uint16_t Framework::rebootCount = 0;
void Framework::callback(char *topic, byte *payload, unsigned int length)
{
    String str;
    for (int i = 0; i < length; i++)
    {
        str += (char)payload[i];
    }

    Debug::AddInfo(PSTR("Subscribe: %s payload: %s"), topic, str.c_str());

    String topicStr = String(topic);
    if (topicStr.endsWith(F("/OTA")))
    {
        Http::OTA(str.endsWith(F(".bin")) ? str : OTA_URL);
    }
    else if (topicStr.endsWith(F("/restart")))
    {
        ESP.reset();
    }
    else if (module)
    {
        module->mqttCallback(topicStr, str);
    }

    Led::led(200);
}

void Framework::connectedCallback()
{
    Mqtt::subscribe(Mqtt::getCmndTopic(F("#")));
    Led::blinkLED(40, 8);
    if (module)
    {
        module->mqttConnected();
    }
}

void Framework::tickerPerSecondDo()
{
    perSecond++;
    if (perSecond == 30)
    {
        Rtc::rtcReboot.fast_reboot_count = 0;
        Rtc::rtcRebootSave();
    }
    if (rebootCount == 3)
    {
        return;
    }
    Rtc::perSecondDo();

    Config::perSecondDo();
    Mqtt::perSecondDo();
    module->perSecondDo();
}

void Framework::one(unsigned long baud)
{
    Rtc::rtcRebootLoad();
    Rtc::rtcReboot.fast_reboot_count++;
    Rtc::rtcRebootSave();
    rebootCount = Rtc::rtcReboot.fast_reboot_count > BOOT_LOOP_OFFSET ? Rtc::rtcReboot.fast_reboot_count - BOOT_LOOP_OFFSET : 0;

    Serial.begin(baud);
    EEPROM.begin(GlobalConfigMessage_size + 6);
    globalConfig.debug.type = 1;
}

void Framework::setup()
{
    Debug::AddError(PSTR("---------------------  v%s  %s  -------------------"), module->getModuleVersion().c_str(), Rtc::GetBuildDateAndTime().c_str());
    if (rebootCount == 1)
    {
        Config::readConfig();
        module->resetConfig();
    }
    else if (rebootCount == 2)
    {
        Config::readConfig();
        module->resetConfig();
    }
    else
    {
        Config::readConfig();
    }
    if (globalConfig.uid[0] != '\0')
    {
        strcpy(UID, globalConfig.uid);
    }
    else
    {
        String mac = WiFi.macAddress();
        mac.replace(":", "");
        mac = mac.substring(6, 12);
        sprintf(UID, "%s_%s", module->getModuleName().c_str(), mac.c_str());
    }
    Util::strlowr(UID);

    Debug::AddInfo(PSTR("UID: %s"), UID);
    // Debug::AddInfo(PSTR("Config Len: %d"), GlobalConfigMessage_size + 6);

    //Config::resetConfig();
    if (MQTT_MAX_PACKET_SIZE == 128)
    {
        Debug::AddError(PSTR("WRONG PUBSUBCLIENT LIBRARY USED PLEASE INSTALL THE ONE FROM OMG LIB FOLDER"));
    }

    if (rebootCount == 3)
    {
        module = NULL;

        tickerPerSecond = new Ticker();
        tickerPerSecond->attach(1, tickerPerSecondDo);

        Http::begin();
        Wifi::connectWifi();
    }
    else
    {
        Mqtt::setClient(Wifi::wifiClient);
        Mqtt::mqttSetConnectedCallback(connectedCallback);
        Mqtt::mqttSetLoopCallback(callback);
        module->init();
        tickerPerSecond = new Ticker();
        tickerPerSecond->attach(1, tickerPerSecondDo);
        Http::begin();
        Wifi::connectWifi();
        Rtc::init();
    }
}

void Framework::loop()
{
    if (rebootCount == 3)
    {
        Wifi::loop();
        Http::loop();
    }
    else
    {
        yield();
        Led::loop();
        yield();
        Mqtt::loop();
        yield();
        module->loop();
        yield();
        Wifi::loop();
        yield();
        Http::loop();
        yield();
        Rtc::loop();
    }
}

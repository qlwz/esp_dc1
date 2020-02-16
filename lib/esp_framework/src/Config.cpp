#include <EEPROM.h>
#include "Module.h"

Module *module;
char UID[16];
char tmpData[512] = {0};
uint32_t perSecond;
Ticker *tickerPerSecond;
GlobalConfigMessage globalConfig;

uint16_t Config::nowCrc;
uint8_t Config::countdown = 60;
bool Config::isDelay = false;

const uint16_t crcTalbe[] = {
    0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
    0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400};

/**
 * 计算Crc16
 */
uint16_t Config::crc16(uint8_t *ptr, uint16_t len)
{
    uint16_t crc = 0xffff;
    for (uint16_t i = 0; i < len; i++)
    {
        const uint8_t ch = *ptr++;
        crc = crcTalbe[(ch ^ crc) & 15] ^ (crc >> 4);
        crc = crcTalbe[((ch >> 4) ^ crc) & 15] ^ (crc >> 4);
    }
    return crc;
}

void Config::resetConfig()
{
    Debug::AddInfo(PSTR("resetConfig . . . OK"));
    memset(&globalConfig, 0, sizeof(GlobalConfigMessage));

#ifdef WIFI_SSID
    strcpy(globalConfig.wifi.ssid, WIFI_SSID);
#endif
#ifdef WIFI_PASS
    strcpy(globalConfig.wifi.pass, WIFI_PASS);

#endif
#ifdef MQTT_SERVER
    strcpy(globalConfig.mqtt.server, MQTT_SERVER);
#endif
#ifdef MQTT_PORT
    globalConfig.mqtt.port = MQTT_PORT;
#endif
#ifdef MQTT_USER
    strcpy(globalConfig.mqtt.user, MQTT_USER);
#endif
#ifdef MQTT_PASS
    strcpy(globalConfig.mqtt.pass, MQTT_PASS);
#endif
    globalConfig.mqtt.discovery = false;
    strcpy(globalConfig.mqtt.discovery_prefix, "homeassistant");

#ifdef MQTT_FULLTOPIC
    strcpy(globalConfig.mqtt.topic, MQTT_FULLTOPIC);
#endif
#ifdef OTA_URL
    strcpy(globalConfig.http.ota_url, OTA_URL);
#endif
    globalConfig.http.port = 80;
    globalConfig.debug.type = 1;

    if (module)
    {
        module->resetConfig();
    }
}

void Config::readConfig()
{
    uint16 len;
    bool status = false;
    uint16 cfg = (EEPROM.read(0) << 8 | EEPROM.read(1));
    if (cfg == GLOBAL_CFG_VERSION)
    {
        len = (EEPROM.read(2) << 8 | EEPROM.read(3));
        nowCrc = (EEPROM.read(4) << 8 | EEPROM.read(5));

        if (len > GlobalConfigMessage_size)
        {
            len = GlobalConfigMessage_size;
        }

        uint16_t crc = 0xffff;
        uint8_t buffer[GlobalConfigMessage_size];
        for (uint16_t i = 0; i < len; ++i)
        {
            buffer[i] = EEPROM.read(i + 6);
            crc = crcTalbe[(buffer[i] ^ crc) & 15] ^ (crc >> 4);
            crc = crcTalbe[((buffer[i] >> 4) ^ crc) & 15] ^ (crc >> 4);
        }
        if (crc == nowCrc)
        {
            memset(&globalConfig, 0, sizeof(GlobalConfigMessage));
            pb_istream_t stream = pb_istream_from_buffer(buffer, len);
            status = pb_decode(&stream, GlobalConfigMessage_fields, &globalConfig);
            if (globalConfig.http.port == 0)
            {
                globalConfig.http.port = 80;
            }
        }
    }

    if (!status)
    {
        globalConfig.debug.type = 1;
        Debug::AddError(PSTR("readConfig . . . Error"));
        resetConfig();
    }
    else
    {
        if (module)
        {
            module->readConfig();
        }
        Debug::AddInfo(PSTR("readConfig       . . . OK Len: %d"), len);
    }
}

bool Config::saveConfig(bool isEverySecond)
{
    countdown = 60;
    if (module)
    {
        module->saveConfig(isEverySecond);
    }
    uint8_t buffer[GlobalConfigMessage_size];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    bool status = pb_encode(&stream, GlobalConfigMessage_fields, &globalConfig);
    size_t len = stream.bytes_written;
    if (!status)
    {
        Debug::AddError(PSTR("saveConfig . . . Error"));
        return false;
    }
    else
    {
        uint16_t crc = crc16(buffer, len);
        if (crc == nowCrc)
        {
            // Debug::AddInfo(PSTR("Check Config CRC . . . Same"));
            return true;
        }
        else
        {
            nowCrc = crc;
        }
    }

    EEPROM.write(0, GLOBAL_CFG_VERSION >> 8);
    EEPROM.write(1, GLOBAL_CFG_VERSION);

    EEPROM.write(2, len >> 8);
    EEPROM.write(3, len);

    EEPROM.write(4, nowCrc >> 8);
    EEPROM.write(5, nowCrc);

    for (uint16_t i = 0; i < len; i++)
    {
        EEPROM.write(i + 6, buffer[i]);
    }
    EEPROM.commit();

    Debug::AddInfo(PSTR("saveConfig . . . OK Len: %d"), len);
    return true;
}

void Config::perSecondDo()
{
    countdown--;
    if (countdown == 0)
    {
        saveConfig(Config::isDelay ? false : true);
        Config::isDelay = false;
    }
}

void Config::delaySaveConfig(uint8_t second)
{
    Config::isDelay = true;
    if (countdown > second)
    {
        countdown = second;
    }
}

void Config::moduleReadConfig(uint16_t version, uint16_t size, const pb_field_t fields[], void *dest_struct)
{
    if (globalConfig.module_cfg.size == 0                                                                         // 没有数据
        || globalConfig.cfg_version != version                                                                    // 版本不一致
        || globalConfig.module_crc != Config::crc16(globalConfig.module_cfg.bytes, globalConfig.module_cfg.size)) // crc错误
    {
        Debug::AddError(PSTR("moduleReadConfig . . . Error %d %d %d"), globalConfig.cfg_version, version, globalConfig.module_cfg.size);
        if (module)
        {
            module->resetConfig();
        }
        return;
    }
    memset(dest_struct, 0, size);
    pb_istream_t stream = pb_istream_from_buffer(globalConfig.module_cfg.bytes, globalConfig.module_cfg.size);
    bool status = pb_decode(&stream, fields, dest_struct);
    if (!status) // 解密失败
    {
        if (module)
        {
            module->resetConfig();
        }
    }
    else
    {
        Debug::AddInfo(PSTR("moduleReadConfig . . . OK Len: %d"), globalConfig.module_cfg.size);
    }
}

bool Config::moduleSaveConfig(uint16_t version, uint16_t size, const pb_field_t fields[], const void *src_struct)
{
    uint8_t buffer[size];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    bool status = pb_encode(&stream, fields, src_struct);
    if (status)
    {
        size_t len = stream.bytes_written;
        uint16_t crc = Config::crc16(buffer, len);
        if (crc != globalConfig.module_crc)
        {
            globalConfig.cfg_version = version;
            globalConfig.module_crc = crc;
            globalConfig.module_cfg.size = len;
            memcpy(globalConfig.module_cfg.bytes, buffer, len);
            //Debug::AddInfo(PSTR("moduleSaveConfig . . . OK Len: %d"), len);
        }
    }
    return status;
}
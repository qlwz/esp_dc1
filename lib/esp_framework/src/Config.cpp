#include <c_types.h>
#include <spi_flash.h>
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

extern uint32_t _EEPROM_start; //See EEPROM.cpp
#define EEPROM_PHYS_ADDR ((uint32_t)(&_EEPROM_start) - 0x40200000)

/**
 * 计算Crc16
 */
uint16_t Config::crc16(uint8_t *ptr, uint16_t len)
{
    uint16_t crc = 0xffff;
    for (uint16_t i = 0; i < len; i++)
    {
        crc = crcTalbe[(ptr[i] ^ crc) & 15] ^ (crc >> 4);
        crc = crcTalbe[((ptr[i] >> 4) ^ crc) & 15] ^ (crc >> 4);
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
    uint8_t buf[6] = {0};
    if (spi_flash_read(EEPROM_PHYS_ADDR, (uint32 *)buf, 6) != SPI_FLASH_RESULT_OK)
    {
    }

    uint16 len;
    bool status = false;
    uint16 cfg = (buf[0] << 8 | buf[1]);
    if (cfg == GLOBAL_CFG_VERSION)
    {
        len = (buf[2] << 8 | buf[3]);
        nowCrc = (buf[4] << 8 | buf[5]);

        if (len > GlobalConfigMessage_size)
        {
            len = GlobalConfigMessage_size;
        }
        //Debug::AddInfo(PSTR("readConfig . . . Len: %d Crc: %d"), len, nowCrc);

        uint8_t *data = (uint8_t *)malloc(len);
        if (spi_flash_read(EEPROM_PHYS_ADDR + 6, (uint32 *)data, len) == SPI_FLASH_RESULT_OK)
        {
            uint16_t crc = crc16(data, len);
            if (crc == nowCrc)
            {
                memset(&globalConfig, 0, sizeof(GlobalConfigMessage));
                pb_istream_t stream = pb_istream_from_buffer(data, len);
                status = pb_decode(&stream, GlobalConfigMessage_fields, &globalConfig);
                if (globalConfig.http.port == 0)
                {
                    globalConfig.http.port = 80;
                }
            }
            else
            {
                Debug::AddError(PSTR("readConfig . . . Error Crc: %d Crc: %d"), crc, nowCrc);
            }
        }
        free(data);
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

    uint8_t *data = (uint8_t *)malloc(len + 6);
    if (spi_flash_erase_sector(EEPROM_PHYS_ADDR / SPI_FLASH_SEC_SIZE) != SPI_FLASH_RESULT_OK)
    {
        free(data);
        Debug::AddError(PSTR("saveConfig . . . Erase Sector Error"));
        return false;
    }

    data[0] = GLOBAL_CFG_VERSION >> 8;
    data[1] = GLOBAL_CFG_VERSION;

    data[2] = len >> 8;
    data[3] = len;

    data[4] = nowCrc >> 8;
    data[5] = nowCrc;

    memcpy(&data[6], buffer, len);

    if (spi_flash_write(EEPROM_PHYS_ADDR, (uint32 *)data, len + 6) != SPI_FLASH_RESULT_OK)
    {
        free(data);
        Debug::AddError(PSTR("saveConfig . . . Write Data Error"));
        return false;
    }
    free(data);

    Debug::AddInfo(PSTR("saveConfig . . . OK Len: %d Crc: %d"), len, nowCrc);
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

#include "DC1.h"
#include "Rtc.h"
#include "Util.h"

#pragma region 继承

void DC1::init()
{
    Led::init(LED_PIN, LOW);
    cat9554 = new CAT9554(CAT9554_SDA_PIN, CAT9554_SCL_PIN);
    cat9554->setIrqPin(CAT9554_IRQ_PIN);
    cat9554->setup();

    cse7766 = new CSE7766(CSE7766_RX_PIN, CSE7766_BAUDRATE);

    // 按键
    pinMode(KEY_0_PIN, INPUT_PULLDOWN_16);
    cat9554->pinMode(KEY_1_PIN, INPUT);
    cat9554->pinMode(KEY_2_PIN, INPUT);
    cat9554->pinMode(KEY_3_PIN, INPUT);

    // 继电器
    cat9554->pinMode(REL_0_PIN, OUTPUT);
    cat9554->pinMode(REL_1_PIN, OUTPUT);
    cat9554->pinMode(REL_2_PIN, OUTPUT);
    cat9554->pinMode(REL_3_PIN, OUTPUT);

    pinMode(LOGO_LED_PIN, OUTPUT);
    logoLed();

    channels = 4;
    for (uint8_t ch = 0; ch < channels; ch++)
    {
        // 0:开关通电时断开  1 : 开关通电时闭合  2 : 开关通电时状态与断电前相反  3 : 开关通电时保持断电前状态
        if (config.power_on_state == 2)
        {
            switchRelay(ch, !bitRead(config.last_state, ch), false); // 开关通电时状态与断电前相反
        }
        else if (config.power_on_state == 3)
        {
            switchRelay(ch, bitRead(config.last_state, ch), false); // 开关通电时保持断电前状态
        }
        else
        {
            switchRelay(ch, config.power_on_state == 1, false); // 开关通电时闭合
        }
        // 总开关关时跳过其他
        if (ch == 0 && !lastState[0] && config.sub_kinkage != 0)
        {
            break;
        }
    }
    energyInit();
}

bool DC1::moduleLed()
{
    if (WiFi.status() == WL_CONNECTED && Mqtt::mqttClient.connected())
    {
        if (config.wifi_led == 0)
        {
            Led::on();
            return true;
        }
        else if (config.wifi_led == 1)
        {
            Led::off();
            return true;
        }
    }
    return false;
}

void DC1::loop()
{
    cse7766->loop();
    for (size_t ch = 0; ch < channels; ch++)
    {
        checkButton(ch);
    }

    if (bitRead(operationFlag, 0))
    {
        bitClear(operationFlag, 0);
        energyUpdate();
    }
}

void DC1::perSecondDo()
{
    bitSet(operationFlag, 0);
}
#pragma endregion

#pragma region 配置

void DC1::readConfig()
{
    Config::moduleReadConfig(MODULE_CFG_VERSION, sizeof(DC1ConfigMessage), DC1ConfigMessage_fields, &config);
}

void DC1::resetConfig()
{
    Debug::AddInfo(PSTR("moduleResetConfig . . . OK"));
    memset(&config, 0, sizeof(DC1ConfigMessage));

    config.power_on_state = 3;
    config.power_mode = 0;
    config.logo_led = 0;
    config.wifi_led = 0;
    config.sub_kinkage = 2;
    config.energy_power_delta = 10;
    config.report_interval = 60;
}

void DC1::saveConfig(bool isEverySecond)
{
    if (bitRead(operationFlag, 1) || !isEverySecond)
    {
        bitClear(operationFlag, 1);
        energySync();
    }
    Config::moduleSaveConfig(MODULE_CFG_VERSION, DC1ConfigMessage_size, DC1ConfigMessage_fields, &config);
}
#pragma endregion

#pragma region MQTT

void DC1::mqttCallback(String topicStr, String str)
{
    if (channels >= 1 && topicStr.endsWith("/POWER") || topicStr.endsWith("/POWER1"))
    {
        switchRelay(0, (str == "ON" ? true : (str == "OFF" ? false : !DC1::lastState[0])));
    }
    else if (channels >= 2 && topicStr.endsWith("/POWER2"))
    {
        switchRelay(1, (str == "ON" ? true : (str == "OFF" ? false : !DC1::lastState[1])));
    }
    else if (channels >= 3 && topicStr.endsWith("/POWER3"))
    {
        switchRelay(2, (str == "ON" ? true : (str == "OFF" ? false : !DC1::lastState[2])));
    }
    else if (channels >= 4 && topicStr.endsWith("/POWER4"))
    {
        switchRelay(3, (str == "ON" ? true : (str == "OFF" ? false : !DC1::lastState[3])));
    }
    else if (topicStr.endsWith("/clear"))
    {
        energyClear();
    }
    else if (topicStr.endsWith("/report"))
    {
        reportPower();
        reportEnergy();
    }
}

void DC1::mqttConnected()
{
    powerTopic = Mqtt::getStatTopic(F("POWER"));
    if (globalConfig.mqtt.discovery)
    {
        mqttDiscovery(true);
    }

    reportPower();
    reportEnergy();
}

void DC1::mqttDiscovery(bool isEnable)
{
    char topic[50];
    char message[500];

    String tmp = Mqtt::getCmndTopic(F("POWER"));
    String availability = Mqtt::getTeleTopic(F("availability"));
    for (size_t ch = 0; ch < channels; ch++)
    {
        sprintf(topic, "%s/switch/%s_%d/config", globalConfig.mqtt.discovery_prefix, UID, (ch + 1));
        if (isEnable)
        {
            sprintf(message, HASS_DISCOVER_DC1_SWICH, UID, (ch + 1),
                    (tmp + (ch + 1)).c_str(),
                    (powerTopic + (ch + 1)).c_str(),
                    availability.c_str());
            Mqtt::publish(topic, message, true);
            //Debug::AddInfo(PSTR("discovery: %s - %s"), topic, message);
        }
        else
        {
            Mqtt::publish(topic, "", true);
        }
    }

    String tims[] = {"voltage", "current", "power", "apparent_power", "reactive_power", "factor", "total", "yesterday", "today", "starttime"};
    String tims2[] = {"V", "A", "W", "VA", "VAr", "", "kWh", "kWh", "kWh", ""};
    String energy = Mqtt::getTeleTopic(F("ENERGY"));
    for (size_t i = 0; i < 10; i++)
    {
        sprintf(topic, "%s/sensor/%s_%s/config", globalConfig.mqtt.discovery_prefix, UID, tims[i].c_str());
        if (isEnable)
        {
            if (tims2[i].length() == 0)
            {
                sprintf(message, HASS_DISCOVER_DC1_SENSOR_WITHOUT_UNIT,
                        UID, tims[i].c_str(),
                        energy.c_str(),
                        tims[i].c_str());
            }
            else
            {
                sprintf(message, HASS_DISCOVER_DC1_SENSOR,
                        UID, tims[i].c_str(),
                        energy.c_str(),
                        tims[i].c_str(),
                        tims2[i].c_str());
            }
            Mqtt::publish(topic, message, true);
            //Debug::AddInfo(PSTR("discovery: %s - %s"), topic, message);
        }
        else
        {
            Mqtt::publish(topic, "", true);
        }
    }
    if (isEnable)
    {
        Mqtt::availability();
        reportPower();
        reportEnergy();
    }
}
#pragma endregion

#pragma region Http

void DC1::httpAdd(ESP8266WebServer *server)
{
    server->on(F("/dc1_do"), std::bind(&DC1::httpDo, this, server));
    server->on(F("/dc1_setting"), std::bind(&DC1::httpSetting, this, server));
    server->on(F("/ha"), std::bind(&DC1::httpHa, this, server));
}

String DC1::httpGetStatus(ESP8266WebServer *server)
{
    String data;
    for (size_t ch = 0; ch < channels; ch++)
    {
        data += ",\"POWER" + String(ch + 1) + "\":";
        data += lastState[ch] ? 1 : 0;
    }
    energyShow(false);
    data += String(tmpData);
    return data.substring(1);
}

void DC1::httpHtml(ESP8266WebServer *server)
{
    String radioJs = F("<script type='text/javascript'>");
    radioJs += F("function setDataSub(data,key){if(key.substr(0,5)=='POWER'){var t=id(key);var v=data[key];t.setAttribute('class',v==1?'btn-success':'btn-info');t.innerHTML=v==1?'开':'关';return true}return false}");
    String page = F("<table class='gridtable'><thead><tr><th colspan='2'>开关状态</th></tr></thead><tbody>");
    page += F("<tr colspan='2' style='text-align:center'><td>");
    for (size_t ch = 0; ch < channels; ch++)
    {
        page += F(" <button type='button' style='width:50px' onclick=\"ajaxPost('/dc1_do', 'do=T&c={ch}');\" id='POWER{ch}' ");
        page.replace(F("{ch}"), String(ch + 1));
        if (lastState[ch])
        {
            page += F("class='btn-success'>开</button>");
        }
        else
        {
            page += F("class='btn-info'>关</button>");
        }
    }
    page += F("</td></tr></tbody></table>");

    page += F("<table class='gridtable'><thead><tr><th colspan='2'>电量统计</th></tr></thead><tbody>");
    page += F("<tr colspan='2'><td><div style='width:260px;margin:0 auto;text-align:left'>");
    page += F("&#12288;&#12288;&#12288;电压：<span id='voltage'>0</span> V");
    page += F("<br>&#12288;&#12288;&#12288;电流：<span id='current'>0</span> A");
    page += F("<br>&#12288;&#12288;&#12288;功率：<span id='power'>0</span> W");
    page += F("<br>&#12288;视在功率：<span id='apparent_power'>0</span> VA");
    page += F("<br>&#12288;无功功率：<span id='reactive_power'></span> VAr");
    page += F("<br>&#12288;功率因数：<span id='factor'>0</span>");
    page += F("<br>今日用电量：<span id='today'>0</span> kWh");
    page += F("<br>昨日用电量：<span id='yesterday'>0</span> kWh");
    page += F("<br>&#12288;总用电量：<span id='total'>0</span> kWh");
    page += F("<br>&#12288;开始时间：{v}");
    page += F("</div></td></tr></tbody></table>");
    page.replace(F("{v}"), kWhtotalTime);

    page += F("<form method='post' action='/dc1_setting' onsubmit='postform(this);return false'>");
    page += F("<table class='gridtable'><thead><tr><th colspan='2'>DC1插线板设置</th></tr></thead><tbody>");
    page += F("<tr><td>上电状态</td><td>");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_on_state' value='0'/><i class='bui-radios'></i> 开关通电时断开</label><br/>");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_on_state' value='1'/><i class='bui-radios'></i> 开关通电时闭合</label><br/>");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_on_state' value='2'/><i class='bui-radios'></i> 开关通电时状态与断电前相反</label><br/>");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_on_state' value='3'/><i class='bui-radios'></i> 开关通电时保持断电前状态</label>");
    page += F("</td></tr>");
    radioJs += F("setRadioValue('power_on_state', '{v}');");
    radioJs.replace(F("{v}"), String(config.power_on_state));

    page += F("<tr><td>开关模式</td><td>");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_mode' value='0'/><i class='bui-radios'></i> 自锁</label>&nbsp;&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='power_mode' value='1'/><i class='bui-radios'></i> 互锁</label>");
    page += F("</td></tr>");
    radioJs += F("setRadioValue('power_mode', '{v}');");
    radioJs.replace(F("{v}"), String(config.power_mode));

    page += F("<tr><td>LOGO LED</td><td>");
    page += F("<label class='bui-radios-label'><input type='radio' name='logo_led' value='0'/><i class='bui-radios'></i> 常亮</label>&nbsp;&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='logo_led' value='1'/><i class='bui-radios'></i> 常灭</label>&nbsp;&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='logo_led' value='2'/><i class='bui-radios'></i> 跟随总开关</label>&nbsp;&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='logo_led' value='3'/><i class='bui-radios'></i> 与总开关相反</label>");
    page += F("</td></tr>");
    radioJs += F("setRadioValue('logo_led', '{v}');");
    radioJs.replace(F("{v}"), String(config.logo_led));

    page += F("<tr><td>WIFI LED</td><td>");
    page += F("<label class='bui-radios-label'><input type='radio' name='wifi_led' value='0'/><i class='bui-radios'></i> 常亮</label>&nbsp;&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='wifi_led' value='1'/><i class='bui-radios'></i> 常灭</label>&nbsp;&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='wifi_led' value='2'/><i class='bui-radios'></i> 闪烁</label><br>未连接WIFI或者MQTT时为快闪");
    page += F("</td></tr>");
    radioJs += F("setRadioValue('wifi_led', '{v}');");
    radioJs.replace(F("{v}"), String(config.wifi_led));

    page += F("<tr><td>分开关联动</td><td>");
    page += F("<label class='bui-radios-label'><input type='radio' name='sub_kinkage' value='0'/><i class='bui-radios'></i> 不联动</label>&nbsp;&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='sub_kinkage' value='1'/><i class='bui-radios'></i> 总关禁开</label>&nbsp;&nbsp;&nbsp;&nbsp;");
    page += F("<label class='bui-radios-label'><input type='radio' name='sub_kinkage' value='2'/><i class='bui-radios'></i> 分开总开</label>");
    page += F("</td></tr>");
    radioJs += F("setRadioValue('sub_kinkage', '{v}');");
    radioJs.replace(F("{v}"), String(config.sub_kinkage));

    page += F("<tr><td>主动上报间隔</td><td><input type='number' min='0' max='3600' name='report_interval' required value='{v}'>&nbsp;秒，0关闭</td></tr>");
    page.replace(F("{v}"), String(config.report_interval));

    page += F("<tr><td>功率波动</td><td><input type='number' min='0' max='4000' name='energy_power_delta' required value='{v}'>&nbsp;0关闭，1-100为%，>100是差值(-100)</td></tr>");
    page.replace(F("{v}"), String(config.energy_power_delta));

    page += F("<tr><td colspan='2'><button type='submit' class='btn-info'>设置</button></td></tr>");
    page += F("<tr><td colspan='2'><button type='button' class='btn-success' onclick='window.location.href=\"/ha\"'>下载HA配置文件</button></td></tr>");
    page += F("</tbody></table></form>");
    radioJs += F("</script>");

    server->sendContent(page);
    server->sendContent(radioJs);
}

void DC1::httpDo(ESP8266WebServer *server)
{
    String c = server->arg(F("c"));
    if (c != F("1") && c != F("2") && c != F("3") && c != F("4"))
    {
        server->send(200, F("text/html"), F("{\"code\":0,\"msg\":\"参数错误。\"}"));
        return;
    }
    uint8_t ch = c.toInt() - 1;
    if (ch > channels)
    {
        server->send(200, F("text/html"), F("{\"code\":0,\"msg\":\"继电器数量错误。\"}"));
        return;
    }
    String str = server->arg(F("do"));
    switchRelay(ch, (str == "ON" ? true : (str == "OFF" ? false : !DC1::lastState[ch])));

    server->send(200, F("text/html"), "{\"code\":1,\"msg\":\"操作成功\",\"data\":{" + httpGetStatus(server) + "}}");
}

void DC1::httpSetting(ESP8266WebServer *server)
{
    config.power_on_state = server->arg(F("power_on_state")).toInt();
    config.power_mode = server->arg(F("power_mode")).toInt();
    config.logo_led = server->arg(F("logo_led")).toInt();
    config.wifi_led = server->arg(F("wifi_led")).toInt();
    config.sub_kinkage = server->arg(F("sub_kinkage")).toInt();

    config.report_interval = server->arg(F("report_interval")).toInt();
    config.energy_power_delta = server->arg(F("energy_power_delta")).toFloat();

    logoLed();

    Config::saveConfig();
    server->send(200, F("text/html"), F("{\"code\":1,\"msg\":\"已经设置成功。\"}"));
}

void DC1::httpHa(ESP8266WebServer *server)
{
    char attachment[100];
    snprintf_P(attachment, sizeof(attachment), PSTR("attachment; filename=%s.yaml"), UID);

    server->setContentLength(CONTENT_LENGTH_UNKNOWN);
    server->sendHeader(F("Content-Disposition"), attachment);
    server->send(200, F("Content-Type: application/octet-stream"), "");

    String availability = Mqtt::getTeleTopic(F("availability"));
    String tmp = Mqtt::getCmndTopic(F("POWER"));
    server->sendContent(F("switch:\r\n"));
    for (size_t ch = 0; ch < channels; ch++)
    {
        server->sendContent(F("  - platform: mqtt\r\n    name: \""));
        server->sendContent(UID);
        server->sendContent(F("_"));
        server->sendContent(String((ch + 1)));
        server->sendContent(F("\"\r\n    state_topic: \""));
        server->sendContent((powerTopic + (ch + 1)));
        server->sendContent(F("\"\r\n    command_topic: \""));
        server->sendContent((tmp + (ch + 1)));
        server->sendContent(F("\"\r\n    payload_on: \"ON\"\r\n    payload_off: \"OFF\"\r\n    availability_topic: \""));
        server->sendContent(availability);
        server->sendContent(F("\"\r\n    payload_available: \"online\"\r\n    payload_not_available: \"offline\"\r\n\r\n"));
    }

    String energy = Mqtt::getTeleTopic(F("ENERGY"));
    String tims[] = {"voltage", "current", "power", "apparent_power", "reactive_power", "factor", "total", "yesterday", "today", "starttime"};
    String tims2[] = {"V", "A", "W", "VA", "VAr", "", "kWh", "kWh", "kWh", ""};
    server->sendContent(F("sensor:\r\n"));
    for (size_t i = 0; i < 10; i++)
    {
        server->sendContent(F("  - platform: mqtt\r\n    name: \""));
        server->sendContent(UID);
        server->sendContent(F("_"));
        server->sendContent(tims[i].c_str());
        server->sendContent(F("\"\r\n    state_topic: \""));
        server->sendContent(energy.c_str());
        server->sendContent(F("\"\r\n    value_template: \"{{value_json."));
        server->sendContent(tims[i].c_str());
        server->sendContent(F("}}"));
        if (tims2[i].length() > 0)
        {
            server->sendContent(F("\"\r\n    unit_of_measurement: \""));
            server->sendContent(tims2[i].c_str());
        }
        server->sendContent(F("\"\r\n\r\n"));
    }
}
#pragma endregion

void DC1::logoLed()
{
    if (config.logo_led == 0)
    {
        digitalWrite(LOGO_LED_PIN, LOW);
    }
    else if (config.logo_led == 1)
    {
        digitalWrite(LOGO_LED_PIN, HIGH);
    }
    else if (config.logo_led == 2)
    {
        digitalWrite(LOGO_LED_PIN, lastState[0] ? LOW : HIGH);
    }
    else if (config.logo_led == 3)
    {
        digitalWrite(LOGO_LED_PIN, lastState[0] ? HIGH : LOW);
    }
}

void DC1::switchRelay(uint8_t ch, bool isOn, bool isSave)
{
    if (ch > channels)
    {
        Debug::AddInfo(PSTR("invalid channel: %d"), ch);
        return;
    }

    if (ch > 0 || (ch == 0 && config.sub_kinkage == 0))
    {
        if (!lastState[0] && isOn && config.sub_kinkage != 0)
        {
            if (config.sub_kinkage == 1 || !isSave)
            {
                isOn = false;
            }
            else if (config.sub_kinkage == 2)
            {
                switchRelay(0, true);
            }
        }

        if (isOn && config.power_mode == 1)
        {
            for (size_t ch2 = (config.sub_kinkage == 0 ? 0 : 1); ch2 < channels; ch2++)
            {
                if (ch2 != ch && lastState[ch2])
                {
                    switchRelay(ch2, false, isSave);
                }
            }
        }
    }
    Debug::AddInfo(PSTR("Relay %d . . . %s"), ch + 1, isOn ? "ON" : "OFF");

    if (!cat9554->digitalWrite(relGPIO[ch], isOn ? HIGH : LOW))
    {
        Debug::AddError(PSTR("CAT9554 digitalWrite Error"));
        if (!cat9554->digitalWrite(relGPIO[ch], isOn ? HIGH : LOW))
        {
            Debug::AddError(PSTR("CAT9554 digitalWrite Error2"));
            return;
        }
    }
    lastState[ch] = isOn;

    Mqtt::publish((powerTopic + (ch + 1)), isOn ? "ON" : "OFF", globalConfig.mqtt.retain);

    if (isSave && config.power_on_state > 0)
    {
        bitWrite(config.last_state, ch, isOn);
        bitSet(operationFlag, 1);
        Config::delaySaveConfig(10);
    }

    if (ch == 0)
    {
        logoLed();
        if (isSave && config.sub_kinkage != 0)
        {
            for (size_t ch2 = 1; ch2 < channels; ch2++)
            {
                if (isOn)
                {
                    if (bitRead(config.last_state, ch2))
                    {
                        switchRelay(ch2, true, false);
                    }
                }
                else
                {
                    switchRelay(ch2, false, false);
                }
            }
        }
    }
}

void DC1::checkButton(uint8_t ch)
{
    bool buttonState = ch == 0 ? digitalRead(btnGPIO[ch]) : cat9554->digitalRead(btnGPIO[ch]);

    if (buttonState == 0)
    {
        if (buttonTiming[ch] == false)
        {
            buttonTiming[ch] = true;
            buttonTimingStart[ch] = millis();
        }
        else
        { // buttonTiming = true
            if (millis() >= (buttonTimingStart[ch] + buttonDebounceTime))
            {
                buttonAction[ch] = 1;
            }
            if (millis() >= (buttonTimingStart[ch] + buttonLongPressTime))
            {
                buttonAction[ch] = 2;
            }
        }
    }
    else
    {
        buttonTiming[ch] = false;
        if (buttonAction[ch] != 0)
        {
            if (buttonAction[ch] == 1) // 执行短按动作
            {
                switchRelay(ch, !lastState[ch], true);
            }
            else if (buttonAction[ch] == 2) // 执行长按动作
            {
                if (ch == 0)
                {
                    Wifi::setupWifiManager(false);
                }
            }
            buttonAction[ch] = 0;
        }
    }
}

void DC1::energyUpdate()
{
    if (Rtc::rtcTime.valid && config.energy_kWhtotal_time == 0)
    {
        config.energy_kWhtotal_time = Rtc::utcTime;
        TIME_T tmpTime;
        Rtc::breakTime(config.energy_kWhtotal_time, tmpTime);
        snprintf_P(kWhtotalTime, sizeof(kWhtotalTime), PSTR("%04d-%02d-%02d %02d:%02d:%02d"), tmpTime.year, tmpTime.month, tmpTime.day_of_month, tmpTime.hour, tmpTime.minute, tmpTime.second);
    }
    if (Rtc::rtcTime.valid && config.energy_kWhdoy != Rtc::rtcTime.day_of_year)
    {
        Debug::AddInfo("day_of_year: %d %d %d", Rtc::rtcTime.day_of_year, Rtc::rtcTime.day_of_month, Rtc::rtcTime.day_of_week);

        energySync();
        config.energy_kWhdoy = Rtc::rtcTime.day_of_year;
        config.energy_kWhyesterday = config.energy_kWhtoday;
        config.energy_kWhtoday = 0;
        Config::saveConfig();

        cse7766->Energy.daily = (float)(config.energy_kWhtoday + cse7766->Energy.kWhtoday) / 100000;
        cse7766->Energy.total = (float)(config.energy_kWhtotal + cse7766->Energy.kWhtoday) / 100000;
    }
    if (cse7766->everySecond())
    {
        energyUpdateToday();
    }
    if (perSecond % 301 == 0 && cse7766->Energy.kWhtoday > 0)
    {
        energySync();
        Config::saveConfig();
    }
    energyMarginCheck();
}

void DC1::energySync()
{
    if (cse7766->Energy.kWhtoday > 0)
    {
        config.energy_kWhtoday += cse7766->Energy.kWhtoday;
        config.energy_kWhtotal += cse7766->Energy.kWhtoday;
        cse7766->Energy.kWhtoday = 0;
    }
}

void DC1::energyInit()
{
    cse7766->Energy.kWhtoday = 0;
    cse7766->Energy.kWhtoday_delta = 0;
    cse7766->Energy.daily = (float)(config.energy_kWhtoday) / 100000;
    cse7766->Energy.total = (float)(config.energy_kWhtotal) / 100000;

    TIME_T tmpTime;
    Rtc::breakTime(config.energy_kWhtotal_time, tmpTime);
    snprintf_P(kWhtotalTime, sizeof(kWhtotalTime), PSTR("%04d-%02d-%02d %02d:%02d:%02d"), tmpTime.year, tmpTime.month, tmpTime.day_of_month, tmpTime.hour, tmpTime.minute, tmpTime.second);
}

void DC1::energyClear()
{
    config.energy_kWhtoday = 0;
    config.energy_kWhyesterday = 0;
    config.energy_kWhtotal = 0;
    config.energy_kWhdoy = 0;
    config.energy_kWhtotal_time = 0;

    cse7766->Energy.kWhtoday_delta = 0;

    energyInit();
    Config::saveConfig();
}

void DC1::energyUpdateToday()
{
    if (cse7766->Energy.kWhtoday_delta > 1000)
    {
        unsigned long delta = cse7766->Energy.kWhtoday_delta / 1000;
        cse7766->Energy.kWhtoday_delta -= (delta * 1000);
        cse7766->Energy.kWhtoday += delta;

        cse7766->Energy.daily = (float)(config.energy_kWhtoday + cse7766->Energy.kWhtoday) / 100000;
        cse7766->Energy.total = (float)(config.energy_kWhtotal + cse7766->Energy.kWhtoday) / 100000;
    }
}

void DC1::energyMarginCheck()
{
    if (cse7766->Energy.power_steady_counter)
    {
        cse7766->Energy.power_steady_counter--;
        return;
    }

    uint16_t energy_power_u = (uint16_t)(cse7766->Energy.active_power);
    uint16_t energy_voltage_u = (uint16_t)(cse7766->Energy.voltage);
    uint16_t energy_current_u = (uint16_t)(cse7766->Energy.current * 1000);

    //Debug::AddInfo(PSTR("NRG: W %d, U %d, I %d"), energy_power_u, energy_voltage_u, energy_current_u);
    if (config.energy_power_delta)
    {
        uint16_t delta = abs(cse7766->Energy.power_history[0] - energy_power_u);
        if (delta > 0)
        {
            if (config.energy_power_delta < 101)
            { // 1..100 = Percentage
                uint16_t min_power = (cse7766->Energy.power_history[0] > energy_power_u) ? energy_power_u : cse7766->Energy.power_history[0];
                if (0 == min_power)
                {
                    min_power++;
                } // Fix divide by 0 exception (#6741)
                if (((delta * 100) / min_power) > config.energy_power_delta)
                {
                    cse7766->Energy.power_delta = true;
                }
            }
            else
            { // 101..32000 = Absolute
                if (delta > (config.energy_power_delta - 100))
                {
                    cse7766->Energy.power_delta = true;
                }
            }
            if (cse7766->Energy.power_delta)
            {
                cse7766->Energy.power_history[1] = cse7766->Energy.active_power; // We only want one report so reset history
                cse7766->Energy.power_history[2] = cse7766->Energy.active_power;
            }
        }
    }

    cse7766->Energy.power_history[0] = cse7766->Energy.power_history[1]; // Shift in history every second allowing power changes to settle for up to three seconds
    cse7766->Energy.power_history[1] = cse7766->Energy.power_history[2];
    cse7766->Energy.power_history[2] = energy_power_u;

    if (config.report_interval > 0 && (perSecond % config.report_interval) == 0)
    {
        reportPower();
        cse7766->Energy.power_delta = true;
    }
    if (cse7766->Energy.power_delta)
    {
        cse7766->Energy.power_delta = false;
        reportEnergy();
    }
}

void DC1::energyShow(bool isMqtt)
{
    const uint8_t current_resolution = 3;
    const uint8_t voltage_resolution = 0;
    const uint8_t wattage_resolution = 0;
    const uint8_t energy_resolution = 3;

    float apparent_power = cse7766->Energy.apparent_power;
    if (isnan(apparent_power))
    {
        apparent_power = cse7766->Energy.voltage * cse7766->Energy.current;
    }
    if (apparent_power < cse7766->Energy.active_power)
    { // Should be impossible
        cse7766->Energy.active_power = apparent_power;
    }

    float power_factor = cse7766->Energy.power_factor;
    if (isnan(power_factor))
    {
        power_factor = (cse7766->Energy.active_power && apparent_power) ? cse7766->Energy.active_power / apparent_power : 0;
        if (power_factor > 1)
        {
            power_factor = 1;
        }
    }

    float reactive_power = cse7766->Energy.reactive_power;
    if (isnan(reactive_power))
    {
        reactive_power = 0;
        uint32_t difference = ((uint32_t)(apparent_power * 100) - (uint32_t)(cse7766->Energy.active_power * 100)) / 10;
        if ((cse7766->Energy.current > 0.005) && ((difference > 15) || (difference > (uint32_t)(apparent_power * 100 / 1000))))
        {
            // calculating reactive power only if current is greater than 0.005A and
            // difference between active and apparent power is greater than 1.5W or 1%
            reactive_power = (float)(Util::RoundSqrtInt((uint32_t)(apparent_power * apparent_power * 100) - (uint32_t)(cse7766->Energy.active_power * cse7766->Energy.active_power * 100))) / 10;
        }
    }

    char apparent_power_chr[16];
    char reactive_power_chr[16];
    char power_factor_chr[16];
    Util::dtostrfd(apparent_power, wattage_resolution, apparent_power_chr);
    Util::dtostrfd(reactive_power, wattage_resolution, reactive_power_chr);
    Util::dtostrfd(power_factor, 2, power_factor_chr);

    char voltage_chr[16];
    char current_chr[16];
    char active_power_chr[16];
    Util::dtostrfd(cse7766->Energy.voltage, voltage_resolution, voltage_chr);
    Util::dtostrfd(cse7766->Energy.current, current_resolution, current_chr);
    Util::dtostrfd(cse7766->Energy.active_power, wattage_resolution, active_power_chr);

    char energy_daily_chr[16];
    char energy_yesterday_chr[16];
    char energy_total_chr[16];
    Util::dtostrfd(cse7766->Energy.daily, energy_resolution, energy_daily_chr);
    Util::dtostrfd((float)config.energy_kWhyesterday / 100000, energy_resolution, energy_yesterday_chr);
    Util::dtostrfd(cse7766->Energy.total, energy_resolution, energy_total_chr);

    sprintf(tmpData, "%s\"starttime\":\"%s\",\"total\":\"%s\",\"yesterday\":\"%s\",\"today\":\"%s\","
                     "\"voltage\":\"%s\",\"current\":\"%s\",\"power\":\"%s\","
                     "\"apparent_power\":\"%s\",\"reactive_power\":\"%s\",\"factor\":\"%s\"%s",
            isMqtt ? "{" : ",",
            kWhtotalTime,
            energy_total_chr, energy_yesterday_chr, energy_daily_chr,
            voltage_chr, current_chr, active_power_chr,
            apparent_power_chr, reactive_power_chr, power_factor_chr,
            isMqtt ? "}" : "");
}

void DC1::reportEnergy()
{
    energyShow(true);
    Mqtt::publish(Mqtt::getTeleTopic("ENERGY"), tmpData, globalConfig.mqtt.retain);
}
void DC1::reportPower()
{
    for (size_t ch = 0; ch < channels; ch++)
    {
        Mqtt::publish((powerTopic + (ch + 1)), lastState[ch] ? "ON" : "OFF", globalConfig.mqtt.retain);
    }
}
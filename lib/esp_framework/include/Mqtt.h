// Mqtt.h

#ifndef _MQTT_h
#define _MQTT_h

#define MQTT_SOCKET_TIMEOUT 5
#define MQTT_MAX_PACKET_SIZE 768

#include <PubSubClient.h>
#include "Arduino.h"

#define MQTT_CONNECTED_CALLBACK_SIGNATURE std::function<void()> connectedcallback

class Mqtt
{
protected:
    static String getTopic(uint8_t prefix, String subtopic);
    static String topicCmnd;
    static String topicStat;
    static String topicTele;
    static uint8_t operationFlag;
    static void doReportHeartbeat();

public:
    static PubSubClient mqttClient;
    static MQTT_CONNECTED_CALLBACK_SIGNATURE;

    static uint32_t lastReconnectAttempt; // 最后尝试重连时间
    static uint32_t kMqttReconnectTime;   // 重新连接尝试之间的延迟（ms）

    static bool mqttConnect();
    static void availability();
    static void loop();
    static void mqttSetLoopCallback(MQTT_CALLBACK_SIGNATURE);
    static void mqttSetConnectedCallback(MQTT_CONNECTED_CALLBACK_SIGNATURE);

    static void setTopic();
    static String getCmndTopic(String topic);
    static String getStatTopic(String topic);
    static String getTeleTopic(String topic);

    static PubSubClient &setClient(Client &client);
    static bool publish(String topic, const char *payload);
    static bool publish(String topic, const char *payload, bool retained);

    static bool publish(const char *topic, const char *payload);
    static bool publish(const char *topic, const char *payload, bool retained);
    static bool publish(const char *topic, const uint8_t *payload, unsigned int plength);
    static bool publish(const char *topic, const uint8_t *payload, unsigned int plength, bool retained);
    static bool publish_P(const char *topic, const char *payload, bool retained);
    static bool publish_P(const char *topic, const uint8_t *payload, unsigned int plength, bool retained);

    static bool subscribe(String topic);
    static bool subscribe(String topic, uint8_t qos);
    static bool unsubscribe(String topic);
    static void perSecondDo();
};

#endif

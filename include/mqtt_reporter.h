/*
 * MQTT Status Reporter
 */

#ifndef MQTT_REPORTER_H
#define MQTT_REPORTER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

class MQTTReporter {
public:
    static void init();
    static void loop();
    static void stop();
    static bool isConnected();

    static void publish(const String& subtopic, const String& payload);
    static void publishStatus();

    // Callback for commands received via MQTT
    static void onMessage(char* topic, byte* payload, unsigned int length);

private:
    static WiFiClient _wifiClient;
    static PubSubClient _mqttClient;
    static unsigned long _lastReport;
    static unsigned long _lastReconnectAttempt;
    static bool _enabled;

    static bool reconnect();
};

#endif // MQTT_REPORTER_H

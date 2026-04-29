/*
 * MQTT Reporter Implementation
 * Publishes status and receives commands via MQTT.
 */

#include "mqtt_reporter.h"
#include "config_manager.h"
#include "nat_manager.h"
#include "firewall.h"
#include "traffic_shaper.h"
#include <ESP8266WiFi.h>

WiFiClient MQTTReporter::_wifiClient;
PubSubClient MQTTReporter::_mqttClient(_wifiClient);
unsigned long MQTTReporter::_lastReport = 0;
unsigned long MQTTReporter::_lastReconnectAttempt = 0;
bool MQTTReporter::_enabled = false;

void MQTTReporter::init() {
    RouterConfig& cfg = ConfigManager::getConfig();
    _enabled = cfg.mqtt_enabled && strlen(cfg.mqtt_server) > 0;

    if (!_enabled) {
        Serial.println(F("[MQTT] MQTT disabled or no server configured"));
        return;
    }

    _mqttClient.setServer(cfg.mqtt_server, cfg.mqtt_port);
    _mqttClient.setCallback(onMessage);
    _mqttClient.setBufferSize(MQTT_MAX_PACKET_SIZE);

    Serial.printf("[MQTT] Initialized: %s:%d topic=%s\n",
                  cfg.mqtt_server, cfg.mqtt_port, cfg.mqtt_topic);
}

void MQTTReporter::loop() {
    if (!_enabled) return;
    if (WiFi.status() != WL_CONNECTED) return;

    if (!_mqttClient.connected()) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt > 5000) {
            _lastReconnectAttempt = now;
            reconnect();
        }
        return;
    }

    _mqttClient.loop();

    // Periodic status report
    unsigned long now = millis();
    if (now - _lastReport >= MQTT_REPORT_INTERVAL) {
        _lastReport = now;
        publishStatus();
    }
}

void MQTTReporter::stop() {
    if (_mqttClient.connected()) {
        RouterConfig& cfg = ConfigManager::getConfig();
        String topic = String(cfg.mqtt_topic) + "/status";
        _mqttClient.publish(topic.c_str(), "offline", true);
        _mqttClient.disconnect();
    }
    _enabled = false;
}

bool MQTTReporter::isConnected() {
    return _mqttClient.connected();
}

bool MQTTReporter::reconnect() {
    RouterConfig& cfg = ConfigManager::getConfig();
    String clientId = "esp-router-" + String(ESP.getChipId(), HEX);
    String willTopic = String(cfg.mqtt_topic) + "/status";

    bool connected;
    if (strlen(cfg.mqtt_user) > 0) {
        connected = _mqttClient.connect(clientId.c_str(),
                                         cfg.mqtt_user, cfg.mqtt_pass,
                                         willTopic.c_str(), 0, true, "offline");
    } else {
        connected = _mqttClient.connect(clientId.c_str(),
                                         willTopic.c_str(), 0, true, "offline");
    }

    if (connected) {
        Serial.println(F("[MQTT] Connected"));
        // Subscribe to command topic
        String cmdTopic = String(cfg.mqtt_topic) + "/cmd/#";
        _mqttClient.subscribe(cmdTopic.c_str());
        // Announce online
        _mqttClient.publish(willTopic.c_str(), "online", true);
        return true;
    }

    Serial.printf("[MQTT] Connection failed, rc=%d\n", _mqttClient.state());
    return false;
}

void MQTTReporter::publish(const String& subtopic, const String& payload) {
    if (!_mqttClient.connected()) return;
    RouterConfig& cfg = ConfigManager::getConfig();
    String topic = String(cfg.mqtt_topic) + "/" + subtopic;
    _mqttClient.publish(topic.c_str(), payload.c_str());
}

void MQTTReporter::publishStatus() {
    RouterConfig& cfg = ConfigManager::getConfig();

    // Build JSON status
    String json = "{";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"sta_ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"sta_rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"sta_ssid\":\"" + WiFi.SSID() + "\",";
    json += "\"ap_ssid\":\"" + String(cfg.ap_ssid) + "\",";
    json += "\"ap_clients\":" + String(WiFi.softAPgetStationNum()) + ",";
    json += "\"nat_active\":" + String(NATManager::isEnabled() ? "true" : "false") + ",";
    json += "\"fw_allowed\":" + String(Firewall::getPacketsAllowed()) + ",";
    json += "\"fw_denied\":" + String(Firewall::getPacketsDenied()) + ",";
    json += "\"bw_kbps\":" + String(TrafficShaper::getCurrentThroughputKbps()) + ",";
    json += "\"version\":\"" + String(VERSION) + "\"";
    json += "}";

    publish("status", json);
}

void MQTTReporter::onMessage(char* topic, byte* payload, unsigned int length) {
    String msg;
    msg.reserve(length);
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }

    String topicStr = String(topic);
    Serial.printf("[MQTT] Received: %s = %s\n", topic, msg.c_str());

    RouterConfig& cfg = ConfigManager::getConfig();
    String prefix = String(cfg.mqtt_topic) + "/cmd/";

    if (topicStr.startsWith(prefix)) {
        String cmd = topicStr.substring(prefix.length());

        if (cmd == "reboot") {
            publish("response", "Rebooting...");
            delay(500);
            ESP.restart();
        } else if (cmd == "status") {
            publishStatus();
        } else if (cmd == "gpio") {
            // Format: \"pin:value\" e.g. \"2:1\"
            int sep = msg.indexOf(':');
            if (sep > 0) {
                uint8_t pin = msg.substring(0, sep).toInt();
                uint8_t val = msg.substring(sep + 1).toInt();
                pinMode(pin, OUTPUT);
                digitalWrite(pin, val);
                publish("response", "GPIO " + String(pin) + " = " + String(val));
            }
        } else {
            publish("response", "Unknown command: " + cmd);
        }
    }
}

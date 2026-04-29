/*
 * Configuration Manager Implementation
 * Handles persistent storage of all settings via LittleFS + ArduinoJson.
 */

#include "config_manager.h"
#include <LittleFS.h>

RouterConfig ConfigManager::_config;
bool ConfigManager::_initialized = false;

bool ConfigManager::init() {
    if (!LittleFS.begin()) {
        Serial.println(F("[CONFIG] LittleFS mount failed, formatting..."));
        LittleFS.format();
        if (!LittleFS.begin()) {
            Serial.println(F("[CONFIG] LittleFS format failed!"));
            return false;
        }
    }
    loadDefaults();
    _initialized = true;
    return load();
}

void ConfigManager::loadDefaults() {
    memset(&_config, 0, sizeof(RouterConfig));

    strncpy(_config.sta_ssid, DEFAULT_STA_SSID, sizeof(_config.sta_ssid) - 1);
    strncpy(_config.sta_password, DEFAULT_STA_PASSWORD, sizeof(_config.sta_password) - 1);
    strncpy(_config.ap_ssid, DEFAULT_AP_SSID, sizeof(_config.ap_ssid) - 1);
    strncpy(_config.ap_password, DEFAULT_AP_PASSWORD, sizeof(_config.ap_password) - 1);
    _config.ap_channel = DEFAULT_AP_CHANNEL;
    _config.ap_hidden = DEFAULT_AP_HIDDEN;
    _config.ap_max_conn = DEFAULT_AP_MAX_CONN;

    strncpy(_config.mqtt_server, DEFAULT_MQTT_SERVER, sizeof(_config.mqtt_server) - 1);
    _config.mqtt_port = DEFAULT_MQTT_PORT;
    strncpy(_config.mqtt_user, DEFAULT_MQTT_USER, sizeof(_config.mqtt_user) - 1);
    strncpy(_config.mqtt_pass, DEFAULT_MQTT_PASS, sizeof(_config.mqtt_pass) - 1);
    strncpy(_config.mqtt_topic, DEFAULT_MQTT_TOPIC, sizeof(_config.mqtt_topic) - 1);
    _config.mqtt_enabled = false;

    _config.bw_limit_kbps = DEFAULT_BW_LIMIT_KBPS;
    _config.automesh_enabled = false;
    strncpy(_config.automesh_prefix, AUTOMESH_PREFIX, sizeof(_config.automesh_prefix) - 1);

    _config.dns1 = IPAddress(8, 8, 8, 8);
    _config.dns2 = IPAddress(8, 8, 4, 4);

    strncpy(_config.hostname, "esp-router", sizeof(_config.hostname) - 1);
}

bool ConfigManager::load() {
    if (!LittleFS.exists(CONFIG_FILE)) {
        Serial.println(F("[CONFIG] No config file found, using defaults"));
        return save(); // Create default file
    }

    File f = LittleFS.open(CONFIG_FILE, "r");
    if (!f) {
        Serial.println(F("[CONFIG] Failed to open config file"));
        return false;
    }

    StaticJsonDocument<1536> doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    if (err) {
        Serial.printf("[CONFIG] JSON parse error: %s\n", err.c_str());
        return false;
    }

    // Load values with defaults as fallback
    strlcpy(_config.sta_ssid, doc["sta_ssid"] | DEFAULT_STA_SSID, sizeof(_config.sta_ssid));
    strlcpy(_config.sta_password, doc["sta_pass"] | DEFAULT_STA_PASSWORD, sizeof(_config.sta_password));
    strlcpy(_config.ap_ssid, doc["ap_ssid"] | DEFAULT_AP_SSID, sizeof(_config.ap_ssid));
    strlcpy(_config.ap_password, doc["ap_pass"] | DEFAULT_AP_PASSWORD, sizeof(_config.ap_password));
    _config.ap_channel = doc["ap_chan"] | DEFAULT_AP_CHANNEL;
    _config.ap_hidden = doc["ap_hidden"] | false;
    _config.ap_max_conn = doc["ap_max"] | DEFAULT_AP_MAX_CONN;

    strlcpy(_config.mqtt_server, doc["mqtt_srv"] | "", sizeof(_config.mqtt_server));
    _config.mqtt_port = doc["mqtt_port"] | DEFAULT_MQTT_PORT;
    strlcpy(_config.mqtt_user, doc["mqtt_user"] | "", sizeof(_config.mqtt_user));
    strlcpy(_config.mqtt_pass, doc["mqtt_pass"] | "", sizeof(_config.mqtt_pass));
    strlcpy(_config.mqtt_topic, doc["mqtt_topic"] | DEFAULT_MQTT_TOPIC, sizeof(_config.mqtt_topic));
    _config.mqtt_enabled = doc["mqtt_en"] | false;

    _config.bw_limit_kbps = doc["bw_limit"] | (uint32_t)0;
    _config.automesh_enabled = doc["mesh_en"] | false;
    strlcpy(_config.automesh_prefix, doc["mesh_pfx"] | AUTOMESH_PREFIX, sizeof(_config.automesh_prefix));

    // DNS
    IPAddress dns1, dns2;
    if (dns1.fromString(doc["dns1"] | "8.8.8.8")) _config.dns1 = dns1;
    if (dns2.fromString(doc["dns2"] | "8.8.4.4")) _config.dns2 = dns2;

    strlcpy(_config.hostname, doc["hostname"] | "esp-router", sizeof(_config.hostname));

    Serial.println(F("[CONFIG] Configuration loaded"));
    return true;
}

bool ConfigManager::save() {
    StaticJsonDocument<1536> doc;

    doc["sta_ssid"] = _config.sta_ssid;
    doc["sta_pass"] = _config.sta_password;
    doc["ap_ssid"] = _config.ap_ssid;
    doc["ap_pass"] = _config.ap_password;
    doc["ap_chan"] = _config.ap_channel;
    doc["ap_hidden"] = _config.ap_hidden;
    doc["ap_max"] = _config.ap_max_conn;

    doc["mqtt_srv"] = _config.mqtt_server;
    doc["mqtt_port"] = _config.mqtt_port;
    doc["mqtt_user"] = _config.mqtt_user;
    doc["mqtt_pass"] = _config.mqtt_pass;
    doc["mqtt_topic"] = _config.mqtt_topic;
    doc["mqtt_en"] = _config.mqtt_enabled;

    doc["bw_limit"] = _config.bw_limit_kbps;
    doc["mesh_en"] = _config.automesh_enabled;
    doc["mesh_pfx"] = _config.automesh_prefix;

    doc["dns1"] = _config.dns1.toString();
    doc["dns2"] = _config.dns2.toString();
    doc["hostname"] = _config.hostname;

    File f = LittleFS.open(CONFIG_FILE, "w");
    if (!f) {
        Serial.println(F("[CONFIG] Failed to open config file for writing"));
        return false;
    }

    serializeJson(doc, f);
    f.close();
    Serial.println(F("[CONFIG] Configuration saved"));
    return true;
}

RouterConfig& ConfigManager::getConfig() {
    return _config;
}

bool ConfigManager::loadACL(ACLRule* rules, uint8_t& count) {
    count = 0;
    if (!LittleFS.exists(ACL_FILE)) return true;

    File f = LittleFS.open(ACL_FILE, "r");
    if (!f) return false;

    StaticJsonDocument<2048> doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) return false;

    JsonArray arr = doc["rules"].as<JsonArray>();
    for (JsonObject obj : arr) {
        if (count >= MAX_ACL_RULES) break;
        ACLRule& r = rules[count];
        r.protocol = (ACLProtocol)(obj["proto"] | 0);
        r.src_ip = (uint32_t)(obj["src_ip"] | (uint32_t)0);
        r.src_mask = (uint32_t)(obj["src_mask"] | (uint32_t)0);
        r.src_port = obj["src_port"] | 0;
        r.dst_ip = (uint32_t)(obj["dst_ip"] | (uint32_t)0);
        r.dst_mask = (uint32_t)(obj["dst_mask"] | (uint32_t)0);
        r.dst_port = obj["dst_port"] | 0;
        r.action = (ACLAction)(obj["action"] | 0);
        r.enabled = obj["enabled"] | true;
        strlcpy(r.description, obj["desc"] | "", sizeof(r.description));
        count++;
    }
    return true;
}

bool ConfigManager::saveACL(const ACLRule* rules, uint8_t count) {
    StaticJsonDocument<2048> doc;
    JsonArray arr = doc.createNestedArray("rules");

    for (uint8_t i = 0; i < count; i++) {
        JsonObject obj = arr.createNestedObject();
        obj["proto"] = (uint8_t)rules[i].protocol;
        obj["src_ip"] = rules[i].src_ip;
        obj["src_mask"] = rules[i].src_mask;
        obj["src_port"] = rules[i].src_port;
        obj["dst_ip"] = rules[i].dst_ip;
        obj["dst_mask"] = rules[i].dst_mask;
        obj["dst_port"] = rules[i].dst_port;
        obj["action"] = (uint8_t)rules[i].action;
        obj["enabled"] = rules[i].enabled;
        obj["desc"] = rules[i].description;
    }

    File f = LittleFS.open(ACL_FILE, "w");
    if (!f) return false;
    serializeJson(doc, f);
    f.close();
    return true;
}

bool ConfigManager::loadPortForwards(PortForwardRule* rules, uint8_t& count) {
    count = 0;
    if (!LittleFS.exists(PORTMAP_FILE)) return true;

    File f = LittleFS.open(PORTMAP_FILE, "r");
    if (!f) return false;

    StaticJsonDocument<2048> doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) return false;

    JsonArray arr = doc["rules"].as<JsonArray>();
    for (JsonObject obj : arr) {
        if (count >= MAX_PORTFORWARD_RULES) break;
        PortForwardRule& r = rules[count];
        r.protocol = (ACLProtocol)(obj["proto"] | 6);
        r.external_port = obj["ext_port"] | 0;
        r.internal_ip = (uint32_t)(obj["int_ip"] | (uint32_t)0);
        r.internal_port = obj["int_port"] | 0;
        r.enabled = obj["enabled"] | true;
        strlcpy(r.description, obj["desc"] | "", sizeof(r.description));
        count++;
    }
    return true;
}

bool ConfigManager::savePortForwards(const PortForwardRule* rules, uint8_t count) {
    StaticJsonDocument<2048> doc;
    JsonArray arr = doc.createNestedArray("rules");

    for (uint8_t i = 0; i < count; i++) {
        JsonObject obj = arr.createNestedObject();
        obj["proto"] = (uint8_t)rules[i].protocol;
        obj["ext_port"] = rules[i].external_port;
        obj["int_ip"] = rules[i].internal_ip;
        obj["int_port"] = rules[i].internal_port;
        obj["enabled"] = rules[i].enabled;
        obj["desc"] = rules[i].description;
    }

    File f = LittleFS.open(PORTMAP_FILE, "w");
    if (!f) return false;
    serializeJson(doc, f);
    f.close();
    return true;
}

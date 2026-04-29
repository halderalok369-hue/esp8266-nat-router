/*
 * Configuration Manager - Persistent storage via LittleFS
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

class ConfigManager {
public:
    static bool init();
    static bool load();
    static bool save();
    static void loadDefaults();

    static RouterConfig& getConfig();

    // ACL management
    static bool loadACL(ACLRule* rules, uint8_t& count);
    static bool saveACL(const ACLRule* rules, uint8_t count);

    // Port forward management
    static bool loadPortForwards(PortForwardRule* rules, uint8_t& count);
    static bool savePortForwards(const PortForwardRule* rules, uint8_t count);

private:
    static RouterConfig _config;
    static bool _initialized;
};

#endif // CONFIG_MANAGER_H

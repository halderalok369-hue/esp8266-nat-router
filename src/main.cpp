/*
 * ESP8266 WiFi NAT Router & Range Extender
 * Main firmware entry point.
 *
 * Architecture:
 *   1. Boot → Load config from LittleFS
 *   2. Start AP (always, for config access)
 *   3. Connect to upstream STA if configured
 *   4. Enable NAPT for transparent NAT
 *   5. Start web portal, CLI (Serial + Telnet), MQTT
 *   6. Main loop services all subsystems
 *
 * Copyright (c) 2024-2026 ESP Router Project
 * License: MIT
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>

#include "config.h"
#include "config_manager.h"
#include "nat_manager.h"
#include "web_portal.h"
#include "cli_handler.h"
#include "firewall.h"
#include "traffic_shaper.h"
#include "mqtt_reporter.h"
#include "automesh.h"
#include "gpio_control.h"

// ─── State Machine ──────────────────────────────────────────
enum RouterState {
    STATE_INIT,
    STATE_AP_ONLY,       // No upstream configured
    STATE_CONNECTING,    // Connecting to upstream
    STATE_RUNNING,       // Full NAT routing active
    STATE_RECONNECTING   // Lost upstream, attempting reconnect
};

static RouterState currentState = STATE_INIT;
static unsigned long lastReconnectAttempt = 0;
static unsigned long lastStatusLED = 0;
static bool ledState = false;

// ─── Status LED Patterns ────────────────────────────────────────────
void updateStatusLED() {
    unsigned long now = millis();
    uint16_t interval;

    switch (currentState) {
        case STATE_AP_ONLY:     interval = 2000; break; // Slow blink
        case STATE_CONNECTING:  interval = 200;  break; // Fast blink
        case STATE_RUNNING:     interval = 0;    break; // Solid on
        case STATE_RECONNECTING: interval = 500; break; // Medium blink
        default:                interval = 100;  break;
    }

    if (interval == 0) {
        digitalWrite(STATUS_LED_PIN, LOW); // Active low = ON
        return;
    }

    if (now - lastStatusLED >= interval) {
        lastStatusLED = now;
        ledState = !ledState;
        digitalWrite(STATUS_LED_PIN, ledState ? LOW : HIGH);
    }
}

// ─── WiFi Event Handlers ───────────────────────────────────────
void onStaConnected(const WiFiEventStationModeConnected& evt) {
    Serial.printf("[WIFI] STA connected to: %s\n", evt.ssid.c_str());
}

void onStaGotIP(const WiFiEventStationModeGotIP& evt) {
    Serial.printf("[WIFI] STA IP: %s, Gateway: %s\n",
                  evt.ip.toString().c_str(), evt.gw.toString().c_str());
}

void onStaDisconnected(const WiFiEventStationModeDisconnected& evt) {
    Serial.printf("[WIFI] STA disconnected from: %s (reason: %d)\n",
                  evt.ssid.c_str(), evt.reason);
    if (currentState == STATE_RUNNING) {
        currentState = STATE_RECONNECTING;
        NATManager::disable();
    }
}

void onApClientConnected(const WiFiEventSoftAPModeStationConnected& evt) {
    Serial.printf("[WIFI] AP client connected: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  evt.mac[0], evt.mac[1], evt.mac[2],
                  evt.mac[3], evt.mac[4], evt.mac[5]);
}

void onApClientDisconnected(const WiFiEventSoftAPModeStationDisconnected& evt) {
    Serial.printf("[WIFI] AP client disconnected: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  evt.mac[0], evt.mac[1], evt.mac[2],
                  evt.mac[3], evt.mac[4], evt.mac[5]);
}

// ─── Setup ────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println(F("╔══════════════════════════════════════════╗"));
    Serial.println(F("║   ESP8266 WiFi NAT Router v" VERSION "       ║"));
    Serial.println(F("║   Range Extender & Mesh Gateway         ║"));
    Serial.println(F("╚══════════════════════════════════════════╝"));
    Serial.println();

    // Initialize GPIO (LED)
    GPIOControl::init();
    digitalWrite(STATUS_LED_PIN, LOW); // LED on during boot

    // Initialize configuration
    if (!ConfigManager::init()) {
        Serial.println(F("[FATAL] Config init failed!"));
        // Continue with defaults
    }

    RouterConfig& cfg = ConfigManager::getConfig();

    // Register WiFi event handlers
    WiFi.onStationModeConnected(onStaConnected);
    WiFi.onStationModeGotIP(onStaGotIP);
    WiFi.onStationModeDisconnected(onStaDisconnected);
    WiFi.onSoftAPModeStationConnected(onApClientConnected);
    WiFi.onSoftAPModeStationDisconnected(onApClientDisconnected);

    // Set WiFi mode: AP+STA
    WiFi.mode(WIFI_AP_STA);
    WiFi.hostname(cfg.hostname);

    // Configure and start Access Point
    WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
    bool apStarted = WiFi.softAP(cfg.ap_ssid, cfg.ap_password,
                                  cfg.ap_channel, cfg.ap_hidden, cfg.ap_max_conn);
    if (apStarted) {
        Serial.printf("[WIFI] AP started: %s (IP: %s)\n",
                      cfg.ap_ssid, WiFi.softAPIP().toString().c_str());
    } else {
        Serial.println(F("[WIFI] AP start FAILED!"));
    }

    // Initialize NAT tables
    if (!NATManager::init()) {
        Serial.println(F("[FATAL] NAT init failed!"));
    }

    // Initialize subsystems
    Firewall::init();
    TrafficShaper::init();
    if (cfg.bw_limit_kbps > 0) {
        TrafficShaper::setBandwidthLimit(cfg.bw_limit_kbps);
    }

    // Start web portal
    WebPortal::init();

    // Start CLI (Serial + Telnet)
    CLIHandler::init();

    // Connect to upstream if configured
    if (strlen(cfg.sta_ssid) > 0) {
        Serial.printf("[WIFI] Connecting to upstream: %s\n", cfg.sta_ssid);
        WiFi.begin(cfg.sta_ssid, cfg.sta_password);
        currentState = STATE_CONNECTING;

        // Wait for connection (with timeout)
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < STA_CONNECT_TIMEOUT) {
            delay(100);
            yield();
            Serial.print(".");
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[WIFI] Connected! STA IP: %s\n", WiFi.localIP().toString().c_str());

            // Enable NAT
            if (NATManager::enable()) {
                currentState = STATE_RUNNING;
                Serial.println(F("[ROUTER] NAT routing ACTIVE"));
            } else {
                Serial.println(F("[ROUTER] NAT enable failed, AP-only mode"));
                currentState = STATE_AP_ONLY;
            }

            // Apply port forwards
            PortForwardRule pfRules[MAX_PORTFORWARD_RULES];
            uint8_t pfCount = 0;
            ConfigManager::loadPortForwards(pfRules, pfCount);
            for (uint8_t i = 0; i < pfCount; i++) {
                NATManager::addPortForward(pfRules[i]);
            }
        } else {
            Serial.println(F("[WIFI] STA connection failed, running in AP-only mode"));
            currentState = STATE_AP_ONLY;
        }
    } else {
        Serial.println(F("[WIFI] No upstream SSID configured, AP-only mode"));
        currentState = STATE_AP_ONLY;
    }

    // Initialize MQTT
    MQTTReporter::init();

    // Initialize Automesh
    Automesh::init();

    Serial.println(F("[ROUTER] Boot complete."));
    Serial.printf("[ROUTER] Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.print(F("router> "));
}

// ─── Main Loop ────────────────────────────────────────────────
void loop() {
    // State machine
    switch (currentState) {
        case STATE_RECONNECTING: {
            unsigned long now = millis();
            if (now - lastReconnectAttempt >= STA_RECONNECT_INTERVAL) {
                lastReconnectAttempt = now;
                RouterConfig& cfg = ConfigManager::getConfig();

                // Try automesh first if enabled
                if (Automesh::isEnabled()) {
                    if (Automesh::scanAndConnect()) {
                        if (WiFi.status() == WL_CONNECTED) {
                            NATManager::enable();
                            currentState = STATE_RUNNING;
                            CLIHandler::broadcast(F("[ROUTER] Reconnected via automesh"));
                        }
                        break;
                    }
                }

                // Normal reconnect
                WiFi.begin(cfg.sta_ssid, cfg.sta_password);
                unsigned long start = millis();
                while (WiFi.status() != WL_CONNECTED && millis() - start < STA_CONNECT_TIMEOUT) {
                    delay(100);
                    yield();
                }
                if (WiFi.status() == WL_CONNECTED) {
                    NATManager::enable();
                    currentState = STATE_RUNNING;
                    CLIHandler::broadcast(F("[ROUTER] Reconnected to upstream"));
                }
            }
            break;
        }

        case STATE_RUNNING:
            if (WiFi.status() != WL_CONNECTED) {
                currentState = STATE_RECONNECTING;
                NATManager::disable();
                CLIHandler::broadcast(F("[ROUTER] Lost upstream connection, reconnecting..."));
            }
            break;

        default:
            break;
    }

    // Service all subsystems
    WebPortal::loop();
    CLIHandler::loop();
    TrafficShaper::loop();
    MQTTReporter::loop();
    Automesh::loop();

    // Status LED
    updateStatusLED();

    // Yield to system tasks
    yield();
}

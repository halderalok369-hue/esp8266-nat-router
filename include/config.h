/*
 * ESP8266 WiFi NAT Router - Configuration Header
 * Central configuration definitions and defaults.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ─── Firmware Version ────────────────────────────────────────────────
#ifndef VERSION
#define VERSION "1.0.0"
#endif

// ─── Default Network Settings ────────────────────────────────
#define DEFAULT_AP_SSID       "ESP-Router"
#define DEFAULT_AP_PASSWORD   "esp8266router"
#define DEFAULT_AP_CHANNEL    6
#define DEFAULT_AP_HIDDEN     0
#define DEFAULT_AP_MAX_CONN   8

// Default upstream (empty = unconfigured)
#define DEFAULT_STA_SSID      ""
#define DEFAULT_STA_PASSWORD  ""

// AP network
#define AP_IP                 IPAddress(192, 168, 4, 1)
#define AP_GATEWAY            IPAddress(192, 168, 4, 1)
#define AP_SUBNET             IPAddress(255, 255, 255, 0)

// DHCP range (handled by ESP8266 DHCP server, 192.168.4.100 - 192.168.4.200)
#define DHCP_START            IPAddress(192, 168, 4, 100)
#define DHCP_END              IPAddress(192, 168, 4, 200)

// ─── NAPT Settings ──────────
#ifndef NAPT_MAX
#define NAPT_MAX              512
#endif
#ifndef PORTMAP_MAX
#define PORTMAP_MAX           32
#endif

// ─── Web Server ──────────────────
#define WEB_SERVER_PORT       80
#define DNS_PORT              53

// ─── Telnet ─────────────────
#define TELNET_PORT           23
#ifndef TELNET_MAX_CLIENTS
#define TELNET_MAX_CLIENTS    2
#endif

// ─── MQTT ──────
#define DEFAULT_MQTT_SERVER   ""
#define DEFAULT_MQTT_PORT     1883
#define DEFAULT_MQTT_USER     ""
#define DEFAULT_MQTT_PASS     ""
#define DEFAULT_MQTT_TOPIC    "esp-router"
#define MQTT_REPORT_INTERVAL  30000  // ms

// ─── Automesh ────────
#define AUTOMESH_SCAN_INTERVAL 60000  // ms
#define AUTOMESH_RSSI_THRESHOLD -80   // dBm minimum
#define AUTOMESH_PREFIX       "ESP-Mesh-"

// ─── GPIO ────────
#define STATUS_LED_PIN        2   // Built-in LED (active low on most boards)
#define GPIO_CONTROL_PINS     {0, 2, 4, 5, 12, 13, 14, 15, 16}

// ─── Traffic Shaping ──────────────────
#define DEFAULT_BW_LIMIT_KBPS 0   // 0 = unlimited
#define SHAPING_BUCKET_MS     100 // Token bucket interval

// ─── Persistent Storage Paths ────────────────
#define CONFIG_FILE           "/config.json"
#define ACL_FILE              "/acl.json"
#define PORTMAP_FILE          "/portmap.json"

// ─── Watchdog ─────────────
#define STA_RECONNECT_INTERVAL 10000 // ms
#define STA_CONNECT_TIMEOUT    15000 // ms
#define MAX_STA_RETRIES        0     // 0 = infinite

// ─── Configuration Structure ────────────────
struct RouterConfig {
    // Upstream WiFi
    char sta_ssid[33];
    char sta_password[65];

    // Local AP
    char ap_ssid[33];
    char ap_password[65];
    uint8_t ap_channel;
    bool ap_hidden;
    uint8_t ap_max_conn;

    // MQTT
    char mqtt_server[65];
    uint16_t mqtt_port;
    char mqtt_user[33];
    char mqtt_pass[65];
    char mqtt_topic[65];
    bool mqtt_enabled;

    // Traffic shaping
    uint32_t bw_limit_kbps;

    // Automesh
    bool automesh_enabled;
    char automesh_prefix[33];

    // DNS
    IPAddress dns1;
    IPAddress dns2;

    // Hostname
    char hostname[33];
};

// ─── ACL Rule ────────────
enum ACLProtocol : uint8_t {
    ACL_PROTO_ANY = 0,
    ACL_PROTO_TCP = 6,
    ACL_PROTO_UDP = 17,
    ACL_PROTO_ICMP = 1
};

enum ACLAction : uint8_t {
    ACL_ALLOW = 0,
    ACL_DENY  = 1
};

struct ACLRule {
    ACLProtocol protocol;
    uint32_t src_ip;
    uint32_t src_mask;
    uint16_t src_port;
    uint32_t dst_ip;
    uint32_t dst_mask;
    uint16_t dst_port;
    ACLAction action;
    bool enabled;
    char description[32];
};

#define MAX_ACL_RULES 16

// ─── Port Forward Rule ──────────────────────
struct PortForwardRule {
    ACLProtocol protocol;
    uint16_t external_port;
    uint32_t internal_ip;
    uint16_t internal_port;
    bool enabled;
    char description[32];
};

#define MAX_PORTFORWARD_RULES 16

#endif // CONFIG_H

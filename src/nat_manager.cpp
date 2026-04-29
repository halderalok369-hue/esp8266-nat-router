/*
 * NAT Manager Implementation
 * Wraps lwIP NAPT functions for transparent NAT routing.
 *
 * The ESP8266 Arduino core (with lwIP2) includes NAPT support.
 * Headers: <lwip/napt.h> and <lwip/dns.h>
 */

#include "nat_manager.h"
#include <ESP8266WiFi.h>
#include <lwip/napt.h>
#include <lwip/dns.h>
#include <dhcpserver.h>

bool NATManager::_enabled = false;

bool NATManager::init() {
    Serial.println(F("[NAT] Initializing NAPT tables..."));
    Serial.printf("[NAT] Max NAT entries: %d, Max port maps: %d\n", NAPT_MAX, PORTMAP_MAX);

    // Allocate NAPT tables
    err_t ret = ip_napt_init(NAPT_MAX, PORTMAP_MAX);
    if (ret != ERR_OK) {
        Serial.printf("[NAT] NAPT init failed with error: %d\n", ret);
        return false;
    }

    Serial.println(F("[NAT] NAPT tables initialized successfully"));
    return true;
}

bool NATManager::enable() {
    if (_enabled) return true;

    // Enable NAPT on the SoftAP interface (interface 1)
    // SOFTAP_IF = 1 in ESP8266
    err_t ret = ip_napt_enable_no(1, 1);
    if (ret != ERR_OK) {
        Serial.printf("[NAT] Failed to enable NAPT: %d\n", ret);
        return false;
    }

    _enabled = true;
    Serial.println(F("[NAT] NAPT enabled on SoftAP interface"));

    // Configure DNS for AP clients to use the upstream DNS
    // Get DNS from the STA connection
    ip_addr_t dnsip = dns_getserver(0);
    if (ip_addr_isany(&dnsip)) {
        // Fallback to Google DNS
        IP4_ADDR(&dnsip, 8, 8, 8, 8);
    }

    dhcps_set_dns(0, &dnsip);
    Serial.printf("[NAT] DHCP DNS set to: %s\n", IPAddress(ip_addr_get_ip4_u32(&dnsip)).toString().c_str());

    return true;
}

bool NATManager::disable() {
    if (!_enabled) return true;

    err_t ret = ip_napt_enable_no(1, 0);
    if (ret != ERR_OK) {
        Serial.printf("[NAT] Failed to disable NAPT: %d\n", ret);
        return false;
    }

    _enabled = false;
    Serial.println(F("[NAT] NAPT disabled"));
    return true;
}

bool NATManager::isEnabled() {
    return _enabled;
}

bool NATManager::addPortForward(PortForwardRule& rule) {
    if (!rule.enabled) return true;

    // Get external (STA) IP
    IPAddress staIP = WiFi.localIP();
    uint32_t maddr = (uint32_t)staIP;

    uint8_t proto;
    switch (rule.protocol) {
        case ACL_PROTO_TCP: proto = IP_PROTO_TCP; break;
        case ACL_PROTO_UDP: proto = IP_PROTO_UDP; break;
        default: proto = IP_PROTO_TCP; break;
    }

    // ip_portmap_add(proto, maddr, mport, daddr, dport)
    // maddr = external (STA) IP, mport = external port
    // daddr = internal IP, dport = internal port
    err_t ret = ip_portmap_add(proto, maddr, rule.external_port,
                                rule.internal_ip, rule.internal_port);
    if (ret != ERR_OK) {
        Serial.printf("[NAT] Port forward add failed: %d\n", ret);
        return false;
    }

    Serial.printf("[NAT] Port forward: %s ext:%d -> %s:%d\n",
                  proto == IP_PROTO_TCP ? "TCP" : "UDP",
                  rule.external_port,
                  IPAddress(rule.internal_ip).toString().c_str(),
                  rule.internal_port);
    return true;
}

bool NATManager::removePortForward(uint16_t external_port, uint8_t protocol) {
    // Remove by setting internal IP/port to 0
    err_t ret = ip_portmap_remove(protocol, external_port);
    if (ret != ERR_OK) {
        Serial.printf("[NAT] Port forward remove failed: %d\n", ret);
        return false;
    }
    return true;
}

void NATManager::clearPortForwards() {
    // Re-initialize to clear all mappings
    ip_napt_init(NAPT_MAX, PORTMAP_MAX);
    if (_enabled) {
        ip_napt_enable_no(1, 1);
    }
    Serial.println(F("[NAT] All port forwards cleared"));
}

uint32_t NATManager::getActiveConnections() {
    // The NAPT table doesn't expose a direct count API,
    // but we can return the max as an estimate placeholder.
    // In production, this would iterate the napt table.
    return 0;
}

/*
 * Firewall - ACL packet filtering engine
 */

#ifndef FIREWALL_H
#define FIREWALL_H

#include <Arduino.h>
#include "config.h"

class Firewall {
public:
    static void init();
    static bool isAllowed(uint8_t protocol, uint32_t src_ip, uint16_t src_port,
                          uint32_t dst_ip, uint16_t dst_port);

    // Rule management
    static bool addRule(const ACLRule& rule);
    static bool removeRule(uint8_t index);
    static void clearRules();
    static uint8_t getRuleCount();
    static const ACLRule* getRule(uint8_t index);
    static const ACLRule* getRules();

    // Persistence
    static bool loadRules();
    static bool saveRules();

    // Stats
    static uint32_t getPacketsAllowed();
    static uint32_t getPacketsDenied();
    static void resetStats();

private:
    static ACLRule _rules[MAX_ACL_RULES];
    static uint8_t _ruleCount;
    static uint32_t _packetsAllowed;
    static uint32_t _packetsDenied;
};

#endif // FIREWALL_H

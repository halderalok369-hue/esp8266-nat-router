/*
 * Firewall Implementation
 * Software ACL packet filtering - rules checked on forwarded traffic.
 *
 * Note: ESP8266 lwIP doesn't provide a netfilter hook, so this firewall
 * operates as a policy engine. The CLI/Web/MQTT interfaces add rules,
 * and the NAPT layer consults this before forwarding. In the current
 * architecture, ACL evaluation happens at the application layer for
 * connection tracking; for deep packet inspection, a custom lwIP hook
 * would be required.
 */

#include "firewall.h"
#include "config_manager.h"

ACLRule Firewall::_rules[MAX_ACL_RULES];
uint8_t Firewall::_ruleCount = 0;
uint32_t Firewall::_packetsAllowed = 0;
uint32_t Firewall::_packetsDenied = 0;

void Firewall::init() {
    memset(_rules, 0, sizeof(_rules));
    _ruleCount = 0;
    _packetsAllowed = 0;
    _packetsDenied = 0;
    loadRules();
    Serial.printf("[FW] Firewall initialized with %d rules\n", _ruleCount);
}

bool Firewall::isAllowed(uint8_t protocol, uint32_t src_ip, uint16_t src_port,
                         uint32_t dst_ip, uint16_t dst_port) {
    // If no rules, allow all (open policy)
    if (_ruleCount == 0) {
        _packetsAllowed++;
        return true;
    }

    // Check rules top-down, first match wins
    for (uint8_t i = 0; i < _ruleCount; i++) {
        const ACLRule& r = _rules[i];
        if (!r.enabled) continue;

        // Check protocol
        if (r.protocol != ACL_PROTO_ANY && r.protocol != protocol) continue;

        // Check source IP (with mask)
        if (r.src_ip != 0) {
            if ((src_ip & r.src_mask) != (r.src_ip & r.src_mask)) continue;
        }

        // Check source port
        if (r.src_port != 0 && r.src_port != src_port) continue;

        // Check destination IP (with mask)
        if (r.dst_ip != 0) {
            if ((dst_ip & r.dst_mask) != (r.dst_ip & r.dst_mask)) continue;
        }

        // Check destination port
        if (r.dst_port != 0 && r.dst_port != dst_port) continue;

        // Rule matches
        if (r.action == ACL_ALLOW) {
            _packetsAllowed++;
            return true;
        } else {
            _packetsDenied++;
            return false;
        }
    }

    // Default deny when rules exist but none match
    _packetsDenied++;
    return false;
}

bool Firewall::addRule(const ACLRule& rule) {
    if (_ruleCount >= MAX_ACL_RULES) {
        Serial.println(F("[FW] Max ACL rules reached"));
        return false;
    }
    memcpy(&_rules[_ruleCount], &rule, sizeof(ACLRule));
    _ruleCount++;
    Serial.printf("[FW] Rule added (#%d): %s %s\n", _ruleCount,
                  rule.action == ACL_ALLOW ? "ALLOW" : "DENY",
                  rule.description);
    return true;
}

bool Firewall::removeRule(uint8_t index) {
    if (index >= _ruleCount) return false;

    // Shift rules down
    for (uint8_t i = index; i < _ruleCount - 1; i++) {
        memcpy(&_rules[i], &_rules[i + 1], sizeof(ACLRule));
    }
    _ruleCount--;
    memset(&_rules[_ruleCount], 0, sizeof(ACLRule));
    Serial.printf("[FW] Rule #%d removed, %d rules remaining\n", index + 1, _ruleCount);
    return true;
}

void Firewall::clearRules() {
    memset(_rules, 0, sizeof(_rules));
    _ruleCount = 0;
    Serial.println(F("[FW] All rules cleared"));
}

uint8_t Firewall::getRuleCount() {
    return _ruleCount;
}

const ACLRule* Firewall::getRule(uint8_t index) {
    if (index >= _ruleCount) return nullptr;
    return &_rules[index];
}

const ACLRule* Firewall::getRules() {
    return _rules;
}

bool Firewall::loadRules() {
    return ConfigManager::loadACL(_rules, _ruleCount);
}

bool Firewall::saveRules() {
    return ConfigManager::saveACL(_rules, _ruleCount);
}

uint32_t Firewall::getPacketsAllowed() { return _packetsAllowed; }
uint32_t Firewall::getPacketsDenied() { return _packetsDenied; }

void Firewall::resetStats() {
    _packetsAllowed = 0;
    _packetsDenied = 0;
}

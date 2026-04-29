/*
 * Unit tests for ESP8266 Router
 * These test the logic components in a native environment.
 * Run with: pio test -e native (if configured) or manually.
 */

#ifdef UNIT_TEST

#include <unity.h>

// Test ACL rule matching logic
// (Simplified standalone test of the matching algorithm)

#include <stdint.h>
#include <string.h>

// Replicate ACL structures for native testing
enum TestACLProtocol : uint8_t {
    TEST_ACL_PROTO_ANY = 0,
    TEST_ACL_PROTO_TCP = 6,
    TEST_ACL_PROTO_UDP = 17
};

enum TestACLAction : uint8_t {
    TEST_ACL_ALLOW = 0,
    TEST_ACL_DENY  = 1
};

struct TestACLRule {
    TestACLProtocol protocol;
    uint32_t src_ip;
    uint32_t src_mask;
    uint16_t src_port;
    uint32_t dst_ip;
    uint32_t dst_mask;
    uint16_t dst_port;
    TestACLAction action;
    bool enabled;
};

#define TEST_MAX_RULES 16

TestACLRule testRules[TEST_MAX_RULES];
uint8_t testRuleCount = 0;

bool testIsAllowed(uint8_t protocol, uint32_t src_ip, uint16_t src_port,
                   uint32_t dst_ip, uint16_t dst_port) {
    if (testRuleCount == 0) return true;

    for (uint8_t i = 0; i < testRuleCount; i++) {
        const TestACLRule& r = testRules[i];
        if (!r.enabled) continue;
        if (r.protocol != TEST_ACL_PROTO_ANY && r.protocol != protocol) continue;
        if (r.src_ip != 0 && ((src_ip & r.src_mask) != (r.src_ip & r.src_mask))) continue;
        if (r.src_port != 0 && r.src_port != src_port) continue;
        if (r.dst_ip != 0 && ((dst_ip & r.dst_mask) != (r.dst_ip & r.dst_mask))) continue;
        if (r.dst_port != 0 && r.dst_port != dst_port) continue;

        return r.action == TEST_ACL_ALLOW;
    }
    return false; // Default deny when rules exist
}

void setUp(void) {
    memset(testRules, 0, sizeof(testRules));
    testRuleCount = 0;
}

void tearDown(void) {}

void test_no_rules_allow_all(void) {
    TEST_ASSERT_TRUE(testIsAllowed(6, 0xC0A80401, 12345, 0x08080808, 80));
}

void test_deny_rule_blocks(void) {
    testRules[0].protocol = TEST_ACL_PROTO_TCP;
    testRules[0].dst_ip = 0x08080808;
    testRules[0].dst_mask = 0xFFFFFFFF;
    testRules[0].dst_port = 80;
    testRules[0].action = TEST_ACL_DENY;
    testRules[0].enabled = true;
    testRuleCount = 1;

    TEST_ASSERT_FALSE(testIsAllowed(6, 0xC0A80401, 12345, 0x08080808, 80));
}

void test_allow_rule_permits(void) {
    testRules[0].protocol = TEST_ACL_PROTO_TCP;
    testRules[0].dst_port = 443;
    testRules[0].action = TEST_ACL_ALLOW;
    testRules[0].enabled = true;
    testRuleCount = 1;

    TEST_ASSERT_TRUE(testIsAllowed(6, 0xC0A80401, 12345, 0x08080808, 443));
}

void test_first_match_wins(void) {
    // Rule 0: Allow TCP 443
    testRules[0].protocol = TEST_ACL_PROTO_TCP;
    testRules[0].dst_port = 443;
    testRules[0].action = TEST_ACL_ALLOW;
    testRules[0].enabled = true;

    // Rule 1: Deny all
    testRules[1].protocol = TEST_ACL_PROTO_ANY;
    testRules[1].action = TEST_ACL_DENY;
    testRules[1].enabled = true;
    testRuleCount = 2;

    // TCP 443 should be allowed (matches rule 0)
    TEST_ASSERT_TRUE(testIsAllowed(6, 0xC0A80401, 12345, 0x08080808, 443));
    // TCP 80 should be denied (matches rule 1)
    TEST_ASSERT_FALSE(testIsAllowed(6, 0xC0A80401, 12345, 0x08080808, 80));
}

void test_protocol_filter(void) {
    testRules[0].protocol = TEST_ACL_PROTO_UDP;
    testRules[0].action = TEST_ACL_ALLOW;
    testRules[0].enabled = true;
    testRuleCount = 1;

    // UDP should be allowed
    TEST_ASSERT_TRUE(testIsAllowed(17, 0xC0A80401, 12345, 0x08080808, 53));
    // TCP should be denied (default deny, UDP rule doesn't match)
    TEST_ASSERT_FALSE(testIsAllowed(6, 0xC0A80401, 12345, 0x08080808, 53));
}

void test_disabled_rule_skipped(void) {
    testRules[0].protocol = TEST_ACL_PROTO_ANY;
    testRules[0].action = TEST_ACL_ALLOW;
    testRules[0].enabled = false; // disabled!
    testRuleCount = 1;

    // Rule exists but disabled, so default deny applies
    TEST_ASSERT_FALSE(testIsAllowed(6, 0xC0A80401, 12345, 0x08080808, 80));
}

void test_subnet_mask(void) {
    testRules[0].protocol = TEST_ACL_PROTO_ANY;
    testRules[0].src_ip = 0xC0A80400;   // 192.168.4.0
    testRules[0].src_mask = 0xFFFFFF00;  // /24
    testRules[0].action = TEST_ACL_ALLOW;
    testRules[0].enabled = true;
    testRuleCount = 1;

    // 192.168.4.100 -> allowed
    TEST_ASSERT_TRUE(testIsAllowed(6, 0xC0A80464, 12345, 0x08080808, 80));
    // 192.168.5.100 -> denied
    TEST_ASSERT_FALSE(testIsAllowed(6, 0xC0A80564, 12345, 0x08080808, 80));
}

void test_any_protocol_matches_all(void) {
    testRules[0].protocol = TEST_ACL_PROTO_ANY;
    testRules[0].action = TEST_ACL_DENY;
    testRules[0].enabled = true;
    testRuleCount = 1;

    TEST_ASSERT_FALSE(testIsAllowed(6, 0xC0A80401, 12345, 0x08080808, 80));
    TEST_ASSERT_FALSE(testIsAllowed(17, 0xC0A80401, 12345, 0x08080808, 53));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_no_rules_allow_all);
    RUN_TEST(test_deny_rule_blocks);
    RUN_TEST(test_allow_rule_permits);
    RUN_TEST(test_first_match_wins);
    RUN_TEST(test_protocol_filter);
    RUN_TEST(test_disabled_rule_skipped);
    RUN_TEST(test_subnet_mask);
    RUN_TEST(test_any_protocol_matches_all);
    return UNITY_END();
}

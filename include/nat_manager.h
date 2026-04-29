/*
 * NAT Manager - NAPT initialization and port mapping
 */

#ifndef NAT_MANAGER_H
#define NAT_MANAGER_H

#include <Arduino.h>
#include "config.h"

class NATManager {
public:
    static bool init();
    static bool enable();
    static bool disable();
    static bool isEnabled();

    // Port forwarding
    static bool addPortForward(PortForwardRule& rule);
    static bool removePortForward(uint16_t external_port, uint8_t protocol);
    static void clearPortForwards();

    // Statistics
    static uint32_t getActiveConnections();

private:
    static bool _enabled;
};

#endif // NAT_MANAGER_H

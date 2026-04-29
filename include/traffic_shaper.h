/*
 * Traffic Shaper - Token bucket bandwidth limiting
 */

#ifndef TRAFFIC_SHAPER_H
#define TRAFFIC_SHAPER_H

#include <Arduino.h>
#include "config.h"

class TrafficShaper {
public:
    static void init();
    static void loop();

    static void setBandwidthLimit(uint32_t kbps);
    static uint32_t getBandwidthLimit();
    static bool isEnabled();

    // Per-client limits (by IP last octet)
    static void setClientLimit(uint8_t client_id, uint32_t kbps);
    static uint32_t getClientLimit(uint8_t client_id);

    // Stats
    static uint32_t getTotalBytesForwarded();
    static uint32_t getCurrentThroughputKbps();
    static void resetStats();

private:
    static uint32_t _globalLimitKbps;
    static uint32_t _clientLimits[256];
    static uint32_t _totalBytes;
    static uint32_t _currentKbps;
    static uint32_t _lastMeasureTime;
    static uint32_t _bytesSinceLastMeasure;
    static uint32_t _tokenBucket;
    static uint32_t _lastBucketRefill;
};

#endif // TRAFFIC_SHAPER_H

/*
 * Automesh - Automatic mesh networking for multiple ESP units
 */

#ifndef AUTOMESH_H
#define AUTOMESH_H

#include <Arduino.h>

struct MeshPeer {
    char ssid[33];
    int32_t rssi;
    uint8_t bssid[6];
    uint8_t channel;
    uint8_t hops;    // hops from root AP
    bool active;
};

#define MAX_MESH_PEERS 8

class Automesh {
public:
    static void init();
    static void loop();
    static void stop();

    static bool isEnabled();
    static void enable(bool state);

    static uint8_t getPeerCount();
    static const MeshPeer* getPeers();

    // Scan and connect to best upstream
    static bool scanAndConnect();

private:
    static bool _enabled;
    static MeshPeer _peers[MAX_MESH_PEERS];
    static uint8_t _peerCount;
    static unsigned long _lastScan;
    static uint8_t _myHops;

    static int comparePeers(const MeshPeer& a, const MeshPeer& b);
    static bool isMeshSSID(const String& ssid);
    static void updateAPSettings();
};

#endif // AUTOMESH_H

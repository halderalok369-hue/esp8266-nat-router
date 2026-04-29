/*
 * Automesh Implementation
 * Scans for mesh peers and auto-connects to the best upstream AP.
 *
 * Mesh topology: Each ESP node connects as STA to an upstream mesh node
 * (or root AP) and runs its own AP with the same SSID/password. Clients
 * and downstream nodes see a unified network.
 *
 * Peer selection: Prefer nodes with fewer hops and stronger RSSI.
 * Hop count is encoded in the AP SSID suffix or via a beacon IE field
 * (simplified: we use SSID prefix matching + RSSI).
 */

#include "automesh.h"
#include "config_manager.h"
#include "nat_manager.h"
#include <ESP8266WiFi.h>

bool Automesh::_enabled = false;
MeshPeer Automesh::_peers[MAX_MESH_PEERS];
uint8_t Automesh::_peerCount = 0;
unsigned long Automesh::_lastScan = 0;
uint8_t Automesh::_myHops = 255;

void Automesh::init() {
    RouterConfig& cfg = ConfigManager::getConfig();
    _enabled = cfg.automesh_enabled;
    _peerCount = 0;
    _myHops = 255;
    memset(_peers, 0, sizeof(_peers));

    if (_enabled) {
        Serial.printf("[MESH] Automesh enabled, prefix: %s\n", cfg.automesh_prefix);
    } else {
        Serial.println(F("[MESH] Automesh disabled"));
    }
}

void Automesh::loop() {
    if (!_enabled) return;

    unsigned long now = millis();
    if (now - _lastScan < AUTOMESH_SCAN_INTERVAL) return;
    _lastScan = now;

    // Only rescan if disconnected or periodically for better peer discovery
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("[MESH] STA disconnected, scanning for mesh peers..."));
        scanAndConnect();
    } else {
        // Periodic passive scan to update peer table
        int n = WiFi.scanNetworks(false, false);
        _peerCount = 0;

        for (int i = 0; i < n && _peerCount < MAX_MESH_PEERS; i++) {
            String ssid = WiFi.SSID(i);
            if (isMeshSSID(ssid) || ssid == String(ConfigManager::getConfig().sta_ssid)) {
                MeshPeer& p = _peers[_peerCount];
                strlcpy(p.ssid, ssid.c_str(), sizeof(p.ssid));
                p.rssi = WiFi.RSSI(i);
                memcpy(p.bssid, WiFi.BSSID(i), 6);
                p.channel = WiFi.channel(i);
                p.hops = 0; // Simplified: assume direct
                p.active = (memcmp(p.bssid, WiFi.BSSID(), 6) == 0);
                _peerCount++;
            }
        }
        WiFi.scanDelete();
    }
}

void Automesh::stop() {
    _enabled = false;
    Serial.println(F("[MESH] Automesh stopped"));
}

bool Automesh::isEnabled() {
    return _enabled;
}

void Automesh::enable(bool state) {
    _enabled = state;
    RouterConfig& cfg = ConfigManager::getConfig();
    cfg.automesh_enabled = state;

    if (state) {
        Serial.println(F("[MESH] Automesh enabled"));
        _lastScan = 0; // Force immediate scan
    } else {
        Serial.println(F("[MESH] Automesh disabled"));
    }
}

uint8_t Automesh::getPeerCount() {
    return _peerCount;
}

const MeshPeer* Automesh::getPeers() {
    return _peers;
}

bool Automesh::scanAndConnect() {
    RouterConfig& cfg = ConfigManager::getConfig();

    Serial.println(F("[MESH] Scanning for upstream networks..."));
    int n = WiFi.scanNetworks(false, false);

    if (n <= 0) {
        Serial.println(F("[MESH] No networks found"));
        return false;
    }

    // Collect mesh-compatible APs
    _peerCount = 0;
    for (int i = 0; i < n && _peerCount < MAX_MESH_PEERS; i++) {
        String ssid = WiFi.SSID(i);
        int32_t rssi = WiFi.RSSI(i);

        // Accept the configured upstream SSID or any mesh-prefixed SSID
        bool isMesh = isMeshSSID(ssid);
        bool isUpstream = (ssid == String(cfg.sta_ssid));

        if ((isMesh || isUpstream) && rssi > AUTOMESH_RSSI_THRESHOLD) {
            MeshPeer& p = _peers[_peerCount];
            strlcpy(p.ssid, ssid.c_str(), sizeof(p.ssid));
            p.rssi = rssi;
            memcpy(p.bssid, WiFi.BSSID(i), 6);
            p.channel = WiFi.channel(i);
            p.hops = isUpstream ? 0 : 1; // Root AP = 0 hops
            p.active = false;
            _peerCount++;
        }
    }
    WiFi.scanDelete();

    if (_peerCount == 0) {
        Serial.println(F("[MESH] No suitable peers found"));
        return false;
    }

    // Sort: prefer fewer hops, then stronger RSSI
    for (uint8_t i = 0; i < _peerCount - 1; i++) {
        for (uint8_t j = i + 1; j < _peerCount; j++) {
            if (comparePeers(_peers[i], _peers[j]) > 0) {
                MeshPeer tmp = _peers[i];
                _peers[i] = _peers[j];
                _peers[j] = tmp;
            }
        }
    }

    // Connect to the best peer
    MeshPeer& best = _peers[0];
    Serial.printf("[MESH] Connecting to: %s (RSSI: %d, hops: %d)\n",
                  best.ssid, best.rssi, best.hops);

    WiFi.begin(best.ssid, cfg.sta_password, best.channel, best.bssid);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < STA_CONNECT_TIMEOUT) {
        delay(100);
        yield();
    }

    if (WiFi.status() == WL_CONNECTED) {
        best.active = true;
        _myHops = best.hops + 1;
        Serial.printf("[MESH] Connected! IP: %s, hops from root: %d\n",
                      WiFi.localIP().toString().c_str(), _myHops);
        updateAPSettings();
        return true;
    }

    Serial.println(F("[MESH] Connection failed"));
    return false;
}

int Automesh::comparePeers(const MeshPeer& a, const MeshPeer& b) {
    // Fewer hops is better
    if (a.hops != b.hops) return (int)a.hops - (int)b.hops;
    // Stronger signal is better (RSSI is negative, higher = stronger)
    return b.rssi - a.rssi;
}

bool Automesh::isMeshSSID(const String& ssid) {
    RouterConfig& cfg = ConfigManager::getConfig();
    return ssid.startsWith(String(cfg.automesh_prefix));
}

void Automesh::updateAPSettings() {
    RouterConfig& cfg = ConfigManager::getConfig();

    // In automesh mode, the AP mirrors the upstream SSID
    // so clients see a unified network. The BSSID differs per node.
    if (_enabled && strlen(cfg.sta_ssid) > 0) {
        // Use upstream SSID for seamless roaming
        WiFi.softAP(cfg.sta_ssid, cfg.sta_password, cfg.ap_channel,
                     cfg.ap_hidden, cfg.ap_max_conn);
        Serial.printf("[MESH] AP SSID updated to match upstream: %s\n", cfg.sta_ssid);
    }
}

# ESP8266 WiFi NAT Router & Range Extender

A full-featured WiFi NAT router, range extender, and mesh gateway firmware for ESP8266 microcontrollers. Connects to an upstream WiFi network as a station (STA) while simultaneously running its own Access Point (AP) with transparent NAT routing.

## Features

### Core Networking
- **Dual WiFi Mode** — Simultaneous STA (client) + SoftAP (access point)
- **Transparent NAT/NAPT** — Full IP masquerading using lwIP NAPT, forwards all TCP/UDP/ICMP traffic from AP clients to the upstream network
- **DHCP Server** — Automatic IP assignment on 192.168.4.0/24 subnet (range: .100-.200)
- **DNS Forwarding** — AP clients automatically use upstream DNS servers
- **Captive Portal** — Automatic redirect to config page for unconfigured devices

### Web Configuration Portal (192.168.4.1)
- Real-time system dashboard with live stats
- WiFi network scanner with signal strength indicators
- Upstream SSID & password configuration
- Local AP SSID, password, channel, and client limit settings
- Connected client list with MAC and IP addresses
- Firewall rule viewer
- Port forwarding viewer
- Traffic shaping controls
- MQTT and Automesh toggles
- Responsive dark-theme UI optimized for mobile

### CLI (Serial + Telnet)
Full command-line interface accessible via:
- **Serial**: 115200 baud, connect via USB
- **Telnet**: Port 23, connect to 192.168.4.1

#### Available Commands:
| Command | Description |
|---------|-------------|
| `help` | Show all available commands |
| `status` | Full system status overview |
| `show <sta\|ap\|mqtt\|mesh\|dns\|all>` | Display configuration |
| `set <key> <value>` | Change settings (sta_ssid, sta_pass, ap_ssid, ap_pass, ap_channel, hostname, dns1, dns2) |
| `acl list` | Show firewall rules |
| `acl add <tcp\|udp\|any> <src\|*> <dst\|*> <allow\|deny> [desc]` | Add ACL rule |
| `acl remove <index>` | Remove ACL rule |
| `acl clear` | Remove all rules |
| `acl stats` | Firewall statistics |
| `portforward list` | Show port forwards |
| `pf add <tcp\|udp> <ext_port> <int_ip> <int_port> [desc]` | Add port forward |
| `pf remove <index>` | Remove port forward |
| `shape status` | Traffic shaping status |
| `shape limit <kbps>` | Set global bandwidth limit |
| `shape client <id> <kbps>` | Per-client bandwidth limit |
| `shape off` | Disable traffic shaping |
| `mqtt status` | MQTT connection status |
| `mqtt enable/disable` | Toggle MQTT reporting |
| `mqtt set <server\|port\|user\|pass\|topic> <value>` | Configure MQTT |
| `gpio status` | Show all GPIO states |
| `gpio set <pin> <0\|1>` | Digital output |
| `gpio pwm <pin> <0-1023>` | PWM output |
| `gpio read <pin>` | Digital input |
| `gpio adc` | Read analog (A0) |
| `mesh status` | Automesh status |
| `mesh enable/disable` | Toggle automesh |
| `mesh scan` | Force peer discovery |
| `mesh peers` | List discovered mesh peers |
| `scan` | Scan WiFi networks |
| `clients` | List connected AP clients |
| `save` | Save configuration to flash |
| `reboot` | Reboot the device |
| `factory-reset` | Erase all settings |

### Firewall ACLs
- Up_to 16 rules with protocol, source/destination IP, port, and action
- Top-down rule matching (first match wins)
- Default policy: allow all when no rules; deny-by-default when rules exist
- Packet counters for allowed/denied traffic
- Persistent storage in LittleFS

### Port Forwarding
- Up_to 16 port forward rules
- TCP and UDP support
- Maps external ports on the STA interface to internal AP clients
- Persistent storage

### Traffic Shaping
- Global bandwidth limiting (token bucket algorithm)
- Per-client bandwidth limits (by IP last octet)
- Real-time throughput monitoring
- Configurable via CLI, web, or MQTT

### MQTT Status Reporting
- Periodic status reports (JSON) every 30 seconds
- Topics: `<prefix>/status`, `<prefix>/cmd/#`
- Remote commands via MQTT: reboot, GPIO control, status query
- Last Will and Testament (LWT) for online/offline detection
- Authentication support

### GPIO Control
- Digital read/write on pins: 0, 2, 4, 5, 12, 13, 14, 15, 16
- PWM output (0-1023) on supported pins
- Analog read on A0
- Accessible via CLI, MQTT, and Web API

### Automesh Mode
- Automatic peer discovery via WiFi scanning
- Connects to the best upstream peer based on hop count and RSSI
- Multiple ESP units form a self-organizing mesh
- Seamless roaming: all mesh nodes broadcast the same SSID
- Periodic re-scanning for better paths
- Configurable RSSI threshold (-80 dBm default)

## Hardware Requirements

| Board | Flash | Notes |
|-------|-------|-------|
| NodeMCU v2/v3 | 4MB | Recommended, full features |
| Wemos D1 Mini | 4MB | Compact form factor |
| ESP-12E/F | 4MB | Module only, needs breakout |
| ESP-01S | 1MB | Minimal build, reduced features |

## License

MIT License. See individual source files for attribution.

## Credits

- [ESP8266 Arduino Core](https://github.com/esp8266/Arduino) — WiFi + lwIP stack
- [martin-ger/esp_wifi_repeater](https://github.com/martin-ger/esp_wifi_repeater) — NAT router inspiration
- [lwIP NAPT](https://github.com/martin-ger/esp-open-lwip) — NAT/NAPT extensions
- [ArduinoJson](https://arduinojson.org/) — JSON serialization
- [PubSubClient](https://pubsubclient.knolleary.net/) — MQTT library

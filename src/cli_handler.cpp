/*
 * CLI Handler Implementation
 * Processes commands from Serial and Telnet connections.
 */

#include "cli_handler.h"
#include "config_manager.h"
#include "nat_manager.h"
#include "firewall.h"
#include "traffic_shaper.h"
#include "mqtt_reporter.h"
#include "automesh.h"
#include "gpio_control.h"
#include <ESP8266WiFi.h>

WiFiServer CLIHandler::_telnetServer(TELNET_PORT);
WiFiClient CLIHandler::_telnetClients[TELNET_MAX_CLIENTS];
String CLIHandler::_serialBuffer;
String CLIHandler::_telnetBuffers[TELNET_MAX_CLIENTS];
bool CLIHandler::_running = false;

void CLIHandler::init() {
    _telnetServer.begin();
    _telnetServer.setNoDelay(true);
    _serialBuffer = "";
    for (int i = 0; i < TELNET_MAX_CLIENTS; i++) {
        _telnetBuffers[i] = "";
    }
    _running = true;
    Serial.println(F("[CLI] Command interface ready (Serial + Telnet:" + String(TELNET_PORT) + ")"));
    Serial.println(F("Type 'help' for commands."));
}

void CLIHandler::loop() {
    if (!_running) return;

    // Accept new telnet clients
    if (_telnetServer.hasClient()) {
        bool accepted = false;
        for (int i = 0; i < TELNET_MAX_CLIENTS; i++) {
            if (!_telnetClients[i] || !_telnetClients[i].connected()) {
                _telnetClients[i] = _telnetServer.accept();
                _telnetClients[i].println(F("=== ESP8266 NAT Router CLI ==="));
                _telnetClients[i].println(F("Type 'help' for commands."));
                _telnetClients[i].print(F("router> "));
                _telnetBuffers[i] = "";
                accepted = true;
                Serial.printf("[CLI] Telnet client %d connected\n", i);
                break;
            }
        }
        if (!accepted) {
            _telnetServer.accept().println(F("Max clients reached. Bye."));
        }
    }

    // Read serial input
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (_serialBuffer.length() > 0) {
                processCommand(_serialBuffer, Serial);
                _serialBuffer = "";
                Serial.print(F("router> "));
            }
        } else {
            _serialBuffer += c;
        }
    }

    // Read telnet input
    for (int i = 0; i < TELNET_MAX_CLIENTS; i++) {
        if (_telnetClients[i] && _telnetClients[i].connected()) {
            while (_telnetClients[i].available()) {
                char c = _telnetClients[i].read();
                if (c == '\n' || c == '\r') {
                    if (_telnetBuffers[i].length() > 0) {
                        processCommand(_telnetBuffers[i], _telnetClients[i]);
                        _telnetBuffers[i] = "";
                        _telnetClients[i].print(F("router> "));
                    }
                } else if (c >= 32 && c < 127) {
                    _telnetBuffers[i] += c;
                }
            }
        }
    }
}

void CLIHandler::stop() {
    for (int i = 0; i < TELNET_MAX_CLIENTS; i++) {
        if (_telnetClients[i]) _telnetClients[i].stop();
    }
    _telnetServer.stop();
    _running = false;
}

void CLIHandler::broadcast(const String& msg) {
    Serial.println(msg);
    for (int i = 0; i < TELNET_MAX_CLIENTS; i++) {
        if (_telnetClients[i] && _telnetClients[i].connected()) {
            _telnetClients[i].println(msg);
        }
    }
}

void CLIHandler::processCommand(const String& cmd, Print& output) {
    String trimmed = cmd;
    trimmed.trim();

    if (trimmed.length() == 0) return;

    // Parse command and arguments
    int spaceIdx = trimmed.indexOf(' ');
    String command, args;
    if (spaceIdx > 0) {
        command = trimmed.substring(0, spaceIdx);
        args = trimmed.substring(spaceIdx + 1);
        args.trim();
    } else {
        command = trimmed;
        args = "";
    }

    command.toLowerCase();

    if (command == "help" || command == "?")          printHelp(output);
    else if (command == "status")                     cmdStatus(output);
    else if (command == "show")                       cmdShow(args, output);
    else if (command == "set")                        cmdSet(args, output);
    else if (command == "acl")                        cmdACL(args, output);
    else if (command == "portforward" || command == "pf") cmdPortForward(args, output);
    else if (command == "shape" || command == "qos")  cmdTrafficShape(args, output);
    else if (command == "mqtt")                       cmdMQTT(args, output);
    else if (command == "gpio")                       cmdGPIO(args, output);
    else if (command == "mesh" || command == "automesh") cmdAutomesh(args, output);
    else if (command == "scan")                       cmdScan(output);
    else if (command == "clients")                    cmdClients(output);
    else if (command == "save")                       cmdSave(output);
    else if (command == "reboot" || command == "restart") cmdReboot(output);
    else if (command == "factory-reset")              cmdFactoryReset(output);
    else output.println("Unknown command: " + command + ". Type 'help' for commands.");
}

void CLIHandler::printHelp(Print& output) {
    output.println(F("=== ESP8266 NAT Router Commands ==="));
    output.println(F("status               - System status overview"));
    output.println(F("show <item>          - Show config (sta|ap|mqtt|mesh|dns|all)"));
    output.println(F("set <key> <value>    - Set config value"));
    output.println(F("  sta_ssid <ssid>    - Set upstream SSID"));
    output.println(F("  sta_pass <pass>    - Set upstream password"));
    output.println(F("  ap_ssid <ssid>     - Set AP SSID"));
    output.println(F("  ap_pass <pass>     - Set AP password"));
    output.println(F("  ap_channel <1-13>  - Set AP channel"));
    output.println(F("  hostname <name>    - Set hostname"));
    output.println(F("  dns1 <ip>          - Set primary DNS"));
    output.println(F("  dns2 <ip>          - Set secondary DNS"));
    output.println(F("acl <action>         - Firewall ACL management"));
    output.println(F("  list               - List all rules"));
    output.println(F("  add <proto> <src> <dst> <allow|deny> [desc]"));
    output.println(F("  remove <index>     - Remove rule by index"));
    output.println(F("  clear              - Remove all rules"));
    output.println(F("  stats              - Show firewall stats"));
    output.println(F("portforward <action> - Port forwarding"));
    output.println(F("  list               - List port forwards"));
    output.println(F("  add <proto> <ext_port> <int_ip> <int_port> [desc]"));
    output.println(F("  remove <index>     - Remove by index"));
    output.println(F("shape <action>       - Traffic shaping"));
    output.println(F("  status             - Show shaping status"));
    output.println(F("  limit <kbps>       - Set global bandwidth limit"));
    output.println(F("  client <id> <kbps> - Set per-client limit"));
    output.println(F("  off                - Disable shaping"));
    output.println(F("mqtt <action>        - MQTT control"));
    output.println(F("  status             - Show MQTT status"));
    output.println(F("  enable/disable     - Toggle MQTT"));
    output.println(F("  set <key> <val>    - Configure (server|port|user|pass|topic)"));
    output.println(F("gpio <action>        - GPIO control"));
    output.println(F("  status             - Show all GPIO states"));
    output.println(F("  set <pin> <0|1>    - Set digital output"));
    output.println(F("  pwm <pin> <0-1023> - Set PWM output"));
    output.println(F("  read <pin>         - Read digital input"));
    output.println(F("  adc                - Read analog (A0)"));
    output.println(F("mesh <action>        - Automesh control"));
    output.println(F("  status             - Show mesh status"));
    output.println(F("  enable/disable     - Toggle automesh"));
    output.println(F("  scan               - Force mesh scan"));
    output.println(F("  peers              - List mesh peers"));
    output.println(F("scan                 - Scan WiFi networks"));
    output.println(F("clients              - List connected AP clients"));
    output.println(F("save                 - Save configuration"));
    output.println(F("reboot               - Reboot the device"));
    output.println(F("factory-reset        - Erase all settings"));
}

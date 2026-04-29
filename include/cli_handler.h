/*
 * CLI Handler - Serial and Telnet command interface
 */

#ifndef CLI_HANDLER_H
#define CLI_HANDLER_H

#include <Arduino.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include "config.h"

class CLIHandler {
public:
    static void init();
    static void loop();
    static void stop();

    // Output to all connected clients + serial
    static void broadcast(const String& msg);

private:
    static WiFiServer _telnetServer;
    static WiFiClient _telnetClients[TELNET_MAX_CLIENTS];
    static String _serialBuffer;
    static String _telnetBuffers[TELNET_MAX_CLIENTS];
    static bool _running;

    // Command processing
    static void processCommand(const String& cmd, Print& output);
    static void printHelp(Print& output);

    // Command handlers
    static void cmdStatus(Print& output);
    static void cmdShow(const String& args, Print& output);
    static void cmdSet(const String& args, Print& output);
    static void cmdACL(const String& args, Print& output);
    static void cmdPortForward(const String& args, Print& output);
    static void cmdTrafficShape(const String& args, Print& output);
    static void cmdMQTT(const String& args, Print& output);
    static void cmdGPIO(const String& args, Print& output);
    static void cmdAutomesh(const String& args, Print& output);
    static void cmdScan(Print& output);
    static void cmdClients(Print& output);
    static void cmdReboot(Print& output);
    static void cmdSave(Print& output);
    static void cmdFactoryReset(Print& output);

    // Helpers
    static IPAddress parseIP(const String& str);
    static String formatMAC(const uint8_t* mac);
};

#endif // CLI_HANDLER_H

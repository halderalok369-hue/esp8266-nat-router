/*
 * Web Configuration Portal
 */

#ifndef WEB_PORTAL_H
#define WEB_PORTAL_H

#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

class WebPortal {
public:
    static void init();
    static void loop();
    static void stop();

private:
    static ESP8266WebServer _server;
    static DNSServer _dnsServer;
    static bool _running;

    // Route handlers
    static void handleRoot();
    static void handleScan();
    static void handleConfig();
    static void handleConfigSave();
    static void handleStatus();
    static void handleReboot();
    static void handleNotFound();

    // File serving
    static String getContentType(const String& filename);
    static bool handleFileRead(String path);

    // Helpers
    static String htmlHeader(const String& title);
    static String htmlFooter();
};

#endif // WEB_PORTAL_H

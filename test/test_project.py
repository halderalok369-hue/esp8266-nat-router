"""
ESP8266 Router Project Validation Tests
Validates project structure, file integrity, code patterns, and completeness.
"""

import os
import re
import json
import pytest

PROJECT_ROOT = os.path.join(os.path.dirname(__file__), '..')


class TestProjectStructure:
    """Verify all required files exist with proper structure."""

    def test_platformio_ini_exists(self):
        assert os.path.isfile(os.path.join(PROJECT_ROOT, 'platformio.ini'))

    def test_readme_exists(self):
        assert os.path.isfile(os.path.join(PROJECT_ROOT, 'README.md'))

    def test_source_directory(self):
        src = os.path.join(PROJECT_ROOT, 'src')
        assert os.path.isdir(src)

    def test_include_directory(self):
        inc = os.path.join(PROJECT_ROOT, 'include')
        assert os.path.isdir(inc)

    def test_data_directory(self):
        data = os.path.join(PROJECT_ROOT, 'data')
        assert os.path.isdir(data)

    @pytest.mark.parametrize("filename", [
        'src/main.cpp',
        'src/config_manager.cpp',
        'src/nat_manager.cpp',
        'src/web_portal.cpp',
        'src/cli_handler.cpp',
        'src/firewall.cpp',
        'src/traffic_shaper.cpp',
        'src/mqtt_reporter.cpp',
        'src/automesh.cpp',
        'src/gpio_control.cpp',
    ])
    def test_source_files_exist(self, filename):
        path = os.path.join(PROJECT_ROOT, filename)
        assert os.path.isfile(path), f"Missing source file: {filename}"

    @pytest.mark.parametrize("filename", [
        'include/config.h',
        'include/config_manager.h',
        'include/nat_manager.h',
        'include/web_portal.h',
        'include/cli_handler.h',
        'include/firewall.h',
        'include/traffic_shaper.h',
        'include/mqtt_reporter.h',
        'include/automesh.h',
        'include/gpio_control.h',
    ])
    def test_header_files_exist(self, filename):
        path = os.path.join(PROJECT_ROOT, filename)
        assert os.path.isfile(path), f"Missing header file: {filename}"

    @pytest.mark.parametrize("filename", [
        'data/index.html',
        'data/style.css',
        'data/app.js',
    ])
    def test_web_files_exist(self, filename):
        path = os.path.join(PROJECT_ROOT, filename)
        assert os.path.isfile(path), f"Missing web file: {filename}"


class TestHeaderGuards:
    """Verify all headers have proper include guards."""

    @pytest.mark.parametrize("header", [
        'config.h', 'config_manager.h', 'nat_manager.h', 'web_portal.h',
        'cli_handler.h', 'firewall.h', 'traffic_shaper.h',
        'mqtt_reporter.h', 'automesh.h', 'gpio_control.h',
    ])
    def test_include_guards(self, header):
        path = os.path.join(PROJECT_ROOT, 'include', header)
        content = open(path).read()
        guard = header.upper().replace('.', '_')
        assert f'#ifndef {guard}' in content, f"Missing #ifndef guard in {header}"
        assert f'#define {guard}' in content, f"Missing #define guard in {header}"
        assert f'#endif' in content, f"Missing #endif in {header}"


class TestSourceImplementations:
    """Verify source files implement their headers properly."""

    def _read(self, relpath):
        return open(os.path.join(PROJECT_ROOT, relpath)).read()

    def test_main_includes_all_headers(self):
        main = self._read('src/main.cpp')
        required = ['config.h', 'config_manager.h', 'nat_manager.h', 'web_portal.h',
                     'cli_handler.h', 'firewall.h', 'traffic_shaper.h',
                     'mqtt_reporter.h', 'automesh.h', 'gpio_control.h']
        for h in required:
            assert f'#include "{h}"' in main, f"main.cpp missing include: {h}"

    def test_main_has_setup_and_loop(self):
        main = self._read('src/main.cpp')
        assert 'void setup()' in main, "main.cpp missing setup()"
        assert 'void loop()' in main, "main.cpp missing loop()"

    def test_main_wifi_ap_sta_mode(self):
        main = self._read('src/main.cpp')
        assert 'WIFI_AP_STA' in main, "main.cpp should use WIFI_AP_STA mode"

    def test_main_napt_init(self):
        main = self._read('src/main.cpp')
        assert 'NATManager::init()' in main, "main.cpp should initialize NAPT"
        assert 'NATManager::enable()' in main, "main.cpp should enable NAPT"

    def test_nat_manager_uses_lwip(self):
        nat = self._read('src/nat_manager.cpp')
        assert 'ip_napt_init' in nat, "NAT manager should call ip_napt_init"
        assert 'ip_napt_enable_no' in nat, "NAT manager should call ip_napt_enable_no"

    def test_config_manager_uses_littlefs(self):
        cfg = self._read('src/config_manager.cpp')
        assert 'LittleFS' in cfg, "Config manager should use LittleFS"
        assert 'ArduinoJson' in cfg or 'deserializeJson' in cfg

    def test_web_portal_has_endpoints(self):
        web = self._read('src/web_portal.cpp')
        required_endpoints = ['/api/config', '/api/status', '/scan', '/api/reboot',
                              '/api/acl', '/api/portforward', '/api/clients']
        for ep in required_endpoints:
            assert ep in web, f"Web portal missing endpoint: {ep}"

    def test_cli_has_all_commands(self):
        cli = self._read('src/cli_handler.cpp')
        commands = ['acl', 'portforward', 'shape', 'mqtt', 'gpio',
                    'automesh', 'scan', 'clients', 'reboot', 'save']
        for cmd in commands:
            assert cmd in cli.lower(), f"CLI missing command handler: {cmd}"

    def test_firewall_rule_matching(self):
        fw = self._read('src/firewall.cpp')
        assert 'isAllowed' in fw
        assert 'ACL_ALLOW' in fw
        assert 'DENY' in fw  # deny case handled via else branch

    def test_mqtt_has_publish_subscribe(self):
        mqtt = self._read('src/mqtt_reporter.cpp')
        assert 'publish' in mqtt.lower()
        assert 'subscribe' in mqtt.lower()
        assert 'onMessage' in mqtt

    def test_automesh_scan_and_connect(self):
        mesh = self._read('src/automesh.cpp')
        assert 'scanAndConnect' in mesh
        assert 'scanNetworks' in mesh
        assert 'isMeshSSID' in mesh

    def test_traffic_shaper_token_bucket(self):
        ts = self._read('src/traffic_shaper.cpp')
        assert 'tokenBucket' in ts or '_tokenBucket' in ts
        assert 'setBandwidthLimit' in ts

    def test_gpio_control_has_operations(self):
        gpio = self._read('src/gpio_control.cpp')
        assert 'setPin' in gpio
        assert 'readPin' in gpio
        assert 'analogRead' in gpio or 'readAnalog' in gpio


class TestConfigDefaults:
    """Verify configuration defaults are sensible."""

    def test_ap_subnet(self):
        config = open(os.path.join(PROJECT_ROOT, 'include/config.h')).read()
        assert '192, 168, 4, 1' in config, "AP should use 192.168.4.1"
        assert '255, 255, 255, 0' in config, "AP should use /24 subnet"

    def test_default_ap_settings(self):
        config = open(os.path.join(PROJECT_ROOT, 'include/config.h')).read()
        assert 'DEFAULT_AP_SSID' in config
        assert 'DEFAULT_AP_PASSWORD' in config

    def test_napt_limits_defined(self):
        config = open(os.path.join(PROJECT_ROOT, 'include/config.h')).read()
        assert 'NAPT_MAX' in config
        assert 'PORTMAP_MAX' in config

    def test_acl_structures_defined(self):
        config = open(os.path.join(PROJECT_ROOT, 'include/config.h')).read()
        assert 'struct ACLRule' in config
        assert 'struct PortForwardRule' in config
        assert 'struct RouterConfig' in config
        assert 'MAX_ACL_RULES' in config


class TestWebUI:
    """Verify web UI completeness."""

    def test_html_has_all_tabs(self):
        html = open(os.path.join(PROJECT_ROOT, 'data/index.html')).read()
        tabs = ['dashboard', 'wifi', 'clients', 'firewall', 'portforward', 'advanced']
        for tab in tabs:
            assert tab in html, f"HTML missing tab: {tab}"

    def test_html_references_css_and_js(self):
        html = open(os.path.join(PROJECT_ROOT, 'data/index.html')).read()
        assert 'style.css' in html
        assert 'app.js' in html

    def test_js_has_api_calls(self):
        js = open(os.path.join(PROJECT_ROOT, 'data/app.js')).read()
        assert '/api/status' in js
        assert '/api/config' in js
        assert '/scan' in js

    def test_css_has_responsive_design(self):
        css = open(os.path.join(PROJECT_ROOT, 'data/style.css')).read()
        assert '@media' in css, "CSS should have responsive media queries"

    def test_html_has_wifi_config_fields(self):
        html = open(os.path.join(PROJECT_ROOT, 'data/index.html')).read()
        assert 'cfg-sta-ssid' in html, "Missing upstream SSID field"
        assert 'cfg-sta-pass' in html, "Missing upstream password field"
        assert 'cfg-ap-ssid' in html, "Missing AP SSID field"
        assert 'cfg-ap-pass' in html, "Missing AP password field"


class TestPlatformIOConfig:
    """Verify PlatformIO project configuration."""

    def test_platformio_has_esp8266(self):
        ini = open(os.path.join(PROJECT_ROOT, 'platformio.ini')).read()
        assert 'espressif8266' in ini
        assert 'framework = arduino' in ini

    def test_platformio_has_dependencies(self):
        ini = open(os.path.join(PROJECT_ROOT, 'platformio.ini')).read()
        assert 'ArduinoJson' in ini
        assert 'PubSubClient' in ini
        assert 'ESPAsyncWebServer' in ini or 'ESP8266WebServer' in ini

    def test_platformio_napt_flags(self):
        ini = open(os.path.join(PROJECT_ROOT, 'platformio.ini')).read()
        assert 'IP_FORWARD' in ini
        assert 'IP_NAPT' in ini
        assert 'LWIP' in ini.upper() or 'lwip' in ini.lower()

    def test_platformio_has_littlefs(self):
        ini = open(os.path.join(PROJECT_ROOT, 'platformio.ini')).read()
        assert 'littlefs' in ini.lower()

    def test_platformio_has_multiple_envs(self):
        ini = open(os.path.join(PROJECT_ROOT, 'platformio.ini')).read()
        assert '[env:esp8266-router]' in ini
        assert '[env:esp01-router]' in ini or '[env:esp8266-router-debug]' in ini


class TestReadme:
    """Verify README completeness."""

    def test_readme_has_features(self):
        readme = open(os.path.join(PROJECT_ROOT, 'README.md')).read()
        features = ['NAT', 'NAPT', 'DHCP', 'Firewall', 'ACL', 'Port Forward',
                     'MQTT', 'Automesh', 'GPIO', 'Traffic Shap']
        for feat in features:
            assert feat.lower() in readme.lower(), f"README missing feature: {feat}"

    def test_readme_has_build_instructions(self):
        readme = open(os.path.join(PROJECT_ROOT, 'README.md')).read()
        assert 'pio run' in readme or 'platformio' in readme.lower()
        assert 'upload' in readme.lower()

    def test_readme_has_api_docs(self):
        readme = open(os.path.join(PROJECT_ROOT, 'README.md')).read()
        assert '/api/status' in readme
        assert '/api/config' in readme

    def test_readme_has_project_structure(self):
        readme = open(os.path.join(PROJECT_ROOT, 'README.md')).read()
        assert 'main.cpp' in readme
        assert 'config.h' in readme


class TestCodeQuality:
    """Verify code quality patterns."""

    def _read_all_cpp(self):
        content = ""
        for d in ['src', 'include']:
            dirpath = os.path.join(PROJECT_ROOT, d)
            for f in os.listdir(dirpath):
                content += open(os.path.join(dirpath, f)).read()
        return content

    def test_no_hardcoded_wifi_passwords(self):
        """Ensure default passwords are clearly marked as defaults."""
        config = open(os.path.join(PROJECT_ROOT, 'include/config.h')).read()
        # Default STA password should be empty
        assert 'DEFAULT_STA_PASSWORD  ""' in config or "DEFAULT_STA_PASSWORD  \"\"" in config

    def test_no_memory_leaks_patterns(self):
        """Check for common ESP8266 memory issues."""
        all_code = self._read_all_cpp()
        # Should use F() macro for string literals to save RAM
        assert 'F(' in all_code, "Should use F() macro for flash strings"

    def test_serial_baud_rate(self):
        main = open(os.path.join(PROJECT_ROOT, 'src/main.cpp')).read()
        assert '115200' in main, "Serial should be configured at 115200 baud"

    def test_yield_in_main_loop(self):
        main = open(os.path.join(PROJECT_ROOT, 'src/main.cpp')).read()
        assert 'yield()' in main, "main loop should call yield() for WDT"

    def test_file_sizes_reasonable(self):
        """ESP8266 has limited flash; check file sizes aren't too large."""
        for fname in ['data/index.html', 'data/style.css', 'data/app.js']:
            path = os.path.join(PROJECT_ROOT, fname)
            size = os.path.getsize(path)
            assert size < 50000, f"{fname} is {size} bytes, should be < 50KB for ESP8266"
            assert size > 100, f"{fname} is suspiciously small ({size} bytes)"

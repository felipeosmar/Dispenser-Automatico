/**
 * ESP32 Dispenser Automatico
 *
 * Features:
 * - WiFi Manager (AP mode and Station mode)
 * - Web interface served from LittleFS
 * - File manager (upload, download, edit, delete)
 * - NTP Manager (time synchronization)
 * - Configuration via JSON file
 * - Over-the-air (OTA) firmware updates
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"
#include "web_server.h"
#include "spiffs_manager.h"
#include "ntp_manager.h"
#include "stepper_manager.h"

// Global objects
WebServerManager webServerManager;
SPIFFSManager spiffsManager;
NTPManager ntpManager;
StepperManager stepperManager;

// Mutex for SPIFFS access
SemaphoreHandle_t spiffsMutex = NULL;

// WiFi configuration (global for web server access)
String wifiSSID = WIFI_SSID_DEFAULT;
String wifiPassword = WIFI_PASS_DEFAULT;
bool wifiAPMode = WIFI_AP_MODE_DEFAULT;
bool wifiConnectedToInternet = false;

// Configuration
struct Config {
    char ssid[32];
    char password[64];
    bool apMode;
} config;

// Function declarations
void setupWiFi();
bool loadConfig();
void setDefaultConfig();

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println(F("\n\n=== ESP32 Dispenser Automatico ==="));

    // Create mutex for SPIFFS access
    spiffsMutex = xSemaphoreCreateMutex();
    if (spiffsMutex == NULL) {
        Serial.println(F("Failed to create SPIFFS mutex!"));
    }

    // Initialize SPIFFS
    Serial.println(F("Initializing LittleFS..."));
    if (!spiffsManager.begin()) {
        Serial.println(F("SPIFFS initialization failed!"));
        Serial.println(F("ERROR: Cannot continue without SPIFFS"));
        while(1) {
            delay(1000);
            Serial.println(F("System halted - SPIFFS required"));
        }
    } else {
        Serial.println(F("LittleFS initialized successfully"));
    }

    // Load configuration from SPIFFS
    if (!loadConfig()) {
        Serial.println(F("Failed to load config, using defaults"));
        setDefaultConfig();
    }

    // Setup WiFi
    setupWiFi();

    // Initialize NTP (only if connected to internet)
    if (wifiConnectedToInternet) {
        ntpManager.begin();
    } else {
        Serial.println(F("NTP Client skipped (no internet connection)"));
    }

    // Initialize Stepper Motor
    stepperManager.begin();

    // Setup web server
    webServerManager.begin(&spiffsManager, &ntpManager, &stepperManager, &spiffsMutex);

    Serial.println(F("\n=== System Ready ==="));
    Serial.print(F("Web interface: http://"));
    if (config.apMode || !wifiConnectedToInternet) {
        Serial.print(WiFi.softAPIP());
    } else {
        Serial.print(WiFi.localIP());
    }
    Serial.println(F("/"));
    Serial.println(F("====================\n"));
}

void loop() {
    // Update NTP
    ntpManager.update();

    // Update WebServer
    webServerManager.loop();

    // Small delay to prevent watchdog issues
    delay(10);
}

void setupWiFi() {
    Serial.println("[WiFi] Setting up WiFi...");

    if (wifiAPMode) {
        // Access Point mode
        Serial.println("[WiFi] Starting in AP mode");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(wifiSSID.c_str(), wifiPassword.c_str());
        Serial.printf("[WiFi] AP SSID: %s\n", wifiSSID.c_str());
        Serial.printf("[WiFi] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
        wifiConnectedToInternet = false;
    } else {
        // Station mode
        Serial.printf("[WiFi] Connecting to %s", wifiSSID.c_str());
        WiFi.mode(WIFI_STA);
        WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 30) {
            delay(500);
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println();
            Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());
            wifiConnectedToInternet = true;

            // Initialize NTP time synchronization
            ntpManager.begin();

        } else {
            Serial.println();
            Serial.println("[WiFi] Connection failed, switching to AP mode");

            WiFi.disconnect(true);
            delay(100);

            WiFi.mode(WIFI_AP);
            WiFi.softAP(WIFI_SSID_DEFAULT, WIFI_PASS_DEFAULT);
            Serial.printf("[WiFi] Fallback AP IP: %s\n", WiFi.softAPIP().toString().c_str());
            wifiConnectedToInternet = false;
            wifiAPMode = true;
        }
    }
}

bool loadConfig() {
    if (!spiffsManager.isReady()) return false;

    File file = LittleFS.open("/config.json", FILE_READ);
    if (!file) {
        Serial.println(F("Config file not found"));
        return false;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println(F("Failed to parse config"));
        return false;
    }

    strlcpy(config.ssid, doc["wifi"]["ssid"] | WIFI_SSID_DEFAULT, sizeof(config.ssid));
    strlcpy(config.password, doc["wifi"]["password"] | WIFI_PASS_DEFAULT, sizeof(config.password));
    config.apMode = doc["wifi"]["ap_mode"] | WIFI_AP_MODE_DEFAULT;

    // Debug: Print password details
    Serial.println("=== WiFi Config Debug ===");
    Serial.printf("SSID: '%s' (len=%d)\n", config.ssid, strlen(config.ssid));
    Serial.printf("Password length: %d\n", strlen(config.password));
    Serial.print("Password hex: ");
    for (size_t i = 0; i < strlen(config.password); i++) {
        Serial.printf("%02X ", (uint8_t)config.password[i]);
    }
    Serial.println();
    Serial.printf("AP Mode: %s\n", config.apMode ? "true" : "false");
    Serial.println("=========================");

    // Load NTP configuration
    ntpManager.loadConfig(doc);

    // Load Stepper configuration
    stepperManager.loadConfig(doc);

    // Update global WiFi variables
    wifiSSID = config.ssid;
    wifiPassword = config.password;
    wifiAPMode = config.apMode;

    Serial.println(F("Configuration loaded from SPIFFS"));
    return true;
}

void setDefaultConfig() {
    strcpy(config.ssid, WIFI_SSID_DEFAULT);
    strcpy(config.password, WIFI_PASS_DEFAULT);
    config.apMode = WIFI_AP_MODE_DEFAULT;
}

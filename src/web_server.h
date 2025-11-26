#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "spiffs_manager.h"
#include "ntp_manager.h"
#include "auth_manager.h"
#include "stepper_manager.h"

class WebServerManager {
public:
    WebServerManager();
    void begin(SPIFFSManager* spiffs, NTPManager* ntp, StepperManager* stepper, SemaphoreHandle_t* spiffsMutex);
    void loop();
    void broadcastLog(const String& message);

private:
    AsyncWebServer server;
    SPIFFSManager* spiffsManager;
    NTPManager* ntpManager;
    StepperManager* stepperManager;
    SemaphoreHandle_t* spiffsMutex;

    bool otaUploadInProgress;
    String otaUploadError;

    // Web Authentication
    String webUsername;
    String webPasswordHash;
    bool firstLogin;

    // Helper functions
    void setupRoutes();
    void serveStaticFile(AsyncWebServerRequest *request, const char* filepath, const char* contentType);
    bool checkAuth(AsyncWebServerRequest *request);
    bool loadWebCredentials();
    bool saveWebCredentials(const String& username, const String& passwordHash, bool firstLogin);
    bool isValidPath(const String& path);

    // Route handlers
    void handleRoot(AsyncWebServerRequest *request);
    void handleStatus(AsyncWebServerRequest *request);
    void handleFileList(AsyncWebServerRequest *request);
    void handleFileDownload(AsyncWebServerRequest *request);
    void handleFileView(AsyncWebServerRequest *request);
    void handleFileRead(AsyncWebServerRequest *request);
    void handleFileWrite(AsyncWebServerRequest *request);
    void handleFileDelete(AsyncWebServerRequest *request);
    void handleFileCreateDir(AsyncWebServerRequest *request);
    void handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

    void handleWiFiScan(AsyncWebServerRequest *request);
    void handleWiFiConnect(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleWiFiStatus(AsyncWebServerRequest *request);

    void handleNTPConfigGet(AsyncWebServerRequest *request);
    void handleNTPConfigPost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleNTPTime(AsyncWebServerRequest *request);

    void handleAuthStatus(AsyncWebServerRequest *request);
    void handleChangePassword(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);

    void handleStepperConfigGet(AsyncWebServerRequest *request);
    void handleStepperConfigPost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleStepperStatus(AsyncWebServerRequest *request);
    void handleStepperMove(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleStepperStep(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleStepperStop(AsyncWebServerRequest *request);
    void handleStepperReset(AsyncWebServerRequest *request);

    void handleOTA(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    bool isValidESP32Firmware(uint8_t *data, size_t len);
};

#endif // WEB_SERVER_H

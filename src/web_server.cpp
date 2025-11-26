#include "web_server.h"
#include <WiFi.h>
#include <Update.h>

WebServerManager::WebServerManager() : server(80), otaUploadInProgress(false) {
}

void WebServerManager::begin(SPIFFSManager* spiffs, NTPManager* ntp, StepperManager* stepper, SemaphoreHandle_t* mutex) {
    this->spiffsManager = spiffs;
    this->ntpManager = ntp;
    this->stepperManager = stepper;
    this->spiffsMutex = mutex;

    // Carregar credenciais web do config.json
    if (!loadWebCredentials()) {
        Serial.println("AVISO: Falha ao carregar credenciais web. Usando valores padrao.");
        webUsername = "admin";
        webPasswordHash = "8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918"; // SHA256("admin")
        firstLogin = true;
    }

    setupRoutes();
    server.begin();
    Serial.println("Web server started");
}

bool WebServerManager::checkAuth(AsyncWebServerRequest *request) {
    if (!request->hasHeader("Authorization")) {
        request->requestAuthentication(NULL, false);
        return false;
    }

    String authHeader = request->header("Authorization");
    if (!authHeader.startsWith("Basic ")) {
        request->requestAuthentication(NULL, false);
        return false;
    }

    String base64Credentials = authHeader.substring(6);
    base64Credentials.trim();

    const size_t MAX_CREDENTIALS_SIZE = 256;
    int inputLen = base64Credentials.length();

    if (inputLen > MAX_CREDENTIALS_SIZE * 4 / 3) {
        request->requestAuthentication(NULL, false);
        return false;
    }

    char decoded[MAX_CREDENTIALS_SIZE];
    int decodedLen = (inputLen * 3) / 4;

    if (decodedLen >= (int)MAX_CREDENTIALS_SIZE) {
        request->requestAuthentication(NULL, false);
        return false;
    }

    const char* base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int val = 0, valb = -8;
    int outIdx = 0;

    for (int i = 0; i < inputLen; i++) {
        const char* p = strchr(base64Chars, base64Credentials[i]);
        if (!p) continue;
        val = (val << 6) + (p - base64Chars);
        valb += 6;
        if (valb >= 0) {
            decoded[outIdx++] = char((val >> valb) & 0xFF);
            valb -= 8;
        }
    }
    decoded[outIdx] = '\0';

    char* separator = strchr(decoded, ':');
    if (separator == nullptr) {
        request->requestAuthentication(NULL, false);
        return false;
    }

    *separator = '\0';
    const char* username = decoded;
    const char* password = separator + 1;

    if (strcmp(username, webUsername.c_str()) != 0) {
        request->requestAuthentication(NULL, false);
        return false;
    }

    if (!AuthManager::verifyPassword(password, webPasswordHash.c_str())) {
        request->requestAuthentication(NULL, false);
        return false;
    }

    return true;
}

bool WebServerManager::loadWebCredentials() {
    if (!spiffsManager->isReady()) {
        return false;
    }

    File file = LittleFS.open("/config.json", FILE_READ);
    if (!file) {
        return false;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        return false;
    }

    if (doc.containsKey("web")) {
        webUsername = doc["web"]["username"].as<String>();
        webPasswordHash = doc["web"]["password_hash"].as<String>();
        firstLogin = doc["web"]["first_login"] | true;
        return true;
    }

    return false;
}

bool WebServerManager::saveWebCredentials(const String& username, const String& passwordHash, bool firstLoginFlag) {
    if (!spiffsManager->isReady()) {
        return false;
    }

    File file = LittleFS.open("/config.json", FILE_READ);
    if (!file) {
        return false;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        return false;
    }

    doc["web"]["username"] = username;
    doc["web"]["password_hash"] = passwordHash;
    doc["web"]["first_login"] = firstLoginFlag;

    file = LittleFS.open("/config.json", FILE_WRITE);
    if (!file) {
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        file.close();
        return false;
    }

    file.close();

    webUsername = username;
    webPasswordHash = passwordHash;
    firstLogin = firstLoginFlag;

    return true;
}

bool WebServerManager::isValidPath(const String& path) {
    if (path.length() == 0) return false;
    if (path.indexOf("..") >= 0) return false;
    if (!path.startsWith("/")) return false;
    if (path.indexOf('\\') >= 0) return false;
    if (path.length() > 128) return false;

    for (size_t i = 0; i < path.length(); i++) {
        char c = path.charAt(i);
        if (!isalnum(c) && c != '/' && c != '-' && c != '_' && c != '.' && c != ' ') {
            return false;
        }
    }

    return true;
}

void WebServerManager::serveStaticFile(AsyncWebServerRequest *request, const char* filepath, const char* contentType) {
    if (!checkAuth(request)) return;

    if (!spiffsManager->isReady()) {
        request->send(503, "text/plain", "SPIFFS not available");
        return;
    }

    if (!LittleFS.exists(filepath)) {
        request->send(404, "text/plain", "File not found");
        return;
    }

    AsyncWebServerResponse *response = request->beginResponse(LittleFS, filepath, contentType);
    if (response) {
        response->addHeader("Cache-Control", "public, max-age=3600");
        request->send(response);
    } else {
        request->send(500, "text/plain", "Failed to serve file");
    }
}

void WebServerManager::setupRoutes() {
    // Root
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        handleRoot(request);
    });

    // Static files
    server.on("/unified.css", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveStaticFile(request, "/web/unified.css", "text/css");
    });
    server.on("/app.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveStaticFile(request, "/web/app.js", "application/javascript");
    });
    server.on("/header.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveStaticFile(request, "/web/header.html", "text/html");
    });
    server.on("/header.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveStaticFile(request, "/web/header.js", "application/javascript");
    });
    server.on("/footer.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveStaticFile(request, "/web/footer.js", "application/javascript");
    });

    // Pages
    server.on("/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        if (spiffsManager->isReady()) request->send(LittleFS, "/web/status.html", "text/html");
        else request->send(503, "text/plain", "SPIFFS not ready");
    });
    server.on("/status.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveStaticFile(request, "/web/status.js", "application/javascript");
    });

    server.on("/filemanager", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        if (spiffsManager->isReady()) request->send(LittleFS, "/web/filemanager.html", "text/html");
        else request->send(503, "text/plain", "SPIFFS not ready");
    });
    server.on("/filemanager.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveStaticFile(request, "/web/filemanager.js", "application/javascript");
    });

    server.on("/firmware", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        if (spiffsManager->isReady()) request->send(LittleFS, "/web/firmware.html", "text/html");
        else request->send(503, "text/plain", "SPIFFS not ready");
    });
    server.on("/firmware.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveStaticFile(request, "/web/firmware.js", "application/javascript");
    });

    server.on("/wifi", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        if (spiffsManager->isReady()) request->send(LittleFS, "/web/wifi.html", "text/html");
        else request->send(503, "text/plain", "SPIFFS not ready");
    });
    server.on("/wifi.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveStaticFile(request, "/web/wifi.js", "application/javascript");
    });

    server.on("/ntp", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        if (spiffsManager->isReady()) request->send(LittleFS, "/web/ntp.html", "text/html");
        else request->send(503, "text/plain", "SPIFFS not ready");
    });
    server.on("/ntp.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveStaticFile(request, "/web/ntp.js", "application/javascript");
    });

    server.on("/auth", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        if (spiffsManager->isReady()) request->send(LittleFS, "/web/auth.html", "text/html");
        else request->send(503, "text/plain", "SPIFFS not ready");
    });
    server.on("/auth.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveStaticFile(request, "/web/auth.js", "application/javascript");
    });

    server.on("/stepper", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        if (spiffsManager->isReady()) request->send(LittleFS, "/web/stepper.html", "text/html");
        else request->send(503, "text/plain", "SPIFFS not ready");
    });
    server.on("/stepper.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        serveStaticFile(request, "/web/stepper.js", "application/javascript");
    });

    // API Endpoints
    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) { handleStatus(request); });

    server.on("/api/files/list", HTTP_GET, [this](AsyncWebServerRequest *request) { handleFileList(request); });
    server.on("/api/files/download", HTTP_GET, [this](AsyncWebServerRequest *request) { handleFileDownload(request); });
    server.on("/api/files/view", HTTP_GET, [this](AsyncWebServerRequest *request) { handleFileView(request); });
    server.on("/api/files/read", HTTP_GET, [this](AsyncWebServerRequest *request) { handleFileRead(request); });
    server.on("/api/files/write", HTTP_POST, [this](AsyncWebServerRequest *request) { handleFileWrite(request); });
    server.on("/api/files/delete", HTTP_POST, [this](AsyncWebServerRequest *request) { handleFileDelete(request); });
    server.on("/api/files/mkdir", HTTP_POST, [this](AsyncWebServerRequest *request) { handleFileCreateDir(request); });

    server.on("/api/files/upload", HTTP_POST,
        [this](AsyncWebServerRequest *request) { request->send(200, "application/json", "{\"status\":\"ok\"}"); },
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            handleFileUpload(request, filename, index, data, len, final);
        }
    );

    server.on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest *request) { handleWiFiScan(request); });
    server.on("/api/wifi/status", HTTP_GET, [this](AsyncWebServerRequest *request) { handleWiFiStatus(request); });
    server.on("/api/wifi/connect", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleWiFiConnect(request, data, len, index, total);
        }
    );

    server.on("/api/ntp/config", HTTP_GET, [this](AsyncWebServerRequest *request) { handleNTPConfigGet(request); });
    server.on("/api/ntp/config", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleNTPConfigPost(request, data, len, index, total);
        }
    );
    server.on("/api/ntp/time", HTTP_GET, [this](AsyncWebServerRequest *request) { handleNTPTime(request); });

    server.on("/api/auth/status", HTTP_GET, [this](AsyncWebServerRequest *request) { handleAuthStatus(request); });
    server.on("/api/auth/change-password", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleChangePassword(request, data, len, index, total);
        }
    );

    server.on("/api/stepper/config", HTTP_GET, [this](AsyncWebServerRequest *request) { handleStepperConfigGet(request); });
    server.on("/api/stepper/config", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleStepperConfigPost(request, data, len, index, total);
        }
    );
    server.on("/api/stepper/status", HTTP_GET, [this](AsyncWebServerRequest *request) { handleStepperStatus(request); });
    server.on("/api/stepper/move", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleStepperMove(request, data, len, index, total);
        }
    );
    server.on("/api/stepper/step", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handleStepperStep(request, data, len, index, total);
        }
    );
    server.on("/api/stepper/stop", HTTP_POST, [this](AsyncWebServerRequest *request) { handleStepperStop(request); });
    server.on("/api/stepper/reset", HTTP_POST, [this](AsyncWebServerRequest *request) { handleStepperReset(request); });

    server.on("/api/firmware/upload", HTTP_POST,
        [this](AsyncWebServerRequest *request) {
            if (otaUploadError.length() > 0) {
                request->send(500, "application/json", "{\"error\":\"" + otaUploadError + "\"}");
                otaUploadError = "";
                return;
            }
            if (Update.hasError()) {
                request->send(500, "application/json", "{\"error\":\"Update failed\"}");
                return;
            }
            request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Firmware updated successfully. Device will reboot now.\"}");
            delay(2000);
            ESP.restart();
        },
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            handleOTA(request, filename, index, data, len, final);
        }
    );

    server.onNotFound([this](AsyncWebServerRequest *request) {
        if (!checkAuth(request)) return;
        request->send(404, "text/plain", "Not found");
    });
}

void WebServerManager::handleRoot(AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;
    if (spiffsManager->isReady()) {
        request->send(LittleFS, "/web/index.html", "text/html");
    } else {
        request->send(200, "text/html", "<html><body><h1>System Initializing...</h1></body></html>");
    }
}

void WebServerManager::handleStatus(AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;
    StaticJsonDocument<1024> doc;

    // System uptime
    unsigned long uptimeMs = millis();
    unsigned long uptimeSec = uptimeMs / 1000;
    unsigned long days = uptimeSec / 86400;
    unsigned long hours = (uptimeSec % 86400) / 3600;
    unsigned long minutes = (uptimeSec % 3600) / 60;
    unsigned long seconds = uptimeSec % 60;

    doc["uptime"]["milliseconds"] = uptimeMs;
    doc["uptime"]["formatted"] = String(days) + "d " + String(hours) + "h " +
                                  String(minutes) + "m " + String(seconds) + "s";

    // Memory
    uint32_t heapTotal = ESP.getHeapSize();
    uint32_t heapFree = ESP.getFreeHeap();
    uint32_t heapUsed = heapTotal - heapFree;
    doc["memory"]["heap"]["total"] = heapTotal;
    doc["memory"]["heap"]["free"] = heapFree;
    doc["memory"]["heap"]["used"] = heapUsed;
    doc["memory"]["heap"]["usage_percent"] = ((float)heapUsed / heapTotal) * 100;

    // WiFi
    doc["wifi"]["connected"] = WiFi.status() == WL_CONNECTED;
    doc["wifi"]["ssid"] = WiFi.SSID();
    doc["wifi"]["rssi"] = WiFi.RSSI();
    doc["wifi"]["ip"] = WiFi.localIP().toString();
    doc["wifi"]["mac"] = WiFi.macAddress();

    int rssi = WiFi.RSSI();
    String signalStrength;
    if (rssi >= -50) signalStrength = "Excelente";
    else if (rssi >= -60) signalStrength = "Bom";
    else if (rssi >= -70) signalStrength = "Razoavel";
    else signalStrength = "Fraco";
    doc["wifi"]["signal_strength"] = signalStrength;

    // SPIFFS
    doc["spiffs"]["ready"] = spiffsManager->isReady();
    if (spiffsManager->isReady()) {
        size_t totalBytes = LittleFS.totalBytes();
        size_t usedBytes = LittleFS.usedBytes();
        doc["spiffs"]["total_bytes"] = totalBytes;
        doc["spiffs"]["used_bytes"] = usedBytes;
        doc["spiffs"]["free_bytes"] = totalBytes - usedBytes;
        doc["spiffs"]["usage_percent"] = totalBytes > 0 ? ((float)usedBytes / totalBytes) * 100 : 0;
    }

    // CPU
    doc["cpu"]["frequency_mhz"] = ESP.getCpuFreqMHz();
    doc["cpu"]["chip_model"] = ESP.getChipModel();

    // NTP
    doc["ntp"]["enabled"] = ntpManager->getConfig().enabled;
    doc["ntp"]["time"] = ntpManager->getFormattedTime();
    doc["ntp"]["synced"] = ntpManager->isTimeSet();

    bool isHealthy = WiFi.status() == WL_CONNECTED && ESP.getFreeHeap() > 50000;
    doc["status"] = isHealthy ? "healthy" : "degraded";
    doc["timestamp"] = uptimeMs;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleFileList(AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;
    if (!spiffsManager->isReady()) {
        request->send(503, "application/json", "{\"error\":\"SPIFFS not ready\"}");
        return;
    }

    String path = "/";
    if (request->hasParam("dir")) {
        path = request->getParam("dir")->value();
        if (!isValidPath(path)) {
            request->send(400, "application/json", "{\"error\":\"Invalid directory path\"}");
            return;
        }
    }

    File root = LittleFS.open(path);
    if (!root || !root.isDirectory()) {
        request->send(404, "application/json", "{\"error\":\"Directory not found\"}");
        return;
    }

    StaticJsonDocument<1024> doc;
    JsonArray files = doc["files"].to<JsonArray>();

    File file = root.openNextFile();
    while (file) {
        JsonObject fileObj = files.add<JsonObject>();
        fileObj["name"] = String(file.name());
        fileObj["size"] = file.size();
        fileObj["isDir"] = file.isDirectory();
        file = root.openNextFile();
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleFileDownload(AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;
    if (!spiffsManager->isReady()) { request->send(503, "text/plain", "SPIFFS not ready"); return; }
    if (!request->hasParam("file")) { request->send(400, "text/plain", "Missing file parameter"); return; }

    String filepath = request->getParam("file")->value();
    if (!isValidPath(filepath)) {
        request->send(400, "text/plain", "Invalid file path");
        return;
    }

    if (!LittleFS.exists(filepath)) { request->send(404, "text/plain", "File not found"); return; }

    request->send(LittleFS, filepath, String(), true);
}

void WebServerManager::handleFileView(AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;
    if (!request->hasParam("file")) { request->send(400, "text/plain", "Missing file parameter"); return; }

    String filepath = request->getParam("file")->value();
    if (!isValidPath(filepath)) {
        request->send(400, "text/plain", "Invalid file path");
        return;
    }

    if (!LittleFS.exists(filepath)) { request->send(404, "text/plain", "File not found"); return; }

    request->send(LittleFS, filepath, "text/plain", false);
}

void WebServerManager::handleFileRead(AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;
    if (!request->hasParam("file")) { request->send(400, "application/json", "{\"error\":\"Missing file parameter\"}"); return; }

    String filepath = request->getParam("file")->value();
    if (!isValidPath(filepath)) {
        request->send(400, "application/json", "{\"error\":\"Invalid file path\"}");
        return;
    }

    if (!LittleFS.exists(filepath)) { request->send(404, "application/json", "{\"error\":\"File not found\"}"); return; }

    File file = LittleFS.open(filepath, FILE_READ);
    if (!file) {
        request->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
        return;
    }

    size_t fileSize = file.size();
    if (fileSize > 51200) {
        file.close();
        request->send(413, "application/json", "{\"error\":\"File too large (max 50KB)\"}");
        return;
    }

    String content = file.readString();
    file.close();

    StaticJsonDocument<256> doc;
    doc["status"] = "ok";
    doc["content"] = content;
    doc["size"] = fileSize;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleFileWrite(AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;
    if (!request->hasParam("file", true) || !request->hasParam("content", true)) {
        request->send(400, "application/json", "{\"error\":\"Missing parameters\"}");
        return;
    }

    String filepath = request->getParam("file", true)->value();
    String content = request->getParam("content", true)->value();

    if (!isValidPath(filepath)) {
        request->send(400, "application/json", "{\"error\":\"Invalid file path\"}");
        return;
    }

    File file = LittleFS.open(filepath, FILE_WRITE);
    if (!file) {
        request->send(500, "application/json", "{\"error\":\"Failed to open file\"}");
        return;
    }

    size_t written = file.print(content);
    file.close();

    if (written > 0) {
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        request->send(500, "application/json", "{\"error\":\"Failed to write\"}");
    }
}

void WebServerManager::handleFileDelete(AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;
    if (!request->hasParam("file", true)) { request->send(400, "application/json", "{\"error\":\"Missing file parameter\"}"); return; }

    String filepath = request->getParam("file", true)->value();
    if (!isValidPath(filepath)) {
        request->send(400, "application/json", "{\"error\":\"Invalid file path\"}");
        return;
    }

    File file = LittleFS.open(filepath);
    if (!file) { request->send(404, "application/json", "{\"error\":\"File not found\"}"); return; }

    bool isDir = file.isDirectory();
    file.close();

    bool success = isDir ? LittleFS.rmdir(filepath) : LittleFS.remove(filepath);
    if (success) request->send(200, "application/json", "{\"status\":\"ok\"}");
    else request->send(500, "application/json", "{\"error\":\"Failed to delete\"}");
}

void WebServerManager::handleFileCreateDir(AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;
    if (!request->hasParam("dir", true)) { request->send(400, "application/json", "{\"error\":\"Missing dir parameter\"}"); return; }

    String dirpath = request->getParam("dir", true)->value();
    if (!isValidPath(dirpath)) {
        request->send(400, "application/json", "{\"error\":\"Invalid directory path\"}");
        return;
    }

    if (LittleFS.mkdir(dirpath)) request->send(200, "application/json", "{\"status\":\"ok\"}");
    else request->send(500, "application/json", "{\"error\":\"Failed to create directory\"}");
}

void WebServerManager::handleFileUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!checkAuth(request)) return;
    static File uploadFile;

    if (index == 0) {
        String path = "/";
        if (request->hasParam("dir", false)) {
            path = request->getParam("dir", false)->value();
            if (!isValidPath(path)) {
                return;
            }
            if (path != "/" && !path.endsWith("/")) path += "/";
        }

        String filepath = path + filename;
        if (!isValidPath(filepath)) {
            return;
        }

        if (LittleFS.exists(filepath)) LittleFS.remove(filepath);
        uploadFile = LittleFS.open(filepath, FILE_WRITE);
    }

    if (uploadFile && len) uploadFile.write(data, len);
    if (final && uploadFile) uploadFile.close();
}

void WebServerManager::handleWiFiScan(AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;

    // Check if scan is already in progress or has results
    int scanResult = WiFi.scanComplete();

    Serial.printf("[WiFi Scan] scanComplete returned: %d\n", scanResult);

    if (scanResult == WIFI_SCAN_FAILED || scanResult == -2) {
        // Start a new async scan
        // Delete any old results first
        WiFi.scanDelete();
        delay(10);

        Serial.println("[WiFi Scan] Starting new async scan...");
        int startResult = WiFi.scanNetworks(true, true);  // async=true, show_hidden=true
        Serial.printf("[WiFi Scan] scanNetworks returned: %d\n", startResult);

        request->send(202, "application/json", "{\"status\":\"scanning\",\"message\":\"Scan started, please retry in 3 seconds\"}");
        return;
    }

    if (scanResult == WIFI_SCAN_RUNNING || scanResult == -1) {
        Serial.println("[WiFi Scan] Scan in progress...");
        request->send(202, "application/json", "{\"status\":\"scanning\",\"message\":\"Scan in progress, please retry\"}");
        return;
    }

    // Scan complete, return results
    Serial.printf("[WiFi Scan] Found %d networks\n", scanResult);

    StaticJsonDocument<4096> doc;
    JsonArray networks = doc["networks"].to<JsonArray>();

    for (int i = 0; i < scanResult; i++) {
        String ssid = WiFi.SSID(i);
        Serial.printf("[WiFi Scan] Network %d: '%s' (%d dBm)\n", i, ssid.c_str(), WiFi.RSSI(i));

        if (ssid.length() > 0) { // Skip hidden networks
            JsonObject network = networks.add<JsonObject>();
            network["ssid"] = ssid;
            network["rssi"] = WiFi.RSSI(i);
            network["encryption"] = (int)WiFi.encryptionType(i);
            network["channel"] = WiFi.channel(i);
        }
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);

    // Clear results to allow new scan next time
    WiFi.scanDelete();
}

void WebServerManager::handleWiFiStatus(AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;
    StaticJsonDocument<512> doc;

    doc["connected"] = WiFi.status() == WL_CONNECTED;
    doc["ssid"] = WiFi.SSID();
    doc["rssi"] = WiFi.RSSI();
    doc["ip"] = WiFi.localIP().toString();
    doc["mac"] = WiFi.macAddress();
    doc["gateway"] = WiFi.gatewayIP().toString();
    doc["subnet"] = WiFi.subnetMask().toString();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleWiFiConnect(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (!checkAuth(request)) return;
    if (index == 0) {
        StaticJsonDocument<256> doc;
        if (deserializeJson(doc, data, len)) { request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}"); return; }

        const char* ssid = doc["ssid"];
        const char* password = doc["password"];

        File configFile = LittleFS.open("/config.json", "r");
        StaticJsonDocument<1024> configDoc;
        if (configFile) { deserializeJson(configDoc, configFile); configFile.close(); }

        configDoc["wifi"]["ssid"] = ssid;
        configDoc["wifi"]["password"] = password ? password : "";
        configDoc["wifi"]["ap_mode"] = false;

        configFile = LittleFS.open("/config.json", "w");
        if (!configFile) {
            request->send(500, "application/json", "{\"error\":\"Failed to save configuration\"}");
            return;
        }

        serializeJson(configDoc, configFile);
        configFile.close();
        request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Configuration saved. Rebooting...\"}");
        delay(2000);
        ESP.restart();
    }
}

void WebServerManager::handleNTPConfigGet(AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;

    const NTPManager::NTPConfig& config = ntpManager->getConfig();

    StaticJsonDocument<512> doc;
    doc["server"] = config.server;
    doc["offset"] = config.offset;
    doc["interval"] = config.interval;
    doc["enabled"] = config.enabled;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleNTPConfigPost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (!checkAuth(request)) return;
    if (index == 0) {
        StaticJsonDocument<256> doc;
        if (deserializeJson(doc, data, len)) {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        const char* server = doc["server"] | "pool.ntp.org";
        long offset = doc["offset"] | -10800;
        int interval = doc["interval"] | 60000;
        bool enabled = doc["enabled"] | true;

        ntpManager->updateConfig(server, offset, interval, enabled);

        // Save to config.json
        File configFile = LittleFS.open("/config.json", "r");
        StaticJsonDocument<1024> configDoc;
        if (configFile) {
            deserializeJson(configDoc, configFile);
            configFile.close();
        }

        ntpManager->saveConfig(configDoc);

        configFile = LittleFS.open("/config.json", "w");
        if (!configFile) {
            request->send(500, "application/json", "{\"error\":\"Failed to save configuration\"}");
            return;
        }

        serializeJson(configDoc, configFile);
        configFile.close();
        request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Configuration saved\"}");
    }
}

void WebServerManager::handleNTPTime(AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;

    StaticJsonDocument<256> doc;
    doc["time"] = ntpManager->getFormattedTime();
    doc["enabled"] = ntpManager->getConfig().enabled;
    doc["synced"] = ntpManager->isTimeSet();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleAuthStatus(AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;

    StaticJsonDocument<512> doc;
    doc["username"] = webUsername;
    doc["first_login"] = firstLogin;
    doc["password_change_required"] = firstLogin;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleChangePassword(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (!checkAuth(request)) return;

    if (index == 0) {
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, data, len);

        if (error) {
            request->send(400, "application/json", "{\"error\":\"JSON invalido\"}");
            return;
        }

        const char* currentPassword = doc["current_password"] | "";
        const char* newPassword = doc["new_password"] | "";
        String newUsername = doc["username"] | webUsername;

        if (!AuthManager::verifyPassword(currentPassword, webPasswordHash.c_str())) {
            request->send(401, "application/json", "{\"error\":\"Senha atual incorreta\"}");
            return;
        }

        char validationError[128];
        if (!AuthManager::getPasswordValidationError(newPassword, validationError, sizeof(validationError))) {
            StaticJsonDocument<256> errorDoc;
            errorDoc["error"] = validationError;
            String errorResponse;
            serializeJson(errorDoc, errorResponse);
            request->send(400, "application/json", errorResponse);
            return;
        }

        char newPasswordHash[65];
        if (!AuthManager::hashPassword(newPassword, newPasswordHash, sizeof(newPasswordHash))) {
            request->send(500, "application/json", "{\"error\":\"Falha ao gerar hash de senha\"}");
            return;
        }

        String newPasswordHashStr(newPasswordHash);
        if (saveWebCredentials(newUsername, newPasswordHashStr, false)) {
            StaticJsonDocument<256> successDoc;
            successDoc["status"] = "ok";
            successDoc["message"] = "Senha alterada com sucesso";
            successDoc["username"] = webUsername;
            successDoc["first_login"] = false;
            String successResponse;
            serializeJson(successDoc, successResponse);
            request->send(200, "application/json", successResponse);
        } else {
            request->send(500, "application/json", "{\"error\":\"Falha ao salvar credenciais\"}");
        }
    }
}

void WebServerManager::handleOTA(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!checkAuth(request)) return;

    if (index == 0) {
        otaUploadError = "";
        otaUploadInProgress = true;

        if (!isValidESP32Firmware(data, len)) {
            otaUploadError = "Invalid ESP32 firmware file";
            otaUploadInProgress = false;
            return;
        }

        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
            otaUploadError = "Failed to begin OTA update: " + String(Update.errorString());
            otaUploadInProgress = false;
            return;
        }
    }

    if (len) {
        yield();
        if (Update.write(data, len) != len) {
            otaUploadError = "Failed to write firmware data";
            Update.abort();
        }
        yield();
    }

    if (final) {
        if (!Update.end(true)) {
            otaUploadError = "Failed to finalize OTA update: " + String(Update.errorString());
        }
        otaUploadInProgress = false;
    }
}

bool WebServerManager::isValidESP32Firmware(uint8_t *data, size_t len) {
    if (len < 1) return false;
    return data[0] == 0xE9;
}

void WebServerManager::loop() {
    // Nothing to do here for now
}

void WebServerManager::broadcastLog(const String& message) {
    // Future: broadcast to WebSocket clients
    // Serial output is handled by the caller (log function in main.cpp)
}

void WebServerManager::handleStepperConfigGet(AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;

    const StepperManager::StepperConfig& config = stepperManager->getConfig();

    StaticJsonDocument<512> doc;
    doc["pin1"] = config.pin1;
    doc["pin2"] = config.pin2;
    doc["pin3"] = config.pin3;
    doc["pin4"] = config.pin4;
    doc["speed"] = config.speed;
    doc["steps_per_rev"] = config.stepsPerRev;
    doc["enabled"] = config.enabled;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleStepperConfigPost(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (!checkAuth(request)) return;
    if (index == 0) {
        StaticJsonDocument<256> doc;
        if (deserializeJson(doc, data, len)) {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        int pin1 = doc["pin1"] | 25;
        int pin2 = doc["pin2"] | 26;
        int pin3 = doc["pin3"] | 27;
        int pin4 = doc["pin4"] | 14;
        int speed = doc["speed"] | 10;
        int stepsPerRev = doc["steps_per_rev"] | 2048;
        bool enabled = doc["enabled"] | true;

        stepperManager->updateConfig(pin1, pin2, pin3, pin4, speed, stepsPerRev, enabled);

        // Save to config.json
        File configFile = LittleFS.open("/config.json", "r");
        StaticJsonDocument<1024> configDoc;
        if (configFile) {
            deserializeJson(configDoc, configFile);
            configFile.close();
        }

        stepperManager->saveConfig(configDoc);

        configFile = LittleFS.open("/config.json", "w");
        if (!configFile) {
            request->send(500, "application/json", "{\"error\":\"Failed to save configuration\"}");
            return;
        }

        serializeJson(configDoc, configFile);
        configFile.close();
        request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"Configuration saved\"}");
    }
}

void WebServerManager::handleStepperStatus(AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;

    StaticJsonDocument<256> doc;
    doc["enabled"] = stepperManager->isEnabled();
    doc["moving"] = stepperManager->isMoving();
    doc["position"] = stepperManager->getCurrentPosition();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void WebServerManager::handleStepperMove(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (!checkAuth(request)) return;
    if (index == 0) {
        StaticJsonDocument<128> doc;
        if (deserializeJson(doc, data, len)) {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        float degrees = doc["degrees"] | 0.0f;

        if (!stepperManager->isEnabled()) {
            request->send(400, "application/json", "{\"error\":\"Stepper motor is disabled\"}");
            return;
        }

        stepperManager->rotate(degrees);
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    }
}

void WebServerManager::handleStepperStep(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (!checkAuth(request)) return;
    if (index == 0) {
        StaticJsonDocument<128> doc;
        if (deserializeJson(doc, data, len)) {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        int steps = doc["steps"] | 0;

        if (!stepperManager->isEnabled()) {
            request->send(400, "application/json", "{\"error\":\"Stepper motor is disabled\"}");
            return;
        }

        stepperManager->step(steps);
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    }
}

void WebServerManager::handleStepperStop(AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;

    stepperManager->stop();
    request->send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerManager::handleStepperReset(AsyncWebServerRequest *request) {
    if (!checkAuth(request)) return;

    stepperManager->resetPosition();
    request->send(200, "application/json", "{\"status\":\"ok\"}");
}

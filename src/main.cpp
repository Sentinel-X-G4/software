#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>

// Import our custom modules
#include "Config.h"
#include "Actuators.h"
#include "Sensors.h"
#include "Display.h"

// Instantiate our objects
Actuators myActuators;
Sensors mySensors;
Display myDisplay;

// Network variables
WiFiClientSecure secureClient;
WebSocketsClient webSocket;
bool isWifiConnected = false;

// Timing variables
unsigned long lastReadTime = 0;
unsigned long lastSendTime = 0;

// --- Network Functions ---

void connectWiFi() {
    myDisplay.showSimpleMessage("Wi-Fi", "Connecting...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        delay(500);
        attempts++;
    }
    isWifiConnected = (WiFi.status() == WL_CONNECTED);
}

void sendDataHTTPS() {
    JsonDocument doc;
    doc["device_id"] = DEVICE_ID;
    doc["temperature"] = mySensors.temperature;
    doc["humidity"] = mySensors.humidity;
    doc["gas_raw"] = mySensors.gasLevel;
    doc["gas_alarm"] = mySensors.gasAlarm;
    doc["motion"] = mySensors.motionDetected;

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    HTTPClient http;
    String url = String("https://") + SERVER_HOST + ":" + SERVER_PORT_HTTPS + ENDPOINT_ALERTS;

    if (http.begin(secureClient, url)) {
        http.addHeader("Content-Type", "application/json");
        http.POST(jsonPayload);
        http.end();
    }
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_TEXT) {
        String message = String((char *) payload);
        JsonDocument commandDoc;
        deserializeJson(commandDoc, message);

        const char *action = commandDoc["command"];

        if (strcmp(action, "buzzer_on") == 0) digitalWrite(PIN_BUZZER, HIGH);
        else if (strcmp(action, "buzzer_off") == 0) digitalWrite(PIN_BUZZER, LOW);
        else if (strcmp(action, "reset") == 0) myActuators.triggerAlert(false);
    }
}

// --- Main Program ---

void setup() {
    Serial.begin(115200);

    myActuators.initialize();
    myDisplay.initialize();
    mySensors.initialize();

    myDisplay.showSimpleMessage("SENTINEL-X", "Booting...");

    connectWiFi();
    secureClient.setFingerprint(TLS_FINGERPRINT);

    if (isWifiConnected) {
        webSocket.begin(SERVER_HOST, WEBSOCKET_PORT, "/ws");
        webSocket.onEvent(webSocketEvent);
        webSocket.setReconnectInterval(5000);
    }
}

void loop() {
    unsigned long currentTime = millis();

    // 1. Read sensors & update display (Every 2 seconds)
    if (currentTime - lastReadTime >= READ_INTERVAL_MS) {
        lastReadTime = currentTime;

        mySensors.readAll();

        bool isDanger = mySensors.checkCriticalThresholds();
        myActuators.triggerAlert(isDanger);

        myDisplay.showDashboard(mySensors, myActuators.isAlertActive, isWifiConnected);
    }

    // 2. Send data to server (Every 5 seconds)
    if (isWifiConnected && (currentTime - lastSendTime >= SEND_INTERVAL_MS)) {
        lastSendTime = currentTime;
        sendDataHTTPS();
    }

    // 3. Process incoming WebSocket commands
    if (isWifiConnected) {
        webSocket.loop();
    }
}

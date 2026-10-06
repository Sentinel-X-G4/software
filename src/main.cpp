#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>

// Import our custom modules
#include "Config.h"
#include "Actuators.h"
#include "Sensors.h"
#include "Display.h"
#include "MqttClient.h"

// Instantiate our objects
Actuators myActuators;
Sensors mySensors;
Display myDisplay;
MqttClient myMqtt;

// Network variables
WebSocketsClient webSocket;
bool isWifiConnected = false;

// Timing variables
unsigned long lastReadTime = 0;
unsigned long lastTelemetryTime = 0;
bool hasFreshClimate = false;
bool isAlertSent = false;

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
    if (isWifiConnected) {
        myMqtt.initialize();

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
        hasFreshClimate = true;

        bool isDanger = mySensors.checkCriticalThresholds();
        myActuators.triggerAlert(isDanger);

        // Local threshold crossed: one MQTT alert per danger episode (retried until published)
        if (!isDanger) isAlertSent = false;
        else if (!isAlertSent) isAlertSent = myMqtt.publishAlert("local_threshold");

        myDisplay.showDashboard(mySensors, myActuators.isAlertActive, isWifiConnected);
    }

    // 2. Publish telemetry over MQTT (Every 200 ms; temp/hum only right after a DHT22 read)
    if (isWifiConnected && (currentTime - lastTelemetryTime >= TELEMETRY_INTERVAL_MS)) {
        lastTelemetryTime = currentTime;
        mySensors.readFast();
        if (myMqtt.publishTelemetry(mySensors, hasFreshClimate, currentTime < MQ2_WARMUP_MS)) {
            hasFreshClimate = false;
        }
    }

    // 3. Keep MQTT alive & process incoming WebSocket commands
    if (isWifiConnected) {
        myMqtt.loop();
        webSocket.loop();
    }
}

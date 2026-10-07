#include <Arduino.h>
#include <ESP8266WiFi.h>

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
bool isWifiConnected = false;
unsigned long lastWifiAttemptTime = 0;
int wifiAttempts = 0;

// Timing variables
unsigned long lastReadTime = 0;
unsigned long lastTelemetryTime = 0;
bool hasFreshClimate = false;
bool isAlertSent = false;

// --- Network Functions ---

void startWiFiAttempt() {
    wifiAttempts++;
    lastWifiAttemptTime = millis();
    Serial.printf("[WIFI] attempt %d to \"%s\"\n", wifiAttempts, WIFI_SSID);
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void logWiFiFailure() {
    // 1 = SSID not found (5 GHz only, hidden, out of range), 4/6 = wrong password or WPA3 only, 7 = still trying
    Serial.printf("[WIFI] \"%s\" failed, status=%d. Visible 2.4 GHz networks:\n", WIFI_SSID, WiFi.status());
    const int count = WiFi.scanNetworks();
    for (int i = 0; i < count; i++) {
        Serial.printf("  \"%s\" ch=%d RSSI=%d enc=%d\n", WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i),
                      WiFi.encryptionType(i));
    }
    WiFi.scanDelete();
}

// Boot: waits for the first attempt, the next ones are made by maintainWiFi()
void connectWiFi() {
    myDisplay.showSimpleMessage("Wi-Fi", "Connecting...");
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    startWiFiAttempt();

    while (WiFi.status() != WL_CONNECTED && millis() - lastWifiAttemptTime < WIFI_RETRY_INTERVAL_MS) {
        delay(500);
    }
}

// Called on every loop(): retries every WIFI_RETRY_INTERVAL_MS until connected, without
// blocking the sensors and the local alarm
void maintainWiFi() {
    const bool connected = (WiFi.status() == WL_CONNECTED);
    if (connected && !isWifiConnected) {
        Serial.printf("[WIFI] connected to \"%s\", IP=%s RSSI=%d dBm\n", WIFI_SSID,
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        wifiAttempts = 0;
    } else if (!connected && isWifiConnected) {
        Serial.println("[WIFI] connection lost");
        lastWifiAttemptTime = millis(); // Leaves the auto-reconnect one interval before forcing a retry
    }
    isWifiConnected = connected;
    if (connected || millis() - lastWifiAttemptTime < WIFI_RETRY_INTERVAL_MS) return;

    logWiFiFailure();
    startWiFiAttempt();
}

// Commands received on sentinelx/{DEVICE_ID}/cmd: returns false if unknown
bool handleCommand(const char *command) {
    if (strcmp(command, "buzzer_on") == 0) digitalWrite(PIN_BUZZER, HIGH);
    else if (strcmp(command, "buzzer_off") == 0) digitalWrite(PIN_BUZZER, LOW);
    else if (strcmp(command, "reset") == 0) myActuators.triggerAlert(false);
    else return false;
    return true;
}

// --- Main Program ---

void setup() {
    Serial.begin(115200);

    myActuators.initialize();
    myDisplay.initialize();
    mySensors.initialize();

    myDisplay.showSimpleMessage("SENTINEL-X", "Booting...");

    connectWiFi();
    myMqtt.initialize(handleCommand);
}

void loop() {
    maintainWiFi();
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

    // 3. Keep MQTT alive (reconnects, incoming commands)
    myMqtt.loop();
}

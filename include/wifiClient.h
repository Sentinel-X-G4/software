#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Config.h"

// Wi-Fi station: retries every WIFI_RETRY_INTERVAL_MS until connected, without blocking loop()
class WifiClient {
private:
    bool connected = false;
    unsigned long lastAttemptTime = 0;
    int attempts = 0;

    void startAttempt() {
        attempts++;
        lastAttemptTime = millis();
        Serial.printf("[WIFI] attempt %d to \"%s\"\n", attempts, WIFI_SSID);
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }

    static void logFailure() {
        // 1 = SSID not found (5 GHz only, hidden, out of range), 4/6 = wrong password or WPA3 only, 7 = still trying
        Serial.printf("[WIFI] \"%s\" failed, status=%d. Visible 2.4 GHz networks:\n", WIFI_SSID, WiFi.status());
        const int count = WiFi.scanNetworks();
        for (int i = 0; i < count; i++) {
            Serial.printf("  \"%s\" ch=%d RSSI=%d enc=%d\n", WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i),
                          WiFi.encryptionType(i));
        }
        WiFi.scanDelete();
    }

public:
    // Boot: waits for the first attempt, the next ones are made by loop()
    void initialize() {
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        startAttempt();

        while (WiFi.status() != WL_CONNECTED && millis() - lastAttemptTime < WIFI_RETRY_INTERVAL_MS) {
            delay(500);
        }
    }

    // Call on every loop()
    void loop() {
        const bool isUp = (WiFi.status() == WL_CONNECTED);
        if (isUp && !connected) {
            Serial.printf("[WIFI] connected to \"%s\", IP=%s RSSI=%d dBm\n", WIFI_SSID,
                          WiFi.localIP().toString().c_str(), WiFi.RSSI());
            attempts = 0;
        } else if (!isUp && connected) {
            Serial.println("[WIFI] connection lost");
            lastAttemptTime = millis(); // Leaves the auto-reconnect one interval before forcing a retry
        }
        connected = isUp;
        if (connected || millis() - lastAttemptTime < WIFI_RETRY_INTERVAL_MS) return;

        logFailure();
        startAttempt();
    }

    bool isConnected() const {
        return connected;
    }
};

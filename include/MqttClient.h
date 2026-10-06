#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>
#include "Config.h"
#include "Sensors.h"
#include "MqttCa.h" // Generated at build time from secrets/ca.crt (scripts/embed_ca.py)

// MQTTS client for the Sentinel-X broker.
// Topics and payloads: backend-iot-alerts/detection-service/docs/MQTT_CONTRACT.md
// Rights (account sentinel_iot): infrastructure/mosquitto/config/acl
class MqttClient {
private:
    BearSSL::X509List caCert;
    WiFiClientSecure tls;
    PubSubClient mqtt;
    unsigned long lastAttemptTime = 0;
    bool buffersSized = false;

    String topicTelemetry;
    String topicAlert;

    static bool isTimeValid() {
        // TLS needs the real date to check the certificate validity period
        return time(nullptr) > 1700000000;
    }

    bool publishJson(const String &topic, JsonDocument &doc) {
        if (!mqtt.connected()) return false;
        char payload[MQTT_PAYLOAD_MAX];
        const size_t length = serializeJson(doc, payload, sizeof(payload));
        return mqtt.publish(topic.c_str(), reinterpret_cast<const uint8_t *>(payload), length);
    }

    bool connect() {
        IPAddress ip;
        const bool isIp = ip.fromString(MQTT_HOST);

        // Smaller TLS buffers (~25 KB of RAM saved) if the broker accepts it
        if (!buffersSized) {
            const bool ok = isIp
                                ? tls.probeMaxFragmentLength(ip, MQTT_PORT, 1024)
                                : tls.probeMaxFragmentLength(MQTT_HOST, MQTT_PORT, 1024);
            if (ok) tls.setBufferSizes(1024, 1024);
            buffersSized = true;
        }

        tls.setX509Time(time(nullptr));
        if (mqtt.connect(DEVICE_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
            Serial.println("[MQTT] connected");
            return true;
        }

        char tlsError[64] = "";
        tls.getLastSSLError(tlsError, sizeof(tlsError));
        // state -2: network / TLS (see tlsError), 4: bad credentials, 5: not authorized
        Serial.printf("[MQTT] connection failed, state=%d tls=\"%s\"\n", mqtt.state(), tlsError);
        return false;
    }

public:
    MqttClient() : caCert(MQTT_CA_CERT), mqtt(tls) {
    }

    // Call once, after Wi-Fi is up
    void initialize() {
        topicTelemetry = String("sentinelx/") + DEVICE_ID + "/telemetry";
        topicAlert = String("sentinelx/") + DEVICE_ID + "/alert";

        configTime(0, 0, "pool.ntp.org", "time.google.com");
        tls.setTrustAnchors(&caCert);

        IPAddress ip;
        // By IP: the chain is checked against the CA, not the host name.
        // By name: the name must be in the broker certificate (mqtt.sentinel.lan).
        if (ip.fromString(MQTT_HOST)) mqtt.setServer(ip, MQTT_PORT);
        else mqtt.setServer(MQTT_HOST, MQTT_PORT);
        mqtt.setBufferSize(MQTT_PAYLOAD_MAX + 64);
    }

    // Call on every loop(): keeps the connection alive and reconnects without blocking for long
    void loop() {
        if (WiFi.status() != WL_CONNECTED) return;

        if (!mqtt.connected()) {
            const unsigned long now = millis();
            if (lastAttemptTime != 0 && now - lastAttemptTime < MQTT_RECONNECT_INTERVAL_MS) return;
            lastAttemptTime = now;
            if (!isTimeValid()) {
                Serial.println("[MQTT] waiting for NTP time");
                return;
            }
            if (!connect()) return;
        }
        mqtt.loop();
    }

    bool isConnected() {
        return mqtt.connected();
    }

    // sentinelx/{DEVICE_ID}/telemetry
    // hasClimate: true only on the message that follows a DHT22 read, otherwise temp/hum are sent as null
    bool publishTelemetry(const Sensors &sensors, bool hasClimate, bool isWarmup) {
        JsonDocument doc;
        if (isTimeValid()) doc["ts"] = static_cast<uint64_t>(time(nullptr)) * 1000ULL;
        if (hasClimate && !isnan(sensors.temperature)) doc["temp"] = sensors.temperature;
        else doc["temp"] = nullptr;
        if (hasClimate && !isnan(sensors.humidity)) doc["hum"] = sensors.humidity;
        else doc["hum"] = nullptr;
        doc["pir"] = sensors.motionDetected ? 1 : 0;
        doc["gas_raw"] = constrain(sensors.gasLevel, 0, 1023);
        doc["gas_do"] = sensors.gasAlarm ? 0 : 1; // Raw MQ-2 DO: 0 = threshold exceeded
        doc["warmup"] = isWarmup;
        return publishJson(topicTelemetry, doc);
    }

    // sentinelx/{DEVICE_ID}/alert: each message creates an alert on the dashboard
    bool publishAlert(const char *type, bool value = true) {
        JsonDocument doc;
        doc["type"] = type;
        doc["value"] = value;
        return publishJson(topicAlert, doc);
    }
};

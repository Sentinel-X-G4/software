#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>
#include <functional>
#include "Config.h"
#include "Sensors.h"
#include "MqttCa.h" // Generated at build time from secrets/ca.crt (scripts/embed_ca.py)

// Command handler: returns nullptr if applied, otherwise the reason of the refusal; fills `state`
// with the current outputs, sent back in the ack
using CommandHandler = std::function<const char *(JsonObjectConst command, JsonObject state)>;

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
    String topicCommand;
    String topicAck;

    CommandHandler commandHandler;

    static bool isTimeValid() {
        // TLS needs the real date to check the certificate validity period
        return time(nullptr) > 1700000000;
    }

    // Firmware build date (UTC assumed), used when NTP is unreachable
    static time_t buildTime() {
        static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
        char month[4] = "";
        tm t = {};
        sscanf(__DATE__, "%3s %d %d", month, &t.tm_mday, &t.tm_year);
        sscanf(__TIME__, "%d:%d:%d", &t.tm_hour, &t.tm_min, &t.tm_sec);
        t.tm_mon = static_cast<int>((strstr(months, month) - months) / 3);
        t.tm_year -= 1900;
        return mktime(&t);
    }

    // sentinelx/{DEVICE_ID}/cmd (published by backend-api), commands in main.cpp handleCommand():
    //   {"id": "...", "command": "alert" | "buzzer" | "led" | "screen" | "reset", "state": "...", "text": "..."}
    // Answered on sentinelx/{DEVICE_ID}/ack, same id:
    //   {"id": "...", "command": "...", "ok": true | false, "error": "...", "state": {"alert", "buzzer", "led", "screen"}}
    void onMessage(const char *topic, const uint8_t *payload, unsigned int length) {
        if (topicCommand != topic) return;

        // The document copies the strings: payload (PubSubClient buffer) is reused by the ack publish
        JsonDocument command;
        if (deserializeJson(command, payload, length)) {
            Serial.println("[MQTT] invalid command (JSON)");
            return;
        }
        JsonDocument ack;
        const char *id = command["id"] | "";
        if (id[0] != '\0' && strlen(id) <= 64) ack["id"] = id;
        ack["command"] = command["command"] | "";

        const char *error = commandHandler
                                ? commandHandler(command.as<JsonObjectConst>(), ack["state"].to<JsonObject>())
                                : "no handler";
        ack["ok"] = error == nullptr;
        if (error) ack["error"] = error;
        Serial.printf("[MQTT] command \"%s\" %s\n", ack["command"].as<const char *>(), error ? error : "ok");
        publishJson(topicAck, ack);
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

        tls.setX509Time(isTimeValid() ? time(nullptr) : buildTime());
        if (mqtt.connect(DEVICE_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
            // QoS 1: a command sent while the broker is reachable is not lost on the last hop
            const bool subscribed = mqtt.subscribe(topicCommand.c_str(), 1);
            Serial.printf("[MQTT] connected, cmd subscription=%d\n", subscribed);
            return true;
        }

        char tlsError[64] = "";
        const int tlsCode = tls.getLastSSLError(tlsError, sizeof(tlsError));
        // state -2: network / TLS (see tlsError), 4: bad credentials, 5: not authorized.
        // "Unknown error code" = no TLS handshake: TCP connection to MQTT_HOST:MQTT_PORT refused or unreachable
        Serial.printf("[MQTT] connection failed to %s:%u, state=%d tls=%d \"%s\"\n", MQTT_HOST, MQTT_PORT,
                      mqtt.state(), tlsCode, tlsError);
        return false;
    }

public:
    MqttClient() : caCert(MQTT_CA_CERT), mqtt(tls) {
    }

    // Call once in setup(); the connection itself is made by loop() once Wi-Fi is up
    void initialize(CommandHandler onCommand) {
        commandHandler = std::move(onCommand);
        topicTelemetry = String("sentinelx/") + DEVICE_ID + "/telemetry";
        topicCommand = String("sentinelx/") + DEVICE_ID + "/cmd";
        topicAck = String("sentinelx/") + DEVICE_ID + "/ack";

        configTime(0, 0, "pool.ntp.org", "time.google.com");
        tls.setTrustAnchors(&caCert);

        IPAddress ip;
        // By IP: the chain is checked against the CA, not the host name.
        // By name: the name must be in the broker certificate (mqtt.sentinel.lan).
        if (ip.fromString(MQTT_HOST)) mqtt.setServer(ip, MQTT_PORT);
        else mqtt.setServer(MQTT_HOST, MQTT_PORT);
        mqtt.setBufferSize(MQTT_PAYLOAD_MAX + 64);
        mqtt.setCallback([this](char *topic, uint8_t *payload, unsigned int length) {
            onMessage(topic, payload, length);
        });
    }

    // Call on every loop(): keeps the connection alive and reconnects without blocking for long
    void loop() {
        if (WiFi.status() != WL_CONNECTED) return;

        if (!mqtt.connected()) {
            const unsigned long now = millis();
            if (lastAttemptTime != 0 && now - lastAttemptTime < MQTT_RECONNECT_INTERVAL_MS) return;
            lastAttemptTime = now;
            if (!isTimeValid() && now < NTP_TIMEOUT_MS) {
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
};

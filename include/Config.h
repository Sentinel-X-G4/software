#pragma once
#include <Arduino.h>

// --- PINS ---
constexpr int PIN_DHT22 = D5;
constexpr int PIN_PIR = D6;
constexpr int PIN_MQ2_DO = D7;
constexpr int PIN_MQ2_AO = A0;
constexpr int PIN_BUZZER = D4;
constexpr int PIN_LED_R = D8;
constexpr int PIN_LED_V = D3;

// --- DISPLAY ---
constexpr uint8_t OLED_ADDRESS = 0x3C;

// --- TIMING (ms) ---
constexpr unsigned long READ_INTERVAL_MS = 2000;
constexpr unsigned long TELEMETRY_INTERVAL_MS = 200; // ~5 msg/s required by the detection service
constexpr unsigned long WIFI_RETRY_INTERVAL_MS = 20000; // Time given to each Wi-Fi attempt before retrying
constexpr unsigned long MQTT_RECONNECT_INTERVAL_MS = 5000;
// No NTP after this delay (network without Internet): TLS checks the certificate dates against the build date
constexpr unsigned long NTP_TIMEOUT_MS = 30000;
constexpr unsigned long MQ2_WARMUP_MS = 180000;
constexpr unsigned long PIR_WARMUP_MS = 60000;

// --- THRESHOLDS ---
constexpr float THRESHOLD_TEMP_MAX = 30.0;
constexpr float THRESHOLD_TEMP_MIN = -10.0;
constexpr int THRESHOLD_GAS_MAX = 400;
constexpr int THRESHOLD_GAS_MIN = 40;

// --- NETWORK & SERVER ---
static const char *WIFI_SSID = "Vicky";
static const char *WIFI_PASSWORD = "11111111";

// MQTTS broker (TLS checked against secrets/ca.crt, see scripts/embed_ca.py)
// IP of the machine running the stack: port 8883 is only published on BIND_IP (main/.env),
// so BIND_IP must be this same IP (192.168.40.1 on the production server)
static const char *MQTT_HOST = "172.20.10.2";
constexpr uint16_t MQTT_PORT = 8883;
static const char *MQTT_USERNAME = "sentinel_iot";
static const char *MQTT_PASSWORD = "dev-pass-123"; // MQTT_ESP_PASSWORD in main/.env
constexpr size_t MQTT_PAYLOAD_MAX = 256;

// Must match the broker ACL: the sentinel_iot account can only publish on sentinelx/esp01/...
static const char *DEVICE_ID = "esp01";

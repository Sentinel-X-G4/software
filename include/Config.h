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
constexpr unsigned long SEND_INTERVAL_MS = 5000;
constexpr unsigned long MQ2_WARMUP_MS = 180000;
constexpr unsigned long PIR_WARMUP_MS = 60000;

// --- THRESHOLDS ---
constexpr float THRESHOLD_TEMP_MAX = 30.0;
constexpr float THRESHOLD_TEMP_MIN = -10.0;
constexpr int THRESHOLD_GAS_MAX = 400;
constexpr int THRESHOLD_GAS_MIN = 40;

// --- NETWORK & SERVER ---
static const char *WIFI_SSID = "YOUR_SSID";
static const char *WIFI_PASSWORD = "YOUR_PASSWORD";

static const char *SERVER_HOST = "192.168.1.100";
constexpr int SERVER_PORT_HTTPS = 443;
static const char *ENDPOINT_ALERTS = "/api/v1/alerts";
constexpr int WEBSOCKET_PORT = 8080;

static const char *TLS_FINGERPRINT = "AA:BB:CC:DD:EE:FF:00:11:22:33:44:55:66:77:88:99:AA:BB:CC:DD";
static const char *DEVICE_ID = "sentinel-x-01";

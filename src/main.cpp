#include <Arduino.h>

// Import our custom modules
#include "Config.h"
#include "Actuators.h"
#include "Sensors.h"
#include "Display.h"
#include "WifiClient.h"
#include "MqttClient.h"

// Instantiate our objects
Actuators myActuators;
Sensors mySensors;
Display myDisplay;
WifiClient myWifi;
MqttClient myMqtt;

// Timing variables
unsigned long lastReadTime = 0;
unsigned long lastTelemetryTime = 0;
bool hasFreshClimate = false;

// --- Remote commands (sentinelx/{DEVICE_ID}/cmd, sent by backend-api) ---
// The only way to raise an alert: no local threshold. Everything persists until "reset" or a reboot.

const char *const ALERT_STATES[] = {"off", "on"};
const char *const BUZZER_MODES[] = {"auto", "on", "off"};                   // BuzzerMode order
const char *const LED_MODES[] = {"auto", "red", "green", "both", "off"};    // LedMode order
const char *const SCREEN_MODES[] = {"auto", "off", "message"};              // ScreenMode order

// Index of `value` in `names`, -1 if absent
template <size_t N>
int findMode(const char *const (&names)[N], const char *value) {
    for (size_t i = 0; i < N; i++) {
        if (strcmp(names[i], value) == 0) return static_cast<int>(i);
    }
    return -1;
}

const char *handleCommand(JsonObjectConst command, JsonObject state) {
    const char *name = command["command"] | "";
    const char *value = command["state"] | "";
    const char *error = nullptr;

    if (strcmp(name, "alert") == 0) {
        const int active = findMode(ALERT_STATES, value);
        if (active < 0) error = "state: on | off";
        else {
            myActuators.setAlert(active == 1);
            lastReadTime = 0; // ALERT line redrawn on the next loop()
        }
    } else if (strcmp(name, "buzzer") == 0) {
        const int mode = findMode(BUZZER_MODES, value);
        if (mode < 0) error = "state: auto | on | off";
        else myActuators.setBuzzer(static_cast<BuzzerMode>(mode));
    } else if (strcmp(name, "led") == 0) {
        const int mode = findMode(LED_MODES, value);
        if (mode < 0) error = "state: auto | red | green | both | off";
        else myActuators.setLed(static_cast<LedMode>(mode));
    } else if (strcmp(name, "screen") == 0) {
        const char *text = command["text"] | "";
        switch (findMode(SCREEN_MODES, value)) {
            case 0: myDisplay.setAuto(); lastReadTime = 0; break; // Dashboard redrawn on the next loop()
            case 1: myDisplay.turnOff(); break;
            case 2:
                if (text[0] == '\0') error = "text required";
                else myDisplay.showMessage(text);
                break;
            default: error = "state: auto | off | message";
        }
    } else if (strcmp(name, "reset") == 0) {
        myActuators.reset();
        myDisplay.setAuto();
        lastReadTime = 0;
    } else {
        error = "command: alert | buzzer | led | screen | reset";
    }

    state["alert"] = ALERT_STATES[myActuators.isAlertActive ? 1 : 0];
    state["buzzer"] = BUZZER_MODES[static_cast<int>(myActuators.buzzerMode)];
    state["led"] = LED_MODES[static_cast<int>(myActuators.ledMode)];
    state["screen"] = SCREEN_MODES[static_cast<int>(myDisplay.mode)];
    return error;
}

// --- Main Program ---

void setup() {
    Serial.begin(115200);

    myActuators.initialize();
    myDisplay.initialize();
    mySensors.initialize();

    myDisplay.showSimpleMessage("SENTINEL-X", "Booting...");

    myDisplay.showSimpleMessage("Wi-Fi", "Connecting...");
    myWifi.initialize();
    myMqtt.initialize(handleCommand);
}

void loop() {
    myWifi.loop();
    unsigned long currentTime = millis();

    // 1. Read sensors & update display (Every 2 seconds)
    if (currentTime - lastReadTime >= READ_INTERVAL_MS) {
        lastReadTime = currentTime;

        mySensors.readAll();
        hasFreshClimate = true;

        myDisplay.showDashboard(mySensors, myActuators.isAlertActive, myWifi.isConnected());
    }

    // 2. Publish telemetry over MQTT (Every 200 ms; temp/hum only right after a DHT22 read)
    if (myWifi.isConnected() && (currentTime - lastTelemetryTime >= TELEMETRY_INTERVAL_MS)) {
        lastTelemetryTime = currentTime;
        mySensors.readMotion();
        if (myMqtt.publishTelemetry(mySensors, hasFreshClimate, currentTime < MQ2_WARMUP_MS)) {
            hasFreshClimate = false;
        }
    }

    // 3. Keep MQTT alive (reconnects, incoming commands)
    myMqtt.loop();
}

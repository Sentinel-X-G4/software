#pragma once
#include <Arduino.h>
#include "Config.h"

// Alerts are only triggered remotely ("alert" command on sentinelx/{DEVICE_ID}/cmd), never by the
// sensors. Auto: the output follows that alert; the other modes force it. Everything persists until
// "reset" or a reboot.
enum class BuzzerMode : uint8_t { Auto, On, Off };
enum class LedMode : uint8_t { Auto, Red, Green, Both, Off };

class Actuators {
private:
    void apply() {
        const bool buzzer = buzzerMode == BuzzerMode::Auto ? isAlertActive : buzzerMode == BuzzerMode::On;
        bool red = false;
        bool green = false;
        switch (ledMode) {
            case LedMode::Auto: red = isAlertActive; green = !isAlertActive; break;
            case LedMode::Red: red = true; break;
            case LedMode::Green: green = true; break;
            case LedMode::Both: red = true; green = true; break;
            case LedMode::Off: break;
        }
        digitalWrite(PIN_BUZZER, buzzer ? HIGH : LOW);
        digitalWrite(PIN_LED_R, red ? HIGH : LOW);
        digitalWrite(PIN_LED_V, green ? HIGH : LOW);
    }

public:
    bool isAlertActive = false;
    BuzzerMode buzzerMode = BuzzerMode::Auto;
    LedMode ledMode = LedMode::Auto;

    void initialize() {
        pinMode(PIN_BUZZER, OUTPUT);
        pinMode(PIN_LED_R, OUTPUT);
        pinMode(PIN_LED_V, OUTPUT);
        apply(); // No alert: buzzer off, green LED
    }

    // Remote alert: only the outputs left in Auto follow it
    void setAlert(bool active) {
        isAlertActive = active;
        apply();
    }

    void setBuzzer(BuzzerMode mode) {
        buzzerMode = mode;
        apply();
    }

    void setLed(LedMode mode) {
        ledMode = mode;
        apply();
    }

    void reset() {
        isAlertActive = false;
        buzzerMode = BuzzerMode::Auto;
        ledMode = LedMode::Auto;
        apply();
    }
};

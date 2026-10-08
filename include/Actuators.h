#pragma once
#include <Arduino.h>
#include "Config.h"
#include "Melody.h"

// Alerts are only triggered remotely ("alert" command on sentinelx/{DEVICE_ID}/cmd), never by the
// sensors. Auto: the output follows that alert; the other modes force it. Everything persists until
// "reset" or a reboot.
enum class BuzzerMode : uint8_t { Auto, On, Off };
enum class LedMode : uint8_t { Auto, Red, Green, Both, Off };

class Actuators {
private:
    // Melody player: tone() runs on a hardware timer, loop() only moves to the next note
    bool isMelodyPlaying = false;
    size_t noteIndex = 0;
    unsigned long noteStartedAt = 0;
    bool isNoteSounding = false;

    void silenceBuzzer() {
        noTone(PIN_BUZZER);
        digitalWrite(PIN_BUZZER, BUZZER_ACTIVE_LOW ? HIGH : LOW);
        isNoteSounding = false;
    }

    void startNote(size_t index) {
        noteIndex = index;
        noteStartedAt = millis();
        const uint16_t frequency = ALERT_MELODY[noteIndex].frequency;
        if (frequency == NOTE_REST) {
            silenceBuzzer();
        } else {
            tone(PIN_BUZZER, frequency);
            isNoteSounding = true;
        }
    }

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
        if (buzzer && !isMelodyPlaying) {
            isMelodyPlaying = true;
            startNote(0);
        } else if (!buzzer) {
            isMelodyPlaying = false;
            silenceBuzzer();
        }
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

    // Called on every loop(): advances the melody without blocking, restarts it at the end
    void loop() {
        if (!isMelodyPlaying) return;
        const unsigned long elapsed = millis() - noteStartedAt;
        const uint16_t duration = ALERT_MELODY[noteIndex].durationMs;
        if (elapsed >= duration) {
            startNote((noteIndex + 1) % ALERT_MELODY_LENGTH);
        } else if (isNoteSounding && elapsed >= duration * NOTE_SOUNDED_PERCENT / 100) {
            silenceBuzzer();
        }
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

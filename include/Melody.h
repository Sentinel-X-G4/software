#pragma once
#include <Arduino.h>

// Played in a loop by the buzzer (passive) while it is on. frequency = 0: silence.
struct Note {
    uint16_t frequency; // Hz
    uint16_t durationMs;
};

// --- NOTES (Hz) ---
constexpr uint16_t NOTE_REST = 0;
constexpr uint16_t NOTE_CS4 = 277;
constexpr uint16_t NOTE_D4 = 294;
constexpr uint16_t NOTE_DS4 = 311;
constexpr uint16_t NOTE_E4 = 330;
constexpr uint16_t NOTE_FS4 = 370;
constexpr uint16_t NOTE_G4 = 392;
constexpr uint16_t NOTE_GS4 = 415;
constexpr uint16_t NOTE_A4 = 440;
constexpr uint16_t NOTE_B4 = 494;
constexpr uint16_t NOTE_CS5 = 554;
constexpr uint16_t NOTE_D5 = 587;
constexpr uint16_t NOTE_EB5 = 622;
constexpr uint16_t NOTE_E5 = 659;

constexpr uint16_t EIGHTH_MS = 263; // Quarter note = 114 bpm

constexpr Note n(uint16_t frequency, uint16_t eighths) { return {frequency, static_cast<uint16_t>(eighths * EIGHTH_MS)}; }

// Mii Channel theme (K. Totaka, arr. Olimar12345 / N. Wilson), melody line of bars 1-12
constexpr Note ALERT_MELODY[] = {
    n(NOTE_FS4, 2), n(NOTE_A4, 1), n(NOTE_CS5, 1), n(NOTE_REST, 1), n(NOTE_A4, 1), n(NOTE_REST, 1), n(NOTE_FS4, 1),
    n(NOTE_D4, 1), n(NOTE_D4, 1), n(NOTE_D4, 1), n(NOTE_REST, 4), n(NOTE_CS4, 1),
    n(NOTE_D4, 1), n(NOTE_FS4, 1), n(NOTE_A4, 1), n(NOTE_CS5, 1), n(NOTE_REST, 1), n(NOTE_A4, 1), n(NOTE_REST, 1), n(NOTE_FS4, 1),
    n(NOTE_E5, 3), n(NOTE_EB5, 1), n(NOTE_D5, 2), n(NOTE_REST, 2),

    n(NOTE_GS4, 2), n(NOTE_CS5, 1), n(NOTE_FS4, 1), n(NOTE_REST, 1), n(NOTE_CS5, 1), n(NOTE_REST, 1), n(NOTE_GS4, 1),
    n(NOTE_REST, 1), n(NOTE_CS5, 1), n(NOTE_REST, 1), n(NOTE_G4, 1), n(NOTE_FS4, 1), n(NOTE_REST, 1), n(NOTE_E4, 1), n(NOTE_REST, 1),
    n(NOTE_E4, 1), n(NOTE_E4, 1), n(NOTE_E4, 1), n(NOTE_REST, 3), n(NOTE_E4, 1), n(NOTE_E4, 1),
    n(NOTE_E4, 1), n(NOTE_REST, 3), n(NOTE_DS4, 2), n(NOTE_D4, 2),

    n(NOTE_CS4, 2), n(NOTE_A4, 1), n(NOTE_CS5, 1), n(NOTE_REST, 1), n(NOTE_A4, 1), n(NOTE_REST, 1), n(NOTE_FS4, 1),
    n(NOTE_E4, 1), n(NOTE_E4, 1), n(NOTE_E4, 1), n(NOTE_REST, 1), n(NOTE_E5, 1), n(NOTE_E5, 1), n(NOTE_E5, 1), n(NOTE_REST, 1),
    n(NOTE_REST, 1), n(NOTE_FS4, 1), n(NOTE_A4, 1), n(NOTE_CS5, 1), n(NOTE_REST, 1), n(NOTE_A4, 1), n(NOTE_REST, 1), n(NOTE_FS4, 1),
    n(NOTE_CS5, 4), n(NOTE_B4, 2), n(NOTE_REST, 2),
};
constexpr size_t ALERT_MELODY_LENGTH = sizeof(ALERT_MELODY) / sizeof(ALERT_MELODY[0]);

// Share of each note actually sounded: the gap separates repeated notes
constexpr uint8_t NOTE_SOUNDED_PERCENT = 90;

#pragma once
#include <Arduino.h>

// Played in a loop by the buzzer (passive) while it is on. frequency = 0: silence.
struct Note {
    uint16_t frequency; // Hz
    uint16_t durationMs;
};

// --- NOTES (Hz) ---
constexpr uint16_t NOTE_REST = 0;
constexpr uint16_t NOTE_F3 = 175;
constexpr uint16_t NOTE_A3 = 220;
constexpr uint16_t NOTE_BB3 = 233;
constexpr uint16_t NOTE_C4 = 262;
constexpr uint16_t NOTE_D4 = 294;
constexpr uint16_t NOTE_EB4 = 311;
constexpr uint16_t NOTE_F4 = 349;
constexpr uint16_t NOTE_G4 = 392;
constexpr uint16_t NOTE_A4 = 440;
constexpr uint16_t NOTE_BB4 = 466;
constexpr uint16_t NOTE_D5 = 587;
constexpr uint16_t NOTE_F5 = 698;
constexpr uint16_t NOTE_G5 = 784;

// Piezo buzzers are faint in the bass: the melody is played this many octaves above the score
constexpr uint8_t MELODY_OCTAVE_SHIFT = 1;

constexpr uint16_t EIGHTH_MS = 188; // Quarter note = 160 bpm

constexpr Note n(uint16_t frequency, uint16_t eighths) { return {frequency, static_cast<uint16_t>(eighths * EIGHTH_MS)}; }

// Pokemon theme (J. Siegler, arr. L. de Wit / N. Wilson), melody line, ties merged:
// verse (bars 5-11, 20) then chorus (bars 21-28); the loop restarts on the verse upbeat
constexpr Note ALERT_MELODY[] = {
    // I wanna be the very best, like no one ever was
    n(NOTE_D4, 2), n(NOTE_D4, 1), n(NOTE_D4, 1), n(NOTE_D4, 1), n(NOTE_D4, 3), n(NOTE_D4, 1),
    n(NOTE_C4, 2), n(NOTE_A3, 1), n(NOTE_F3, 4), n(NOTE_F3, 1),
    n(NOTE_D4, 2), n(NOTE_D4, 2), n(NOTE_C4, 1), n(NOTE_BB3, 1), n(NOTE_C4, 5), n(NOTE_C4, 2), n(NOTE_D4, 3),
    // To catch them is my real test, to train them is my cause
    n(NOTE_EB4, 1), n(NOTE_EB4, 1), n(NOTE_EB4, 2), n(NOTE_EB4, 2), n(NOTE_EB4, 1), n(NOTE_D4, 2),
    n(NOTE_C4, 2), n(NOTE_BB3, 4), n(NOTE_BB3, 1),
    n(NOTE_D4, 2), n(NOTE_D4, 1), n(NOTE_C4, 2), n(NOTE_BB3, 1), n(NOTE_D4, 5), n(NOTE_D4, 2),
    // Pokemon! (Gotta catch 'em all)
    n(NOTE_D5, 1), n(NOTE_F5, 1), n(NOTE_G5, 2), n(NOTE_REST, 1),
    n(NOTE_D4, 1), n(NOTE_D4, 1), n(NOTE_F4, 1), n(NOTE_G4, 2), n(NOTE_G4, 1),
    n(NOTE_F4, 1), n(NOTE_D4, 1), n(NOTE_C4, 1), n(NOTE_C4, 1), n(NOTE_BB3, 4),
    // It's you and me, I know it's my destiny
    n(NOTE_REST, 3), n(NOTE_G4, 1), n(NOTE_G4, 1), n(NOTE_A4, 1), n(NOTE_BB4, 1), n(NOTE_A4, 2),
    n(NOTE_G4, 1), n(NOTE_F4, 1), n(NOTE_F4, 2),
    // Pokemon! Oh, you're my best friend, in a world we must defend
    n(NOTE_D5, 1), n(NOTE_F5, 1), n(NOTE_G5, 2), n(NOTE_REST, 1), n(NOTE_G4, 4), n(NOTE_REST, 1), n(NOTE_G4, 1),
    n(NOTE_F4, 1), n(NOTE_D4, 1), n(NOTE_C4, 1), n(NOTE_C4, 1), n(NOTE_BB3, 2), n(NOTE_BB3, 1), n(NOTE_C4, 1),
    n(NOTE_EB4, 2), n(NOTE_D4, 1), n(NOTE_C4, 2), n(NOTE_BB3, 1), n(NOTE_D4, 5), n(NOTE_D4, 2),
    n(NOTE_REST, 2),
};
constexpr size_t ALERT_MELODY_LENGTH = sizeof(ALERT_MELODY) / sizeof(ALERT_MELODY[0]);

// Share of each note actually sounded: the gap separates repeated notes
constexpr uint8_t NOTE_SOUNDED_PERCENT = 90;

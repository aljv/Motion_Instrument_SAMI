// midi.c
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include "VS1053_interface.h"
#include "midi.h"

LOG_MODULE_REGISTER(midi_test, LOG_LEVEL_INF);

// Standard MIDI Notes (Middle C = 60)
#define MIDI_NOTE_C3    48
#define MIDI_NOTE_C4    60   // Middle C
#define MIDI_NOTE_C5    72
#define MIDI_NOTE_D4    62
#define MIDI_NOTE_E4    64
#define MIDI_NOTE_F4    65
#define MIDI_NOTE_G4    67
#define MIDI_NOTE_A4    69   // A440
#define MIDI_NOTE_B4    71

// VS1053 MIDI Implementation using your header definitions
void midi_send_byte(uint8_t data) {
    uint8_t packet[2] = {0x00, data};
    VS1053WriteSdi(packet, 2); 
    k_msleep(10);
}

// Implement your MIDI functions for VS1053
void midiSetInstrument(uint8_t chan, uint8_t inst) {
    if (chan > 15 || inst > 127) return;

    LOG_INF("MIDI Set Instrument: Ch=%d, Inst=%d", chan, inst);

    midi_send_byte(program_chng | chan);  // Use your header definition
    midi_send_byte(inst);
}

void midiSetChannelVolume(uint8_t chan, uint8_t vol) {
    if (chan > 15 || vol > 127) return;

    LOG_INF("MIDI Set Volume: Ch=%d, Vol=%d", chan, vol);

    midi_send_byte(control_change | chan);  // Use your header definition
    midi_send_byte(0x07);  // Volume controller
    midi_send_byte(vol);
}

void midiSetChannelBank(uint8_t chan, uint8_t bank) {
    if (chan > 15 || bank > 127) return;

    LOG_INF("MIDI Set Bank: Ch=%d, Bank=%d", chan, bank);

    midi_send_byte(control_change | chan);
    midi_send_byte((uint8_t)0x00);  // Bank select MSB
    midi_send_byte(bank);
}

void midiNoteOn(uint8_t chan, uint8_t note, uint8_t vel) {
    if (chan > 15 || note > 127 || vel > 127) return;

    LOG_INF("MIDI Note ON: Ch=%d, Note=%d, Vel=%d", chan, note, vel);

    midi_send_byte(note_on | chan);  // Use your header definition
    midi_send_byte(note);
    midi_send_byte(vel);
}

void midiNoteOff(uint8_t chan, uint8_t note, uint8_t vel) {
    if (chan > 15 || note > 127 || vel > 127) return;

    LOG_INF("MIDI Note OFF: Ch=%d, Note=%d, Vel=%d", chan, note, vel);

    midi_send_byte(note_off | chan);  // Use your header definition
    midi_send_byte(note);
    midi_send_byte(vel);
}

// Additional helper functions
void midi_all_notes_off(uint8_t channel) {
    LOG_INF("MIDI All Notes Off: Ch=%d", channel);
    midi_send_byte(control_change | channel);
    midi_send_byte(0x7B);  // All Notes Off controller
    midi_send_byte(0x00);
}

void midi_all_sound_off(uint8_t channel) {
    LOG_INF("MIDI All Sound Off: Ch=%d", channel);
    midi_send_byte(control_change | channel);
    midi_send_byte(0x78);  // All Sound Off controller
    midi_send_byte(0x00);
}

// Test Functions
void test_single_note(void) {
    LOG_INF("=== Testing Single Note ===");

    // Set acoustic grand piano on channel 0 (using your definitions)
    midiSetInstrument(0, ACOUSTIC_GRAND_PIANO);
    k_msleep(100);

    // Set volume to medium
    midiSetChannelVolume(0, 100);
    k_msleep(100);

    // Play middle C
    midiNoteOn(0, MIDI_NOTE_C4, 127);
    k_msleep(1000);  // Hold for 1 second
    midiNoteOff(0, MIDI_NOTE_C4, 64);

    k_msleep(500);
    LOG_INF("Single note test complete");
}

void test_scale(void) {
    LOG_INF("=== Testing C Major Scale ===");

    uint8_t scale[] = {
        MIDI_NOTE_C4,     // C
        MIDI_NOTE_D4,     // D
        MIDI_NOTE_E4,     // E
        MIDI_NOTE_F4,     // F
        MIDI_NOTE_G4,     // G
        MIDI_NOTE_A4,     // A
        MIDI_NOTE_B4,     // B
        MIDI_NOTE_C5      // C (octave)
    };

    // Set acoustic grand piano
    midiSetInstrument(0, ACOUSTIC_GRAND_PIANO);
    k_msleep(100);

    for (int i = 0; i < 8; i++) {
        LOG_INF("Playing note %d of scale", i + 1);
        midiNoteOn(0, scale[i], 100);
        k_msleep(500);
        midiNoteOff(0, scale[i], 64);
        k_msleep(100);
    }

    LOG_INF("Scale test complete");
}

void test_chord(void) {
    LOG_INF("=== Testing C Major Chord ===");

    // Set electric grand piano
    midiSetInstrument(0, ELECTRIC_GRAND_PIANO);
    k_msleep(100);

    // Play C major chord (C-E-G)
    LOG_INF("Playing C major chord");
    midiNoteOn(0, MIDI_NOTE_C4, 100);      // C
    k_msleep(50);
    midiNoteOn(0, MIDI_NOTE_E4, 100);      // E
    k_msleep(50);
    midiNoteOn(0, MIDI_NOTE_G4, 100);      // G

    k_msleep(2000);  // Hold chord for 2 seconds

    // Release chord
    midiNoteOff(0, MIDI_NOTE_C4, 64);
    midiNoteOff(0, MIDI_NOTE_E4, 64);
    midiNoteOff(0, MIDI_NOTE_G4, 64);

    k_msleep(500);
    LOG_INF("Chord test complete");
}

void test_instruments(void) {
    LOG_INF("=== Testing Different Instruments (No Sustain Issues) ===");

    // Use instruments from your header that you've verified work well
    uint8_t instruments[] = {
        ACOUSTIC_GRAND_PIANO,
        ELECTRIC_GRAND_PIANO,
        ACOUSTIC_GUITAR,
        ELECTRIC_GUITAR,
        XYLOPHONE,
        ELECTRIC_BASS,
        WOOD_BLOCK,
        ALTO_SAXOPHONE,
        TRUMPET,
        FLUTE,
        TROMBONE,
        VIOLIN
    };

    const char* instrument_names[] = {
        "Acoustic Grand Piano",
        "Electric Grand Piano",
        "Acoustic Guitar",
        "Electric Guitar",
        "Xylophone",
        "Electric Bass",
        "Wood Block",
        "Alto Saxophone",
        "Trumpet",
        "Flute",
        "Trombone",
        "Violin"
    };

    for (int i = 0; i < 12; i++) {
        LOG_INF("Testing instrument: %s", instrument_names[i]);

        midiSetInstrument(0, instruments[i]);
        k_msleep(200);

        // Play a simple melody
        midiNoteOn(0, MIDI_NOTE_C4, 100);
        k_msleep(400);
        midiNoteOff(0, MIDI_NOTE_C4, 64);
        k_msleep(100);

        midiNoteOn(0, MIDI_NOTE_E4, 100);
        k_msleep(400);
        midiNoteOff(0, MIDI_NOTE_E4, 64);
        k_msleep(100);

        midiNoteOn(0, MIDI_NOTE_G4, 100);
        k_msleep(400);
        midiNoteOff(0, MIDI_NOTE_G4, 64);
        k_msleep(300);
    }

    LOG_INF("Instrument test complete");
}

void test_multi_channel(void) {
    LOG_INF("=== Testing Multi-Channel ===");

    // Set different instruments on different channels
    midiSetInstrument(0, ACOUSTIC_GRAND_PIANO);  // Channel 0: Piano
    k_msleep(100);
    midiSetInstrument(1, VIOLIN);                // Channel 1: Violin
    k_msleep(100);
    midiSetInstrument(2, TRUMPET);               // Channel 2: Trumpet
    k_msleep(100);

    // Set volumes
    midiSetChannelVolume(0, 100);  // Piano medium
    midiSetChannelVolume(1, 80);   // Violin softer
    midiSetChannelVolume(2, 120);  // Trumpet louder
    k_msleep(100);

    // Play harmony
    LOG_INF("Playing multi-channel harmony");
    midiNoteOn(0, MIDI_NOTE_C4, 100);      // Piano: C
    k_msleep(200);
    midiNoteOn(1, MIDI_NOTE_E4, 90);       // Violin: E
    k_msleep(200);
    midiNoteOn(2, MIDI_NOTE_G4, 110);      // Trumpet: G

    k_msleep(2000);  // Hold harmony

    // Release all
    midiNoteOff(0, MIDI_NOTE_C4, 64);
    midiNoteOff(1, MIDI_NOTE_E4, 64);
    midiNoteOff(2, MIDI_NOTE_G4, 64);

    k_msleep(500);
    LOG_INF("Multi-channel test complete");
}

void test_volume_control(void) {
    LOG_INF("=== Testing Volume Control ===");

    midiSetInstrument(0, ACOUSTIC_GRAND_PIANO);
    k_msleep(100);

    uint8_t volumes[] = {30, 60, 90, MAXIMUM_MIDI_VOLUME};

    for (int i = 0; i < 4; i++) {
        LOG_INF("Playing at volume %d", volumes[i]);
        midiSetChannelVolume(0, volumes[i]);
        k_msleep(100);

        midiNoteOn(0, MIDI_NOTE_C4, 100);
        k_msleep(800);
        midiNoteOff(0, MIDI_NOTE_C4, 64);
        k_msleep(300);
    }

    LOG_INF("Volume control test complete");
}

void test_special_instruments(void) {
    LOG_INF("=== Testing Special/Effect Instruments ===");

    // Test some of the more unique instruments from your header
    uint8_t special_instruments[] = {
        BAG_PIPE,
        SCIFI_FX,
        SYNTH_DRUM,
        XYLOPHONE
    };

    const char* special_names[] = {
        "Bag Pipe",
        "Sci-Fi FX",
        "Synth Drum",
        "Xylophone"
    };

    for (int i = 0; i < 4; i++) {
        LOG_INF("Testing special instrument: %s", special_names[i]);

        midiSetInstrument(0, special_instruments[i]);
        k_msleep(200);

        // Play a pattern that shows off the instrument character
        for (int note = 0; note < 4; note++) {
            uint8_t play_note = MIDI_NOTE_C4 + (note * 2);  // C, D, E, F#
            midiNoteOn(0, play_note, 100);
            k_msleep(300);
            midiNoteOff(0, play_note, 64);
            k_msleep(100);
        }

        k_msleep(500);
    }

    LOG_INF("Special instruments test complete");
}

void cleanup_midi(void) {
    LOG_INF("=== Cleaning up MIDI ===");

    // Stop all notes on all channels
    for (int ch = 0; ch < 16; ch++) {
        midi_all_notes_off(ch);
        k_msleep(10);
        midi_all_sound_off(ch);
        k_msleep(10);
    }

    LOG_INF("MIDI cleanup complete");
}


// Main MIDI Test Function
void vs1053_midi_test_suite(void) {
    LOG_INF("=== VS1053 MIDI Test Suite Starting ===");

    // Run individual tests
    test_single_note();
    k_msleep(1000);

    test_scale();
    k_msleep(1000);

    test_chord();
    k_msleep(1000);

    test_instruments();
    k_msleep(1000);

    test_special_instruments();
    k_msleep(1000);

    test_multi_channel();
    k_msleep(1000);

    test_volume_control();
    k_msleep(1000);

    // Cleanup
    cleanup_midi();

    LOG_INF("=== VS1053 MIDI Test Suite Complete ===");
}

// Simple test for quick verification
void vs1053_midi_quick_test(void) {
    LOG_INF("=== VS1053 MIDI Quick Test ===");

    //setup_vs1053_midi_mode();
    //k_msleep(200);

    midiNoteOn(0, MIDI_NOTE_C4, 108);
    k_msleep(250);
    midiNoteOff(0, MIDI_NOTE_C4, 0);

    midiNoteOn(0, MIDI_NOTE_D4, 108);
    k_msleep(250);
    midiNoteOff(0, MIDI_NOTE_D4, 0);

    midiNoteOn(0, MIDI_NOTE_C4, 108);
    k_msleep(250);
    midiNoteOff(0, MIDI_NOTE_C4, 0);
    midiNoteOn(0, MIDI_NOTE_D4, 108);
    k_msleep(250);
    midiNoteOff(0, MIDI_NOTE_D4, 0);

    //cleanup_midi();

    LOG_INF("MIDI quick test complete");
}

// Function to call from main
void run_midi_tests(void) {
    LOG_INF("Starting MIDI tests in 2 seconds...");
    k_msleep(2000);

    // Run quick test first
    vs1053_midi_quick_test();
    k_msleep(2000);

    // Then run full suite
    //vs1053_midi_test_suite();
}

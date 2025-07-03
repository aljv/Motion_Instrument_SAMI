#ifndef _MIDIFILE_H_
#define _MIDIFILE_H_

#include <stdint.h>
/* Application definitions */

/* definitions for MIDI file parsing code */
extern int (*Mf_getc)();
extern void (*Mf_header)();
extern void (*Mf_trackstart)();
extern void (*Mf_trackend)();
extern void (*Mf_noteon)();
extern void (*Mf_noteoff)();
extern void (*Mf_pressure)();
extern void (*Mf_parameter)();
extern void (*Mf_pitchbend)();
extern void (*Mf_program)();
extern void (*Mf_chanpressure)();
extern void (*Mf_sysex)();
extern void (*Mf_metamisc)();
extern void (*Mf_seqspecific)();
extern void (*Mf_seqnum)();
extern void (*Mf_text)();
extern void (*Mf_eot)();
extern void (*Mf_timesig)();
extern void (*Mf_smpte)();
extern void (*Mf_tempo)();
extern void (*Mf_keysig)();
extern void (*Mf_arbitrary)();
extern void (*Mf_error)();
extern long Mf_currtime;
extern int Mf_nomerge;

/* definitions for MIDI file writing code */
extern int (*Mf_putc)();
extern int (*Mf_writetrack)();
extern int (*Mf_writetempotrack)();
float mf_ticks2sec();
unsigned long mf_sec2ticks();
void mfwrite();

/* MIDI status commands most significant bit is 1 */
#define note_off         	0x80
#define note_on          	0x90
#define poly_aftertouch  	0xa0
#define control_change    	0xB0
#define program_chng     	0xC0
#define channel_aftertouch      0xd0
#define pitch_wheel      	0xe0
#define system_exclusive      	0xf0
#define delay_packet	 	(1111)

/* 7 bit controllers */
#define damper_pedal            0x40
#define portamento	        0x41
#define sostenuto	        0x42
#define soft_pedal	        0x43
#define general_4               0x44
#define	hold_2		        0x45
#define	general_5	        0x50
#define	general_6	        0x51
#define general_7	        0x52
#define general_8	        0x53
#define tremolo_depth	        0x5c
#define chorus_depth	        0x5d
#define	detune		        0x5e
#define phaser_depth	        0x5f

/* parameter values */
#define data_inc	        0x60
#define data_dec	        0x61

/* parameter selection */
#define non_reg_lsb	        0x62
#define non_reg_msb	        0x63
#define reg_lsb		        0x64
#define reg_msb		        0x65

/* Standard MIDI Files meta event definitions */
#define	meta_event		0xFF
#define	sequence_number 	0x00
#define	text_event		0x01
#define copyright_notice 	0x02
#define sequence_name    	0x03
#define instrument_name 	0x04
#define lyric	        	0x05
#define marker			0x06
#define	cue_point		0x07
#define channel_prefix		0x20
#define	end_of_track		0x2f
#define	set_tempo		0x51
#define	smpte_offset		0x54
#define	time_signature		0x58
#define	key_signature		0x59
#define	sequencer_specific	0x74

/* Manufacturer's ID number */
#define Seq_Circuits (0x01) /* Sequential Circuits Inc. */
#define Big_Briar    (0x02) /* Big Briar Inc.           */
#define Octave       (0x03) /* Octave/Plateau           */
#define Moog         (0x04) /* Moog Music               */
#define Passport     (0x05) /* Passport Designs         */
#define Lexicon      (0x06) /* Lexicon 			*/
#define Tempi        (0x20) /* Bon Tempi                */
#define Siel         (0x21) /* S.I.E.L.                 */
#define Kawai        (0x41)
#define Roland       (0x42)
#define Korg         (0x42)
#define Yamaha       (0x43)

/* miscellaneous definitions */
#define MThd 0x4d546864
#define MTrk 0x4d54726b
#define lowerbyte(x) ((unsigned char)(x & 0xff))
#define upperbyte(x) ((unsigned char)((x & 0xff00)>>8))

// All insruments can be found on page 32 of the VS1053 datasheet
// These instruments were chosen as they provided no sustain.. need more investigation on
// what causes this problem on other instruments.
#define ACOUSTIC_GRAND_PIANO  0
#define ELECTRIC_GRAND_PIANO  2
#define ACOUSTIC_GUITAR       24
#define ELECTRIC_GUITAR       26
#define XYLOPHONE             13
#define ELECTRIC_BASS         34
#define BAG_PIPE              109
#define WOOD_BLOCK            115
#define SCIFI_FX              103
#define ALTO_SAXOPHONE        65
#define TRUMPET               56
#define FLUTE                 73
#define TROMBONE              57
#define VIOLIN                40
#define SYNTH_DRUM            119

#define MAXIMUM_MIDI_VOLUME   127

#define NUM_MIDI_NOTES 128 //0-127

#define MAX_MIDI_DATA_LENGTH        4092
#define MAX_MIDI_TRACK_LENGTH       4092
#define MAX_MIDI_TRACKS             2

#define MAX_INSTRUMENTS 13
#define MIN_INSTRUMENTS 1

#define MAX_TEMPO 240
#define MIN_TEMPO 40

#define DEFAULT_MIDI_NOTE_DURATION  960


void midiSetInstrument(uint8_t chan, uint8_t inst);
void midiSetChannelVolume(uint8_t chan, uint8_t vol);
void midiSetChannelBank(uint8_t chan, uint8_t bank);
void midiNoteOn(uint8_t chan, uint8_t n, uint8_t vel);
void midiNoteOff(uint8_t chan, uint8_t n, uint8_t vel);

// VS1053 MIDI Test Function Prototypes
void vs1053_midi_test_suite(void);
void vs1053_midi_quick_test(void);
void run_midi_tests(void);
void setup_vs1053_midi_mode(void);
void cleanup_midi(void);

// Individual test functions
void test_single_note(void);
void test_scale(void);
void test_chord(void);
void test_instruments(void);
void test_multi_channel(void);
void test_volume_control(void);

#endif // _MIDIFILE_H_

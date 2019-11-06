// Configuration for MIDI controller
// by Brian J. Johnson 2017

// This file configures the MIDI controller for your particular project.
// It begins with a section of compile-time constants to control logging
// and other features.  Next is the definition of the physical I/O pins
// on your board, including what they are used for (analog input, digital
// output to a LED, etc.) and their ranges.  Last is an array defining
// how pin changes generate output events, such as MIDI messages and LED
// changes.
// 
// In theory, this is the only part of the sketch you need to modify to
// configure it for a particular project.

//
// General configuration constants.  Define or undef them as needed.
//

// Support MIDI over serial
#define USE_SERIAL_MIDI

// Support MIDI over USB
#define USE_USB_MIDI

// Use the serial log.  If undefined, then WAIT_FOR_SERIAL and LOG_*
// will be undefined as well.
#define USE_SERIAL_PRINT

// Wait for the serial port to connect before running
#undef WAIT_FOR_SERIAL

// If defined, only handle program changes for bank numbers in this list:
#define BANKS_TO_HANDLE \
  0x7000,

// If defined, ignore program changes for bank numbers in this list:
#undef BANKS_TO_IGNORE

// Which types of data to log to the serial console

#undef  LOG_PINS    // Input pin values
#define LOG_EVENTS  // Output events
#define LOG_PROGRAM // Program changes
#undef  LOG_MIDIIN  // Other MIDI input
#define LOG_ERRORS  // Configuration errors

// Amount to delay between polls of the input pins.  Increase this if
// MIDI data is being generated too quickly.
#define LOOP_DELAY 1 // milliseconds

// Enable the code in Fscale.cpp to provide exponentially weighted,
// floating-point scaling for analog inputs.  See the "curve" fields
// in the "Pins" array.  You can disable this if you want to save code
// space and/or avoid floating-point math.  If disabled, the sketch
// will fall back to ordinary linear interpolation.

#define USE_FSCALE

// Input pins.  Edit these to reflect the physical board.
//   Type:  Analog, Digital, DigitalPullup, AnalogOut, or DigitalOut
//   Pin#:  Pin number as known to the Arduino libraries
//   Min:   Expected reading at the physical "minimum" position
//   Max:   Expected reading at the physical "maximum" position
//   Curve: Exponential weighting factor.
//
//  Min, max, and curve are mainly useful for Analog inputs.  Readings
//  outside the min..max range will be clamped.  Max can be less than
//  min if the pin reads higher in the "minimum" position, i.e. the
//  input is physically reversed.  The curve value changes the
//  weighting between input and output at different points in the
//  range.  Use negative values to emphasize the low end, and positive
//  values to emphasize the high end.  Zero produces an ordinary
//  linear mapping.  This can be quite useful to correct for
//  nonlinearities in physical input hardwre, or for our ears'
//  exponential perception of volume.  See Fscale.cpp

const Pin Pins[] = {
  // Idx     Type       pin number   min   max   curve
  /* 0 */ {Analog,        A0,        645,  80,   -1.25}, // Direction reversed
  /* 1 */ {DigitalPullup, 11,        0,    1023,  0.0},
  /* 2 */ {DigitalPullup, 12,        0,    1023,  0.0},
  /* 3 */ {DigitalOut,    3,         0,    255,   0.0},
  /* 4 */ {DigitalOut,    4,         0,    255,   0.0},
};


// Output MIDI events.  Edit these according to the output you want

// Controller change:  channel, number, "on" value, "off" value
#define CNTL_EVENT(_ch, _n, _on, _off) \
  {.Controller = {ControllerEventType, (_ch), (_n), (_on), (_off)}}

// Program change:  channel, program, bank (16 bits)
#define PRGM_EVENT(_ch, _p, _b)        \
  {.Program = {ProgramEventType, (_ch), (_p), (_b) & 0xff, ((_b) >> 8) & 0xff}}

// Note:  channel, note, "on" velocity, "off" velocity
#define NOTE_EVENT(_ch, _n, _on, _off) \
  {.Note = {NoteEventType, (_ch), (_n), (_on), (_off)}}

// Set a pin:  pin, "on" PWM value, "off" PWM value
#define OUTP_EVENT(_p, _on, _off) \
  {.Out = {OutEventType, (_p), (_on), (_off)}}

// Header to indicate the start of a new program:
// channel, program, bank (16 bits)
#define PROGRAM(_ch, _p, _b)                   \
  {PROGRAM_PIN, Momentary, PRGM_EVENT((_ch), (_p), (_b))}

const EventMap DefaultEventList[] = {
  // Pin array idx, Handling, Event macro
  // Illegal value, so first in table is only the default.  But
  // a handled bank number, so it's not ignored on initial boot.
  PROGRAM(255, 0, 0x7000),
  // Turn off modulation on all layers, in case the wheel got bumped
  {INIT_PIN, Momentary, CNTL_EVENT(1, 0x1  /*modwheel*/, 0, 0)}, // Mod=0
  {INIT_PIN, Momentary, CNTL_EVENT(2, 0x1  /*modwheel*/, 0, 0)}, // Mod=0
  {INIT_PIN, Momentary, CNTL_EVENT(3, 0x1  /*modwheel*/, 0, 0)}, // Mod=0
  {INIT_PIN, Momentary, CNTL_EVENT(4, 0x1  /*modwheel*/, 0, 0)}, // Mod=0
  // Turn off the LEDs
  {INIT_PIN, Momentary, OUTP_EVENT(3, 0x0, 0x0)},
  {INIT_PIN, Momentary, OUTP_EVENT(4, 0x0, 0x0)},
  // Pedal controlls expression for all layers
  {0, Continuous,  CNTL_EVENT(1, 0x0b  /*expression*/, 127, 0)},
  {0, Continuous,  CNTL_EVENT(2, 0x0b  /*expression*/, 127, 0)},
  {0, Continuous,  CNTL_EVENT(3, 0x0b  /*expression*/, 127, 0)},
  {0, Continuous,  CNTL_EVENT(4, 0x0b  /*expression*/, 127, 0)},
  // Button 1 controls hold/damper on all layers
  {1, Momentary, CNTL_EVENT(1, 0x40  /*damper*/, 127, 0)},
  {1, Momentary, CNTL_EVENT(2, 0x40  /*damper*/, 127, 0)},
  {1, Momentary, CNTL_EVENT(3, 0x40  /*damper*/, 127, 0)},
  {1, Momentary, CNTL_EVENT(4, 0x40  /*damper*/, 127, 0)},
  // Button 2 controls sostenuto on all layers
  {2, Momentary, CNTL_EVENT(1, 0x42  /*sostenuto*/, 127, 0)},
  {2, Momentary, CNTL_EVENT(2, 0x42  /*sostenuto*/, 127, 0)},
  {2, Momentary, CNTL_EVENT(3, 0x42  /*sostenuto*/, 127, 0)},
  {2, Momentary, CNTL_EVENT(4, 0x42  /*sostenuto*/, 127, 0)},
  // On exit, restore all expression levels to max
  {EXIT_PIN, Momentary, CNTL_EVENT(1, 0xb  /*expr*/,   127, 127)},
  {EXIT_PIN, Momentary, CNTL_EVENT(2, 0xb  /*expr*/,   127, 127)},
  {EXIT_PIN, Momentary, CNTL_EVENT(3, 0xb  /*expr*/,   127, 127)},
  {EXIT_PIN, Momentary, CNTL_EVENT(4, 0xb  /*expr*/,   127, 127)},

  PROGRAM(1, 90, 0x7000), // 4ZonesBJJ
  PROGRAM(1, 98, 0x7000), // 4ZonesJOY
  PROGRAM(1, 99, 0x7000), // BJJViolin
  {INIT_PIN, Momentary, CNTL_EVENT(2, 0x7  /*vol */, 0, 0)}, // Disable layer 2
  {INIT_PIN, Momentary, CNTL_EVENT(3, 0xb  /*expr*/, 0, 0)}, // Disable layer 3
  {INIT_PIN, Momentary, CNTL_EVENT(4, 0xb  /*expr*/, 0, 0)}, // Disable layer 4
  {INIT_PIN, Momentary, OUTP_EVENT(3, 0x0, 0x0)}, // Turn off LED
  {INIT_PIN, Momentary, OUTP_EVENT(4, 0x0, 0x0)}, // Turn off LED
  // Pedal controls volume for layers 3 (string) and 4 (pad)
  {0, Continuous,  CNTL_EVENT(3, 0x7  /*volume*/, 110, 0)}, // Layer volume
  {0, Continuous,  CNTL_EVENT(4, 0x7  /*volume*/, 120, 0)},
  // Button 1 controls enable/disable of layer 3 via expression controller
  {1, LatchingOff, CNTL_EVENT(3, 0xb  /*expr*/,   127, 0)},
  {1, LatchingOff, OUTP_EVENT(3, 0xff, 0x0)}, // Light LED based on input pin
  // Button 2 does the same for layer 4
  {2, LatchingOff, CNTL_EVENT(4, 0xb  /*expr*/,   127, 0)}, // Layer 4 enable
  {2, LatchingOff, OUTP_EVENT(4, 0xff, 0x0)}, // Light LED based on input pin
  // On exit, restore all volume and expression levels to max
  {EXIT_PIN, Momentary, CNTL_EVENT(2, 0x7  /*volume*/, 127, 127)}, // Vol=max
  {EXIT_PIN, Momentary, CNTL_EVENT(3, 0x7  /*volume*/, 127, 127)},
  {EXIT_PIN, Momentary, CNTL_EVENT(4, 0x7  /*volume*/, 127, 127)},
  {EXIT_PIN, Momentary, CNTL_EVENT(2, 0xb  /*expr*/,   127, 127)}, // Expr=max
  {EXIT_PIN, Momentary, CNTL_EVENT(3, 0xb  /*expr*/,   127, 127)},
  {EXIT_PIN, Momentary, CNTL_EVENT(4, 0xb  /*expr*/,   127, 127)},

  PROGRAM(1, 97, 0x7000), // 4ZonesBJBass
  {INIT_PIN, Momentary, CNTL_EVENT(3, 0xb  /*expr*/, 0, 0)}, // Disable layer 3
  {INIT_PIN, Momentary, CNTL_EVENT(4, 0xb  /*expr*/, 0, 0)}, // Disable layer 4
  {INIT_PIN, Momentary, OUTP_EVENT(3, 0x0, 0x0)}, // Turn off LED
  {INIT_PIN, Momentary, OUTP_EVENT(4, 0x0, 0x0)}, // Turn off LED
  // Pedal controls volume for layers 3 (string) and 4 (pad)
  {0, Continuous,  CNTL_EVENT(3, 0x7  /*volume*/, 110, 0)}, // Layer volume
  {0, Continuous,  CNTL_EVENT(4, 0x7  /*volume*/, 120, 0)},
  // Button 1 controls enable/disable of layer 3 via expression controller
  {1, LatchingOff, CNTL_EVENT(3, 0xb  /*expr*/,   127, 0)},
  {1, LatchingOff, OUTP_EVENT(3, 0xff, 0x0)}, // Light LED based on input pin
  // Button 2 does the same for layer 4
  {2, LatchingOff, CNTL_EVENT(4, 0xb  /*expr*/,   127, 0)}, // Layer 4 enable
  {2, LatchingOff, OUTP_EVENT(4, 0xff, 0x0)}, // Light LED based on input pin
  // On exit, restore all volume and expression levels to max
  {EXIT_PIN, Momentary, CNTL_EVENT(3, 0x7  /*volume*/, 127, 127)},
  {EXIT_PIN, Momentary, CNTL_EVENT(4, 0x7  /*volume*/, 127, 127)},
  {EXIT_PIN, Momentary, CNTL_EVENT(3, 0xb  /*expr*/,   127, 127)},
  {EXIT_PIN, Momentary, CNTL_EVENT(4, 0xb  /*expr*/,   127, 127)},

  PROGRAM(1, 91, 0x7000), // 9Draw Org6
  // Initialize to rotating speakers on "fast"
  {INIT_PIN, Momentary, CNTL_EVENT(1, 0x1  /*modwheel*/, 0, 0)}, // Mod=0
  {INIT_PIN, Momentary, CNTL_EVENT(1, 0x12 /*DSP #3*/, 127, 127)}, // Rot=fast
  {INIT_PIN, Momentary, CNTL_EVENT(2, 0x12 /*DSP #3*/, 127, 127)},
  {INIT_PIN, Momentary, OUTP_EVENT(3, 0xff, 0xff)}, // Turn on LED
  {INIT_PIN, Momentary, OUTP_EVENT(4, 0x0, 0x0)},   // Turn off LED
  // Pedal controls volume via expression
  {0, Continuous,  CNTL_EVENT(1, 0x0b  /*expression*/, 127, 0)}, // Volume
  {0, Continuous,  CNTL_EVENT(2, 0x0b  /*expression*/, 127, 0)},
  {0, Continuous,  CNTL_EVENT(3, 0x0b  /*expression*/, 127, 0)},
  // Switch 1 controls rotating speaker speed
  {1, LatchingOn,  CNTL_EVENT(1, 0x12  /*DSP #3*/, 127, 0)},
  {1, LatchingOn,  CNTL_EVENT(2, 0x12  /*DSP #3*/, 127, 0)},
  {1, LatchingOn,  OUTP_EVENT(3, 0xff, 0x0)}, // Light LED based on input pin
  // Switch 2 controls rotating speaker brake
  {2, LatchingOff, CNTL_EVENT(1, 0x13  /*DSP #4*/, 127, 0)},
  {2, LatchingOff, CNTL_EVENT(2, 0x13  /*DSP #4*/, 127, 0)},
  {2, LatchingOff, OUTP_EVENT(4, 0xff, 0x0)}, // Light LED based on input pin
  // On exit, restore all expression levels to max
  {EXIT_PIN, Momentary, CNTL_EVENT(1, 0xb  /*expr*/,   127, 127)}, // Expr=max
  {EXIT_PIN, Momentary, CNTL_EVENT(2, 0xb  /*expr*/,   127, 127)},
  {EXIT_PIN, Momentary, CNTL_EVENT(3, 0xb  /*expr*/,   127, 127)},

  PROGRAM(1, 93, 0x7000), // Illuminated2
  {INIT_PIN, Momentary, OUTP_EVENT(3, 0x0, 0x0)}, // Turn off LED
  {INIT_PIN, Momentary, OUTP_EVENT(4, 0x0, 0x0)}, // Turn off LED
  // Pedal controls volume for layer 2 (pad)
  {0, Continuous,  CNTL_EVENT(2, 0x7  /*volume*/, 127, 0)}, // Layer volume
  // On exit, restore all volume levels to max
  {EXIT_PIN, Momentary, CNTL_EVENT(2, 0x7  /*volume*/, 127, 127)}, // Vol=max

  PROGRAM(1, 95, 0x7000), // PolySynthBJJ
  // Pedal controls cutoff
  {0, Continuous,  CNTL_EVENT(1, 0x4a /* cutoff */, 127, 0)},
  // Switch 1 toggles portamento time between two values
  {1, LatchingOff, CNTL_EVENT(1, 0x05 /* portamento */, 60, 0)},
  {1, LatchingOff, OUTP_EVENT(3, 0xff, 0x0)}, // Light LED based on input pin
  // Switch 2 controls resonance
  {2, LatchingOff, CNTL_EVENT(1, 0x47 /* resonance */, 127, 0)},
  {2, LatchingOff, OUTP_EVENT(4, 0xff, 0x0)}, // Light LED based on input pin
};

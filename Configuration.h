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

// General configuration constants.  Define or undefine them as needed.

#undef WAIT_FOR_SERIAL // Wait for serial port to connect before running

// Which types of data to log to the serial console

#undef  LOG_PINS    // Input pin values
#define LOG_EVENTS  // Output events
#define LOG_PROGRAM // Program changes
#undef  LOG_MIDIIN  // Other MIDI input
#define LOG_ERRORS  // Configuration errors

// Input pins.  Edit these to reflect the physical board

const Pin Pins[] = {
  // Idx     Type       pin number   min   max   curve
  /* 0 */ {Analog,        A0,        645,  80,   -1.25}, // Direction reversed
  /* 1 */ {DigitalPullup, 11,        0,    1023,  0.0},
  /* 2 */ {DigitalPullup, 12,        0,    1023,  0.0},
  /* 3 */ {DigitalOut,    3,         0,    255,   0.0},
  /* 4 */ {DigitalOut,    4,         0,    255,   0.0},
};


// Output MIDI events.  Edit these according to the output you want

#define CNTL_EVENT(_ch, _n, _on, _off) \
  {.Controller = {ControllerEventType, (_ch), (_n), (_on), (_off)}}

#define PRGM_EVENT(_ch, _p) \
  {.Program = {ProgramEventType, (_ch), (_p), 0, 0}}

#define NOTE_EVENT(_ch, _n, _on, _off) \
  {.Note = {NoteEventType, (_ch), (_n), (_on), (_off)}}

#define OUTP_EVENT(_p, _on, _off) \
  {.Out = {OutEventType, (_p), (_on), (_off)}}

// Header to indicate the start of a new program

#define PROGRAM(_ch, _p) \
  {PROGRAM_PIN, Momentary, PRGM_EVENT(_ch, _p)}

const EventMap DefaultEventList[] = {
  // First event in the table is the default
  // Pin array idx, Handling, Event macro
  PROGRAM(1, 90), // 4ZonesBJJ
  {INIT_PIN, Momentary, CNTL_EVENT(1, 0x1  /*modwheel*/, 0, 0)}, // Mod=0
  {INIT_PIN, Momentary, CNTL_EVENT(2, 0x1  /*modwheel*/, 0, 0)}, // Mod=0
  {INIT_PIN, Momentary, CNTL_EVENT(3, 0x1  /*modwheel*/, 0, 0)}, // Mod=0
  {INIT_PIN, Momentary, CNTL_EVENT(4, 0x1  /*modwheel*/, 0, 0)}, // Mod=0
  {INIT_PIN, Momentary, OUTP_EVENT(3, 0x0, 0x0)}, // Turn off LED
  {INIT_PIN, Momentary, OUTP_EVENT(4, 0x0, 0x0)}, // Turn off LED
  {0, Continuous,  CNTL_EVENT(3, 0x7  /*volume*/, 127, 0)}, // Layer volume
  {0, Continuous,  CNTL_EVENT(4, 0x7  /*volume*/, 127, 0)},
  {1, LatchingOff, CNTL_EVENT(3, 0xb  /*expr*/,   127, 0)}, // Layer 3 enable
  {1, LatchingOff, OUTP_EVENT(3, 0xff, 0x0)}, // Light LED based on input pin
  {2, LatchingOff, CNTL_EVENT(4, 0xb  /*expr*/,   127, 0)}, // Layer 4 enable
  {2, LatchingOff, OUTP_EVENT(4, 0xff, 0x0)}, // Light LED based on input pin
  {EXIT_PIN, Momentary, CNTL_EVENT(3, 0x7  /*volume*/, 127, 127)}, // Vol=max
  {EXIT_PIN, Momentary, CNTL_EVENT(4, 0x7  /*volume*/, 127, 127)},
  {EXIT_PIN, Momentary, CNTL_EVENT(3, 0xb  /*expr*/,   127, 127)}, // Expr=max
  {EXIT_PIN, Momentary, CNTL_EVENT(4, 0xb  /*expr*/,   127, 127)},

  PROGRAM(1, 91), // 9Draw Org6
  {INIT_PIN, Momentary, CNTL_EVENT(1, 0x1  /*modwheel*/, 0, 0)}, // Mod=0
  {INIT_PIN, Momentary, CNTL_EVENT(0, 0x12 /*DSP #3*/, 127, 127)}, // Rot=fast
  {INIT_PIN, Momentary, CNTL_EVENT(1, 0x12 /*DSP #3*/, 127, 127)},
  {INIT_PIN, Momentary, OUTP_EVENT(3, 0xff, 0xff)}, // Turn on LED
  {INIT_PIN, Momentary, OUTP_EVENT(4, 0x0, 0x0)},   // Turn off LED
  {0, Continuous,  CNTL_EVENT(0, 0x0b  /*expression*/, 127, 0)}, // Volume
  {0, Continuous,  CNTL_EVENT(1, 0x0b  /*expression*/, 127, 0)},
  {0, Continuous,  CNTL_EVENT(2, 0x0b  /*expression*/, 127, 0)},
  {1, LatchingOn,  CNTL_EVENT(0, 0x12  /*DSP #3*/, 127, 0)},   // Rot. speed
  {1, LatchingOn,  CNTL_EVENT(1, 0x12  /*DSP #3*/, 127, 0)},
  {1, LatchingOn,  OUTP_EVENT(3, 0xff, 0x0)}, // Light LED based on input pin
  {2, LatchingOff, CNTL_EVENT(0, 0x13  /*DSP #4*/, 127, 0)},   // Rot on/off
  {2, LatchingOff, CNTL_EVENT(1, 0x13  /*DSP #4*/, 127, 0)},
  {2, LatchingOff, OUTP_EVENT(4, 0xff, 0x0)}, // Light LED based on input pin
  {EXIT_PIN, Momentary, CNTL_EVENT(0, 0xb  /*expr*/,   127, 127)}, // Expr=max
  {EXIT_PIN, Momentary, CNTL_EVENT(1, 0xb  /*expr*/,   127, 127)},
  {EXIT_PIN, Momentary, CNTL_EVENT(2, 0xb  /*expr*/,   127, 127)},
};

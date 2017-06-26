// Configuration for MIDI controller
// by Brian J. Johnson  5/21/2017

// Input pins.  Edit these to reflect the physical board

Pin Pins[] = {
  // Idx     Type       pin number   min   max
  /* 0 */ {Analog,        0,         133,  680},
  /* 1 */ {DigitalPullup, 12,        0,    1023},
  /* 2 */ {DigitalPullup, 11,        0,    1023},
  /* 3 */ {AnalogOut,     22,        0,    255},
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

EventMap DefaultEventList[] = {
  // Pin array idx, Handling, Event macro
  PROGRAM(1, 90), // 4ZonesBJJ
  {INIT_PIN, Momentary, CNTL_EVENT(0, 0x1  /*modwheel*/, 0, 0)}, // Mod=0
  {INIT_PIN, Momentary, CNTL_EVENT(1, 0x1  /*modwheel*/, 0, 0)}, // Mod=0
  {INIT_PIN, Momentary, CNTL_EVENT(2, 0x1  /*modwheel*/, 0, 0)}, // Mod=0
  {INIT_PIN, Momentary, CNTL_EVENT(3, 0x1  /*modwheel*/, 0, 0)}, // Mod=0
  {0, Continuous, CNTL_EVENT(2, 0x7  /*volume*/, 127, 0)}, // Layer volume
  {0, Continuous, CNTL_EVENT(3, 0x7  /*volume*/, 127, 0)},
  {1, Latching,   CNTL_EVENT(2, 0xb  /*expr*/,   127, 0)}, // Layer enable
  {2, Latching,   CNTL_EVENT(3, 0xb  /*expr*/,   127, 0)}, // Layer enable
  {1, Latching,   OUTP_EVENT(3, 0xff, 0x0)}, // Light LED based on input pin

  PROGRAM(1, 91), // 9Draw Org6 -- FIXME
  {INIT_PIN, Momentary, CNTL_EVENT(1, 0x1  /*modwheel*/, 0, 0)}, // Mod=0
  {0, Continuous, CNTL_EVENT(0, 0x0b  /*expression*/, 127, 0)}, // Volume
  {0, Continuous, CNTL_EVENT(1, 0x0b  /*expression*/, 127, 0)},
  {1, Latching,   CNTL_EVENT(0, 0x1   /*modwheel*/, 127, 0)},   // Rot. Spkr
  {1, Latching,   CNTL_EVENT(1, 0x1   /*modwheel*/, 127, 0)},
  // Something for pin 2?
};

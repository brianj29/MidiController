// Configuration for MIDI controller
// by Brian J. Johnson  4/17/2017

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

Controller Controllers[] = {
  // Pin array idx, Handling, Event macro
  {0, Continuous, CNTL_EVENT(1, 0x1  /*modwheel*/, 127, 0)},
  {1, Momentary,  CNTL_EVENT(1, 0x40 /*damper*/,   127, 0)},
  {1, Momentary,  NOTE_EVENT(1, 0x20 /*a note*/,   96,  0)},
  {2, Latching,   PRGM_EVENT(1, 0x34 /*ptch 3/4*/)},
  {2, Latching,   OUTP_EVENT(3, 0xff, 0x0)}, // Light LED based on input pin
};

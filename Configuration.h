// Configuration for MIDI controller
// by Brian J. Johnson  4/17/2017

// Input pins.  Edit these to reflect the physical board

Pin Pins[] = {
  // Type,      pin number   min   max   handling
  {Analog,        0,         133,  680,  Continuous},
  {DigitalPullup, 12,        0,    1023, Momentary},
  {DigitalPullup, 11,        0,    1023, Latching}
};


// Output MIDI events.  Edit these according to the output you want

#define CTL_EVENT(_ch, _n, _on, _off) \
  {.Controller = {ControllerEventType, (_ch), (_n), (_on), (_off)}}

Controller Controllers[] = {
  // Pin array idx, Event macro
  {0, CTL_EVENT(1, 0x1  /*modwheel*/, 127, 0)},
  {1, CTL_EVENT(1, 0x40 /*damper*/,   127, 0)},
  {1, CTL_EVENT(1, 0x41 /*?*/,        125, 4)},
  {2, CTL_EVENT(1, 0x4  /*foot ctl*/, 127, 0)},
};

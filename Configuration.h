// Configuration for MIDI controller
// by Brian J. Johnson  8/21/2016

// Input pins.  Edit these to reflect the physical board

Pin Pins[] = {
  // Type,      pin number   min   max
  {Analog,        0,         133,  680},
  {DigitalPullup, 12,        0,    1023},
  {DigitalPullup, 11,        0,    1023}
};


// Output MIDI controllers

Controller Controllers[] = {
  // Type, Pin array idx, channel, controller
  {Continuous, 0, 1, 1 /* modwheel */},
  {Momentary,  1, 1, 0x40 /* damper */},
  {Latching,   2, 1, 0x4 /* foot cntlr */}
};


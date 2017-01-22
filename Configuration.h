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

ControllerEvent ModWheelMinEvent[] = {
  // Event type, controller, value, channel
  {ControllerEventType, 0x1 /*modwheel*/, 0, 1}
};

ControllerEvent ModWheelMaxEvent[] = {
  // Event type, controller, value, channel
  {ControllerEventType, 0x1 /*modwheel*/, 127, 1}
};

ControllerEvent DamperOnEvent[] = {
  // Event type, controller, value, channel
  {ControllerEventType, 0x40 /*damper*/, 127, 1},
  {ControllerEventType, 0x40 /*damper*/, 125, 1}
};

ControllerEvent DamperOffEvent[] = {
  // Event type, controller, value, channel
  {ControllerEventType, 0x40 /*damper*/, 0, 1},
  {ControllerEventType, 0x40 /*damper*/, 4, 1}
};

ControllerEvent FootOnEvent[] = {
  // Event type, controller, value, channel
  {ControllerEventType, 0x4 /*foot cntlr*/, 127, 1}
};

ControllerEvent FootOffEvent[] = {
  // Event type, controller, value, channel
  {ControllerEventType, 0x4 /*foot cntlr*/, 0, 1}
};

Controller Controllers[] = {
#if 0 // Old format
  // Type, Pin array idx, channel, controller
  {Continuous, 0, 1, 1 /* modwheel */},
  {Momentary,  1, 1, 0x40 /* damper */},
  {Latching,   2, 1, 0x4 /* foot cntlr */}
#else
  // Type, Pin array idx, "on" events, "off" events
  {Continuous, 0, {1, ModWheelMinEvent}, {1, ModWheelMaxEvent}},
  {Momentary,  1, {2, DamperOffEvent}, {2, DamperOnEvent}},
  {Latching,   2, {1, FootOffEvent}, {1, FootOnEvent}},
#endif
};


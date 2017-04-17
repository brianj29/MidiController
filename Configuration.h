// Configuration for MIDI controller
// by Brian J. Johnson  8/21/2016

// Input pins.  Edit these to reflect the physical board

Pin Pins[] = {
  // Type,      pin number   min   max   handling
  {Analog,        0,         133,  680,  Continuous},
  {DigitalPullup, 12,        0,    1023, Momentary},
  {DigitalPullup, 11,        0,    1023, Latching}
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
  {ControllerEventType, 0x41 /*?*/, 125, 1}
};

ControllerEvent DamperOffEvent[] = {
  // Event type, controller, value, channel
  {ControllerEventType, 0x40 /*damper*/, 0, 1},
  {ControllerEventType, 0x41 /*?*/, 4, 1}
};

ControllerEvent FootOnEvent[] = {
  // Event type, controller, value, channel
  {ControllerEventType, 0x4 /*foot cntlr*/, 127, 1}
};

ControllerEvent FootOffEvent[] = {
  // Event type, controller, value, channel
  {ControllerEventType, 0x4 /*foot cntlr*/, 0, 1}
};

#define EVENTS(_x) sizeof(_x) / sizeof(_x[0]), (Event *)(_x)

#define CTL_EVENT(_ch, _n, _on, _off) \
  {.Controller = {ControllerEventType, (_ch), (_n), (_on), (_off)}}

Controller Controllers[] = {
#if 0 // Old format
  // Type, Pin array idx, channel, controller
  {Continuous, 0, 1, 1 /* modwheel */},
  {Momentary,  1, 1, 0x40 /* damper */},
  {Latching,   2, 1, 0x4 /* foot cntlr */}
#elif 0 // 2nd old format
  // Type, Pin array idx, "on" event count, "on" events, "off" event count, "off" events
  {Continuous, 0, EVENTS(ModWheelMinEvent), EVENTS(ModWheelMaxEvent)},
  {Momentary,  1, EVENTS(DamperOnEvent), EVENTS(DamperOffEvent)},
  {Latching,   2, EVENTS(FootOnEvent), EVENTS(FootOffEvent)},
#else // New format
  // Pin array idx, Event
  {0, CTL_EVENT(1, 0x1  /*modwheel*/, 127, 0)},
  {1, CTL_EVENT(1, 0x40 /*damper*/,   127, 0)},
  {1, CTL_EVENT(1, 0x41 /*?*/,        125, 4)},
  {2, CTL_EVENT(1, 0x4  /*foot ctl*/, 127, 0)},
#endif
};

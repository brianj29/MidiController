// Data structures for MIDI controller
// by Brian J. Johnson  8/21/2016

// Input pins

typedef enum _pinType {
  Analog,
  Digital,
  DigitalPullup
} PinType;

typedef struct _pin {
  PinType Type;
  int     Num;  // Hardware pin number
  int     Min;  // Min/max values, for calibration
  int     Max;
} Pin;

extern Pin Pins[];


// Output controllers

typedef enum _controllerType {
  Momentary,
  Latching,
  Continuous
} ControllerType;

typedef enum _eventType {
  Note       = 0,
  Controller = 1,
  Program    = 2,
  // FIXME:  add others?
} EventType;

typedef struct _GenericEvent {
  EventType Type;
  uint8_t   Data1;
  uint8_t   Data2;
  uint8_t   Data3;
} GenericEvent;

typedef struct _NoteEvent {
  EventType Type;
  uint8_t   Note;
  uint8_t   Velocity;
  uint8_t   Channel;
} NoteEvent;

typedef struct _ControllerEvent {
  EventType Type;
  uint8_t   Controller;
  uint8_t   Value;
  uint8_t   Channel;
} ControllerEvent;

typedef struct _ProgramEvent {
  EventType Type;
  uint8_t   Program;
  uint8_t   unused;
  uint8_t   Channel;
} ProgramEvent;

typedef struct _events {
  int  Length; // in events
  union _event {
    GenericEvent,
    NoteEvent,
    ControllerEvent,
    ProgramEvent,
  } *Event;
} Events;

typedef struct _controller {
  ControllerType  Type;
  int     Pin;  // Input Pins[] array index
  Events  On;   // Events sent when switch turns "on"
  Events  Off;  // Events sent when switch turns "off"
} Controller;


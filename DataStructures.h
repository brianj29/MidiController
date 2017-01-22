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
  NoteEventType       = 0,
  ControllerEventType = 1,
  ProgramEventType    = 2,
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
    void *VoidPtr;
    GenericEvent *Generic;
    NoteEvent *Note;
    ControllerEvent *Controller;
    ProgramEvent *Program;
  } Event[1];
} Events;

typedef struct _controller {
  ControllerType  Type;
  int     Pin;  // Input Pins[] array index
#if 0 // old format
  int     Channel; // MIDI channel
  int     Controller;  // MIDI controller number
#else
  Events  Off;  // Events sent when switch turns "off"
  Events  On;   // Events sent when switch turns "on"
#endif
} Controller;


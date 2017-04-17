// Data structures for MIDI controller
// by Brian J. Johnson  4/17/2017

// Input pins

typedef enum _pinType {
  Analog,
  Digital,
  DigitalPullup
} PinType;

typedef enum _pinHandling {
  Momentary,
  Latching,
  Continuous
} PinHandling;

typedef struct _pin {
  PinType     Type;
  int         Num;  // Hardware pin number
  int         Min;  // Min/max values, for calibration
  int         Max;
  PinHandling Handling;
} Pin;

extern Pin Pins[];


// Output controllers

typedef enum _eventType {
  NoteEventType       = 0,
  ControllerEventType = 1,
  ProgramEventType    = 2,
  // FIXME:  add others?  LED output, 
} EventType;

typedef struct _GenericEvent {
  EventType Type;
  uint8_t   Data1;
  uint8_t   Data2;
  uint8_t   Data3;
} GenericEvent;

typedef struct _NoteEvent {
  EventType Type;
  uint8_t   Channel;
  uint8_t   Note;
  uint8_t   OnVelocity;
  uint8_t   OffVelocity; // Useful?
} NoteEvent;

typedef struct _ControllerEvent {
  EventType Type;
  uint8_t   Channel;
  uint8_t   Controller;
  uint8_t   OnValue;
  uint8_t   OffValue;
} ControllerEvent;

typedef struct _ProgramEvent {
  EventType Type;
  uint8_t   Channel;
  uint8_t   Program;
  uint8_t   unused1;
  uint8_t   unused2;
} ProgramEvent;

typedef union _event {
  GenericEvent    Generic;
  NoteEvent       Note;
  ControllerEvent Controller;
  ProgramEvent    Program;
} Event;

typedef struct _controller {
  int     Pin;  // Input Pins[] array index
  Event   Evt;  // Output to generate when this pin changes state
} Controller;



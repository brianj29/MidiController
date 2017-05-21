// Data structures for MIDI controller
// by Brian J. Johnson  5/21/2017

// Input pins

typedef enum _pinType {
  Analog,
  Digital,
  DigitalPullup,
  AnalogOut
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
} Pin;

// Fake pin number to trigger at initialization time
#define INIT_PIN 255

extern Pin Pins[];


// Output controllers

typedef enum _eventType {
  NoteEventType       = 0,
  ControllerEventType = 1,
  ProgramEventType    = 2,
  OutEventType        = 3,
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

typedef struct _OutEvent {
  EventType Type;
  uint8_t   OutPin;
  uint8_t   OnValue;
  uint8_t   OffValue;
  uint8_t   unused1;
} OutEvent;

typedef union _event {
  GenericEvent    Generic;
  NoteEvent       Note;
  ControllerEvent Controller;
  ProgramEvent    Program;
  OutEvent        Out;
} Event;

typedef struct _eventMap {
  int         Pin;  // Input Pins[] array index
  PinHandling Handling;
  Event       Evt;  // Output to generate when this pin changes state
} EventMap;



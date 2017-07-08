// Data structures for MIDI controller
// by Brian J. Johnson 2017

// Input pins

typedef enum _pinType {
  Analog,
  Digital,
  DigitalPullup,
  AnalogOut,
  DigitalOut
} PinType;

typedef enum _pinHandling {
  Momentary,   // On with button press, off with release
  LatchingOff, // On with button press, off with next press
  LatchingOn,  // Off with button press, on with next press
  Continuous   // Output value scales with input value
} PinHandling;

typedef struct _pin {
  PinType     Type;
  int         Num;  // Hardware pin number
  int         Min;  // Min/max values, for calibration
  int         Max;
} Pin;

// Fake pin number to trigger at initialization time
#define INIT_PIN 255

// Fake pin number to indicate the start of a program
#define PROGRAM_PIN 254

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



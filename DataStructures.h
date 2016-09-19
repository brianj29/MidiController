// Data structures for MIDI controller
// by Brian J. Johnson  8/21/2016

// Input pins

typedef enum _pinType {
  Analog,
  Digital,
  DigitalPullup
} PinType;

typedef enum _pinUse {
  Momentary,
  Latching,
  Continuous
} ControllerType;

typedef struct _pin {
  PinType Type;
  int     Num;  // Hardware pin number
  int     Min;  // Min/max values, for calibration
  int     Max;
} Pin;

extern Pin Pins[];


// Output controllers

typedef struct _controller {
  ControllerType  Type;
  int     Pin;  // Input Pins array index
} Controller;


#include <MIDI.h>

#include <Bounce2.h>

// Midi controller
// by Brian J. Johnson  4/17/2017

#include "DataStructures.h"
#include "Configuration.h"

// Global state

#define PIN_COUNT (sizeof(Pins) / sizeof(Pins[0]))
Bounce PinBounce[PIN_COUNT]; // Debouncer state
int    PinState[PIN_COUNT];  // Previous value, indexed by pin
int    NewState[PIN_COUNT];  // Current value, indexed by pin

unsigned NumControllers = sizeof(Controllers) / sizeof(Controllers[0]);
 // Last value calculated for controller
uint8_t  LastValue[sizeof(Controllers) / sizeof(Controllers[0])];

void setup() {
  Serial.begin(38400);  // For debugging
#if 0
  while (!Serial) { }   // Wait for USB serial to connect
#endif

  // Turn on the LED, so we can see the board is on

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Initialize MIDI
  
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
  
  // Configure the i/o pins

  for (unsigned i = 0; i < PIN_COUNT; i++) {
    Pin *p = &Pins[i];
    switch (p->Type) {
      case Analog:
      case AnalogOut:
        // No setup needed
        break;
      case Digital:
      case DigitalPullup:
        pinMode(p->Num, (p->Type == Digital) ? INPUT : INPUT_PULLUP);
        PinBounce[i] = Bounce();
        PinBounce[i].attach(p->Num);
        PinBounce[i].interval(5);
        break;
      default:
        Serial.print("Bad pin type in Pins entry ");
        Serial.println(i);
    }
    PinState[i] = 0;
  }

  // Initialize state

  for (unsigned i = 0; i < NumControllers; i++) {
    LastValue[i] = 0;
  }
}

void loop() {
  unsigned pinNum;

  // Consume any incoming MIDI data, to keep from hanging the host

  while (usbMIDI.read()) {
    // Do nothing
  }
  while (MIDI.read()) {
    // Do nothing
  }

  // Poll the input pins

  for (pinNum = 0; pinNum < PIN_COUNT; pinNum++) {
    Pin *p = &Pins[pinNum];
    int val;     // Value read, 0-1023
    int state;   // Output state of the pin

    // Poll the input pin, convert to range 0-1023

    switch (p->Type) {
    case Analog:
      // Scale the input value to find the pin state
      val = analogRead(p->Num);
      state = val;
      if (state < p->Min) {
        state = p->Min;
      }
      else if (state > p->Max) {
        state = p->Max;
      }
      state = 1023 * (state - p->Min) / (p->Max - p->Min);
      break;
    case Digital:
    case DigitalPullup:
      // Simply convert the input level to a pin state
      PinBounce[pinNum].update();
      val = PinBounce[pinNum].read() * 1023;
      state = 1023 - val; // Active low
      break;
    default:
      val = 0;
      state = 0;
      break;
    }

    // Save the pin state for the controller loop below
    NewState[pinNum] = state;
    if (NewState[pinNum] != PinState[pinNum]) {
        Serial.println(String("Pin ") + p->Num + ":" + val + ";" + state);
    }
  } // for (pinNum)

  // Convert pin state changes to output events

  for (unsigned i = 0; i < NumControllers; i++) {
    Controller      *c = &Controllers[i];
    GenericEvent    *genEvt = &c->Evt.Generic;
    ControllerEvent *ctlEvt;
    NoteEvent       *noteEvt;
    ProgramEvent    *pgmEvt;
    OutEvent        *outEvt;
    int             changed;
    int             state;
    uint8_t         value;

    // Detect changes, for use in the event type handlers below

    pinNum = c->Pin;
    switch (c->Handling) {
    case Continuous:
    case Momentary:
      state   = NewState[pinNum];
      changed = (NewState[pinNum] != PinState[pinNum]);
      break;

    case Latching:
      // Fetch current latch state based on last value sent
      if (LastValue[i] < 64) {
        state = 0;
      }
      else {
        state = 1023;
      }

      // Toggle state on button press
      if (PinState[pinNum] < 512 && NewState[pinNum] >= 512) {
        state = 1023 - state;
        changed = 1;
      }
      else {
        changed = 0;
      }
      break;

    default:
      state = 0;
      changed = 0;
      break;
    }

    // Don't generate any events if pin state hasn't changed

    if (changed == 0) {
      continue;
    }

    // Process the Controllers[] array entry

    //Serial.println("  Evt " + String(i) + " t" + genEvt->Type + ": ");
    switch (genEvt->Type) {
    case NoteEventType:
      // Send note on for off->on transition, note off for on->off transition
      noteEvt = &c->Evt.Note;
      if (state > 512) {
        Serial.println(String("On  ") + noteEvt->Channel + "/" + noteEvt->Note + ": " + noteEvt->OnVelocity);
        usbMIDI.sendNoteOn(noteEvt->Note, noteEvt->OnVelocity, noteEvt->Channel);
        MIDI.sendNoteOn(noteEvt->Note, noteEvt->OnVelocity, noteEvt->Channel);
      }
      else {
        Serial.println(String("Off ") + noteEvt->Channel + "/" + noteEvt->Note + ": " + noteEvt->OffVelocity);
        usbMIDI.sendNoteOn(noteEvt->Note, noteEvt->OffVelocity, noteEvt->Channel);
        MIDI.sendNoteOn(noteEvt->Note, noteEvt->OffVelocity, noteEvt->Channel);
      }
      value = state / 8; // Scale to 0-127 range
      break;

    case ControllerEventType:
      // Scale controller value between OffValue and OnValue
      ctlEvt = &c->Evt.Controller;
      value =
        ctlEvt->OffValue +
        (state * (ctlEvt->OnValue - ctlEvt->OffValue) / 1023);
      if (value != LastValue[i]) {
        Serial.println(String("Ctl ") + ctlEvt->Channel + "/" + ctlEvt->Controller + ": " + value);
        usbMIDI.sendControlChange(ctlEvt->Controller, value, ctlEvt->Channel);
        MIDI.sendControlChange(ctlEvt->Controller, value, ctlEvt->Channel);
      }
      break;

    case ProgramEventType:
      // Send program change on button off->on transition
      if (state > 512) {
        pgmEvt = &c->Evt.Program;
        Serial.println(String("Pgm ") + pgmEvt->Channel + "/" + pgmEvt->Program);
        usbMIDI.sendProgramChange(pgmEvt->Program, pgmEvt->Channel);
        MIDI.sendProgramChange(pgmEvt->Program, pgmEvt->Channel);
      }
      else {
        Serial.println("<none>");
      }
      value = state / 8; // Scale to 0-127 range
      break;

    case OutEventType:
      // Scale the LED/PWM output between OffValue and OnValue
      outEvt = &c->Evt.Out;
      value =
        outEvt->OffValue +
        (state * (outEvt->OnValue - outEvt->OffValue) / 1023);
      if (value != LastValue[i]) {
        Serial.println(String("Out ") + outEvt->OutPin + ": " + value);
        analogWrite(Pins[outEvt->OutPin].Num, value);
      }
      break;

    default:
      value = 0;
      break;
    }

    // Remember the last value generated

    LastValue[i] = value;
  }
  
  // Save pin states for the next loop iteration

  for (pinNum = 0; pinNum < PIN_COUNT; pinNum++) {
    PinState[pinNum] = NewState[pinNum];
  }

  // Send any queued USB MIDI data
  usbMIDI.send_now();

  delay(100); // FIXME:  use rate-limiting on MIDI interface
}

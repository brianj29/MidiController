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

void setup() {
  Serial.begin(38400);  // For debugging
  //while (!Serial) { }   // Wait for USB serial to connect

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
    int state;   // Output state of the controller
    int changed; // Whether or not a digital input changed state

    // Poll the input pin, convert to range 0-1023

    changed = 0;
    switch (p->Type) {
    case Analog:
      val = analogRead(p->Num);
      break;
    case Digital:
    case DigitalPullup:
      changed = PinBounce[pinNum].update();
      val = PinBounce[pinNum].read() * 1023;
      break;
    default:
      val = 0;
      break;
    }

    // Process/normalize the value to 0-1023 range

    switch (p->Handling) {
    case Momentary:
      // Simply convert the input level to a pin state
      state = 1023 - val; // Active low
      break;
    case Latching:
      // Toggle state on deasserted->asserted transition (button release)
      if (changed && val == 0) {
        state = 1023 - PinState[pinNum];
      }
      else {
        state = PinState[pinNum];
      }
      break;
    case Continuous:
      // Scale the input value to find the pin state
      state = val;
      if (state < p->Min) {
        state = p->Min;
      }
      else if (state > p->Max) {
        state = p->Max;
      }
      state = 1023 * (state - p->Min) / (p->Max - p->Min);
      break;
    default:
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
    uint8_t         value;

    // Don't generate new events if pin state hasn't changed
    pinNum = c->Pin;
    if (NewState[pinNum] == PinState[pinNum]) {
      continue;
    }

    Serial.print("  Evt " + String(i) + " t" + genEvt->Type + ": ");
    switch (genEvt->Type) {
    case NoteEventType:
      // Send note on for off->on transition, note off for on->off transition
      noteEvt = &c->Evt.Note;
      if (PinState[pinNum] < 512 && NewState[pinNum] > 512) {
        Serial.println(String("On  ") + noteEvt->Channel + "/" + noteEvt->Note + ": " + noteEvt->OnVelocity);
        usbMIDI.sendNoteOn(noteEvt->Note, noteEvt->OnVelocity, noteEvt->Channel);
        MIDI.sendNoteOn(noteEvt->Note, noteEvt->OnVelocity, noteEvt->Channel);
      }
      else {
        Serial.println(String("Off ") + noteEvt->Channel + "/" + noteEvt->Note + ": " + noteEvt->OffVelocity);
        usbMIDI.sendNoteOn(noteEvt->Note, noteEvt->OffVelocity, noteEvt->Channel);
        MIDI.sendNoteOn(noteEvt->Note, noteEvt->OffVelocity, noteEvt->Channel);
      }
      break;

    case ControllerEventType:
      // Scale controller value between OffValue and OnValue
      ctlEvt = &c->Evt.Controller;
      value =
        ctlEvt->OffValue +
        (NewState[pinNum] * (ctlEvt->OnValue - ctlEvt->OffValue) / 1023);
      Serial.println(String("Ctl ") + ctlEvt->Channel + "/" + ctlEvt->Controller + ": " + value);
      usbMIDI.sendControlChange(ctlEvt->Controller, value, ctlEvt->Channel);
      MIDI.sendControlChange(ctlEvt->Controller, value, ctlEvt->Channel);
      break;

    case ProgramEventType:
      // Send program change on button off->on transition
      if (PinState[pinNum] < 512 && NewState[pinNum] > 512) {
        pgmEvt = &c->Evt.Program;
        Serial.println(String("Pgm ") + pgmEvt->Channel + "/" + pgmEvt->Program);
        usbMIDI.sendProgramChange(pgmEvt->Program, pgmEvt->Channel);
        MIDI.sendProgramChange(pgmEvt->Program, pgmEvt->Channel);
      }
      else {
        Serial.println("<none>");
      }
      break;

    case OutEventType:
      // Scale the LED/PWM output between OffValue and OnValue
      outEvt = &c->Evt.Out;
      value =
        outEvt->OffValue +
        (NewState[pinNum] * (outEvt->OnValue - outEvt->OffValue) / 1023);
      Serial.println(String("Out ") + outEvt->OutPin + ": " + value);
      analogWrite(Pins[outEvt->OutPin].Num, value);
      break;
    }
  }
  
  // Save pin states for the next loop iteration

  for (pinNum = 0; pinNum < PIN_COUNT; pinNum++) {
    PinState[pinNum] = NewState[pinNum];
  }

  // Send any queued USB MIDI data
  usbMIDI.send_now();

  delay(100); // FIXME:  use rate-limiting on MIDI interface
}

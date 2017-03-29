#include <MIDI.h>

#include <Bounce2.h>

// Midi controller
// by Brian J. Johnson  8/21/2016

#include "DataStructures.h"
#include "Configuration.h"

// Global state

#define PIN_COUNT (sizeof(Pins) / sizeof(Pins[0]))
int    PinVal[PIN_COUNT];         // Last value read, indexed by pin
Bounce PinBounce[PIN_COUNT];      // Debouncer state
int    OutputState[] = {0, 0, 0}; // Last value written, idx by controller

unsigned NumControllers = sizeof(Controllers) / sizeof(Controllers[0]);

void setup() {
  Serial.begin(38400);  // For debugging
  while (!Serial) { }   // Wait for USB serial to connect

  // Turn on the LED, so we can see the board is on

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Initialize MIDI
  
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
  
  // Configure the input pins

  for (unsigned i = 0; i < PIN_COUNT; i++) {
    Pin *p = &Pins[i];
    switch (p->Type) {
      case Analog:
        // No setup needed
        break;
      case Digital:
      case DigitalPullup:
        pinMode(p->Num, (p->Type == Digital) ? INPUT : INPUT_PULLUP);
        PinVal[i] = 0;
        PinBounce[i] = Bounce();
        PinBounce[i].attach(p->Num);
        PinBounce[i].interval(5);
        break;
      default:
        Serial.print("Bad pin type in Pins entry ");
        Serial.println(i);
    }
  }
}

void loop() {

  // Consume any incoming MIDI data, to keep from hanging the host

  while (usbMIDI.read()) {
    // Do nothing
  }
  while (MIDI.read()) {
    // Do nothing
  }

  // Process each controller

  for (unsigned i = 0; i < NumControllers; i++) {
    Controller *c = &Controllers[i];
    int PinNum = c->Pin;
    Pin *p = &Pins[PinNum];
    int val;   // Value read, 0-1023
    int state; // Output state of the controller

    // Poll the input pin, convert to range 0-1023

    switch (p->Type) {
    case Analog:
      val = analogRead(p->Num);
      break;
    case Digital:
    case DigitalPullup:
      PinBounce[PinNum].update();
      val = PinBounce[PinNum].read() * 1023;
      break;
    default:
      val = 0;
      break;
    }

    // Process changes in input pins

    if (PinVal[PinNum] != val) {
      state = OutputState[i];
      switch (c->Type) {
      case Momentary:
        // Simply convert the input level to an output state
        state = 1023 - val; // Active low
        break;
      case Latching:
       // Switch output state on deasserted->asserted transition
        if (PinVal[i] == 1023 && val == 0) {
          state = 1023 - state;
        }
        break;
      case Continuous:
        // Scale the input value to the output
        state = val;
        if (state < p->Min) {
          state = p->Min;
        }
        else if (state > p->Max) {
          state = p->Max;
        }
        state = 1023 * (state - p->Min) / (p->Max - p->Min);
        break;
      }
      PinVal[PinNum] = val; // Save for next time through the loop

      // Write the output state
      if (OutputState[i] != state) {
        Event *eventList;
        int   eventListLen;
        
        OutputState[i] = state;
        Serial.print(String("Pin ") + p->Num + ":" + val + "  ");

        if (c->Type == Continuous || state < 512) {
          eventList = c->Off;
          eventListLen = c->OffLen;
        }
        else {
          eventList = c->On;
          eventListLen = c->OnLen;
        }

        // Send all events in the event list
        for (int j = 0; j < eventListLen; j++) {
          ControllerEvent *evt;

          Serial.println("Evt " + String(j) + "/" + eventListLen + ": " + eventList[j].Generic.Type);
          switch (eventList[j].Generic.Type) {
          case NoteEventType:
            break;
          case ControllerEventType:
            evt = &eventList[j].Controller;
            Serial.println(String("Ctl ") + evt->Channel + "/" + evt->Controller + ": " + evt->Value);
            usbMIDI.sendControlChange(evt->Controller, evt->Value, evt->Channel);
            MIDI.sendControlChange(evt->Controller, evt->Value, evt->Channel);
            break;
          case ProgramEventType:
            break;
          }
        }
      }
    }
  }

  // Send any queued USB MIDI data
  usbMIDI.send_now();

  delay(100); // FIXME:  use rate-limiting on MIDI interface
}

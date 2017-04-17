#include <MIDI.h>

#include <Bounce2.h>

// Midi controller
// by Brian J. Johnson  8/21/2016

#include "DataStructures.h"
#include "Configuration.h"

// Global state

#define PIN_COUNT (sizeof(Pins) / sizeof(Pins[0]))
//int    PinVal[PIN_COUNT];         // Last value read, indexed by pin
Bounce PinBounce[PIN_COUNT]; // Debouncer state
int    PinState[PIN_COUNT];  // Previous value, indexed by pin
int    NewState[PIN_COUNT];  // Current value, indexed by pin

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
        //        PinVal[i] = 0;
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
    Controller *c = &Controllers[i];
    GenericEvent *genEvt = &c->Evt.Generic;
    ControllerEvent *ctlEvt;
    uint8_t value;

    // Don't generate new events if pin state hasn't changed
    pinNum = c->Pin;
    if (NewState[pinNum] == PinState[pinNum]) {
      continue;
    }

    Serial.print("  Evt " + String(i) + " t" + genEvt->Type + ": ");
    switch (genEvt->Type) {
    case NoteEventType:
      break;

    case ControllerEventType:
      ctlEvt = &c->Evt.Controller;
      value =
        ctlEvt->OffValue +
        (NewState[pinNum] * (ctlEvt->OnValue - ctlEvt->OffValue) / 1023);
      Serial.println(String("Ctl ") + ctlEvt->Channel + "/" + ctlEvt->Controller + ": " + value);
      usbMIDI.sendControlChange(ctlEvt->Controller, value, ctlEvt->Channel);
      MIDI.sendControlChange(ctlEvt->Controller, value, ctlEvt->Channel);
      break;

    case ProgramEventType:
      break;
    }
  }
  
  // Save pin states for the next loop iteration
  for (pinNum = 0; pinNum < PIN_COUNT; pinNum++) {
    PinState[pinNum] = NewState[pinNum];
  }

#if 0// bjj test hack
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
      state = PinState[i];
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
      if (PinState[i] != state) {
        int   eventListLen;
        
        PinState[i] = state;
        Serial.print(String("Pin ") + p->Num + ":" + val + ";" + state + "  ");

        if (c->Type == Continuous) {
          Event *eventList1;
          Event *eventList2;
          
          // Interpolate between c->Off and c->On values
          eventList1 = c->On;
          eventList2 = c->Off;
          eventListLen = min(c->OffLen, c->OnLen);
          for (int j = 0; j < eventListLen; j++) {
            ControllerEvent *evt1;
            ControllerEvent *evt2;
            uint8_t value;

            Serial.println("Evt " + String(j) + "/" + eventListLen + ": " + eventList1[j].Generic.Type);
            switch (eventList1[j].Generic.Type) {
            case NoteEventType:
              break;
            case ControllerEventType:
              evt1 = &eventList1[j].Controller;
              evt2 = &eventList2[j].Controller;
              value = evt1->Value + (state * (evt2->Value - evt1->Value) / 1023);
              Serial.println(String("Ctl ") + evt1->Channel + "/" + evt1->Controller + ": " + value);
              usbMIDI.sendControlChange(evt1->Controller, value, evt1->Channel);
              MIDI.sendControlChange(evt1->Controller, value, evt1->Channel);
              break;
            case ProgramEventType:
              break;
            }
          }
        }
        else { // Latching or Momentary
          Event *eventList;

          if (state < 512) {
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
        } // c->Type
      } // PinState[i]
    }
  }
#endif // bjj

  // Send any queued USB MIDI data
  usbMIDI.send_now();

  delay(100); // FIXME:  use rate-limiting on MIDI interface
}

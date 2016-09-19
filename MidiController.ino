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

  // Turn on the LED, so we can see the board is on

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

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
        OutputState[i] = state;
        Serial.print(String("Pin ") + p->Num + ":" + val + "  ");
        Serial.println(String("Ctl ") + i + ":" + state / 8); // scale for MIDI
      }
    }
  }

  delay(100); // FIXME:  use rate-limiting on MIDI interface
}

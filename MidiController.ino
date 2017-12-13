#include <MIDI.h>
#ifndef CORE_TEENSY // Leonardo/Pro Micro USB MIDI library
#include <midi_UsbTransport.h>

static const unsigned sUsbTransportBufferSize = 16;
typedef midi::UsbTransport<sUsbTransportBufferSize> UsbTransport;

UsbTransport sUsbTransport;

MIDI_CREATE_INSTANCE(UsbTransport, sUsbTransport, usbMIDI); // USB
MIDI_CREATE_DEFAULT_INSTANCE(); // Serial
#endif // CORE_TEENSY

#include <Bounce2.h>

// Midi controller
// by Brian J. Johnson 2017

#include "DataStructures.h"
#include "Configuration.h"

extern float fscale(float originalMin, float originalMax,
                    float newBegin, float newEnd,
                    float inputValue, float curve);

// Global state

#define PIN_COUNT (sizeof(Pins) / sizeof(Pins[0]))
Bounce PinBounce[PIN_COUNT]; // Debouncer state
int    PinState[PIN_COUNT];  // Previous value, indexed by pin
int    NewState[PIN_COUNT];  // Current value, indexed by pin

EventMap *EventList;  // List of events to handle
unsigned NumEvents;   // Number of entries in EventList
uint8_t  *LastValue;  // Last value calculated for controller
uint8_t  *LatchState; // Last state recorded for pin


// Generate an output event, based on an input pin state and
// the previous value generated

uint8_t GenerateEvent(Event *Evt, int state, uint8_t lastValue) {
  uint8_t value;

  //Serial.println("  Evt " + String(i) + " t" + genEvt->Type + ": ");
  value = 0;
  switch (Evt->Generic.Type) {
  case NoteEventType:
    // Send note on for off->on transition, note off for on->off transition
    NoteEvent *noteEvt;
    noteEvt = &Evt->Note;
    if (state > 512) {
#ifdef LOG_EVENTS
      Serial.println(String("On  ") + noteEvt->Channel + "/" + noteEvt->Note + ": " + noteEvt->OnVelocity);
#endif
      usbMIDI.sendNoteOn(noteEvt->Note, noteEvt->OnVelocity, noteEvt->Channel);
      MIDI.sendNoteOn(noteEvt->Note, noteEvt->OnVelocity, noteEvt->Channel);
    }
    else {
#ifdef LOG_EVENTS
      Serial.println(String("Off ") + noteEvt->Channel + "/" + noteEvt->Note + ": " + noteEvt->OffVelocity);
#endif
      usbMIDI.sendNoteOff(noteEvt->Note, noteEvt->OffVelocity, noteEvt->Channel);
      MIDI.sendNoteOff(noteEvt->Note, noteEvt->OffVelocity, noteEvt->Channel);
    }
    value = state / 8; // Scale to 0-127 range
    break;

  case ControllerEventType:
    // Scale controller value between OffValue and OnValue
    ControllerEvent *ctlEvt;
    ctlEvt = &Evt->Controller;
    value = map(state, 0, 1023, ctlEvt->OffValue, ctlEvt->OnValue);
    if (value != lastValue) {
#ifdef LOG_EVENTS
      Serial.println(String("Ctl ") + ctlEvt->Channel + "/" + ctlEvt->Controller + ": " + value);
#endif
      usbMIDI.sendControlChange(ctlEvt->Controller, value, ctlEvt->Channel);
      MIDI.sendControlChange(ctlEvt->Controller, value, ctlEvt->Channel);
    }
    break;

  case ProgramEventType:
    // Send program change on button off->on transition
    ProgramEvent *pgmEvt;
    if (state > 512) {
      pgmEvt = &Evt->Program;
#ifdef LOG_EVENTS
      Serial.println(String("Pgm ") + pgmEvt->Channel + "/" + pgmEvt->Program);
#endif
      usbMIDI.sendProgramChange(pgmEvt->Program, pgmEvt->Channel);
      MIDI.sendProgramChange(pgmEvt->Program, pgmEvt->Channel);
    }
    else {
#ifdef LOG_EVENTS
      Serial.println("<none>");
#endif
    }
    value = state / 8; // Scale to 0-127 range
    break;

  case OutEventType:
    // Scale the LED/PWM output between OffValue and OnValue
    OutEvent *outEvt;
    outEvt = &Evt->Out;
    if (outEvt->OutPin >= PIN_COUNT) {
#ifdef LOG_ERRORS
      Serial.println(String("Out ") + outEvt->OutPin + " out of range");
#endif
      break;
    }
    value = map(state, 0, 1023, outEvt->OffValue, outEvt->OnValue);
    if (value != lastValue) {
#ifdef LOG_EVENTS
      Serial.println(String("Out ") + outEvt->OutPin + ": " + value);
#endif
      switch (Pins[outEvt->OutPin].Type) {
      case AnalogOut:
        analogWrite(Pins[outEvt->OutPin].Num, value);
        break;
      case DigitalOut:
        digitalWrite(Pins[outEvt->OutPin].Num, ((value < 128) ? LOW : HIGH));
        break;
      default:
        break;
      }
    }
    break;

  default:
    break;
  }

  return value;
}


void FindProgram (byte channelNum, byte programNum) {
  unsigned i;
  unsigned count;

#ifdef LOG_PROGRAM
  Serial.println(String("->Pgm ") + channelNum + "." + programNum);
#endif
  
  // Send any patch exit events for the old program

  for (i = 0; i < NumEvents; i++) {
    EventMap *m = &EventList[i];

    if (m->Pin == EXIT_PIN) {
      // Call GenerateEvent with state=1023 (the max), and an impossible
      // lastValue which guarantees new data will be sent
      LastValue[i] = GenerateEvent(&m->Evt, 1023, 0x80);
    }
  }

  // Find the requested channel and program number in the list,
  // or default to the first entry

  for (i = 0; i < sizeof(DefaultEventList) / sizeof(DefaultEventList[0]); i++) {
    const ProgramEvent *pgm;

    if (DefaultEventList[i].Pin != PROGRAM_PIN) {
      continue;
    }

    pgm = &DefaultEventList[i].Evt.Program;
    if (pgm->Type != ProgramEventType) {
#ifdef LOG_ERRORS
      Serial.println("Bad program entry at index " + i);
#endif
      continue;
    }

    if (pgm->Channel == channelNum && pgm->Program == programNum) {
      break; // Found it!
    }
  }

  if (i >= sizeof(DefaultEventList) / sizeof(DefaultEventList[0])) {
    // Not found, use the first table entry as the default
#ifdef LOG_PROGRAM
    Serial.println(" (not found)");
#endif
    i = 0;
  }

  // Count the entries

  i++; // Skip the PROGRAM_PIN element
  for (count = 0; (i + count) < sizeof(DefaultEventList) / sizeof(DefaultEventList[0]); count++) {
    if (DefaultEventList[i + count].Pin == PROGRAM_PIN) {
      break;
    }
  }

  // Initialize EventList, NumEvents, LastValue, LatchState
  NumEvents = count;
  EventList = (EventMap *)realloc (EventList, NumEvents * sizeof (*EventList));
  memcpy(EventList, &DefaultEventList[i], NumEvents * sizeof (*EventList));

  LastValue = (uint8_t *)realloc (LastValue, NumEvents * sizeof (*LastValue));
  memset(LastValue, 0, NumEvents * sizeof (*LastValue));

  LatchState = (uint8_t *)realloc (LatchState, NumEvents * sizeof (*LatchState));
  memset(LatchState, 0, NumEvents * sizeof (*LatchState));

  // Send any initialization events, and initialize LastValue[]

  for (unsigned i = 0; i < NumEvents; i++) {
    EventMap *m = &EventList[i];

    if (m->Pin == INIT_PIN) {
      // Call GenerateEvent with state=1023 (the max), and an impossible
      // lastValue which guarantees new data will be sent
      LastValue[i] = GenerateEvent(&m->Evt, 1023, 0x80);
    }
    else if (m->Handling == LatchingOn) {
      // Default the value to "on"
      LastValue[i] = 0x7f;
      LatchState[i] = 1;
    }
    else {
      // Default the value to "off"
      LastValue[i] = 0;
      LatchState[i] = 0;
    }
  }
}


void setup() {
  Serial.begin(38400);  // For debugging
#ifdef WAIT_FOR_SERIAL
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
    const Pin *p = &Pins[i];
    switch (p->Type) {
      case Analog:
        // Do nothing
        break;
      case AnalogOut:
      case DigitalOut:
        pinMode(p->Num, OUTPUT);
        break;
      case Digital:
      case DigitalPullup:
        pinMode(p->Num, (p->Type == Digital) ? INPUT : INPUT_PULLUP);
        PinBounce[i] = Bounce();
        PinBounce[i].attach(p->Num);
        PinBounce[i].interval(5);
        break;
      default:
#ifdef LOG_ERRORS
        Serial.print("Bad pin type in Pins entry ");
        Serial.println(i);
#endif
    }
    PinState[i] = 0;
  }

  // Initialize to first program in table

  NumEvents = 0;
  FindProgram (DefaultEventList[0].Evt.Program.Channel,
               DefaultEventList[0].Evt.Program.Program);
}

void loop() {
  unsigned pinNum;

  // See if we have received a program change.  Discard all other incoming
  // MIDI data, to keep from hanging the host.

  while (usbMIDI.read()) {
    byte msgType = usbMIDI.getType();
    if (msgType == 4 /*ProgramChange*/) {
      FindProgram(usbMIDI.getChannel(), usbMIDI.getData1());  // Or just use callbacks?
    }
    else {
#ifdef LOG_MIDIIN
      Serial.println(String("->Msg ") + msgType + " " +
                     usbMIDI.getData1() + " " + usbMIDI.getData2());
#endif
    }
  }
  while (MIDI.read()) {
    byte msgType = MIDI.getType();
    if (msgType == ProgramChange) {
      FindProgram(MIDI.getChannel(), MIDI.getData1());  // Or just use callbacks?
    }
    else {
#ifdef LOG_MIDIIN
      Serial.println(String("->Msg ") + String(msgType, HEX) + " " +
                     MIDI.getData1() + " " + MIDI.getData2());
#endif
    }
  }

  // Poll the input pins

  for (pinNum = 0; pinNum < PIN_COUNT; pinNum++) {
    const Pin *p = &Pins[pinNum];
    int val;     // Value read, 0-1023
    int state;   // Output state of the pin

    // Poll the input pin, convert to range 0-1023

    switch (p->Type) {
    case Analog:
      // Scale the input value to find the pin state
      val = analogRead(p->Num);
      state = val;
      if (p->Min < p->Max) {
        state = fscale(p->Min, p->Max, 0, 1023, state, p->Curve);
      }
      else {
        state = fscale(p->Max, p->Min, 1023, 0, state, p->Curve);
      }
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
#ifdef LOG_PINS
        Serial.println(String("Pin ") + p->Num + ":" + val + ";" + state);
#endif
    }
  } // for (pinNum)

  // Convert pin state changes to output events

  for (unsigned i = 0; i < NumEvents; i++) {
    EventMap        *m = &EventList[i];
    int             changed;
    int             state;
    uint8_t         value;

    // Detect changes, for use in the event type handlers below

    pinNum = m->Pin;
    if (pinNum == INIT_PIN || pinNum == EXIT_PIN || pinNum == PROGRAM_PIN) {
      // Skip special pin IDs
      continue;
    }

    switch (m->Handling) {
    case Continuous:
    case Momentary:
      state   = NewState[pinNum];
      changed = (NewState[pinNum] != PinState[pinNum]);
      break;

    case LatchingOff:
    case LatchingOn:
      // Toggle state on button press
      if (PinState[pinNum] < 512 && NewState[pinNum] >= 512) {
        LatchState[i] ^= 1;
        changed = 1;
      }
      else {
        changed = 0;
      }
      state = (LatchState[i] == 0) ? 0 : 1023;
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

    // Generate the output event

    value = GenerateEvent(&m->Evt, state, LastValue[i]);

    // Remember the last value generated

    LastValue[i] = value;
  }
  
  // Save pin states for the next loop iteration

  for (pinNum = 0; pinNum < PIN_COUNT; pinNum++) {
    PinState[pinNum] = NewState[pinNum];
  }

#ifdef CORE_TEENSY  // Send any queued USB MIDI data
  usbMIDI.send_now();
  USE_SERIAL_PORT.flush(); // Serial port underlying MIDI library
#endif

  delay(1); // Don't send too quickly.  FIXME do something more?
}

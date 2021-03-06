#include <MIDI.h>
#ifndef CORE_TEENSY // Leonardo/Pro Micro USB MIDI library
#ifdef USE_USB_MIDI
#include <midi_UsbTransport.h>

static const unsigned sUsbTransportBufferSize = 16;
typedef midi::UsbTransport<sUsbTransportBufferSize> UsbTransport;

UsbTransport sUsbTransport;

MIDI_CREATE_INSTANCE(UsbTransport, sUsbTransport, usbMIDI); // USB
#endif
#ifdef USE_SERIAL_MIDI
MIDI_CREATE_DEFAULT_INSTANCE(); // Serial
#endif
#endif // CORE_TEENSY

#include <Bounce2.h>

// Midi controller
// by Brian J. Johnson 2017

#include "DataStructures.h"
#include "Configuration.h"

#ifdef USE_FSCALE
extern float fscale(float originalMin, float originalMax,
                    float newBegin, float newEnd,
                    float inputValue, float curve);
#endif

// If the serial console isn't enabled, then disable all logging
#ifndef USE_SERIAL_PRINT
#undef WAIT_FOR_SERIAL
#undef LOG_PINS    // Input pin values
#undef LOG_EVENTS  // Output events
#undef LOG_PROGRAM // Program changes
#undef LOG_MIDIIN  // Other MIDI input
#undef LOG_ERRORS  // Configuration errors
#endif


// Global state

#define PIN_COUNT (sizeof(Pins) / sizeof(Pins[0]))
Bounce PinBounce[PIN_COUNT]; // Debouncer state
int    PinState[PIN_COUNT];  // Previous value, indexed by pin
int    NewState[PIN_COUNT];  // Current value, indexed by pin

EventMap *EventList;  // List of events to handle
unsigned NumEvents;   // Number of entries in EventList
uint8_t  *LastValue;  // Last value calculated for controller
uint8_t  *LatchState; // Last state recorded for pin

uint8_t LastBankLsb = 0; // Last bank change sequence seen
uint8_t LastBankMsb = 0;

// Generate an output event, based on an input pin state and
// the previous value generated

uint8_t GenerateEvent(Event *Evt, int state, uint8_t lastValue) {
  uint8_t value;

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
#ifdef USE_USB_MIDI
      usbMIDI.sendNoteOn(noteEvt->Note, noteEvt->OnVelocity, noteEvt->Channel);
#endif
#ifdef USE_SERIAL_MIDI
      MIDI.sendNoteOn(noteEvt->Note, noteEvt->OnVelocity, noteEvt->Channel);
#endif
    }
    else {
#ifdef LOG_EVENTS
      Serial.println(String("Off ") + noteEvt->Channel + "/" + noteEvt->Note + ": " + noteEvt->OffVelocity);
#endif
#ifdef USE_USB_MIDI
      usbMIDI.sendNoteOff(noteEvt->Note, noteEvt->OffVelocity, noteEvt->Channel);
#endif
#ifdef USE_SERIAL_MIDI
      MIDI.sendNoteOff(noteEvt->Note, noteEvt->OffVelocity, noteEvt->Channel);
#endif
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
#ifdef USE_USB_MIDI
      usbMIDI.sendControlChange(ctlEvt->Controller, value, ctlEvt->Channel);
#endif
#ifdef USE_SERIAL_MIDI
      MIDI.sendControlChange(ctlEvt->Controller, value, ctlEvt->Channel);
#endif
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
#ifdef USE_USB_MIDI
      usbMIDI.sendProgramChange(pgmEvt->Program, pgmEvt->Channel);
#endif
#ifdef USE_SERIAL_MIDI
      MIDI.sendProgramChange(pgmEvt->Program, pgmEvt->Channel);
#endif
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


void FindProgram (byte channelNum, byte programNum, byte bankLsb, byte bankMsb) {
  unsigned i;
  unsigned count;
  uint16_t bank;

  bank = (bankMsb << 8) | bankLsb;
#ifdef LOG_PROGRAM
  Serial.println(String("->Pgm ") +
                 channelNum + ".0x" +
                 String(bank, HEX) + "." +
                 programNum);
#endif

  // Validate the bank number

#ifdef BANKS_TO_HANDLE
  static const uint16_t BanksToHandle[] = {BANKS_TO_HANDLE};
  for (i = 0; i < sizeof(BanksToHandle) / sizeof(BanksToHandle[0]); i++) {
    if (BanksToHandle[i] == bank) {
      break;
    }
  }

  // If it wasn't in the list of banks to handle, ignore it
  if (i >= sizeof(BanksToHandle) / sizeof(BanksToHandle[0])) {
#ifdef LOG_PROGRAM
    Serial.println(String("  (not handled)"));
    return;
#endif
  }
#endif

#ifdef BANKS_TO_IGNORE
  // If it's in BanksToIgnore, skip it
  static const uint16_t BanksToIgnore[] = {BANKS_TO_IGNORE};
  for (i = 0; i < sizeof(BanksToIgnore) / sizeof(BanksToIgnore[0]); i++) {
    if (BanksToIgnore[i] == bank) {
#ifdef LOG_PROGRAM
      Serial.println("  (ignored)");
#endif
      return;
    }
  }
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

  // Find the requested channel, bank, and program number in the list,
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

    if (pgm->Channel == channelNum && pgm->Program == programNum &&
        pgm->BankLsb == bankLsb && pgm->BankMsb == bankMsb) {
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

  // Skip the PROGRAM_PIN entries

  for (; i < sizeof(DefaultEventList) / sizeof(DefaultEventList[0]); i++) {
    if (DefaultEventList[i].Pin != PROGRAM_PIN) {
      break;
    }
  }

  // Count the entries

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


void HandleMidiMsg(byte channel, byte msgType, byte data1, byte data2) {

  // Process bank number changes

  if (msgType == ControlChange) {
    byte controllerNum = data1;
    byte value = data2;

    if (controllerNum == 0x00 /* Bank MSB */) {
      LastBankMsb = value;
#ifdef LOG_MIDIIN
      Serial.println(String("->Msb 0x") + String(value, HEX));
#endif
    }
    else if (controllerNum == 0x20 /* Bank LSB */) {
      LastBankLsb = value;
#ifdef LOG_MIDIIN
      Serial.println(String("->Lsb 0x") + String(value, HEX));
#endif
    }
    else {
#ifdef LOG_MIDIIN
      Serial.println(String("->Ctl ") + " " + controllerNum + " " + value);
#endif
    }
  }

  // Process program changes

  else if (msgType == ProgramChange) {
    FindProgram(channel, data1, LastBankLsb, LastBankMsb);
  }

#ifdef LOG_MIDIIN
  // Dump other messages
  else {
    Serial.println(String("->Msg ") + String(msgType, HEX) + " " +
                   data1 + " " + data2);
  }
#endif
}


void setup() {
#ifdef USE_SERIAL_PRINT
  Serial.begin(38400);  // For debugging
#endif
#ifdef WAIT_FOR_SERIAL
  while (!Serial) { }   // Wait for USB serial to connect
  delay(1000);
#endif

  // Turn on the LED, so we can see the board is on

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Initialize MIDI
  
#ifdef USE_SERIAL_MIDI
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
#endif

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
        break;
    }
    PinState[i] = 0;
  }

  // Initialize to first program in table

  NumEvents = 0;
  FindProgram (DefaultEventList[0].Evt.Program.Channel,
               DefaultEventList[0].Evt.Program.Program,
               DefaultEventList[0].Evt.Program.BankLsb,
               DefaultEventList[0].Evt.Program.BankMsb);
}

void loop() {
  unsigned pinNum;

  // See if we have received a message we're interested in.  Discard
  // all other incoming MIDI data, to keep from hanging the host.

#ifdef USE_USB_MIDI
  while (usbMIDI.read()) {
    HandleMidiMsg(
      usbMIDI.getChannel(),
      (usbMIDI.getType() << 4) | 0x80, // Convert to MIDI.h values
      usbMIDI.getData1(),
      usbMIDI.getData2());
  }
#endif
#ifdef USE_SERIAL_MIDI
  while (MIDI.read()) {
    HandleMidiMsg(
      MIDI.getChannel(),
      MIDI.getType(),
      MIDI.getData1(),
      MIDI.getData2());
  }
#endif

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
#ifdef USE_FSCALE
      if (p->Min < p->Max) {
        state = fscale(p->Min, p->Max, 0, 1023, state, p->Curve);
      }
      else {
        state = fscale(p->Max, p->Min, 1023, 0, state, p->Curve);
      }
#else
      if (p->Min < p->Max) {
        state = constrain(state, p->Min, p->Max);
      }
      else {
        state = constrain(state, p->Max, p->Min);
      }
      state = map(state, p->Min, p->Max, 0, 1023);
      break;
#endif
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
#ifdef USE_USB_MIDI
  usbMIDI.send_now();
#endif
#ifdef USE_SERIAL_MIDI
  USE_SERIAL_PORT.flush(); // Serial port underlying MIDI library
#endif
#endif

  delay(LOOP_DELAY); // Don't send too quickly
}

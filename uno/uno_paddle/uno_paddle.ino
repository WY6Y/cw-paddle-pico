// WY6Y CW Paddle, Arduino Uno R3 stand-in for the Pico (2026-11-01).
//
// The R3's USB chip is a serial bridge, not a MIDI device, so this sends the
// lever state over serial and uno_bridge.py on the host turns it into the same
// "WY6Y CW Paddle" MIDI port the Pico provides (dit Note 20, dah Note 21).
//
// Wiring, matching the Pico (contacts close to ground, internal pull-ups):
//   D3 = dit, D2 = dah, jack common -> GND.
//
// Protocol, 115200 baud: one byte 0x40 | dit | dah << 1, sent on every change
// and once a second. A lever closed over 5 s is reported open until it opens
// (same stuck-key cutoff as the Pico firmware).

const uint8_t DIT_PIN = 3;
const uint8_t DAH_PIN = 2;
const unsigned long LOCKOUT_MS = 12;   // the mini paddle bounces 4-8 ms
const unsigned long STUCK_MS = 5000;
const unsigned long RESYNC_MS = 1000;

struct Lever {
  uint8_t pin;
  bool closed;
  bool stuck;
  unsigned long closed_at;
  unsigned long quiet_until;
};

Lever levers[2] = { { DIT_PIN }, { DAH_PIN } };
uint8_t last_sent = 0xFF;
unsigned long next_resync = 0;

static bool keyed(const Lever &l) { return l.closed && !l.stuck; }

void setup() {
  for (auto &l : levers) pinMode(l.pin, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  unsigned long now = millis();
  for (auto &l : levers) {
    if ((long)(now - l.quiet_until) >= 0) {
      bool closed = digitalRead(l.pin) == LOW;
      if (closed != l.closed) {
        l.closed = closed;
        l.quiet_until = now + LOCKOUT_MS;
        if (closed) l.closed_at = now;
        else l.stuck = false;
      }
    }
    if (keyed(l) && now - l.closed_at > STUCK_MS) l.stuck = true;
  }
  uint8_t state = 0x40 | (keyed(levers[0]) ? 1 : 0) | (keyed(levers[1]) ? 2 : 0);
  if (state != last_sent || (long)(now - next_resync) >= 0) {
    Serial.write(state);
    last_sent = state;
    next_resync = now + RESYNC_MS;
  }
  digitalWrite(LED_BUILTIN, levers[0].closed || levers[1].closed);
}

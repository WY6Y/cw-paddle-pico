// WY6Y CW Paddle: an iambic paddle as a USB MIDI device for AetherSDR.
//
// AetherSDR's host-side keyer does the timing; this only reports contact
// state. Dit = Note 20, dah = Note 21, channel 1 (the SmartSDR/CTR2
// "leftpaddle"/"rightpaddle" convention): Note On vel 127 on close, Note Off
// on open. Two jacks are wired in parallel (OR'd) so either paddle works.
//
// Wiring (contacts close to ground, internal pull-ups):
//   Jack A: GP3 (pin 5) = dit, GP2 (pin 4) = dah, sleeve -> GND (pin 3)
//   Jack B: GP5 (pin 7) = dit, GP4 (pin 6) = dah, sleeve -> GND (pin 8)
//   (Swapped 2026-11-01 to match WY6Y's soldered, left-handed paddle; a paddle
//   that feels backwards can also use the keyer's paddle-swap setting.)
//
// LED: on while a paddle is closed; slow blink until the host enumerates it.
// SysEx F0 7D 57 59 42 F7 ("WYB") reboots into BOOTSEL for re-flashing.
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "tusb.h"

#define MIDI_CH        0        // channel 1
#define NOTE_DIT       20
#define NOTE_DAH       21
#define LOCKOUT_US     12000    // ignore contact bounce for 12 ms after an edge (the
                                // mini paddle bounces 4-8 ms; a 40 WPM dit is 30 ms)
#define RESYNC_US      1000000  // re-send held state if the host drops a note

static const uint DIT_PINS[] = { 3, 5 };
static const uint DAH_PINS[] = { 2, 4 };

typedef struct {
    const uint *pins;
    uint8_t note;
    bool down;
    absolute_time_t quiet_until;
} paddle_t;

static paddle_t paddles[] = {
    { DIT_PINS, NOTE_DIT, false, 0 },
    { DAH_PINS, NOTE_DAH, false, 0 },
};

static bool read_closed(const uint *pins) {
    return !gpio_get(pins[0]) || !gpio_get(pins[1]);
}

static void send_note(uint8_t note, bool on) {
    uint8_t msg[3] = { (uint8_t)((on ? 0x90 : 0x80) | MIDI_CH), note, on ? 127 : 0 };
    tud_midi_stream_write(0, msg, sizeof msg);
}

static void poll_paddles(void) {
    absolute_time_t now = get_absolute_time();
    for (size_t i = 0; i < count_of(paddles); i++) {
        paddle_t *p = &paddles[i];
        if (absolute_time_diff_us(now, p->quiet_until) > 0) continue;
        bool closed = read_closed(p->pins);
        if (closed == p->down) continue;
        p->down = closed;
        p->quiet_until = delayed_by_us(now, LOCKOUT_US);
        if (tud_mounted()) send_note(p->note, closed);
    }
}

// Re-send the current state once a second while anything is held, and one
// Note Off per paddle at mount, so a missed message can't leave a key stuck.
static void resync(void) {
    static absolute_time_t next;
    static bool was_mounted;
    bool mounted = tud_mounted();
    if (mounted && !was_mounted) {
        for (size_t i = 0; i < count_of(paddles); i++) send_note(paddles[i].note, paddles[i].down);
        next = make_timeout_time_us(RESYNC_US);
    }
    was_mounted = mounted;
    if (!mounted || absolute_time_diff_us(get_absolute_time(), next) > 0) return;
    next = make_timeout_time_us(RESYNC_US);
    for (size_t i = 0; i < count_of(paddles); i++)
        if (paddles[i].down) send_note(paddles[i].note, true);
}

static void read_host(void) {
    static const uint8_t BOOTSEL_SYSEX[] = { 0xF0, 0x7D, 'W', 'Y', 'B', 0xF7 };
    static size_t matched;
    uint8_t buf[48];
    while (tud_midi_available()) {
        uint32_t n = tud_midi_stream_read(buf, sizeof buf);
        for (uint32_t i = 0; i < n; i++) {
            matched = (buf[i] == BOOTSEL_SYSEX[matched]) ? matched + 1
                    : (buf[i] == BOOTSEL_SYSEX[0]) ? 1 : 0;
            if (matched == sizeof BOOTSEL_SYSEX) reset_usb_boot(0, 0);
        }
    }
}

static void led(void) {
    bool held = paddles[0].down || paddles[1].down;
    bool on = tud_mounted() ? held : (to_ms_since_boot(get_absolute_time()) / 500) & 1;
    gpio_put(PICO_DEFAULT_LED_PIN, on);
}

int main(void) {
    const uint pins[] = { 2, 3, 4, 5 };
    for (size_t i = 0; i < count_of(pins); i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_pull_up(pins[i]);
    }
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    tud_init(0);

    while (true) {
        tud_task();
        poll_paddles();
        resync();
        read_host();
        led();
    }
}

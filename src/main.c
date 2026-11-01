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
// Key-down safety (2026-11-01, after a contaminated board held dit closed and
// a marginal USB link went silent while staying enumerated):
//   - A lever closed longer than STUCK_US is a fault: the host gets Note Off
//     and the lever is ignored until its contact opens.
//   - Every RESYNC_US the host gets both levers' state, released included, so
//     a lost Note Off can't leave the keyer down.
//   - If notes stop draining to the host for LINK_STALL_US, the USB link is
//     dropped and re-attached (what a replug did by hand).
//   - A hardware watchdog reboots the Pico if the main loop ever stops.
//
// LED: slow blink until the host enumerates it, then on while a lever is
// closed. On a Pico 2 W the LED is on the Wi-Fi chip (build with
// PICO_BOARD=pico2_w). SysEx F0 7D 57 59 42 F7 ("WYB") reboots into BOOTSEL.
#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "tusb.h"
#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#define MIDI_CH        0        // channel 1
#define NOTE_DIT       20
#define NOTE_DAH       21
#define LOCKOUT_US     12000    // ignore contact bounce for 12 ms after an edge (the
                                // mini paddle bounces 4-8 ms; a 40 WPM dit is 30 ms)
#define RESYNC_US      1000000  // re-send both levers' state this often
#define STUCK_US       5000000  // a lever closed this long is a fault, not keying
#define LINK_STALL_US  1500000  // notes not draining this long: re-attach USB
#define RELINK_US      300000   // how long the link stays detached
#define WATCHDOG_MS    1000

static const uint DIT_PINS[] = { 3, 5 };
static const uint DAH_PINS[] = { 2, 4 };

typedef struct {
    const uint *pins;
    uint8_t note;
    bool closed;                // debounced contact
    bool stuck;                 // closed past STUCK_US; ignored until it opens
    absolute_time_t closed_at;
    absolute_time_t quiet_until;
} paddle_t;

static paddle_t paddles[] = {
    { DIT_PINS, NOTE_DIT },
    { DAH_PINS, NOTE_DAH },
};

static absolute_time_t stall_since;   // nil while notes are draining

static bool keyed(const paddle_t *p) { return p->closed && !p->stuck; }

static bool read_closed(const uint *pins) {
    return !gpio_get(pins[0]) || !gpio_get(pins[1]);
}

static void send_note(uint8_t note, bool on) {
    if (!tud_mounted()) return;
    uint8_t msg[3] = { (uint8_t)((on ? 0x90 : 0x80) | MIDI_CH), note, on ? 127 : 0 };
    if (tud_midi_stream_write(0, msg, sizeof msg) == sizeof msg) {
        stall_since = nil_time;
    } else if (is_nil_time(stall_since)) {
        stall_since = get_absolute_time();
    }
}

static void poll_paddles(void) {
    absolute_time_t now = get_absolute_time();
    for (size_t i = 0; i < count_of(paddles); i++) {
        paddle_t *p = &paddles[i];
        bool was = keyed(p);
        if (absolute_time_diff_us(now, p->quiet_until) <= 0) {
            bool closed = read_closed(p->pins);
            if (closed != p->closed) {
                p->closed = closed;
                p->quiet_until = delayed_by_us(now, LOCKOUT_US);
                if (closed) p->closed_at = now;
                else p->stuck = false;
            }
        }
        if (keyed(p) && absolute_time_diff_us(p->closed_at, now) > STUCK_US) p->stuck = true;
        if (keyed(p) != was) send_note(p->note, keyed(p));
    }
}

// Both levers' state at mount and once a second after, so a missed message
// can't leave a key stuck down on the host.
static void resync(void) {
    static absolute_time_t next;
    static bool was_mounted;
    bool mounted = tud_mounted();
    bool due = mounted && (!was_mounted || absolute_time_diff_us(get_absolute_time(), next) <= 0);
    was_mounted = mounted;
    if (!due) return;
    next = make_timeout_time_us(RESYNC_US);
    for (size_t i = 0; i < count_of(paddles); i++) send_note(paddles[i].note, keyed(&paddles[i]));
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

// Enumerated but not taking notes: detach and re-attach, like a replug.
static void check_link(void) {
    if (!tud_mounted()) { stall_since = nil_time; return; }
    if (is_nil_time(stall_since) ||
        absolute_time_diff_us(stall_since, get_absolute_time()) < LINK_STALL_US) return;
    tud_disconnect();
    absolute_time_t until = make_timeout_time_us(RELINK_US);
    while (absolute_time_diff_us(get_absolute_time(), until) > 0) {
        watchdog_update();
        tud_task();
    }
    tud_connect();
    stall_since = nil_time;
}

static void led_put(bool on) {
    static int last = -1;
    if (on == last) return;     // the Pico 2 W LED is an SPI write; only on change
    last = on;
#ifdef CYW43_WL_GPIO_LED_PIN
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
#else
    gpio_put(PICO_DEFAULT_LED_PIN, on);
#endif
}

static void led(void) {
    bool held = paddles[0].closed || paddles[1].closed;
    led_put(tud_mounted() ? held : (to_ms_since_boot(get_absolute_time()) / 500) & 1);
}

int main(void) {
    const uint pins[] = { 2, 3, 4, 5 };
    for (size_t i = 0; i < count_of(pins); i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_pull_up(pins[i]);
    }
#ifdef CYW43_WL_GPIO_LED_PIN
    cyw43_arch_init();          // no Wi-Fi is started; this is only for the LED
#else
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif
    stall_since = nil_time;

    tud_init(0);
    watchdog_enable(WATCHDOG_MS, true);

    while (true) {
        watchdog_update();
        tud_task();
        poll_paddles();
        resync();
        read_host();
        check_link();
        led();
    }
}

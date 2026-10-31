# WY6Y CW Paddle (Pico 2 USB MIDI)

An iambic paddle as a USB MIDI device for AetherSDR's host-side keyer
(HL2 remote CW). The Pico only reports contact state; AetherSDR times the
elements, makes sidetone, and keys the HL2.

| Paddle | MIDI (channel 1) | AetherSDR |
|---|---|---|
| Dit (tip) | Note 20 On/Off | `leftpaddle` → `cwdit` |
| Dah (ring) | Note 21 On/Off | `rightpaddle` → `cwdah` |

Board: Raspberry Pi Pico 2 (RP2350). USB ID `2e8a:10ca`, name "WY6Y CW Paddle".

## Wiring

Contacts close to ground; internal pull-ups, no resistors. Two jacks are in
parallel, so either paddle works (the heavy paddle through the 1/4" adapter).

| Jack | Tip (dit) | Ring (dah) | Sleeve |
|---|---|---|---|
| A | GP2 (pin 4) | GP3 (pin 5) | GND (pin 3) |
| B | GP4 (pin 6) | GP5 (pin 7) | GND (pin 8) |

The breakout boards' 5 pins are unlabeled: put a plug in and find T/R/S with
a continuity meter (the 2 extra pins are usually switch contacts; leave them).

LED: slow blink = not enumerated by a host; solid = a paddle is closed.

## Test and map

1. `./paddle_test.py` prints `DIT DOWN/up`, `DAH DOWN/up` as you key.
   Dit and dah swapped? Swap the tip and ring wires (or the keyer's paddle-swap setting).
2. AetherSDR: MIDI mapping dialog → Import → `wy6y-cw-paddle.map`
   (or Learn `cwdit`/`cwdah` and press each paddle).

## Build and flash

    cmake -B build -DPICO_SDK_PATH=$HOME/pico-sdk && make -C build -j4
    ./paddle_test.py bootsel        # or hold BOOTSEL while plugging in
    cp build/cw_paddle.uf2 /media/$USER/RP2350/

Built on the Pi 5 with pico-sdk 2.2.0 and Debian `gcc-arm-none-eabi`.

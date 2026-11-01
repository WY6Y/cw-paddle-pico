# WY6Y CW Paddle (Pico 2 USB MIDI)

An iambic paddle as a USB MIDI device for AetherSDR's host-side keyer
(HL2 remote CW). The Pico only reports contact state; AetherSDR times the
elements, makes sidetone, and keys the HL2.

| Paddle | MIDI (channel 1) | AetherSDR |
|---|---|---|
| Dit | Note 20 On/Off | `leftpaddle` → `cwdit` |
| Dah | Note 21 On/Off | `rightpaddle` → `cwdah` |

Board: Raspberry Pi Pico 2 or 2 W (RP2350). USB ID `2e8a:10ca`, name "WY6Y CW Paddle".

## Wiring

Contacts close to ground; internal pull-ups, no resistors. Two jacks are in
parallel, so either paddle works (the heavy paddle through the 1/4" adapter).

| Jack | Tip (dit) | Ring (dah) | Sleeve |
|---|---|---|---|
| A | GP3 (pin 5) | GP2 (pin 4) | GND (pin 3) |
| B | GP5 (pin 7) | GP4 (pin 6) | GND (pin 8) |

The breakout boards' 5 pins are unlabeled: put a plug in and find T/R/S with
a continuity meter (the 2 extra pins are usually switch contacts; leave them).

As built for WY6Y (left-handed, Putikeeg mini paddle, Pico 2 W, soldered
2026-11-01): jack board pin 1 (common) to GND (pin 3, between GP1 and GP2), the
two lever pins (2 and 5) to GP2 and GP3. The firmware reads **GP3 as dit and GP2
as dah** (and GP5 dit / GP4 dah for jack B), swapped 2026-11-01 so the soldered
paddle feels right. Jack board pins 3 and 4 are switch contacts, not connected.

Use rosin-core solder or an electronics flux only. Plumbing flux (Nokorode,
Oatey No. 95) left a conductive film that held GP2 low (a stuck dit the
continuity beeper couldn't find) and broke USB enumeration (`-71`) until the
boards were washed in soapy water, rinsed with distilled water, and dried.

The mini paddle's contacts bounce 4-8 ms on press; the firmware's 12 ms
lockout absorbs it.

LED: slow blink = not enumerated by a host; solid = a paddle is closed. On a
Pico 2 W (WY6Y's board) the LED hangs off the Wi-Fi chip, so it stays dark
until the firmware is built for `pico2_w`.

## Test and map

1. `./paddle_test.py` prints `DIT DOWN/up`, `DAH DOWN/up` as you key.
   Dit and dah swapped? Swap the tip and ring wires (or the keyer's paddle-swap setting).
   The test script holds the MIDI port; quit it before starting AetherSDR.
2. AetherSDR (v26.10.8): Settings → MIDI Mapping. Port "WY6Y CW Paddle" →
   Connect, tick Auto-connect. Import... `wy6y-cw-paddle.map` only stores it as a
   profile: pick `wy6y-cw-paddle` in the Profile box and press **Load**, or
   nothing is bound (or Learn `cwdit`/`cwdah` and press each paddle).
   Slice in CW mode; the CW panel's Breakin off = sidetone only, no RF.
   After the Pico re-enumerates (replug, reflash) AetherSDR doesn't re-attach:
   press Connect again (or `aconnect 'WY6Y CW Paddle' 'RtMidi Input Client'`).

## Build and flash

    cmake -B build -DPICO_SDK_PATH=$HOME/pico-sdk && make -C build -j4
    ./paddle_test.py bootsel        # or hold BOOTSEL while plugging in
    cp build/cw_paddle.uf2 /media/$USER/RP2350/

Built on the Pi 5 with pico-sdk 2.2.0 and Debian `gcc-arm-none-eabi`.

## Case

`case/case.scad` (OpenSCAD, parametric) with ready STLs for 1 or 2 jacks:
about 87 x 28 x 21 mm (1 jack) or 87 x 43 (2 jacks), press-fit lid. USB out
one end, jacks out the other. Print both flat, no supports, PETG or PLA.

- Pico 2 H (factory headers down): the header plastic rests on two rails,
  the pin tips drop into slots in them, an end stop takes the USB push, and
  four lid pegs go into the mounting holes. No screws.
- Each jack board sits on ledges, butts against the jack wall (takes plug
  pulls), has a stop behind it (takes plug pushes); lid ribs hold it down.
- Thin window in the lid over the Pico LED.
- Lid snaps on: a ridge on each long side of the lid lip clicks into a groove in the
  base wall (0.4 mm catch). Pry at the USB end to open.
- Pico pegs are 1.95 mm with a cone tip, snug in the 2.1 mm holes.

Wire by soldering to the header pin stubs on the Pico's top side (GP2-GP5,
GND); jumpers on the pins underneath won't fit in the rails. Check the values
marked MEASURE in `case.scad` (header plastic, pin length, jack position),
then re-export: `openscad -D 'part="base"' -D n_jacks=1 -o base.stl case/case.scad`.

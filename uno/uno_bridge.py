#!/usr/bin/env python3
"""Serial -> MIDI bridge for the Arduino Uno R3 stand-in paddle (uno_paddle.ino).

Opens a virtual ALSA MIDI port named "WY6Y CW Paddle" (what AetherSDR and
CyberCW already look for) and turns the Uno's state bytes into Note 20 (dit)
and Note 21 (dah), channel 1, like the Pico firmware.

Key-down safety: both levers' state is re-sent every second, released
included; if the Uno goes quiet for 2.5 s or its port disappears, both notes
are released and the port is reopened when it comes back.

    uno_bridge.py [/dev/ttyACM0]      (default: the first Arduino by-id path)
Needs python-rtmidi and pyserial.
"""
import glob, sys, time

import rtmidi
import serial

NOTES = (20, 21)            # dit, dah
SILENT_S = 2.5              # the Uno sends at least once a second
RESYNC_S = 1.0


def find_port():
    if len(sys.argv) > 1:
        return sys.argv[1]
    ids = sorted(glob.glob("/dev/serial/by-id/usb-Arduino*"))
    return ids[0] if ids else None


def log(msg):
    print(time.strftime("%H:%M:%S"), msg, flush=True)


def main():
    out = rtmidi.MidiOut(rtmidi.API_LINUX_ALSA, name="WY6Y CW Paddle")
    out.open_virtual_port("WY6Y CW Paddle MIDI 1")
    state = [False, False]

    def send(i, on):
        out.send_message([0x90 if on else 0x80, NOTES[i], 127 if on else 0])

    def release(why):
        if any(state):
            log(f"releasing both levers: {why}")
        for i in range(2):
            state[i] = False
            send(i, False)

    log("virtual MIDI port 'WY6Y CW Paddle' open")
    while True:
        path = find_port()
        if not path:
            time.sleep(1)
            continue
        try:
            ser = serial.Serial(path, 115200, timeout=0.05)
        except serial.SerialException as e:
            log(f"can't open {path}: {e}")
            time.sleep(1)
            continue
        log(f"reading {path}")
        last_rx = time.time()       # the Uno resets on open; allow its boot
        next_resync = 0.0
        try:
            while True:
                data = ser.read(max(1, ser.in_waiting))   # returns on the first byte
                now = time.time()
                for b in data:
                    if b & 0xFC != 0x40:
                        continue
                    last_rx = now
                    for i in range(2):
                        on = bool(b >> i & 1)
                        if on != state[i]:
                            state[i] = on
                            send(i, on)
                if now - last_rx > SILENT_S:
                    release("Uno went quiet")
                    last_rx = now
                if now >= next_resync:
                    for i in range(2):
                        send(i, state[i])
                    next_resync = now + RESYNC_S
        except (serial.SerialException, OSError) as e:
            log(f"lost {path}: {e}")
            release("Uno disconnected")
            try:
                ser.close()
            except Exception:
                pass
            time.sleep(1)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass

#!/usr/bin/env python3
"""Bench test for the WY6Y CW Paddle: prints DIT/DAH down/up with timestamps.
   paddle_test.py          watch the paddle (Ctrl-C to stop)
   paddle_test.py bootsel  reboot the Pico into BOOTSEL for re-flashing"""
import glob, os, re, sys, time

NOTES = {20: "DIT", 21: "DAH"}


def find_port():
    for path in glob.glob("/proc/asound/card*/id"):
        card = re.search(r"card(\d+)", path).group(1)
        if open(path).read().strip() == "Paddle":
            return f"/dev/snd/midiC{card}D0"
    sys.exit("WY6Y CW Paddle not found (is it plugged in and flashed?)")


def main():
    port = find_port()
    if sys.argv[1:] == ["bootsel"]:
        with open(port, "wb") as fh:
            fh.write(bytes([0xF0, 0x7D, ord("W"), ord("Y"), ord("B"), 0xF7]))
        print("sent BOOTSEL; the RP2350 drive should appear in a few seconds")
        return
    print(f"listening on {port}; key the paddle (Ctrl-C to stop)")
    fd = os.open(port, os.O_RDONLY)
    t0 = time.monotonic()
    buf = b""
    while True:
        buf += os.read(fd, 64)
        while len(buf) >= 3:
            status, note, vel = buf[0], buf[1], buf[2]
            if status & 0x80 == 0:          # out of sync: drop a byte
                buf = buf[1:]
                continue
            buf = buf[3:]
            kind = status & 0xF0
            down = kind == 0x90 and vel > 0
            if kind in (0x80, 0x90):
                name = NOTES.get(note, f"note {note}")
                print(f"{time.monotonic() - t0:9.3f}s  {name:4} {'DOWN' if down else 'up'}"
                      f"  (ch {(status & 0x0F) + 1})", flush=True)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass

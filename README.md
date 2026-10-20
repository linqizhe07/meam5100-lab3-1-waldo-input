# MEAM 5100 Lab 3.1 -- Waldo input side (Adafruit ItsyBitsy 32u4)

Code for the **input (master) side** of the planar two-DOF Waldo of MEAM 5100 *Mechatronics*,
University of Pennsylvania, Fall 2026. Author: Linqi Zhe. The master arm's shoulder and elbow are
Bourns 3310 potentiometers; this repository reads them. The output (servo) side is in the separate
Lab 3.2 repository `meam5100-lab3-2-waldo-output`.

| File | Program | Report |
|---|---|---|
| `3_1_2.c` | generic ADC subroutines `adc_setup(ch)`, `adc_read()`, `adc_read_channel(ch)` with a small test `main()` | Section 3.1.2 |
| `3_1_3.c` | reads the shoulder and elbow potentiometers and prints the joint angles over USB serial (10 Hz) | Section 3.1.3 |
| `lib/m_usb.c`, `inc/m_usb.h` | course USB-serial library (J. Fiene & J. Romano, MAEVARM) | -- |
| `inc/MEAM_general.h` | course `set()` / `clear()` register macros (adapted by P. Stegall / M. Yim) | -- |

## Pins

| Joint | Potentiometer wiper |
|---|---|
| 1 -- shoulder | **A0 = PF7 / ADC7** |
| 2 -- elbow    | **A1 = PF6 / ADC6** |

Both pots run between the ItsyBitsy 5 V (USB) pin and GND (ratiometric to AVcc). Joint angle:
`theta = (adc - 512) * 270 / 1023` degrees, with the pot clamped at mid-travel when the link is straight.

## Build and flash

Requires `avr-gcc`, `avr-libc` and `avrdude` (`brew install avr-gcc avrdude` on macOS). The
`Makefile` is the course Makefile with a `PROG` variable selecting the program:

```sh
make PROG=3_1_2.c            # -> main.hex
make PROG=3_1_3.c            # default when PROG is not given
make PROG=3_1_3.c flash PORT=/dev/tty.usbmodemXXXX   # double-tap reset, then flash (avr109)
make clean
```

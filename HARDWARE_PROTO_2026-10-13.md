# Cardboard scanner prototype — electronics bring-up (2026-10-13)

This note refers to the cardboard prototype photographs shared on 2026-10-13.
It is **not** proof that the June 2026 top-card-feeder design matches the current cardboard motion mechanism. Do not enable auto-feeding yet.

## Visible parts

- Arduino Nano (ATmega328P).
- NEMA17 motor marked US-17HS4401S on the photographed unit.
- Red 16-pin breakout **appears** to be an A4988 stepper driver; verify the IC and printed labels on both sides before connecting power.
- Red screw-terminal 3-pin control board appears to be an IRF520 MOSFET module; **do not use it to drive this 4-wire bipolar stepper**.
- A four-wire motor cable (colors not a reliable pinout). A power supply, VMOT capacitor, buttons, and passage sensor are not clearly shown.

## Goal of this firmware

**One safe motor movement on demand**, using Serial Monitor. Do NOT assume the motor is detached from the mechanics: first use 1 to 10 steps while clear of cards, with room for the mechanism to move without hitting stops.

This is an electrical/mechanical checkout only. It is intentionally **not** a continuous feeder.

## Suggested minimal wiring, AFTER driver identification

| Device | Pin | Goes to |
|---|---|---|
| Arduino Nano | D2 | A4988 STEP |
| Arduino Nano | D3 | A4988 DIR |
| Arduino Nano | D4 | A4988 ENABLE (active LOW) |
| Arduino Nano | 5V | A4988 VDD (logic) |
| Arduino Nano | GND | A4988 logic GND |
| A4988 | RESET + SLEEP | Tie together to logic VDD (5V) |
| A4988 | MS1, MS2, MS3 | GND (full-step) or leave unconnected on conventional A4988 carrier with internal pulldowns |
| A4988 | VMOT | Regulated +12V separate motor supply |
| A4988 | GND beside VMOT | Motor supply negative; common GND with Nano |
| A4988 | 1A, 1B | Motor winding / coil A (verify resistance) |
| A4988 | 2A, 2B | Motor winding / coil B (verify resistance) |

Optional: add ~10k pullup resistor between EN and VDD to ensure driver stays disabled during Nano reset (firmware sets D4 HIGH at startup).

**Before VMOT is powered**: Add a 100uF electrolytic (minimum 47uF, rated 25V or higher for 12V supply) with + to VMOT and - to motor GND, physically close to the A4988. Confirm capacitor polarity. The motor supply is NEVER wired to Nano 5V or Nano VIN. The Arduino can be USB-powered during these bench tests.

## Check before switching the motor power on

1. Identify the 16 pins on the *specific* red driver board, including whether it really is A4988. Photo orientation is not sufficient.
2. Confirm the two winding pairs with a multimeter in resistance mode **while unplugged**: a winding pair reads a few ohms; unrelated wires read open circuit. Do not assume pairs from insulation colors.
3. Find the A4988 current-sense resistor values, normally marked R050/R068/R100 or similar, and set the VREF/current limit with a multimeter. Different board revisions use different values. For initial bench movements, use a conservative current setting, e.g. approximately 0.5-0.8A as supported by the driver/thermal design. See the manufacturer's specification for the actual formula and adjustment method.
4. Unplug or disconnect all power before changing motor wiring. Never unplug a motor while the A4988 has VMOT power.
5. Keep an accessible motor power switch/disconnect. Software x is NOT an emergency stop.
6. Keep fingers, cards and wires clear. Do not move more than ten steps at first, especially if the shaft is attached to a lever or spindle.

Official A4988 reference: https://www.pololu.com/product/1182 (other brands may use different current-sense values).

## Upload and run

1. Connect Nano to computer over USB.
2. Arduino IDE: select board "Arduino Nano", ATmega328P; try the "Old Bootloader" variant if upload fails (common with Nano clones). Select COM/USB port.
3. Upload `firmware/bench_stepper/bench_stepper.ino`.
4. Open Serial Monitor at **115200 baud** with line ending **Newline**.
5. With driver properly installed, capacitor fitted, power supply checked, and VREF adjusted, send:

```text
?
v 100
f 10
b 10
x
```

Each line is sent separately. `f` and `b` are relative directions; motor cable polarity and mechanical geometry determine which actually ejects the card.

## What is needed to automate cards next?

The cardboard build differs from the documented top-stack layout. Before implementing the feeder state machine, establish whether the shaft directly turns a roller, winds a cord, or actuates a lever; its travel; where the initial and terminal positions are; and whether the mechanism needs reverse/homing. We then add start/pause, exit sensing and jam limits.

Existing MVP expectations from `DESIGN_DOC.md`: SETTLE -> SCAN_DWELL -> EJECT -> VERIFY_EXIT -> RESET_FRAME -> SETTLE. Do **not** time blind auto-feeding until a dry-run can move a card out without jam or damage.

# MTG Automatic Card Scanner and Sorter

## Design Document v0.5

**Full product target:** Automatically scan, identify, inventory, and route Magic: The Gathering cards into A/B outputs.

**MVP target:** Mechanically present one card at a time to an off-the-shelf phone scanner app and move every scanned card into one output tray.

**Primary app candidate:** ManaBox in Quick Mode.

**Last updated:** 2026-06-08

---

## Table of Contents

1. [Product Direction and MVP Boundary](#1-product-direction-and-mvp-boundary)
2. [Scanner App Assumptions](#2-scanner-app-assumptions)
3. [Duplicate Card Problem](#3-duplicate-card-problem)
4. [MVP System Architecture](#4-mvp-system-architecture)
5. [Mechanical Design](#5-mechanical-design)
6. [Electronics and Firmware](#6-electronics-and-firmware)
7. [Critical Tests](#7-critical-tests)
8. [Prototype Phases](#8-prototype-phases)
9. [MVP Bill of Materials](#9-mvp-bill-of-materials)
10. [Future Expansion](#10-future-expansion)

---

## 1. Product Direction and MVP Boundary

### 1.1 Full Intended Version

The intended full product is an automatic MTG card intake machine:

```text
INPUT STACK
  -> single-card feed
  -> included scan / recognition
  -> inventory lookup and rule decision
  -> A/B splitter
  -> output A or output B
```

The full version should eventually include:

- Integrated scanning or image capture controlled by the machine.
- Card identification without relying on a separate phone scanner app.
- Inventory/export workflow tied to the recognition result.
- Configurable A/B routing rules, such as keep/sell, known/unknown, valuable/bulk, or needs-review/accepted.
- Two physical output paths with a reliable splitter or diverter.
- Scan-complete feedback before the mechanism advances.
- Better handling for duplicates, set/printing ambiguity, foils, language, and condition review.

That full product remains the north star, but it is intentionally not the first build.

### 1.2 MVP

The MVP proves the hardest mechanical and workflow assumptions before building a complete scanner/sorter.

The original design attempted to solve recognition, inventory management, feeding, and two-pile sorting at the same time. The simplified MVP delegates recognition and inventory management to an existing phone app and focuses only on the mechanical problem:

> Reliably expose exactly one card, hold it still for scanning, remove it, show a reset frame, and repeat.

MVP boundary:

| Area | Full version | MVP |
|---|---|---|
| Scanning | Included in the machine | Outsourced to phone app |
| Recognition | Custom or integrated recognition pipeline | Phone app identifies cards |
| Inventory | Built-in export/reconciliation | Phone app list reviewed by user |
| Output | A/B split into two destinations | One output tray |
| Sorting decision | Automatic rules | None |
| Machine feedback | Scan-complete signal | Fixed timing plus user review |
| Controller | Embedded controller for mechanics and routing | Arduino controls only feeding/timing/sensing |

### 1.3 MVP Included

- Phone cradle with adjustable camera alignment.
- Controlled, diffuse lighting.
- Stack holder with a repeatable scan plane.
- Single-card top-feed mechanism.
- One output tray.
- Motor control with adjustable timing.
- Card-exit sensing.
- A visible and accessible phone screen.
- Duplicate-safe workflow testing.

### 1.4 MVP Excluded

- Custom OCR or image recognition.
- Local Scryfall database.
- Custom Android or iOS app.
- Phone-to-Arduino communication.
- Two-pile sorting.
- Solenoid-actuated sorting gates.
- Automatic set, foil, language, or condition correction.
- Guaranteed autonomous operation without scanner feedback.

### 1.5 MVP Success Criteria

The MVP passes when it can:

1. Feed 100 bare cards with fewer than two jams or double-feeds.
2. Cause no visible card damage during the test.
3. Keep the complete card border visible and stationary during scanning.
4. Present an empty or fully occluded scan area between cards.
5. Scan a mixed 50-card test stack with at least 95% correct entries after review.
6. Handle consecutive identical cards through either automatic reset behavior or a documented manual tap fallback.

The first prototype targets bare, standard-size MTG cards. Sleeved, warped, foil, and damaged cards are later validation cases.

### 1.6 Current Design Direction

The MVP remains an Arduino-controlled mechanical feeder paired with an existing phone scanning app. Recognition is outsourced to the app; physical card presentation, timing, sensing, and jam handling belong to this project.

Recent concept sketches explored a document-camera-style scanner with a hopper, visible phone cradle, capture lane, positive transport rollers, and a lateral catch bin. Those sketches are useful as a solutioning style, not as final architecture:

- Model the card path as mechanism states, such as loaded, captured, reset, and offloaded.
- Prefer positive drive through the whole card path over designs where one card must push another card.
- Treat offload and output-bin geometry as first-class design problems, not afterthoughts.
- Keep the phone camera aligned over the scan area while leaving the phone screen accessible.
- Use CAD and rendered views early to catch mechanical contradictions before printing.

The SCAD concepts from that jot session are preliminary. They should inform the next mechanical iteration, but they do not replace the Arduino, NEMA 17, A4988, IR sensing, and timing-control path described below.

---

## 2. Scanner App Assumptions

### 2.1 ManaBox

ManaBox is the primary MVP app because its scanner:

- Automatically detects a card in the camera view.
- Supports continuous scanning sessions.
- Offers Quick Mode, which automatically accepts the first matching version.
- Can enable scan sounds.
- Supports set locking when scanning a known set.

Official guide:

`https://manabox.app/guides/scanner/getting-started/`

The app identifies cards mainly from artwork. Reprints with identical artwork may require later correction. The machine must not claim exact edition accuracy unless the user reviews the scanned list.

### 2.2 TCGplayer

TCGplayer is a secondary candidate. It supports one-card-at-a-time scanning and configurable verification prompting. It should be tested with the same jig before the mechanical geometry is finalized.

Official scanner settings:

`https://help.tcgplayer.com/hc/en-us/articles/115010308788-Choosing-Your-Settings`

### 2.3 TopDecked

TopDecked may be evaluated, but its public documentation does not currently provide enough detail to make automatic capture a design dependency. It is not the baseline app until tested directly.

### 2.4 App-Imposed Geometry

The jig must follow the scanner apps' common requirements:

- Entire card visible, including all four borders.
- Plain, non-patterned background with strong contrast.
- No overlapping card edges.
- Even illumination with minimal glare and shadow.
- Camera fixed perpendicular to the card.
- Phone focus and exposure kept stable.

---

## 3. Duplicate Card Problem

### 3.1 Failure Mode

Scanner apps generally avoid recording the same stationary card repeatedly. ManaBox explicitly requires the user to tap the scanner screen to scan the same card consecutively.

This creates an ambiguity:

- Card A is scanned.
- Card A is removed.
- Another copy of Card A becomes visible.
- The app may treat it as the original card and not create a second entry.

A top-of-stack design is particularly vulnerable because the next card appears immediately under the removed card, with little or no empty frame between them.

### 3.2 MVP Strategy: Force a Reset Frame

The mechanism must deliberately create a visually distinct state between cards:

```text
CARD VISIBLE -> CARD EJECTS -> EMPTY/OPAQUE RESET FRAME -> NEXT CARD VISIBLE
```

The reset frame should remain visible for a configurable duration, initially 500 ms.

Two implementations are acceptable:

1. **True empty frame:** The scanned card moves to a separate scan bed, then exits completely before the next card enters.
2. **Opaque shutter:** A matte black or white shutter briefly covers the card area between top-card ejection and exposure of the next card.

The opaque shutter is preferred for the compact top-stack jig because it preserves a constant scan plane without adding a full transport path.

### 3.3 Required Duplicate Test

Before assuming the reset frame solves the problem, test:

- Ten pairs of identical cards.
- Five runs of three identical cards.
- Reset durations of 200, 500, and 1000 ms.
- Both ManaBox Quick Mode and the selected alternative app.

Record whether each copy creates a separate scan entry.

### 3.4 Fallbacks

If the app still refuses to auto-capture identical consecutive cards:

1. **MVP fallback:** Pause feeding and let the user tap the phone screen once. The cradle must leave the screen accessible.
2. **Operational fallback:** Shuffle or separate known duplicate groups before scanning.
3. **Automation experiment:** Detect the app's scan sound. If no sound occurs after a new card settles, actuate a capacitive screen tapper.
4. **Later software option:** Build a minimal companion app or accessibility integration that provides a confirmed scan event.

A blind tap after every card is not acceptable. If a different card has already auto-scanned, an unconditional tap could add a duplicate entry.

### 3.5 Automation Limitation

Without feedback from the scanner app, the Arduino knows only that a card moved, not that it was recognized. Fixed timing can automate presentation but cannot guarantee catalog accuracy.

For the MVP, the user must remain present and review the app's scanned-card count against the physical output count.

---

## 4. MVP System Architecture

```text
                     PHONE
              camera points downward
                      |
             adjustable rigid cradle
                      |
                diffuse lighting
                      |
          +-------------------------+
          |      TOP CARD           |
          | complete border visible |
          +-------------------------+
                      |
             rubber eject roller
                      |
            single-card exit slot
                      |
               IR exit sensor
                      |
                 output tray

       Spring/elevator keeps the stack top
       at a repeatable camera focal plane.
```

### 4.1 Functional Roles

| Component | Responsibility |
|---|---|
| Phone app | Detect, identify, and record cards |
| Phone cradle | Maintain camera position and screen access |
| Lighting | Produce a consistent, low-glare image |
| Stack elevator | Maintain a constant top-card height |
| NEMA 17 + roller | Eject one top card |
| Separator lip/pad | Prevent the second card from following |
| Reset shutter | Create a distinct frame between cards |
| IR sensor | Confirm that a card left the scan position |
| Arduino Nano | Run timing, motor, sensor, pause, and fault logic |

### 4.2 Design Review Method

Each mechanical concept should be reviewed as a full card cycle, not as a static assembly. Before committing to printed parts, render or sketch at least these states:

1. **Loaded:** top card is ready at the feed plane.
2. **Captured:** one card is held still under the phone camera.
3. **Reset:** the app sees an empty, opaque, or otherwise clearly changed frame.
4. **Offloaded:** the scanned card has left the scan area and is entering or stacked in the output tray.

The review should explicitly answer:

- What positively drives the card at each point?
- Where can a second card start to follow the first?
- Can the scanned card jam against a wall, lip, detent, or previous card?
- Does the output tray accept the second, third, and fiftieth card as cleanly as the first?
- Does any roller, guide, or shutter occlude the artwork while the app is supposed to scan?

### 4.3 Operating Cycle

```text
1. Hold the top card stationary.
2. Wait for the scanner dwell time.
3. Eject one card sideways.
4. Confirm exit with the IR sensor.
5. Show an empty or opaque reset frame.
6. Let the next card rise and settle.
7. Repeat.
```

Initial conservative timings:

| Stage | Initial value | Adjustable range |
|---|---:|---:|
| Card settle | 500 ms | 200-1500 ms |
| Scan dwell | 1500 ms | 500-5000 ms |
| Eject motion | Calibrated by steps | Per mechanism |
| Reset frame | 500 ms | 200-1500 ms |
| Exit timeout | 1500 ms | 500-3000 ms |

Speed is secondary to feed reliability and correct inventory count.

---

## 5. Mechanical Design

### 5.1 Recommended Layout: Top-Card Ejection

Cards sit face-up in a vertical hopper. A spring-loaded platform or weighted elevator pushes the stack upward against fixed height stops. A rubber roller contacts only the top card and pushes it sideways into an output tray.

Advantages:

- Phone remains above the cards with its screen accessible.
- No transparent scan window.
- No camera below the table.
- Simple output path.
- Easy to observe jams and card movement.

Risks:

- Top card rubs across the next card.
- Roller pressure changes as the stack changes.
- Warped cards may not remain flat.
- Double-feeds require careful separator tuning.
- The next card appears immediately unless a shutter creates a reset frame.

### 5.2 Stack Elevator

The top card must stop at the same height regardless of stack size.

Prototype options:

1. Compression springs under a moving platform.
2. Gravity weight and pulley.
3. Constant-force spring.

Start with two or four light compression springs and a guided platform. Use adjustable hard stops to define the final card height rather than relying on spring force alone.

### 5.3 Drive Mechanism

The first motorized MVP can start with a single rubber eject roller, but the design should not assume that a single roller is sufficient until the full card path is tested.

Initial geometry:

- Silicone or urethane roller, approximately 20-30 mm diameter.
- Soft surface with low contact pressure.
- Roller axis perpendicular to the ejection direction.
- Contact near the card centerline to reduce rotation.
- NEMA 17 driven directly or through a printed hub.

The roller should rotate only during ejection. Continuous spinning increases card wear.

If the card loses positive drive before it is fully offloaded, evaluate a two-stage transport:

- A separator roller at the hopper exit pulls exactly one card from the stack.
- A downstream transport roller or paired edge rollers continue moving the same card through the scan lane and into the output path.
- The rollers are spaced so the card is always controlled by at least one driven contact point.
- Any edge rollers must avoid hiding card artwork or border areas required by the scanner app during the scan dwell.

This is a candidate refinement, not yet a locked requirement. Its purpose is to avoid fragile "card pushes previous card" behavior.

### 5.4 Single-Card Separation

Use a height-adjustable exit lip with a gap slightly above one card thickness and below two-card thickness.

Add a low-friction support surface and a small high-friction separation pad near the exit if needed. The separator must be replaceable because material choice will require experimentation.

Do not tune the slot against a single card. Test thickness variation across at least 50 cards.

Avoid any separator, lip, or detent that the card must climb after the scan. A ridge that is harmless in CAD can still buckle a thin card if the card is pushed horizontally into it.

### 5.5 Reset Shutter

The shutter provides a matte, uniform view that is visually unlike any card.

Requirements:

- Covers the complete card detection area.
- Does not touch the phone.
- Does not scrape the visible card.
- Moves only after the ejected card clears.
- Retracts before the next scan dwell begins.
- Can remain open manually during setup.

Possible mechanisms:

- A thin sliding plate linked to the eject carriage.
- A rotating flag driven by a small servo.
- A passive flap moved by the exiting card and spring-returned.

The passive flap is the first concept to test because it avoids another motor. If it does not produce a long enough reset frame, use an actively timed servo or linkage.

If a separate capture lane is used instead of scanning the top card directly, the reset frame may be created by fully clearing the scan area before the next card arrives. This must be proven with duplicate-card tests rather than assumed.

### 5.6 Offload and Output Tray

The output tray must be designed as part of the feed path. A tray that works for the first card can still jam once cards begin stacking.

Requirements:

- The scanned card must be positively driven or guided until it is committed to the output path.
- The card should not be pushed flat into a vertical wall.
- Entry into the output tray needs clearance for an accumulating stack.
- The tray should register cards neatly enough that they do not rebound into the feed path.
- The IR exit sensor should count a card only after it has cleared the scan position, not merely after it begins moving.

Possible implementations:

1. **Direct side output:** The eject roller drives the card completely out of the scan area into a shallow tray.
2. **Capture lane with drop or chute:** A downstream roller drives the scanned card off the lane, then gravity guides it into a lower catch bin.
3. **Opaque reset shutter with top-stack scan:** The card exits directly while the shutter creates the reset frame over the next top card.

The chute/bin concept from the preliminary SCAD sketches is worth exploring, but only after verifying that the second and later cards do not collide, shingle, or jam at the tray mouth.

### 5.7 Phone Cradle

Requirements:

- Fits phones approximately 60-85 mm wide.
- Adjustable camera aperture for different lens positions.
- Camera approximately 100-150 mm above the card.
- Phone remains removable without losing the mechanical calibration.
- Screen remains visible and tappable.
- Charging cable pass-through.
- Foam pads prevent movement and scratching.
- Fine X/Y adjustment for centering.

The cradle should be treated like a small copy stand. The phone body does not need to be centered over the card; the rear camera does. Account for corner-mounted phone cameras with an adjustable aperture or sliding phone stop.

### 5.8 Lighting

The current single WS2812 module is useful only for early experiments. It is not enough for uniform card illumination.

Preferred lighting:

- Diffuse white LED ring or multiple white LEDs around the scan area.
- Light positioned at a shallow angle rather than reflected directly into the camera.
- Matte white diffuser.
- Matte dark background around the card border.

Foils should be tested separately because they may require lower brightness or asymmetric lighting.

### 5.9 Bottom-Up Alternative

A bottom-up design would place the phone below a transparent surface and remove cards from the bottom of the stack.

It is not recommended for the first prototype because:

- Acrylic or glass adds reflections.
- Cards can scratch against the window.
- Lighting and screen access are harder.
- The phone mount becomes less convenient.
- Bottom-card separation is harder to observe and tune.

It remains a fallback if maintaining constant top-card height proves unreliable.

---

## 6. Electronics and Firmware

### 6.1 Required Electronics

- Arduino Nano.
- NEMA 17 stepper motor.
- A4988 stepper driver.
- Regulated motor power supply.
- At least 47-100 uF electrolytic capacitor near A4988 VMOT.
- IR emitter and receiver with suitable resistors.
- Start/pause button.
- Emergency stop or easy power disconnect.
- Optional speed/dwell potentiometer.

The purchased NEMA 17 is rated at 1.4 A. A generic A4988 should initially be current-limited below that value, typically around 0.8-1.0 A unless cooling and the specific board rating are verified. Set the current limit with a multimeter before running the motor under load.

### 6.2 Existing Parts Not Required for the MVP

- Two mini solenoids.
- Two 130 DC motors.
- L298N motor driver.
- IRF520 module.
- Additional A4988 drivers.

Keep these for experiments and later sorting, but do not add them to the first scanner feeder.

### 6.3 Firmware State Machine

```text
IDLE
  -> SETTLE
  -> SCAN_DWELL
  -> EJECT
  -> VERIFY_EXIT
  -> RESET_FRAME
  -> SETTLE

Any state
  -> PAUSED
  -> FAULT when exit confirmation times out
```

The firmware does not attempt to identify cards or infer scanner success.

### 6.4 Controls

Minimum controls:

- Start/pause button.
- Single-step button for setup and recovery.
- Dwell-time adjustment in firmware or by potentiometer.
- Status LED:
  - Green: running.
  - Yellow: paused.
  - Red: exit timeout or jam.

The single-step control is important for duplicate-card and lighting tests.

---

## 7. Critical Tests

### P1: Manual Scanner Baseline

**Goal:** Prove the chosen app and phone can scan reliably before motorizing anything.

Procedure:

1. Fix the phone above a plain background.
2. Scan 50 varied cards manually.
3. Record correct card name, correct printing, scan time, and failures.
4. Repeat with ten foil cards.

Pass criteria:

- At least 95% correct card names.
- Acceptable workflow in Quick Mode.
- Full-card border consistently detected.

### P2: Consecutive Duplicate Behavior

**Goal:** Determine exactly when the app requires a screen tap.

Procedure:

1. Present identical cards without an empty frame.
2. Repeat with 200, 500, and 1000 ms empty frames.
3. Repeat using an opaque shutter.
4. Test pairs and triples.
5. Record whether each copy appears as a separate entry.

Pass criteria:

- Preferred: 100% duplicate copies recorded through reset-frame behavior.
- Acceptable MVP: user can pause and tap without removing the phone.

This test determines whether a screen-tapper experiment is necessary.

### P3: Single-Card Feed Reliability

Procedure:

1. Run 100 bare cards without scanning.
2. Count successful feeds, double-feeds, jams, rotations, and visible damage.
3. Repeat after changing stack size from 100 cards to 10 cards.

Pass criteria:

- Fewer than two feed failures per 100 cards.
- No visible damage.
- Exit sensor count matches physical output count.

### P3A: Offload and Output Tray Reliability

**Goal:** Prove that the scanned card leaves the scan area and stacks without relying on fragile card-to-card pushing.

Procedure:

1. Run at least 50 dry-feed cycles with the phone removed.
2. Observe the first, second, third, tenth, and fiftieth card entering the output tray.
3. Intentionally stop with cards partially in the output path to check recovery.
4. Repeat with a small stack already in the output tray.

Pass criteria:

- No card stalls at a lip, wall, detent, or tray entry.
- No shingling that blocks the next feed.
- Output cards remain contained and do not rebound into the scan area.
- IR count matches the number of cards that fully cleared the scan position.

### P4: Camera Plane Repeatability

Procedure:

1. Photograph a printed alignment grid.
2. Remove and replace the phone 20 times.
3. Compare card-border position and focus.

Pass criteria:

- Card remains fully within the app's scan frame every time.
- No manual camera repositioning is needed after normal phone removal.

### P5: Integrated Scan Count

Procedure:

1. Prepare a known 50-card stack containing unique cards and duplicate runs.
2. Record the expected card sequence and quantity.
3. Run the feeder.
4. Compare physical output count, IR count, and app entry count.

Pass criteria:

- Physical output count equals IR count.
- App entry count equals physical count after any documented manual duplicate taps.
- At least 95% correct card names after review.

---

## 8. Prototype Phases

### Phase 0: Manual Jig

Build:

- Fixed phone stand.
- Plain scan background.
- Temporary diffuse lighting.
- Manual card placement.
- Simple CAD or sketch model of the proposed card path.

Validate:

- ManaBox Quick Mode workflow.
- Card distance, focus, and exposure.
- Duplicate-card reset behavior.
- Required reset-frame duration.
- Card-path states before committing to hardware: loaded, captured, reset, and offloaded.

No Arduino or motor is needed in this phase.

### Phase 1: Manual Feeder Mechanics

Build:

- Hopper.
- Guided spring elevator.
- Adjustable top stop.
- Hand-turned rubber roller.
- Separator lip.
- Output tray.
- Mock capture lane, shutter, or offload path.

Validate:

- Single-card separation.
- Card wear.
- Stack pressure from 100 to 10 cards.
- Passive reset shutter concept.
- Output tray behavior across repeated cards, not only the first card.
- Whether positive downstream transport is needed before motorizing.

### Phase 2: Motorized Feeder

Add:

- NEMA 17.
- A4988 with current limiting.
- Arduino Nano.
- Start/pause and single-step controls.
- IR exit sensor.

Validate:

- 100-card dry feed.
- Jam timeout.
- Adjustable dwell and reset timing.

### Phase 3: Integrated Scanner MVP

Run:

- 50-card mixed scanning test.
- Consecutive duplicate tests.
- Foil and warped-card tests.
- Review workflow and count reconciliation.

Deliverable:

- One input stack becomes one scanned output stack.
- No custom phone software.
- Manual tap fallback remains available.

### Phase 4: Optional Duplicate Automation

Only if reset-frame testing fails:

- Add a sound sensor near the phone speaker.
- Detect successful scan sounds.
- If no sound occurs by timeout, pause or actuate a capacitive tapper.
- Prevent unconditional taps that could duplicate an already scanned card.

This phase is optional and must not delay the mechanical MVP.

---

## 9. MVP Bill of Materials

This inventory is based on the Mercado Livre order list supplied on 2026-06-06. A purchase is counted as available only when its listing clearly identifies the required part. The contents of assortment kits must be inspected before relying on them.

### 9.1 Procurement Summary

| Area | Status | Main gap |
|---|---|---|
| Controller and stepper | Mostly complete | Multimeter, power supply, and VMOT capacitor |
| Card-exit sensing | Parts available for experiment | Resistor values and mechanical sensor mount |
| Mechanical feeder | Not ready | Roller, hub, springs, structure, guides, and fasteners |
| Phone scanning station | Partially ready | Cradle, background, adequate white lighting, and diffuser |
| Assembly tools | Basic tools available | Multimeter still required |
| Duplicate-card handling | Not built | Passive or active reset shutter |

The electronics can be tested on a breadboard after obtaining the power and protection items. The complete mechanical MVP cannot yet be assembled.

### 9.2 Confirmed Purchased and Useful for the MVP

| Purchased item | Quantity | MVP use | Delivery/status |
|---|---:|---|---|
| Arduino Nano compatible V3, ATmega328P, cable included | 1 | Main feeder controller | Delivered 2026-06-06 |
| NEMA 17 stepper, 4.2 kgf, 1.4 A | 1 | Drive the top-card eject roller | Delivered 2026-06-06 |
| A4988 stepper drivers | 4 | One required; three spares | Delivered 2026-06-06 |
| IR emitter/receiver pairs, 5 mm | 10 pairs | Exit sensor and bench experiments | Delivered 2026-06-05 |
| 400-point breadboard, jumper set, and LEDs | 1 kit | Initial circuit prototype | Delivered 2026-06-06 |
| Male-male, male-female, and female-female jumpers | 120 total | Prototype wiring | Delivered 2026-06-06 |
| Colored heat-shrink tubing kit | 530 pieces | Insulate final wiring | Delivered 2026-06-06 |
| Addressable WS2812 RGB LED module | 1 | Early lighting and firmware test only | Delivered 2026-06-05 |
| Arduino/RPi component assortment FK3-E33 | 1 kit | May supply small passive parts and controls | Delivered 2026-06-05; inspect contents |
| Portable wire stripper | 1 | Wiring assembly | Delivered 2026-06-06 |
| Rechargeable USB soldering iron | 1 | Wiring assembly | Delivered 2026-06-06 |

### 9.3 Purchased but Not Required for the Scanner MVP

| Purchased item | Quantity | Current disposition |
|---|---:|---|
| Mini push-pull solenoids, 6 V | 2 | Reserve for a future active shutter or sorter |
| L298N dual H-bridge | 1 | Reserve for future DC-motor experiments |
| IRF520 PWM driver module | 1 | Not needed; also not preferred for low-voltage logic switching |
| 130-size 3-6 V DC motors | 2 | Reserve for later mechanisms; order receipt was awaiting confirmation |

These parts should not be included in the first build merely because they are available. The simplest MVP uses one stepper motor and one A4988.

### 9.4 Inspect the Component Kit Before Buying

The FK3-E33 kit may contain some of the following, but the product title does not establish quantities, values, or ratings.

| Item to check | Minimum needed | Acceptable for MVP |
|---|---:|---|
| Electrolytic capacitor for A4988 VMOT | 1 | 47-100 uF or larger, rated at least 25 V |
| IR emitter series resistor | 1 | Start around 100-220 ohm, then verify experimentally |
| IR receiver pull-up/pull-down resistor | 1 | Common values around 4.7-10 kohm |
| Momentary push buttons | 2 | One start/pause and one single-step |
| Potentiometer | 1 optional | Approximately 10 kohm for dwell adjustment |
| Buzzer | 1 optional | Jam/fault notification |
| Status LEDs and resistors | 3 optional | Green, yellow, and red indication |

Do not purchase duplicates until the kit is opened and these parts are identified.

### 9.5 Missing Required Electronics and Safety Items

| Missing item | Quantity | Suggested specification | Why required |
|---|---:|---|---|
| Digital multimeter | 1 | DC voltage and resistance measurement | Set A4988 current limit and verify power rails |
| Motor power supply | 1 | Regulated 12 V, approximately 3-5 A | Power the NEMA 17 through the A4988 |
| A4988 VMOT capacitor | 1 | 47-100 uF minimum, 25 V or higher | Suppress motor-supply spikes; omit only if confirmed in the kit |
| Power switch or accessible disconnect | 1 | Rated for the selected supply | Stop the mechanism safely |
| Screw terminals/connectors | As needed | Rated for motor current | Secure power and motor wiring |

The Arduino may be powered by USB during bench testing. Do not power the NEMA 17 from the Arduino USB or 5 V pin.

### 9.6 Missing Required Mechanical Items

| Missing item | Quantity | Suggested specification | MVP use |
|---|---:|---|---|
| Rubber eject roller | 1 | Silicone or urethane, approximately 20-30 mm diameter | Grip and move the top card |
| Roller hub or shaft coupler | 1 | Match the NEMA 17 shaft, normally 5 mm | Connect motor to roller |
| Compression springs | 2-4 matched | Light springs with enough travel for the intended stack | Raise the card platform |
| Guided elevator platform | 1 assembly | Printed platform with two or four guides | Keep the stack level |
| Adjustable separator lip/pad | 1 assembly | Replaceable, fine height adjustment | Prevent double-feeds |
| Reset shutter/flap | 1 | Matte and large enough to cover the scan area | Reset the app between cards |
| Output tray | 1 | Low-friction card-safe surface | Receive ejected cards |
| Phone cradle | 1 | Adjustable width and camera position | Hold phone above scan plane |
| Foam or rubber cradle pads | As needed | Non-marking and compressible | Prevent phone movement and scratches |
| Fasteners | Assortment | M3/M4 screws, nuts, washers, and inserts | Assemble and adjust the mechanism |
| PLA or PETG | Approximately 0.5-1 kg | Printed prototype structure | Hopper, guides, cradle, and shutter |
| 3D printer access | 1 | Owned, borrowed, or printing service | Produce mechanical parts |

### 9.7 Missing Scan-Station Items

| Missing item | Quantity | Suggested specification | Why required |
|---|---:|---|---|
| Diffuse white lighting | 1 set | White LED ring or multiple white LEDs | Uniform card illumination |
| Light diffuser | 1 | Matte translucent plastic, paper, or fabric prototype | Reduce hotspots and foil glare |
| Matte scan background | 1 | Plain dark or light sheet contrasting with card borders | Help the app detect the complete card |
| Phone and charger | 1 | Existing compatible phone is acceptable | Run the scanner app |
| Test-card stack | At least 100 | Include duplicates, foils, and warped cards | Validate feed and scan behavior |

The purchased single WS2812 module is not sufficient as final lighting because it cannot illuminate a full card uniformly.

### 9.8 Optional Items

| Optional item | Purpose |
|---|---|
| 10 kohm potentiometer | Adjust scan dwell without reprogramming |
| Buzzer | Audible jam warning |
| Small servo | Actively time the reset shutter if a passive flap fails |
| Sound sensor or microphone module | Detect the scanner app's successful-scan sound |
| Capacitive screen tapper mechanism | Handle consecutive duplicates only if reset-frame tests fail |
| Limit switch | Detect elevator position or open access panels |

### 9.9 Minimum Next Purchase

To progress from parts inventory to a motorized bench prototype, obtain or confirm:

1. Digital multimeter.
2. Regulated 12 V power supply.
3. Suitable A4988 VMOT capacitor.
4. One rubber roller and matching NEMA 17 hub.
5. Two start/control buttons unless present in the component kit.
6. Springs, fasteners, filament, and printer access.
7. Diffuse white lighting and matte background material.

After those items are available, the remaining work is primarily CAD, mechanical tuning, firmware, and validation rather than additional electronics procurement.

---

## 10. Future Expansion

After the scanner feeder is reliable, possible upgrades are:

1. Sound-gated duplicate handling.
2. Sleeved-card support.
3. Better foil lighting.
4. Automatic jam recovery.
5. Two-pile sorting after scanning.
6. Custom companion app with explicit scan-complete events.
7. Local OCR or image matching only if third-party apps become limiting.

The two-pile sorter remains a future project, not an MVP dependency. The current priority is proving that cards can be presented, scanned, counted, and removed reliably without damage.

# Deck Sorter

Design notes for an automatic Magic: The Gathering card scanner and sorter.

## Full Product

The intended full version automatically feeds, scans, identifies, inventories, and routes MTG cards into A/B outputs. It should eventually include machine-controlled scanning, card recognition, configurable routing rules, and a reliable two-output splitter.

## MVP

The MVP deliberately narrows that goal. It uses an Arduino-controlled mechanical feeder to present one card at a time to an off-the-shelf phone scanner app, then moves every scanned card to a single output tray.

For the MVP, recognition, inventory management, and A/B sorting are intentionally left out. The immediate job is to prove reliable card presentation, timing, reset behavior, offload, and count reconciliation.

Current mechanical exploration is CAD-first and state-based: every feeder concept should show how a card is loaded, captured under the camera, reset for the scanner app, and offloaded into the output tray. Early SCAD sketches are preliminary solutioning references, not the final design.

See [DESIGN_DOC.md](DESIGN_DOC.md) for the full design document.

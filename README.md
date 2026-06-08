# Deck Sorter

Design notes for an automatic Magic: The Gathering card scanner feeder.

The current MVP focuses on an Arduino-controlled mechanical feeder that presents one Magic: The Gathering card at a time to an off-the-shelf phone scanner app, then moves the card to a single output tray. Recognition, inventory management, and sorting are intentionally left to existing apps and later iterations.

Current mechanical exploration is CAD-first and state-based: every feeder concept should show how a card is loaded, captured under the camera, reset for the scanner app, and offloaded into the output tray. Early SCAD sketches are preliminary solutioning references, not the final design.

See [DESIGN_DOC.md](DESIGN_DOC.md) for the full design document.

<img width="997" height="608" alt="RF_Snipper_Lite" src="https://github.com/user-attachments/assets/6fa77260-6db9-4701-85f4-1738003c9387" />

# RF Sniper Lite

The first RF analyzer in the RF Sniper family.

RF Sniper Lite is a compact RF measurement instrument built around an ATmega8 microcontroller, a 16x2 HD44780 LCD, and a Si5351 synthesizer. It is designed as a lightweight, portable RF analyzer for quick visual inspection of antenna behavior, SWR trends, and frequency response.

This project represents the smallest and most compact version of the original RF Sniper concept: a minimal but practical instrument built with very limited hardware resources, yet capable of producing a useful RF sweep and a clear SWR indication.

---

## Overview

RF Sniper Lite is not intended to replace a modern vector analyzer or a full-featured spectrum analyzer. Instead, it focuses on a different goal: providing a fast, intuitive, and compact way to observe how an antenna behaves across a frequency range using only a minimal embedded platform.

The device is designed around:

- ATmega8 as the main controller
- HD44780 16x2 LCD as the user display
- Si5351 as the synthesizer / RF source
- resistive 50 Ω bridge for directional measurement
- ADC input pair for FWD and REV signals
- rotary encoder for navigation and control
- a single multifunction button for user interaction

---

## Hardware

- ATmega8, final version running at 8 MHz
- HD44780 16x2 LCD
- Si5351 frequency synthesizer
- 50 Ω resistive bridge
- Two ADC inputs for FWD and REV
- Rotary encoder
- Single multifunction button

---

## RF Micrograph Display

The LCD uses 8 custom CGRAM characters to create a micrograph with 24 independent RF points:

- 8 custom characters
- 3 micro-bars per character
- total of 24 RF points

The graph is updated progressively, and the marker can be moved across all 24 positions.

This design is a key feature of the project: it pushes the ATmega8 to its functional limits while still delivering a readable and practical RF display using a very small display area.

---

## Marker Function

The marker is a vertical dotted XOR line overlaid directly on the selected micro-bar.

The rotary encoder moves the marker point by point, and the display shows the corresponding frequency and SWR value.

This allows the user to inspect the response at precise points in the sweep without needing a large graphical display or a more complex UI.

---

## Normal Sweep and Panning

In SINGLE mode, after a sweep completes, the marker moves across the 24 points.

When the marker reaches the edge, continued rotation of the encoder produces edge-panning: the graph shifts by one position and a new section of the sweep becomes visible.

This gives the user a simple but effective way to inspect a wider frequency range using a compact display.

---

## RF Range

The instrument operates over a restricted RF range:

- 1 MHz to 50 MHz

This range is appropriate for the hardware and the intended use case: compact, practical antenna analysis with minimal parts count.

---

## Continuous Scan

A long press on the button enters continuous scan mode.

In this mode:

- the graph is rewritten progressively without full clearing
- a XOR cursor acts as a scan head
- the encoder can move the center frequency
- the display continuously refreshes without losing the motion feel of a live sweep

This mode is especially useful for checking the relative response of an antenna or tuner in real time.

---

## Minimum SWR Detection

After each completed sweep, the firmware automatically determines the minimum SWR and the corresponding frequency.

In continuous scan mode, the current minimum SWR value and the frequency at which it was found are displayed.

This makes the device useful not only as a visual indicator but also as a practical tuning aid.

---

## STEP and SPAN

The current implementation is adapted to the 24-point micrograph display.

- STEP defines the frequency increment between micro-bars
- SPAN is derived from the geometry of the 24-point display

This keeps the instrument simple while preserving a useful representation of frequency behavior across a selected band.

---

## User Interface

The device keeps a deliberately minimal interface:

- rotary encoder
- one multifunction button
- 16x2 LCD display

Despite this simplicity, it provides:

- progressive sweep display
- marker navigation
- panning across the graph
- continuous scan mode
- automatic minimum SWR detection

---

## Resource Usage

The final version uses almost the entire flash memory of the ATmega8, reaching roughly 99% utilization.

At the same time, SRAM remains within comfortable limits.

This makes the project a good example of how far the ATmega8 can be pushed in a practical RF instrumentation application.

---

## Project Philosophy

RF Sniper Lite does not try to replace a modern vector network analyzer or a high-end spectrum analyzer.

Its purpose is simple: to provide a fast, responsive, and intuitive view of antenna behavior using the smallest possible hardware footprint and a very compact display.

It focuses on practical field use, low complexity, and the essential RF information needed for tuning and evaluation.

This first device later evolved into RF Sniper 2 and RF Sniper 3. However, RF Sniper Lite remains the clearest expression of the original idea: a very small RF instrument built from minimal resources but still useful in real-world operation.

---

## Project Evolution

RF Sniper Lite is the first project in the RF Sniper family.

From this original concept, the platform evolved into:

- RF Sniper 2
- RF Sniper 3

Each generation expanded capability and complexity, but RF Sniper Lite remains the compact reference design from which the larger versions evolved.

---

## Status

This project is a compact experimental RF measurement tool built for practical antenna observation and tuning support.

It is intended for:

- antenna inspection
- frequency sweep visualization
- SWR monitoring
- educational RF work
- compact, low-cost RF prototyping

---

## License

This project is released under the terms of the repository license.

See the LICENSE file for full licensing details.

---

## Notes

RF Sniper Lite is not a laboratory-grade instrument, but it is a valid example of how a very small and constrained embedded platform can still be used to display meaningful RF behavior.

It remains a compact, useful, and historically important step in the RF Sniper family of projects.

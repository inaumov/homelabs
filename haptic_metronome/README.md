# ESP32 Haptic Metronome

## Overview

The ESP32 Haptic Metronome is a microcontroller-based device that provides rhythmic feedback through vibration instead of relying exclusively on audible clicks. An OLED display provides visual feedback, while a potentiometer allows the user to adjust the tempo.

The project is built around an ESP32 development board and uses a non-blocking programming approach to handle button input, tempo adjustment, display updates, and vibration timing.

## Features

- **Adjustable tempo:** 40-200 BPM, controlled by a potentiometer.
- **Haptic feedback:** A vibration motor produces a short pulse for every beat.
- **OLED interface:** Displays a welcome screen, the current tempo, and the active beat.
- **Four-beat visualization:** Four dots represent a 4/4 measure. The active beat is highlighted.
- **Button-controlled operation:** A single button cycles through the operating states.
- **Non-blocking execution:** Uses `millis()` instead of `delay()` to manage timing without blocking the main loop.
- **Filtered analog input:** An exponential moving average smooths potentiometer readings.
- **Button debouncing:** Software filtering prevents a single physical press from being interpreted as multiple presses.

## Hardware Components

| Component | Purpose |
|---|---|
| ESP32 DevKit | Main controller |
| SSD1306-compatible OLED, 128x64 | Visual interface |
| Potentiometer | Tempo adjustment |
| Momentary push button | State transitions |
| Vibration motor or motor driver module | Haptic feedback |
| Breadboard and jumper wires | Prototyping and connections |

### Software Dependencies

- Arduino framework for ESP32
- `Wire.h` — I²C communication
- `Adafruit_GFX.h` — graphics primitives and text rendering
- `Adafruit_SSD1306.h` — OLED display driver

## Current Wiring

| Component | Pin / Connection | ESP32 |
|---|---|---|
| OLED | SDA | GPIO 21 |
| OLED | SCL | GPIO 22 |
| OLED | VCC | 3.3 V |
| OLED | GND | GND |
| Potentiometer | Middle pin (wiper) | GPIO 34 |
| Potentiometer | Outer pins | 3.3 V and GND |
| Push button | One side | GPIO 19 |
| Push button | Other side | GND |
| Vibration control | Control signal | GPIO 18 |

The button uses the ESP32's internal pull-up resistor (`INPUT_PULLUP`). Its input is `LOW` when pressed.

The potentiometer is read using the ESP32's 12-bit ADC. The application maps the readings to the configured tempo range of 40-200 BPM.

**Motor safety:** GPIO 18 is a control output. A bare vibration motor must not be powered directly from an ESP32 GPIO; use a suitable transistor or MOSFET driver and appropriate inductive protection. The wiring above assumes a compatible motor driver or module where required.

## State Machine

The application uses four states:

`OFF → WELCOME → READY → METRONOME → OFF`

### 1. OFF

- The OLED display is cleared.
- The vibration motor is turned off.
- The controller remains powered and continues executing the main loop.

A button press transitions the application to `WELCOME`.

### 2. WELCOME

- Displays `Privet:)` and `ESP32 METRONOME`.
- The welcome screen remains visible for 3 seconds.
- Button presses are ignored during this state.
- After the timeout, the application automatically transitions to `READY`.

### 3. READY

- Displays the current BPM.
- The potentiometer can adjust the tempo.
- The metronome is not producing rhythmic vibration.

A button press starts the metronome and transitions to `METRONOME`.

### 4. METRONOME

- Displays the current BPM.
- Shows four dots representing the beats of a 4/4 measure.
- Highlights the active beat.
- Produces one vibration pulse per beat.
- The first beat is triggered immediately when the metronome starts.

A button press stops the vibration and returns the application to `OFF`.

## Timing and Input Handling

The main loop repeatedly processes input, state transitions, beat timing, and motor control without blocking delays.

### Tempo Calculation

The potentiometer is sampled every 20 ms. Its ADC readings are smoothed using an exponential moving average:

```cpp
filteredAdc = filteredAdc * 0.95f + rawAdc * 0.05f;
```

The filtered value is mapped to a BPM range of 40-200. Whenever the resulting integer BPM changes, the application updates the beat interval:

```cpp
beatInterval = 60000UL / bpm;
```

The beat interval is expressed in milliseconds.

### Beat Scheduling

The metronome uses `millis()` to determine when the next beat is due. When the elapsed time reaches the current beat interval, the application advances the beat indicator and triggers a vibration pulse.

### Vibration Control

The motor is activated for 100 ms per beat. Its timeout is handled independently of the main loop, allowing the ESP32 to continue processing inputs and updating the display while the motor is active.

### Button Debouncing

The button uses a 50 ms debounce interval. With `INPUT_PULLUP`, a transition to `LOW` indicates a press.

## Current Limitations

- The current rhythm visualization supports a fixed 4/4 measure.
- Vibration occurs on each beat; subdivisions such as off-beat or eighth-note pulses are not currently implemented.
- The `OFF` state disables the metronome's output and clears the display but does not physically disconnect power.
- The current tempo range and vibration duration are configured in the firmware.

## Project Goal

The project aims to provide a simple, wearable-compatible metronome that combines adjustable tempo, visual beat tracking, and tactile rhythm feedback. Its non-blocking architecture provides a foundation for future improvements, such as additional rhythm patterns, power management, and battery operation.

# Advanced Functional and Tactical Flashlight Program for Hexbright (`funtactic`)

An advanced everyday-carry (EDC) and tactical firmware combining stepped brightness, accelerometer-based tilt control, direct tactical shortcuts, pocket lockout, and battery/thermal safety.

---

## Operating Modes

| Mode | Description |
| :--- | :--- |
| `OFF_MODE` | Main LED off, system in low-power standby |
| `MIN_MODE` | Main LED at **12.5%** |
| `LOW_MODE` | Main LED at **25%** |
| `MID_MODE` | Main LED at **50%** |
| `HIGH_MODE` | Main LED at **75%** |
| `MAX_MODE` | Main LED at **100% (Turbo)** |
| `ACCEL_MODE` | Dynamic brightness adjusted continuously by flashlight tilt angle |
| `ACCEL_SET_MODE` | Hands-free hold of customized tilt brightness |
| `STROBE_MODE` | Dynamic strobe flash rate adjusted by flashlight tilt angle |
| `STROBE_SET_MODE` | Hands-free hold of chosen strobe frequency |

---

## Key Features & Shortcuts

### 1. Direct Shortcuts (From OFF)
* **Single Click**: Turns on into `MIN_MODE` (12.5%).
* **Double-Click**: Jumps straight into **100% Turbo (`MAX_MODE`)** from OFF or from any active mode.
* **Triple-Click**: Jumps straight into **Direct Tactical Strobe (`STROBE_SET_MODE`)** at ~13 Hz hands-free.
* **Press & Hold (> 500 ms)**: Enters live tilt-adjustable `STROBE_MODE`. Releasing locks in the frequency.

### 2. Timeout-to-Off (Direct-Off)
* Quick clicks within **1.2 seconds** advance through brightness levels (`MIN` $\to$ `LOW` $\to$ `MID` $\to$ `HIGH` $\to$ `MAX` $\to$ `OFF`).
* Once the light has been active for **longer than 1.2 seconds**, the next single short click turns it **straight OFF**.
* A medium press (300–500 ms) also shuts down immediately at any time.

### 3. Pocket Lockout
* **4 Rapid Clicks from OFF**: Toggles Lockout mode.
  * Entering Lockout: confirmed by **3 Red flashes** on the tailcap.
  * Exiting Lockout: confirmed by **3 Green flashes** on the tailcap.
* While locked, accidental button presses are ignored and blink the **tailcap Red LED 2 times** (*"Locked"*), preventing bag/pocket overheating and battery drain.

### 4. Tilt Brightness & Frequency Control
* **Tilt Dimming (`ACCEL_MODE`)**: From any active stepped mode, press and hold (> 500 ms) to enter tilt control. Tilt to adjust brightness; release to lock in (`ACCEL_SET_MODE`).
* **Tilt Strobe Tuning (`STROBE_MODE`)**: Press and hold (> 500 ms) while strobing to tilt-tune the frequency. Release to lock in (`STROBE_SET_MODE`).

### 5. Thermal & Battery Management
* **Internal Celsius Temperature Blinking**: Under normal operation, the rear LEDs continuously blink internal temperature (**tens on Green LED, ones on Red LED**).
* **Low-Battery Auto Step-Down & Warning**: When the 18650 cell drops below 3.2V under load, high modes (`MAX` or `HIGH`) automatically step down to `MID_MODE` (50%) to prevent voltage collapse, and the tail Red LED pulses every 1.5 seconds.
* **Breathing Charge Glow (`GLOW_CHARGE`)**: When plugged into USB, the tailcap Green LED smoothly dims on and off (breathes) rather than harsh blinking.

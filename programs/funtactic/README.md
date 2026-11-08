# Advanced Functional and Tactical Flashlight Program for Hexbright (`funtactic`)

An advanced everyday-carry (EDC) and tactical firmware combining stepped brightness, accelerometer-based tilt control, direct tactical shortcuts, pocket lockout, and real-time thermal/battery telemetry.

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

### 1. Direct Shortcuts (From OFF — Dark Settling)
While tapping from OFF, the main beam stays dark so multi-clicks don't pre-flash. After **280 ms**:
* **1 Click**: Turns on cleanly into `MIN_MODE` (12.5%).
* **Double-Click**: Jumps straight into **100% Turbo (`MAX_MODE`)** with zero pre-flash.
* **Triple-Click**: Jumps straight into **Direct Tactical Strobe (`STROBE_SET_MODE`)** at ~13 Hz hands-free with 100% element of surprise!
* **Press & Hold (> 500 ms)**: Enters live tilt-adjustable `STROBE_MODE`. Releasing locks in the frequency.

### 2. Timeout-to-Off (Direct-Off)
* Quick clicks within **1.2 seconds** advance through brightness levels (`MIN` $\to$ `LOW` $\to$ `MID` $\to$ `HIGH` $\to$ `MAX` $\to$ `OFF`).
* Once the light has been active for **longer than 1.2 seconds**, the next single short click turns it **straight OFF**.
* A medium press (300–500 ms) also shuts down immediately at any time.

### 3. Pocket Lockout (Non-Volatile EEPROM)
* **4 Rapid Clicks from OFF**: Toggles Lockout mode.
  * Entering Lockout: confirmed by a solid **Red tailcap flash**. Persists in EEPROM across complete battery power cuts.
  * Exiting Lockout: confirmed by a solid **Green tailcap flash**.
* While locked, accidental button presses are blocked and blink the **tailcap Red LED once** (*"Locked"*), preventing dangerous bag/pocket overheating and battery drain.

### 4. Tilt Brightness & Frequency Control
* **Tilt Dimming (`ACCEL_MODE`)**: From any active stepped mode, press and hold (> 500 ms) to enter tilt control. Tilt to adjust brightness; release to lock in (`ACCEL_SET_MODE`).
* **Tilt Strobe Tuning (`STROBE_MODE`)**: Press and hold (> 500 ms) while strobing to tilt-tune the frequency. Release to lock in (`STROBE_SET_MODE`).

### 5. Enhanced Status Display Telemetry
While the light is active in any stepped mode, the tail LEDs continuously display a repeating telemetry cycle:
1. **Internal Temperature (°C)**:
   * **Tens digit** = Green LED blinks
   * **Ones digit** = Red LED blinks  
   *(e.g., 2 Green + 4 Red = 24°C)*
2. **Pre-Separator Pause**:
   * 500 ms dark pause after temperature finishes.
3. **Inverted Transition Separator**:
   * Distinct, fast-blink pattern: **Red $\to$ Green $\to$ Red** (70 ms pulses).
4. **Post-Separator Dark Delay**:
   * **1.0-second full dark delay** so the separator is completely distinct from the upcoming battery reading.
5. **Battery State of Charge (SOC %)**:
   * **Tens digit** = Green LED blinks
   * **Ones digit** = Red LED blinks  
   *(e.g., 8 Green + 5 Red = 85% SOC; 9 Green + 5 Red = 95% SOC)*
6. **Cycle Pause**:
   * 2.5-second dark pause, then the sequence repeats starting back at temperature.

### 6. Battery Safety & Charging
* **Low-Battery Auto Step-Down & Warning**: When the cell drops below 3.2V under load, high modes (`MAX` or `HIGH`) automatically step down to `MID_MODE` (50%), telemetry is replaced with an urgent Red pulse every 1.5 seconds.
* **Breathing Charge Glow (`GLOW_CHARGE`)**: When plugged into USB, the tailcap Green LED smoothly breathes on and off rather than harsh blinking.

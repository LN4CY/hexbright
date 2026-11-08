/*
Copyright (c) 2013-2026, "David Remba" <david.remba@gmail.com>
All rights reserved.

Reference code "David Hilton" <dhiltonp@gmail.com> functional.ino and hexbright.h library 

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met: 

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <hexbright.h>
#include <EEPROM.h>

// EEPROM Storage Addresses
#define EEPROM_LOCKED     0  // Stores non-volatile lockout state (1 = locked, 0 = unlocked)

// Defined modes
#define OFF_MODE          0  // main LED off, system in low-power standby
#define MIN_MODE          1  // brightness at 12.5%
#define LOW_MODE          2  // brightness at 25%
#define MID_MODE          3  // brightness at 50%
#define HIGH_MODE         4  // brightness at 75%
#define MAX_MODE          5  // brightness at 100% (Turbo)
#define ACCEL_MODE        6  // dynamic brightness proportional to tilt angle
#define ACCEL_SET_MODE    7  // hold the custom tilt brightness
#define STROBE_MODE       8  // dynamic strobe frequency proportional to tilt angle
#define STROBE_SET_MODE   9  // hold chosen strobe rate

// Configuration timings
#define TIMEOUT_TO_OFF    1200 // ms of active runtime before a single short click turns off directly
#define CLICK_WINDOW       280 // ms window to settle multi-clicks before turning on selected mode
#define TACTICAL_STROBE_DT 0.15f // Default fast tactical strobe interval (~13 Hz)

int mode = OFF_MODE;
const int levels[MAX_MODE] = {125, 250, 500, 750, 1000};
float delta;
float delta_t = TACTICAL_STROBE_DT;
unsigned char blocked = 0;
bool locked = false;

unsigned long mode_enter_time = 0;
unsigned long last_click_time = 0;
byte off_clicks = 0;
byte on_clicks = 0;
unsigned long last_on_click_time = 0;

hexbright hb;

void common_active_actions(void) {
  // Feature 4: Low-battery management + Celsius temperature blinking
  if (hb.low_voltage_state()) {
    // Under low battery, automatically step down high output modes to preserve cell
    if (mode > MID_MODE) {
      mode = MID_MODE;
    }
    // Suppress temp blinking and pulse tail Red LED every 1.5s as an urgent warning
    static unsigned long last_warn_time = 0;
    if (millis() - last_warn_time > 1500) {
      last_warn_time = millis();
      hb.set_led(RLED, 50);
    }
  } else {
    // Normal battery: blink internal Celsius temperature (tens on green, ones on red)
    if (!hb.printing_number()) {
      hb.print_number(hb.get_celsius());
    }
  }

  if (mode >= MIN_MODE && mode <= MAX_MODE) {
    hb.set_light(CURRENT_LEVEL, levels[mode - 1], 300);
  }
}

void setup() {
  hb.init_hardware();

  // Restore non-volatile Lockout state across power-downs
  byte val = EEPROM.read(EEPROM_LOCKED);
  if (val == 1) {
    locked = true;
  } else if (val != 0) {
    // First boot or uninitialized EEPROM (0xFF)
    EEPROM.update(EEPROM_LOCKED, 0);
    locked = false;
  }
}

void loop() {
  hb.update();
  unsigned long now = millis();

  // --- STATE MACHINE ---
  switch (mode) {

    // ==========================================
    // OFF MODE
    // ==========================================
    case OFF_MODE:
      if (off_clicks > 0) {
        // Keep CPU latched ON and Cree LED dark (0) while settling multi-clicks
        hb.set_light(CURRENT_LEVEL, 0, NOW);
      } else {
        // Fully unlatch power regulator when idle in OFF mode
        hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);
      }

      if (!hb.printing_number()) {
        hb.print_power(); // Handles breathing charge glow & low-voltage indication when off
      }

      // Press-and-hold (>500ms) from OFF -> Tilt-adjustable Strobe (only when unlocked)
      if (!locked && off_clicks == 0 && hb.button_pressed() && hb.button_pressed_time() > 500) {
        mode = STROBE_MODE;
        mode_enter_time = now;
        break;
      }

      // Accumulate clicks on button release while OFF (light stays completely dark)
      if (hb.button_just_released()) {
        if (hb.button_pressed_time() < 450) {
          off_clicks++;
          last_click_time = now;
        }
      }

      // Once the 280 ms click window expires after the last release, evaluate clicks:
      if (off_clicks > 0 && (now - last_click_time > CLICK_WINDOW)) {
        if (locked) {
          if (off_clicks >= 4) {
            // 4 Clicks while locked -> Unlock!
            locked = false;
            EEPROM.update(EEPROM_LOCKED, 0);
            hb.set_led(GLED, 400); // Solid green confirmation
          } else {
            // 1-3 Clicks while locked -> Non-blocking Red warn
            hb.set_led(RLED, 100);
          }
        } else {
          switch (off_clicks) {
            case 1:
              // 1 Click: Turn on cleanly to MIN_MODE (no pre-flash)
              mode = MIN_MODE;
              mode_enter_time = now;
              break;
            case 2:
              // 2 Clicks: Turn on directly to 100% Turbo (MAX_MODE)
              mode = MAX_MODE;
              mode_enter_time = now;
              break;
            case 3:
              // 3 Clicks: Turn on directly to Tactical Strobe (~13 Hz)
              delta_t = TACTICAL_STROBE_DT;
              mode = STROBE_SET_MODE;
              mode_enter_time = now;
              break;
            default:
              // 4 or more clicks: Pocket Lockout (persist in EEPROM)
              locked = true;
              EEPROM.update(EEPROM_LOCKED, 1);
              hb.set_led(RLED, 400); // Solid red confirmation
              break;
          }
        }
        off_clicks = 0;
      }
      break;

    // ==========================================
    // STEPPED MODES: MIN, LOW, MID, HIGH, MAX
    // ==========================================
    case MIN_MODE:
    case LOW_MODE:
    case MID_MODE:
    case HIGH_MODE:
    case MAX_MODE:
      common_active_actions();

      if (hb.button_just_released()) {
        unsigned long pressed_dur = hb.button_pressed_time();

        // Track rapid clicks while ON
        if (now - last_on_click_time < CLICK_WINDOW) {
          on_clicks++;
        } else {
          on_clicks = 1;
        }
        last_on_click_time = now;

        // Double-click shortcut to Turbo from any active mode
        if (on_clicks == 2 && pressed_dur < 300) {
          mode = MAX_MODE;
          mode_enter_time = now;
          on_clicks = 0;
          break;
        }

        if (pressed_dur < 300) {
          // Feature 2: Timeout-to-Off (Direct-Off)
          if (now - mode_enter_time > TIMEOUT_TO_OFF) {
            // Active for > 1.2s: single click turns straight OFF
            mode = OFF_MODE;
            on_clicks = 0;
          } else {
            // Rapid successive click within 1.2s: cycle to next brightness level
            mode = (mode == MAX_MODE) ? OFF_MODE : (mode + 1);
            mode_enter_time = now;
          }
        } else if (pressed_dur < 500) {
          // Medium press (300-500ms): turn OFF immediately at any time
          mode = OFF_MODE;
          on_clicks = 0;
        }
      } else if (hb.button_pressed_time() > 500) {
        // Press-and-hold (>500ms): smooth transition to tilt-dimming (ACCEL_MODE)
        mode = ACCEL_MODE;
        on_clicks = 0;
      }
      break;

    // ==========================================
    // ACCEL MODE: Dynamic tilt-based brightness
    // ==========================================
    case ACCEL_MODE:
      delta = hb.difference_from_down();
      if (delta > 0.5f) {
        delta = 0.5f;
      }
      if (delta >= 0.01f && delta <= 0.5f) {
        delta = delta * 2.0f;
        hb.set_light(CURRENT_LEVEL, (int)(delta * 1000.0f), 100);
      }
      if (hb.button_just_released()) {
        // Releasing locks chosen brightness into ACCEL_SET_MODE
        mode = ACCEL_SET_MODE;
        mode_enter_time = now;
        blocked = 1;
        on_clicks = 0;
      }
      break;

    // ==========================================
    // ACCEL SET MODE: Hold customized brightness
    // ==========================================
    case ACCEL_SET_MODE:
      if (hb.button_just_pressed()) {
        blocked = 0;
      }
      if (hb.button_just_released() && !blocked) {
        unsigned long pressed_dur = hb.button_pressed_time();
        if (pressed_dur < 300) {
          // Feature 2: Timeout-to-off
          if (now - mode_enter_time > TIMEOUT_TO_OFF) {
            mode = OFF_MODE;
            on_clicks = 0;
          } else {
            // Map current brightness to nearest stepped mode
            int current_level = hb.get_light_level();
            if (current_level <= 125) mode = LOW_MODE;
            else if (current_level <= 250) mode = MID_MODE;
            else if (current_level <= 500) mode = HIGH_MODE;
            else if (current_level <= 750) mode = MAX_MODE;
            else mode = OFF_MODE;
            mode_enter_time = now;
          }
        } else if (pressed_dur < 500) {
          mode = OFF_MODE;
          on_clicks = 0;
        }
      }
      if (hb.button_pressed_time() > 500 && !blocked) {
        // Hold again to re-enter tilt-adjustment
        mode = ACCEL_MODE;
        on_clicks = 0;
      }
      break;

    // ==========================================
    // STROBE MODE: Dynamic tilt-based frequency
    // ==========================================
    case STROBE_MODE: {
      static unsigned long flash_time = 0;
      delta = 0.5f - hb.difference_from_down();
      if (delta > 0.5f) delta = 0.5f;
      if (delta < 0.05f) delta = 0.05f;
      if (delta >= 0.05f && delta <= 0.5f) {
        delta_t = 2.0f * delta;
      }
      if ((flash_time + (int)(delta_t * 500.0f)) < now) {
        flash_time = now;
        hb.set_light(MAX_LEVEL, 0, 20);
      }
      if (hb.button_just_released()) {
        // Releasing locks chosen frequency into STROBE_SET_MODE
        mode = STROBE_SET_MODE;
        blocked = 1;
        mode_enter_time = now;
        on_clicks = 0;
      }
      break;
    }

    // ==========================================
    // STROBE SET MODE: Hold chosen strobe rate
    // ==========================================
    case STROBE_SET_MODE: {
      static unsigned long flash_time = 0;
      if ((flash_time + (int)(delta_t * 500.0f)) < now) {
        flash_time = now;
        hb.set_light(MAX_LEVEL, 0, 20);
      }
      if (hb.button_just_pressed()) {
        blocked = 0;
      }
      if (hb.button_just_released() && !blocked) {
        if (hb.button_pressed_time() < 500) {
          // Single short click turns OFF
          mode = OFF_MODE;
          on_clicks = 0;
        }
      }
      if (hb.button_pressed_time() > 500 && !blocked) {
        // Press-and-hold (>500ms) re-enters live tilt frequency adjustment!
        mode = STROBE_MODE;
        on_clicks = 0;
      }
      break;
    }
  }
}

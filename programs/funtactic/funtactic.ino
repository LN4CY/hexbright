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

// Debug output toggle: set to 1 to enable Serial logs, 0 to optimize space
#define DEBUG 0
#if DEBUG
  #define DBG(x) x
#else
  #define DBG(x)
#endif

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
#define TIMEOUT_TO_OFF    1200 // ms of runtime before a single short click turns off directly
#define CLICK_WINDOW       350 // ms window to register multi-clicks (double, triple, quad)
#define TACTICAL_STROBE_DT 0.15f // Default fast tactical strobe interval (~13 Hz)

int mode = OFF_MODE;
const int levels[MAX_MODE] = {125, 250, 500, 750, 1000};
float delta;
float delta_t = TACTICAL_STROBE_DT;
unsigned char blocked = 0;
bool locked = false;

unsigned long mode_enter_time = 0;
unsigned long last_click_time = 0;
byte click_count = 0;

hexbright hb;

void blink_feedback(byte led, byte count) {
  for (byte i = 0; i < count; i++) {
    hb.set_led(led, 80);
    delay(120);
  }
}

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
#if DEBUG
  Serial.begin(9600);
#endif
}

void loop() {
  hb.update();
  unsigned long now = millis();

  // Multi-click detection window timeout
  if (click_count > 0 && (now - last_click_time > CLICK_WINDOW)) {
    if (locked) {
      // If locked and user clicked 1-3 times, warn that light is locked
      blink_feedback(RLED, 2);
    }
    click_count = 0;
  }

  // --- STATE MACHINE ---
  switch (mode) {

    // ==========================================
    // OFF MODE
    // ==========================================
    case OFF_MODE:
      hb.set_light(CURRENT_LEVEL, OFF_LEVEL, NOW);

      if (!hb.printing_number()) {
        hb.print_power(); // Handles breathing charge glow & low-voltage indication when off
      }

      if (hb.button_just_released()) {
        unsigned long pressed_dur = hb.button_pressed_time();

        // Count clicks within the multi-click window
        if (now - last_click_time < CLICK_WINDOW) {
          click_count++;
        } else {
          click_count = 1;
        }
        last_click_time = now;

        // Feature 1: Pocket Lockout (4 rapid clicks toggles lock)
        if (click_count >= 4) {
          locked = !locked;
          click_count = 0;
          if (locked) {
            blink_feedback(RLED, 3); // 3 Red flashes = Locked
          } else {
            blink_feedback(GLED, 3); // 3 Green flashes = Unlocked
          }
          break;
        }

        if (locked) {
          // While locked, prevent turning on
          break;
        }

        // Feature 3: Shortcuts from OFF
        if (click_count == 2 && pressed_dur < 350) {
          // Double-click -> Instant Turbo (MAX_MODE 100%)
          mode = MAX_MODE;
          mode_enter_time = now;
          click_count = 0;
          DBG(Serial.println(F("TURBO (Double-click)")));
          break;
        } else if (click_count == 3 && pressed_dur < 350) {
          // Triple-click -> Direct Tactical Strobe hands-free (Approach A)
          delta_t = TACTICAL_STROBE_DT;
          mode = STROBE_SET_MODE;
          mode_enter_time = now;
          click_count = 0;
          DBG(Serial.println(F("DIRECT STROBE (Triple-click)")));
          break;
        }

        // Standard 1-click turn on
        if (pressed_dur < 500) {
          mode = MIN_MODE;
          mode_enter_time = now;
          DBG(Serial.println(F("MIN_MODE")));
        }
      } else if (!locked && hb.button_pressed_time() > 500) {
        // Press-and-hold (>500ms) from OFF -> Strobe mode with live tilt tuning
        mode = STROBE_MODE;
        mode_enter_time = now;
        click_count = 0;
        DBG(Serial.println(F("STROBE_MODE")));
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

        // Multi-click tracking while ON
        if (now - last_click_time < CLICK_WINDOW) {
          click_count++;
        } else {
          click_count = 1;
        }
        last_click_time = now;

        // Feature 3: Double-click from ANY stepped mode jumps straight to Turbo
        if (click_count == 2 && pressed_dur < 300) {
          mode = MAX_MODE;
          mode_enter_time = now;
          click_count = 0;
          DBG(Serial.println(F("TURBO (Double-click)")));
          break;
        }

        if (pressed_dur < 300) {
          // Feature 2: Timeout-to-Off (Direct-Off)
          if (now - mode_enter_time > TIMEOUT_TO_OFF) {
            // Active for > 1.2s: turn straight OFF
            mode = OFF_MODE;
            DBG(Serial.println(F("DIRECT OFF")));
          } else {
            // Rapid successive click: step to next brightness level
            mode = (mode == MAX_MODE) ? OFF_MODE : (mode + 1);
            mode_enter_time = now;
            DBG(Serial.println(F("CYCLE MODE")));
          }
        } else if (pressed_dur < 500) {
          // Medium press (300-500ms): turn OFF immediately at any time
          mode = OFF_MODE;
          DBG(Serial.println(F("OFF_MODE")));
        }
      } else if (hb.button_pressed_time() > 500) {
        // Press-and-hold (>500ms): smooth transition to tilt-dimming (ACCEL_MODE)
        mode = ACCEL_MODE;
        click_count = 0;
        DBG(Serial.println(F("ACCEL_MODE")));
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
        DBG(Serial.println(F("ACCEL_SET_MODE")));
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
        }
      }
      if (hb.button_pressed_time() > 500 && !blocked) {
        // Hold again to re-enter tilt-adjustment
        mode = ACCEL_MODE;
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
        DBG(Serial.println(F("STROBE_SET_MODE")));
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
          DBG(Serial.println(F("OFF_MODE")));
        }
      }
      if (hb.button_pressed_time() > 500 && !blocked) {
        // Press-and-hold (>500ms) re-enters live tilt frequency adjustment!
        mode = STROBE_MODE;
        DBG(Serial.println(F("STROBE_MODE (tilt tuning)")));
      }
      break;
    }
  }
}

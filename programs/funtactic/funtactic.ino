/*
Copyright (c) 2013, "David Remba" <david.remba@gmail.com>
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

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies, 
either expressed or implied, of the FreeBSD Project.
*/

#include <hexbright.h>

#include <Wire.h>

//defined modes
#define OFF_MODE          0  //main LED off, system off
#define MIN_MODE          1  //system on, main LED brightness at 12.5%
#define LOW_MODE          2  //system on, main LED brightness at 25%
#define MID_MODE          3  //system on, main LED brightness at 50%
#define HIGH_MODE         4  //system on, main LED brightness at 75%
#define MAX_MODE          5  //system on, main LED brightness at 100%
#define ACCEL_MODE        6  //system on, main LED brightness proportional to accel angle
#define ACCEL_SET_MODE    7  //system on, hold the value from the accel mode
#define STROBE_MODE       8  //system on, strobe mode select
#define STROBE_SET_MODE   9  //system on, strobe mode


int mode = OFF_MODE;
int levels[MAX_MODE] = {125,250,500,750,1000};
double delta;
double delta_t = 0.5;
unsigned char blocked = 0;

#define MS 8.333 // now internally set, keeping us in lockstep with the accelerometer
hexbright hb;

void setup() {
  hb.init_hardware();
} 

void loop() 
{
  hb.update();

  switch(mode)
  {
     case OFF_MODE:
     //Action
     hb.set_light(0, 0, 1);
     hb.shutdown();
     if(!hb.printing_number())
     {
        hb.print_charge(GLED);
     }
     //Transition
     if(hb.button_just_released())
     {
        if(hb.button_pressed_time() < 500)
        {
           mode = MIN_MODE;
           Serial.println("MIN_MODE");
        }
     }
     else if(hb.button_pressed_time() > 500)
     {
        mode = STROBE_MODE;
        Serial.println("STROBE_MODE");
     }
     break;
     case MIN_MODE:
     //Action
     common_active_actions();
     //Transition
     if(hb.button_just_released())
     {
        if(hb.button_pressed_time() < 300)
        {
           mode = LOW_MODE;
           Serial.println("LOW_MODE");

        }   
        else if(hb.button_pressed_time() < 500)
        {
           mode = OFF_MODE;
           Serial.println("OFF_MODE");
        }
     }
     else if(hb.button_pressed_time() > 500)
     {
        mode = ACCEL_MODE;
        Serial.println("ACCEL_MODE");
     }
     break;
     case LOW_MODE:
     //Action
     common_active_actions();
     //Transition
     if(hb.button_just_released())
     {
        if(hb.button_pressed_time() < 300)
        {
           mode = MID_MODE;
           Serial.println("MID_MODE");
        }   
        else if(hb.button_pressed_time() < 500)
        {
           mode = OFF_MODE;
           Serial.println("OFF_MODE");
        }
     }
     else if(hb.button_pressed_time() > 500)
     {
        mode = ACCEL_MODE;
        Serial.println("ACCEL_MODE");
     }
     break;
     case MID_MODE:
     //Action
     common_active_actions();
     //Transition
     if(hb.button_just_released())
     {
        if(hb.button_pressed_time() < 300)
        {
           mode = HIGH_MODE;
           Serial.println("HIGH_MODE");

        }   
        else if(hb.button_pressed_time() < 500)
        {
           mode = OFF_MODE;
           Serial.println("OFF_MODE");
        }
     }
     else if(hb.button_pressed_time() > 500)
     {
        mode = ACCEL_MODE;
        Serial.println("ACCEL_MODE");
     }
     break;
     case HIGH_MODE:
     //Action
     common_active_actions();
     //Transition
     if(hb.button_just_released())
     {
        if(hb.button_pressed_time() < 300)
        {
           mode = MAX_MODE;
           Serial.println("MAX_MODE");
        }   
        else if(hb.button_pressed_time() < 500)
        {
           mode = OFF_MODE;
           Serial.println("OFF_MODE");
        }
     }
     else if(hb.button_pressed_time() > 500)
     {
        mode = ACCEL_MODE;
        Serial.println("ACCEL_MODE");
     }
     break;
     case MAX_MODE:
     //Action
     common_active_actions();
     //Transition
     if(hb.button_just_released())
     {
        if(hb.button_pressed_time() < 500)
        {
           mode = OFF_MODE;
           Serial.println("OFF_MODE");
        }
     }
     else if(hb.button_pressed_time() > 500)
     {
        mode = ACCEL_MODE;
        Serial.println("ACCEL_MODE");
     }
     break;  
     case ACCEL_MODE:
     //Action
     delta = hb.difference_from_down();
     if(delta > 0.5)
     {
           delta = 0.5;
     }
     if(delta>=0.01 && delta<=0.5)
     {
        delta = delta * 2;
	hb.set_light(CURRENT_LEVEL, (int)(delta * 1000.0), 100);
     }
     Serial.println(delta);
     if(hb.button_just_released())
     {
        mode = ACCEL_SET_MODE;
        Serial.println("ACCEL_SET_MODE");
        blocked = 1;
     }
     break;
     case ACCEL_SET_MODE:
     if(hb.button_just_pressed())
     {
        blocked = 0;
     }
     if(hb.button_just_released() && !blocked)
     {
        if(hb.button_pressed_time() < 300)
        {
           int current_level = hb.get_light_level();
           if(current_level <= 125)
           { 
              mode = LOW_MODE;
              Serial.println("LOW_MODE");
           }
           else if(current_level <= 250) 
           {
              mode = MID_MODE;
              Serial.println("MID_MODE");
           }
           else if(current_level <= 500)
           {
              mode = HIGH_MODE;
              Serial.println("HIGH_MODE");
           }
           else if(current_level <= 750)
           { 
              mode = MAX_MODE;
              Serial.println("MAX_MODE");   
           }
           else if(current_level <= 1000)
           { 
              mode = OFF_MODE;
              Serial.println("OFF_MODE");
           }
        }
        else if(hb.button_pressed_time() < 500)
        {
           mode = OFF_MODE;
           Serial.println("OFF_MODE");
        }
     }  
     if(hb.button_pressed_time() > 500 && !blocked)
     {
        mode = ACCEL_MODE;
        Serial.println("ACCEL_MODE");
     }
     break;
     case STROBE_MODE:
     //Action
     static unsigned long flash_time = millis(); 
     delta = 0.5 - hb.difference_from_down();
     if(delta > 0.5)
     {
        delta = 0.5;
     }
     if(delta < 0.05)
     {
        delta = 0.05;
     }
     if(delta>=0.05 && delta<=0.5)
     {
        delta_t = 2 * delta;
     }
     if((flash_time+(int)(delta_t*500))<millis()) // flash every 100 milliseconds
     {
       flash_time = millis(); // reset flash_time
       hb.set_light(MAX_LEVEL, 0, 20); // and pulse (going from max to min over 20 milliseconds)
                                       // actually, because of the refresh rate, it's more like 'go from max brightness on high
                                       //  to max brightness on low to off.
     }
     //Transition
     Serial.println(delta_t);
     if(hb.button_just_released())
     {
        mode = STROBE_SET_MODE;
        Serial.println("STROBE_SET_MODE");
        blocked = 1;
     }
     break;
     case STROBE_SET_MODE:
     //Action
     if((flash_time+(int)(delta_t*500))<millis()) // flash every 100 milliseconds
     {
       flash_time = millis(); // reset flash_time
       hb.set_light(MAX_LEVEL, 0, 20); // and pulse (going from max to min over 20 milliseconds)
                                       // actually, because of the refresh rate, it's more like 'go from max brightness on high
                                       //  to max brightness on low to off.
     }
     //Transition
     if(hb.button_just_pressed())
     {
        blocked = 0;
     }
     if(hb.button_just_released() && !blocked)
     {
       if(hb.button_pressed_time() < 500)
        {
           mode = OFF_MODE;
           Serial.println("OFF_MODE");
        }
     }  
     if(hb.button_pressed_time() > 500 && !blocked)
     {
        mode = STROBE_MODE;
        Serial.println("STROBE_MODE");
     }
     break;
  }
}

void common_active_actions(void)
{
   if(!hb.printing_number())
   {
      hb.print_number(hb.get_celsius());
   } 
   if(mode <= MAX_MODE)
   {
      hb.set_light(CURRENT_LEVEL, levels[mode-1], 300);
   }
}

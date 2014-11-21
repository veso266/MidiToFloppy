/*
 The MIT License (MIT)

 Copyright (c) 2014 Andrzej Ressel (jereksel@gmail.com)

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include <limits.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <avr/pgmspace.h>
#include "scale.h"
#include "song.h"
#include "device.h"
#include "samurai.h"


bool swagger_bool;
unsigned long swagger_time = 0;

int song[3] = {
  0, 0, 0
};

#define I2C_ADDR    0x27
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7

#define button_1 53
#define button_8 52

LiquidCrystal_I2C       lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);

const long floppyConv = 31400000;

const int freq[] PROGMEM = {
  3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6, 6,
  6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 12, 12,
  13, 14, 15, 16, 17, 18, 19, 20, 21, 23, 24, 25,
  27, 29, 30, 32, 34, 36, 38, 41, 43, 46, 48, 51,
  55, 58, 61, 65, 69, 73, 77, 82, 87, 92, 97, 103,
  110, 116, 123, 130, 138, 146, 155, 164, 174, 184, 195, 207,
  220, 233, 246, 261, 277, 293, 311, 329, 349, 369, 391, 415,
  440, 466, 493, 523, 554, 587, 622, 659, 698, 739, 783, 830,
  880, 932, 987, 1046, 1108, 1174, 1244, 1318, 1396, 1479, 1567, 1661,
  1760, 1864, 1975, 2093, 2217, 2349, 2489, 2637, 2793, 2959, 3135, 3322,
  3520, 3729, 3951, 4186, 4434, 4698, 4978, 5274, 5587, 5919, 6271, 6644,
  7040, 7458, 7902, 8372, 8869, 9397, 9956, 10548, 11175, 11839, 12543, 13289
};

long endTime[devices];
long pauseTime[devices];
int note_number[devices];
int dir[devices];
long pause[devices];

byte swagger[devices];
bool swagger_pin[devices];
bool waiting[devices];

Song *s;

int a = 0;

void setup(void)
{

  Serial.begin(9600);

  pinMode(button_1, INPUT);
  pinMode(button_8, INPUT);

  cleandata();

  for (byte i = 0; i < devices; i++)
  {
    pinMode(pins[i][0], OUTPUT);
    pinMode(pins[i][1], OUTPUT);
  }

  menu();

}

void menu()
{

  lcd.begin (20, 4);
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.home();
  lcd.print("1: Steel Samurai");
  lcd.setCursor ( 0, 1 );
  lcd.print("8: Test/reset");

}

void cleandata()
{
  swagger_bool = true;
  swagger_time = 0;
  for (byte i = 0; i < devices; i++)
  {
    swagger[i] = 0;
    swagger_pin[i] = 1;
    waiting[i] = false;
    note_number[i] = -1;
    endTime[i] = 0;
    pauseTime[i] = 0;
    dir[i] = 0;
    pause[i] = 0;
  }
}

void loop()
{

  if (digitalRead(button_1) == HIGH) {
    s = new Samurai();
    play();
  }
  if (digitalRead(button_8) == HIGH) {
    test();
  }

}

void play()
{
  delay(1000);
  lcd.begin (20, 4);
  lcd.home();
  lcd.print("1: Toggle swagger");
  lcd.setCursor ( 0, 1 );
  lcd.print("8: Stop");
  playsong();
  cleandata();
  menu();
}

void test()
{
  for (byte i = 0; i < devices; i++)
  {
    for (byte s = 0; s < 100; s++) {
      digitalWrite(pins[i][0], LOW);
      digitalWrite(pins[i][1], HIGH);
      digitalWrite(pins[i][1], LOW);
      delay(5);
    }

    for (byte s = 0; s < 10; s++) {
      digitalWrite(pins[i][0], HIGH);
      digitalWrite(pins[i][1], HIGH);
      digitalWrite(pins[i][1], LOW);
      delay(5);
    }
  }
}

void playsong()
{
  while (1)
  {
    for (int i = 0; i < devices; i++)
    {
      if (digitalRead(button_8) == HIGH) {
        return;
      }

      if (digitalRead(button_1) == HIGH && millis() > swagger_time) {
        swagger_bool = !swagger_bool;
        swagger_time = millis() + 500;
      }

      if (millis() >= endTime[i])
      {
        note_number[i] = note_number[i] + 1;
        a = note_number[i];
        s->getMusic(i, a, song);

        if (song[0] == 254) {
          return;
        }
        else if (song[0] != 255)
        {
          int adjustedfreq = pgm_read_byte(&freq[(song[1] + 3 + changes[i]) * 12 + song[0]]);
          pause[i] = (floppyConv / (adjustedfreq)) / 100;
          pauseTime[i] = (micros() + pause[i]);
          if (swagger_bool)
          {
            pauseTime[i] = pauseTime[i] - 2500;
          }
        }
        else
        {
          pauseTime[i] = LONG_MAX;
        }
        endTime[i] = millis() + song[2];
      }
      if (micros() >= pauseTime[i])
      {

        if (swagger_bool)
        {
          if (waiting[i])
          {
            waiting[i] = false;
          }
          else
          {
            digitalWrite(pins[i][0], swagger_pin[i]); // Go in reverse
            digitalWrite(pins[i][1], HIGH);
            digitalWrite(pins[i][1], LOW);

            swagger[i]++;

            if (swagger[i] >= 60)
            {
              swagger[i] = 0;
              swagger_pin[i] = !swagger_pin[i];
            }

            pauseTime[i] = (micros() + 2500);
            waiting[i] = true;
            continue;
          }
        }

        if (dir[i] == 0)
          dir[i] = 1;
        else
          dir[i] = 0;

        digitalWrite(pins[i][0], dir[i]);
        pauseTime[i] = micros() + pause[i];
        if (swagger_bool)
        {
          pauseTime[i] = pauseTime[i] - 2500;
        }
        digitalWrite(pins[i][1], HIGH);
        digitalWrite(pins[i][1], LOW);

      }
    }
  }
}
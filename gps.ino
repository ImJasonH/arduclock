#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

Adafruit_7segment rightLED = Adafruit_7segment(); // rightmost 4 digits
Adafruit_7segment midLED = Adafruit_7segment();   // middle 4 digits
Adafruit_7segment leftLED = Adafruit_7segment();  // leftmost 2 digits

#include <SoftwareSerial.h>
SoftwareSerial ss(2, 3);

#include <Adafruit_GPS.h>
Adafruit_GPS GPS(&ss);

// Set to true to echo GPS data to Serial
#define GPSECHO  false

// this keeps track of whether we're using the interrupt
// off by default!
boolean usingInterrupt = false;
void useInterrupt(boolean); // Func prototype keeps Arduino 0023 happy

void setup() {
  Serial.begin(9600);

  // https://learn.adafruit.com/adafruit-led-backpack/changing-i2c-address
  rightLED.begin(0x70); // Don't solder any I2C jumpers on rightLED
  midLED.begin(0x71);   // TODO: Solder A0 on midLED
  leftLED.begin(0x72);  // TODO: Solder A1 on leftLED

  // connect at 115200 so we can read the GPS fast enuf and
  // also spit it out
  Serial.begin(115200);
  Serial.println("Adafruit GPS library basic test!");

  // 9600 NMEA is the default baud rate for MTK - some use 4800
  GPS.begin(9600);

  // You can adjust which sentences to have the module emit, below

  // Receive RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);

  // Set the update rate
  // 5 Hz update rate- for 9600 baud you'll have to set the output to RMC or RMCGGA only (see above)
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_5HZ);
  GPS.sendCommand(PMTK_API_SET_FIX_CTL_5HZ);

  // Request updates on antenna status, comment out to keep quiet
  GPS.sendCommand(PGCMD_ANTENNA);

  // the nice thing about this code is you can have a timer0 interrupt go off
  // every 1 millisecond, and read data from the GPS for you. that makes the
  // loop code a heck of a lot easier!
  useInterrupt(true);
}

// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  GPS.read();
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}

void loop() {
  if (GPS.newNMEAreceived()) {
    GPS.parse(GPS.lastNMEA());
    long t = toUnix(
               GPS.year,
               GPS.month,
               GPS.day,
               GPS.hour,
               GPS.minute,
               GPS.seconds);
    Serial.println(t);
    display(t);
  }
}

// Start counting seconds at the beginning of 2017, since we
// can't go further back than that. This accounts for all leap
// seconds until the beginning of 2017.
const long start2017 = 1483228800; // Jan 1 2017, midnight
const int daysPerMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const int daysPerYear = 365;

long toUnix(uint8_t year,
            uint8_t month,
            uint8_t day,
            uint8_t hour,
            uint8_t minute,
            uint8_t second) {

  // Days since start of 2017.
  long days = (year - 17) * daysPerYear;

  // Number of days in years since beginning-of-2017 that were
  // leap days.
  days += (year - 16) / 4;
  days -= (year - 16) / 100; // no leap day every 100 years.
  days += (year - 16) / 400; // ...except every 400 years.

  // Days so far this year.
  // Days in previous months this year.
  for (int i = 1; i < month; i++) {
    days += daysPerMonth[i];
  }
  // Complete days so far this month.
  days += day - 1;

  return start2017
         + long(days * 24 * 60 * 60)
         + long(hour * 60 * 60)
         + long(minute * 60)
         + long(second);
}

// display displays the number across 3 LED backpacks.
void display(long num) {
  long right = num % 10000;
  long mid = num / 10000 % 10000;
  long left = num / 10000 / 10000;
  Serial.println(String(left) + "|" + String(mid) + "|" + String(right));

  rightLED.print(right);
  midLED.print(mid);
  leftLED.print(left);

  rightLED.writeDisplay();
  midLED.writeDisplay();
  leftLED.writeDisplay();
  delay(200);
}

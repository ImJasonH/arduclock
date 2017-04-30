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
  rightLED.begin(0x70); // Don't solder any I2C jumpers on rightLED
  // https://learn.adafruit.com/adafruit-led-backpack/0-54-alphanumeric?view=all#changing-i2c-address
  midLED.begin(0x71);// TODO: Solder A0 on midLED
  leftLED.begin(0x72);// TODO: Solder A1 on leftLED
  
  // connect at 115200 so we can read the GPS fast enuf and
  // also spit it out
  Serial.begin(115200);
  Serial.println("Adafruit GPS library basic test!");

  // 9600 NMEA is the default baud rate for MTK - some use 4800
  GPS.begin(9600);
  
  // You can adjust which sentences to have the module emit, below
  
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data for high update rates!
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // uncomment this line to turn on all the available data - for 9600 baud you'll want 1 Hz rate
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_ALLDATA);
  
  // Set the update rate
  // Note you must send both commands below to change both the output rate (how often the position
  // is written to the serial line), and the position fix rate.
  // 1 Hz update rate
  //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  //GPS.sendCommand(PMTK_API_SET_FIX_CTL_1HZ);
  // 5 Hz update rate- for 9600 baud you'll have to set the output to RMC or RMCGGA only (see above)
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_5HZ);
  GPS.sendCommand(PMTK_API_SET_FIX_CTL_5HZ);
  // 10 Hz update rate - for 9600 baud you'll have to set the output to RMC only (see above)
  // Note the position can only be updated at most 5 times a second so it will lag behind serial output.
  //GPS.sendCommand(PMTK_SET_NMEA_UPDATE_10HZ);
  //GPS.sendCommand(PMTK_API_SET_FIX_CTL_5HZ);

  // Request updates on antenna status, comment out to keep quiet
  GPS.sendCommand(PGCMD_ANTENNA);

  // the nice thing about this code is you can have a timer0 interrupt go off
  // every 1 millisecond, and read data from the GPS for you. that makes the
  // loop code a heck of a lot easier!
  useInterrupt(true);
}

// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  if (GPSECHO)
    if (c) {
      UDR0 = c;
    }
    // writing direct to UDR0 is much much faster than Serial.print 
    // but only one character can be written at a time. 
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

#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

Adafruit_AlphaNum4 rightLED = Adafruit_AlphaNum4(); // rightmost 4 digits
Adafruit_AlphaNum4 midLED = Adafruit_AlphaNum4();   // middle 4 digits
Adafruit_AlphaNum4 leftLED = Adafruit_AlphaNum4();  // leftmost 2 digits

// display displays the number across 3 LED backpacks.
void display(long num) {
  long right = num % 10000;
  long mid = num / 10000 % 10000;
  long left = num / 10000 / 10000;

  String rstr = String(right);
  rightLED.writeDigitAscii(0, rstr[0]);
  rightLED.writeDigitAscii(1, rstr[1]);
  rightLED.writeDigitAscii(2, rstr[2]);
  rightLED.writeDigitAscii(3, rstr[3]);
  rightLED.writeDisplay();

  String mstr = String(mid);
  midLED.writeDigitAscii(0, mstr[0]);
  midLED.writeDigitAscii(1, mstr[1]);
  midLED.writeDigitAscii(2, mstr[2]);
  midLED.writeDigitAscii(3, mstr[3]);
  midLED.writeDisplay();

  String lstr = String(left);
  leftLED.writeDigitAscii(0, lstr[0]);
  leftLED.writeDigitAscii(1, lstr[1]);
  // TODO: Fix this before late 2286.
  leftLED.writeDigitRaw(2, 0x0); // nothing.
  leftLED.writeDigitRaw(3, 0x0); // nothing.
  leftLED.writeDisplay();

  delay(200);
}

// Test code for Adafruit GPS modules using MTK3329/MTK3339 driver
//
// This code shows how to listen to the GPS module in an interrupt
// which allows the program to have more 'freedom' - just parse
// when a new NMEA sentence is available! Then access data when
// desired.
//
// Tested and works great with the Adafruit Ultimate GPS module
// using MTK33x9 chipset
//    ------> http://www.adafruit.com/products/746
// Pick one up today at the Adafruit electronics shop 
// and help support open source hardware & software! -ada

//#define SERIAL_OUT
#undef SERIAL_OUT

#include <Adafruit_GPS.h>
#include "Wire.h"
#include <inttypes.h>
#include <LCDi2cNHD.h>                    
#include <SoftwareSerial.h>
#include <Time.h>  

SoftwareSerial mySerial(3, 2);

// if using Arduino v23 or earlier, uncomment these
// two lines and comment out the above. You will
// need to install NewSoftSerial
//  #include <NewSoftSerial.h>
//  NewSoftSerial mySerial(3, 2);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO  false

// Connect the GPS Power pin to 5V
// Connect the GPS Ground pin to ground
// Connect the GPS TX (transmit) pin to Digital 3
// Connect the GPS RX (receive) pin to Digital 2
Adafruit_GPS GPS(&mySerial);

// this keeps track of whether we're using the interrupt
// off by default!
boolean usingInterrupt = false;

LCDi2cNHD lcd = LCDi2cNHD(4,20,0x50>>1,0);

void setup()  
{
  // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  // also spit it out
#ifdef SERIAL_OUT
  Serial.begin(115200);
  Serial.println("Adafruit GPS library basic test!");
#endif

  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);

  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
  // the parser doesn't care about other sentences at this time

  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz

  // the nice thing about this code is you can have a timer0 interrupt go off
  // every 1 millisecond, and read data from the GPS for you. that makes the
  // loop code a heck of a lot easier!
  useInterrupt(true);
  lcd.init();
  lcd.setCursor(0,0);
  lcd.print("** Doug's GPS v01 **");

  delay(1000);
}

// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  if (GPSECHO)
    if (c) UDR0 = c;  
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
  } 
  else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}

//uint16_t timer = millis();
unsigned long timer = millis();

void loop()                     // run over and over again
{
  // in case you are not using the interrupt above, you'll
  // need to 'hand query' the GPS, not suggested :(
  if (! usingInterrupt) {
    // read data from the GPS in the 'main loop'
    char c = GPS.read();
    // if you want to debug, this is a good time to do it!
    if (GPSECHO)
      if (c) UDR0 = c;
    // writing direct to UDR0 is much much faster than Serial.print 
    // but only one character can be written at a time. 
  }

  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences! 
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    //Serial.println(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false

    if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
      return;  // we can fail to parse a sentence in which case we should just wait for another
  }

  // approximately every 2 seconds or so, print out the current stats
  if (millis() - timer > 2000) { 
    timer = millis(); // reset the timer
#ifdef SERIAL_OUT
    Serial.print("\nTime: ");
    Serial.print(GPS.hour, DEC); 
    Serial.print(':');
    Serial.print(GPS.minute, DEC); 
    Serial.print(':');
    Serial.print(GPS.seconds, DEC); 
    Serial.print('.');
    Serial.println(GPS.milliseconds);
    Serial.print("Date: ");
    Serial.print(GPS.day, DEC); 
    Serial.print('/');
    Serial.print(GPS.month, DEC); 
    Serial.print("/20");
    Serial.println(GPS.year, DEC);
#endif
    lcd.setCursor(1,0);
    lcd.print(GPS.hour, DEC);
    lcd.print(":");
    lcd.print(GPS.minute, DEC);
    lcd.print(":");
    lcd.print(GPS.seconds, DEC); 
    lcd.print(" ");
    lcd.print(GPS.month, DEC);
    lcd.print("/");
    lcd.print(GPS.day, DEC);
    lcd.print("/20");
    lcd.print(GPS.year, DEC);
    lcd.print("   ");

#ifdef SERIAL_OUT
    Serial.print("Fix: "); 
    Serial.print(GPS.fix);
    Serial.print(" quality: "); 
    Serial.println(GPS.fixquality); 
#endif
    if (GPS.fix) 
    {
#ifdef SERIAL_OUT
      Serial.print("Location: ");
      Serial.print(GPS.latitude, 4); 
      Serial.print(GPS.lat);
      Serial.print(", "); 
      Serial.print(GPS.longitude, 4); 
      Serial.println(GPS.lon);
#endif
      lcd.setCursor(2,0);
      lcd.print("Lat ");
      lcd.print(GPS.latitude, 4);
      lcd.print(GPS.lat);
      lcd.setCursor(3,0);
      lcd.print("Lon ");
      lcd.print(GPS.longitude, 4);
      lcd.print(GPS.lon);
#ifdef SERIAL_OUT
      Serial.print("Speed (knots): "); 
      Serial.println(GPS.speed);
      Serial.print("Angle: "); 
      Serial.println(GPS.angle);
      Serial.print("Altitude: "); 
      Serial.println(GPS.altitude);
      Serial.print("Satellites: "); 
      Serial.println(GPS.satellites);
#endif
    }
    else
    {
      lcd.setCursor(1,0);
      lcd.print("Acquiring a fix");
    }
  }
}


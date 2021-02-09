/**
 * Jedi Timer
 */

// Gesture Sensor
// Arduino Wire library is for communicating with any I2C device
#include <Wire.h>
#include "paj7620.h"

// 7 Segment Display
#include <Arduino.h>
#include <TM1637Display.h>

#define CLK 7         // TM1637 Clock Digital IO port on Arduino: 7
#define DIO 8         // TM1637 Digital IO port on Arduino: 10

// Timer Constants
#define SECOND 1000   // 1 second = 1,000 milliseconds
#define MINUTE 60000  // 1 minute = 60,000 milliseconds
#define READWAIT 50   // 5 seconds of no new readings will start the timer countdown


// Buzzer + pin
int speakerPin = 12;
// Alarm
int numTones = 12;
int tones[] = {261, 277, 294, 311, 330, 349, 370, 392, 415, 440,  493, 523};
//            mid C  C#   D    D#   E    F    F#   G    G#   A     B    C
//                DO     RE         MI   FA       SOL        LA   SI   DO

TM1637Display display(CLK, DIO);
int tally;
char counterstate;
int wait;
int lastwait;
int mytimer;
int accrueddelay;
uint8_t data[] = { 0xff, 0xff, 0xff, 0xff };
uint8_t blank[] = { 0x00, 0x00, 0x00, 0x00 };

void displayTime(int rawtally){
  int minutes = (rawtally / 60)*100 ;
  int seconds = rawtally % 60 ;
  display.showNumberDecEx((minutes + seconds),0b11100000);
}

void setup() {
  // Initialise serial connection to display results
  Serial.begin(9600);

  display.setBrightness(0x0f);

  // All segments on
  display.setSegments(data);
  delay(SECOND);
  display.setSegments(blank);


  // Initialise the sensor
  int error = paj7620Init();
  if(error) {
    Serial.print(F("Initialisation error code: "));
    Serial.println(error);
  }
  else {
    Serial.println(F("Ready!"));
  }
}

void loop() {
/*  Debugging to serial console
  Serial.print("LastWait: ");
  Serial.println(lastwait);
  Serial.print("T: ");
  Serial.println(tally);

  if ((wait % 50) == 0 ) {
    Serial.print("Wait: ");
    Serial.println(wait);
    Serial.print("Tally: ");
    Serial.println(tally);
  }
*/  

  wait = wait + 1;


  if ( ( tally > 0 ) && (( wait - lastwait ) >= READWAIT ) ){
    //Serial.println("################TIMER STARTING#################");
    wait = READWAIT + 1;
    lastwait = 0;
    if ( counterstate == 'Y') {
      mytimer = tally - (READWAIT/10) - 3;
      counterstate = 'X';
    } else {
      mytimer = mytimer - 1 ;
      tally = mytimer ;
      displayTime(mytimer); 
      delay(SECOND);
      // Reached end of count, reset all counters, Play BELL
      if ( mytimer == 0 ) {
        counterstate = 'Y';
        tally = 0;
        wait = 0;
        lastwait = 0;
        // Play BELL
        for (int i = 0 ; i <= 25 ; i++) {       
          tone(speakerPin, tones[2]);
          delay(100);
          tone(speakerPin, tones[10]);
          delay(200);
        }
        noTone(speakerPin);
      } 
    }
  }


  // Create a variable to hold the value of any gesture recognised
  byte gesture;
  // Error variable holds any error code
  int error;
  // Read Reg 0x43 of Bank 0 to get result of any recognised gesture, and store in 'gesture' variable
  error = paj7620ReadReg(0x43, 1, &gesture);
 
  if(!error) {
    switch (gesture) {
      case GES_RIGHT_FLAG:
        Serial.println(F("Right"));
        tone(speakerPin, tones[9]);
        delay(500);
        noTone(speakerPin);
        tally = tally + 300;   // ADD 5 Minutes when waving LEFT
        counterstate = 'Y';
        lastwait = wait ;
        for (int i = 0; i <= 4; i++) {
          displayTime(tally); 
          delay(100);
          display.setSegments(blank);
          delay(100);
        }
        break;
      case GES_LEFT_FLAG: 
        Serial.println(F("Left"));
        tone(speakerPin, tones[6]);
        delay(500);
        noTone(speakerPin);
        tally = tally + 60;    // ADD 1 Minute when waving LEFT
        counterstate = 'Y';
        lastwait = wait ;
        for (int i = 0; i <= 5; i++) {
          displayTime(tally); 
          delay(100);
          display.setSegments(blank);
          delay(100);
        }
        display.setSegments(blank);
        
        break;
      case GES_UP_FLAG:
        Serial.println(F("Up"));
        tone(speakerPin, tones[0]);
        delay(500);
        noTone(speakerPin);                  
        /*
        data[0] = display.encodeDigit(0);      // Just display 0000 when waving UP
        data[1] = display.encodeDigit(0);
        data[2] = display.encodeDigit(0);
        data[3] = display.encodeDigit(0);
        for (int i = 0 ; i <= 2 ; i++) {
          display.setSegments(data);
          delay(200);
          display.setSegments(blank);
          delay(200);        
        }
        */
        break;
      case GES_DOWN_FLAG:                      // Have to figure out what to do with the rest of these gestures
        Serial.println(F("Down"));
        tone(speakerPin, tones[3]);
        delay(SECOND/2);
        noTone(speakerPin);
        //for(int k=0; k <= 4; k++) {
        //   display.showNumberDecEx(0, (0x80 >> k), true);
        //   delay(SECOND);
        // }
        displayTime(tally);
        delay(SECOND);
        break;
      case GES_FORWARD_FLAG:
        Serial.println(F("Forward"));
        break;
      case GES_BACKWARD_FLAG:     
        Serial.println(F("Backward"));
        break;
      default:
        break;
    }
  }
 else {
    Serial.print(F("Error code: "));
    Serial.println(error);
 }
  // Add a small delay before polling the sensor
  delay(100);
}

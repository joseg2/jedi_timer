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
int speakerPin = 5;
// Alarm
int numTones = 12;
int tones[] = {261, 277, 294, 311, 330, 349, 370, 392, 415, 440,  493, 523};
//            mid C  C#   D    D#   E    F    F#   G    G#   A     B    C
//                DO     RE         MI   FA       SOL        LA   SI   DO

TM1637Display display(CLK, DIO);
char counterstate;      // Controls whether to start the timer counter
char resetstate;        // Reset counters flag
unsigned int wait;      // The main loop counter
unsigned int lastwait;  // The comparison value to keep track of counter
unsigned int tally;     // The entered time value
unsigned int mytimer;   // The timer value to render on the display
unsigned int recorded;  // The initial value recorded, to flash at the end of timer countdown
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

  // All display segments on
  display.setSegments(data);
  delay(SECOND);
  display.setSegments(blank);


  // Initialise the gesture sensor
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

  // RESET state has been recorded, set counters to zero now
  if ( resetstate == 'R' ) {
    tally = 0 ;
    wait = 0 ;
    resetstate = '0' ;
    recorded = 0 ;
    display.clear();

  }


  // START displaying Timer countdown now
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
          display.setSegments(blank);
          delay(100);
          tone(speakerPin, tones[10]);
          displayTime(recorded);
          delay(200);
        }
        noTone(speakerPin);
        display.setSegments(blank);
      }
    }
  }


  // READ Gesture input here

  byte gesture;                                // Create a variable to hold the value of any gesture recognised
  int error;                                   // Error variable holds any error code
  error = paj7620ReadReg(0x43, 1, &gesture);   // Read Reg 0x43 of Bank 0 to get result of any recognised gesture,
                                               //    and store in 'gesture' variable
  if(!error) {
    switch (gesture) {
      case GES_RIGHT_FLAG:
        Serial.println(F("Right"));
        tone(speakerPin, tones[9]);
        delay(500);
        noTone(speakerPin);
        tally = tally + 300;   // ADD 5 Minutes when waving RIGHT
        counterstate = 'Y';
        lastwait = wait ;
        for (int i = 0; i <= 4; i++) {
          displayTime(tally);
          delay(100);
          display.setSegments(blank);
          delay(100);
        }
        display.setSegments(blank);
        recorded = tally;
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
        recorded = tally;
        break;
      case GES_UP_FLAG:
        Serial.println(F("Up"));
        tone(speakerPin, tones[0]);
        delay(500);
        noTone(speakerPin);
        break;
      case GES_DOWN_FLAG:                      // Have to figure out what to do with the rest of these gestures
        Serial.println(F("Down"));
        tone(speakerPin, tones[3]);
        delay(SECOND/2);
        noTone(speakerPin);
        break;
      case GES_FORWARD_FLAG:                  // Reset counters when forward gesture is detected
        Serial.println(F("Forward"));
        tone(speakerPin, tones[8]);
        delay(SECOND/8);
        noTone(speakerPin);
        resetstate = 'R';
        break;
      case GES_BACKWARD_FLAG:                 // Reset counters when backward gesture is detected
        Serial.println(F("Backward"));
        tone(speakerPin, tones[9]);
        delay(SECOND/8);
        noTone(speakerPin);
        resetstate = 'R';
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

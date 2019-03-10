// Simple gamepad example that demonstraits how to read five Arduino
// digital pins and map them to the Arduino Joystick library.
//
// The digital pins 2 - 6 are grounded when they are pressed.
// Pin 2 = UP
// Pin 3 = RIGHT
// Pin 4 = DOWN
// Pin 5 = LEFT
// Pin 6 = FIRE
//
// NOTE: This sketch file is for use with Arduino Leonardo and
//       Arduino Micro only.
//
// by Matthew Heironimus
// 2016-11-24
//--------------------------------------------------------------------

#include <Joystick.h>
#include <EEPROM.h>

#define KEYPAD_UP     (10)
#define KEYPAD_DOWN   (16)
#define KEYPAD_LEFT   (14)
#define KEYPAD_RIGHT  (15)
#define KEYPAD_START  (A0)
#define KEYPAD_SELECT (A1)
#define KEYPAD_A      (2)
#define KEYPAD_B      (3)
#define KEYPAD_X      (4)
#define KEYPAD_Y      (5)
#define KEYPAD_L1     (6)
#define KEYPAD_R1     (7)
#define KEYPAD_FN     (9)
#define KEYPAD_ANALOG_X     (A2)
#define KEYPAD_ANALOG_Y     (A3)

#define BUTTON_COUNT   (2+4+2)

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_GAMEPAD,
  BUTTON_COUNT + 4 + BUTTON_COUNT, 0,       // Button Count, Hat Switch Count
  true, true, false,     // X and Y, but no Z Axis
  true, true, false,   // No Rx, Ry, or Rz
  false, false,          // No rudder or throttle
  false, false, false);  // No accelerator, brake, or steering

int buttonPort[4+BUTTON_COUNT+1] = {KEYPAD_UP, KEYPAD_DOWN, KEYPAD_LEFT, KEYPAD_RIGHT, 
                       KEYPAD_START, KEYPAD_SELECT,
                       KEYPAD_A, KEYPAD_B, KEYPAD_X, KEYPAD_Y, 
                       KEYPAD_L1, KEYPAD_R1,
                       KEYPAD_FN};
// Last state of the buttons
int lastShiftState[4+BUTTON_COUNT] = {0,};
int lastButtonState[4+BUTTON_COUNT] = {0,};

int minX, maxX;
int minY, maxY;
int anaCalMode = 0;

int fnOldButtonState = 0;
unsigned long lastFnPushTime = 0;
unsigned long lastFnReleaseTime = 0;
int fnMultiClickCount = 0;

void setup() {
  // Initialize Button Pins
  for (int i = 0; i < (4+BUTTON_COUNT+1); i++){
    pinMode(buttonPort[i], INPUT_PULLUP);
  }

  // read eeprom
  char sig[4];
  for(int i = 0; i < 4; i++){
    sig[i] = EEPROM.read(i);
  }
  if(sig[0] == 'C' && sig[1] == 'A' && sig[2] == 'L' && sig[3] == 'I'){
    minX = ((int)EEPROM.read(4)) << 8 | (int)EEPROM.read(5);
    maxX = ((int)EEPROM.read(6)) << 8 | (int)EEPROM.read(7);
    minY = ((int)EEPROM.read(8)) << 8 | (int)EEPROM.read(9);
    maxY = ((int)EEPROM.read(10)) << 8 | (int)EEPROM.read(11);
  } else {
    minX = 0;
    maxX = 1023;
    minY = 0;
    maxY = 1023;
  }

  // Initialize Joystick Library
  Joystick.begin();
  Joystick.setXAxisRange(-1, 1);
  Joystick.setYAxisRange(-1, 1);
  Joystick.setRxAxisRange(minX, maxX);
  Joystick.setRyAxisRange(minY, maxY);
}


void loop() {
  int fnButtonState = !digitalRead(KEYPAD_FN);
  unsigned long fnTime = millis();
  
  int anaX = analogRead(KEYPAD_ANALOG_X);
  int anaY = analogRead(KEYPAD_ANALOG_Y);

  if(fnButtonState != fnOldButtonState){
    if(!anaCalMode){
      if(fnButtonState && (fnTime - lastFnReleaseTime) < 700 && fnMultiClickCount >= 2){
        minX = maxX = anaX;
        minY = maxY = anaY;
        anaCalMode = 1;
      }
    } else {
      Joystick.setRxAxisRange(minX, maxX);
      Joystick.setRyAxisRange(minY, maxY);

      EEPROM.write(0, 'C');
      EEPROM.write(1, 'A');
      EEPROM.write(2, 'L');
      EEPROM.write(3, 'I');
      EEPROM.write(4, minX >> 8);
      EEPROM.write(5, minX & 0xFF);
      EEPROM.write(6, maxX >> 8);
      EEPROM.write(7, maxX & 0xFF);
      EEPROM.write(8, minY >> 8);
      EEPROM.write(9, minY & 0xFF);
      EEPROM.write(10, maxY >> 8);
      EEPROM.write(11, maxY & 0xFF);
      
      anaCalMode = 0;
    }
    
    if(fnButtonState){
      if(fnTime - lastFnReleaseTime < 700){
        fnMultiClickCount ++;
      } else {
        fnMultiClickCount = 0;
      }
      lastFnPushTime = fnTime;
    } else {
      if(fnTime - lastFnPushTime > 700){
        fnMultiClickCount = 0;
      }
      lastFnReleaseTime = fnTime;
    }
    fnOldButtonState = fnButtonState;
  } else {
    if(anaCalMode) {
      if(anaX < minX) minX = anaX;
      if(anaX > maxX) maxX = anaX;
      if(anaY < minY) minY = anaY;
      if(anaY > maxY) maxY = anaY;

      return;
    }
  }

  // Read pin values
  for (int i = 0; i < (4+BUTTON_COUNT); i++)
  {
    int currentButtonState = !digitalRead(buttonPort[i]);
    if (currentButtonState != lastButtonState[i])
    {
      int idx = -1;
      switch (i) {
        case 0: // UP
          if (currentButtonState == 1) {
            if(fnButtonState){
              idx = BUTTON_COUNT+0;
            } else {
              Joystick.setYAxis(-1);
            }
          } else {
            if(lastShiftState[i]){
              idx = BUTTON_COUNT+0;
            } else {
              Joystick.setYAxis(0);
            }
          }
          break;
        case 1: // DOWN
          if (currentButtonState == 1) {
            if(fnButtonState){
              idx = BUTTON_COUNT+1;
            } else {
              Joystick.setYAxis(1);
            }
          } else {
            if(lastShiftState[i]){
              idx = BUTTON_COUNT+1;
            } else {
              Joystick.setYAxis(0);
            }
          }
          break;
        case 2: // LEFT
          if (currentButtonState == 1) {
            if(fnButtonState){
              idx = BUTTON_COUNT+2;
            } else {
              Joystick.setXAxis(-1);
            }
          } else {
            if(lastShiftState[i]){
              idx = BUTTON_COUNT+2;
            } else {
              Joystick.setXAxis(0);
            }
          }
          break;
        case 3: // RIGHT
          if (currentButtonState == 1) {
            if(fnButtonState){
              idx = BUTTON_COUNT+3;
            } else {
              Joystick.setXAxis(1);
            }
          } else {
            if(lastShiftState[i]){
              idx = BUTTON_COUNT+3;
            } else {
              Joystick.setXAxis(0);
            }
          }
          break;
        case 4: // START
        case 5: // SELECT
        case 6: // A
        case 7: // B
        case 8: // X
        case 9: // Y
        case 10: // L1
        case 11: // R1
          idx = i - 4;
          break;
      }
      
      if(idx >= 0){
        if(currentButtonState){
          if(fnButtonState){
            if(idx < BUTTON_COUNT) idx += 4+BUTTON_COUNT;
          }
        } else {
          if(lastShiftState[i]){
            if(idx < BUTTON_COUNT) idx += 4+BUTTON_COUNT;
          }
        }
        Joystick.setButton(idx, currentButtonState);
      }
      
      lastShiftState[i] = fnButtonState;
      lastButtonState[i] = currentButtonState;
    }
  }

  if(true) {
    if(anaX < minX) anaX = minX;
    if(anaX > maxX) anaX = maxX;
    if(anaY < minY) anaY = minY;
    if(anaY > maxY) anaY = maxY;
    Joystick.setRxAxis(anaX);
    Joystick.setRyAxis(anaY);
  }
  
  delay(50);
}

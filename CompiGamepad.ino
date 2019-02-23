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
#define KEYPAD_L1      (6)
#define KEYPAD_R1      (7)

#define BUTTON_COUNT   (2+4+2)

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_GAMEPAD,
  BUTTON_COUNT, 0,       // Button Count, Hat Switch Count
  true, true, false,     // X and Y, but no Z Axis
  false, false, false,   // No Rx, Ry, or Rz
  false, false,          // No rudder or throttle
  false, false, false);  // No accelerator, brake, or steering

int buttonPort[4+BUTTON_COUNT] = {KEYPAD_UP, KEYPAD_DOWN, KEYPAD_LEFT, KEYPAD_RIGHT, 
                       KEYPAD_START, KEYPAD_SELECT,
                       KEYPAD_A, KEYPAD_B, KEYPAD_X, KEYPAD_Y, 
                       KEYPAD_L1, KEYPAD_R1 };
// Last state of the buttons
int lastButtonState[4+BUTTON_COUNT] = {0,};


void setup() {
  // Initialize Button Pins
  for (int i = 0; i < (4+BUTTON_COUNT); i++){
    pinMode(buttonPort[i], INPUT_PULLUP);
  }

  // Initialize Joystick Library
  Joystick.begin();
  Joystick.setXAxisRange(-1, 1);
  Joystick.setYAxisRange(-1, 1);
}


void loop() {

  // Read pin values
  for (int i = 0; i < (4+BUTTON_COUNT); i++)
  {
    int currentButtonState = !digitalRead(buttonPort[i]);
    if (currentButtonState != lastButtonState[i])
    {
      switch (i) {
        case 0: // UP
          if (currentButtonState == 1) {
            Joystick.setYAxis(-1);
          } else {
            Joystick.setYAxis(0);
          }
          break;
        case 1: // DOWN
          if (currentButtonState == 1) {
            Joystick.setYAxis(1);
          } else {
            Joystick.setYAxis(0);
          }
          break;
        case 2: // LEFT
          if (currentButtonState == 1) {
            Joystick.setXAxis(-1);
          } else {
            Joystick.setXAxis(0);
          }
          break;
        case 3: // RIGHT
          if (currentButtonState == 1) {
            Joystick.setXAxis(1);
          } else {
            Joystick.setXAxis(0);
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
          Joystick.setButton(i-4, currentButtonState);
          break;
      }
      lastButtonState[i] = currentButtonState;
    }
  }

  delay(10);
}


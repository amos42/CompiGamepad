#include <Joystick.h>
#include <EEPROM.h>


#define ON  (1)
#define OFF (0)

#define VERSION ("0.2.0")

#define USES_ANALOG_STICK (OFF)
#define USES_CMD_SHELL (ON)
#define USES_BATTERY_CHECK (ON)

#define FUNC_KEY_OFF     (0)
#define FUNC_KEY_FN      (1)
#define FUNC_KEY_HOTKEY  (2)
#define FUNC_KEY_MODE FUNC_KEY_HOTKEY

#define ANALOG_X_REVERSE (OFF)
#define ANALOG_Y_REVERSE (ON)

#define ANALOG_RETRY_COUNT        (5)
#define MULTICLICK_THRESHOLD_TIME (700)


#define GPIO_KEYPAD_UP     (9)
#define GPIO_KEYPAD_DOWN   (8)
#define GPIO_KEYPAD_LEFT   (7)
#define GPIO_KEYPAD_RIGHT  (6)
#define GPIO_KEYPAD_START  (5)
#define GPIO_KEYPAD_SELECT (4)
#define GPIO_KEYPAD_A      (3)
#define GPIO_KEYPAD_B      (2)
#define GPIO_KEYPAD_X      (A2)
#define GPIO_KEYPAD_Y      (A1)
#define GPIO_KEYPAD_L1     (A0)
#define GPIO_KEYPAD_R1     (15)
#define GPIO_KEYPAD_L2     (14)
#define GPIO_KEYPAD_R2     (16)
#define GPIO_KEYPAD_FN     (10)

#if USES_ANALOG_STICK
#define KEYPAD_ANALOG_X     (A2)
#define KEYPAD_ANALOG_Y     (A3)
#endif
#if USES_BATTERY_CHECK
#define BATTERY_CHECK       (A3)
#define BATTERY_MUL         (4.6)  /// on Arduino Pro Micro
#endif

#define ARROW_COUNT         (4)
#if FUNC_KEY_MODE == FUNC_KEY_HOTKEY
#define BUTTON_COUNT        (2+4+2+2+1)
#else
#define BUTTON_COUNT        (2+4+2+2)
#endif
#define REAL_BUTTON_COUNT   (ARROW_COUNT + BUTTON_COUNT)

#if USES_ANALOG_STICK
#define VIRTU_BUTTON_COUNT      (REAL_BUTTON_COUNT * 2)
#else
#define VIRTU_BUTTON_COUNT      (ARROW_COUNT + BUTTON_COUNT * 2)
#endif


#define ABS(a) (((a) >= 0)? (a) : -(a))


Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_GAMEPAD,
  VIRTU_BUTTON_COUNT,    // Button Count
  0,                     // Hat Switch Count
  true, true, false,     // X and Y, but no Z Axis
  false, false, false,   // No Rx, Ry, or Rz
  false, false,          // No rudder or throttle
  false, false, false);  // No accelerator, or brake, steering

int buttonPort[] = {  GPIO_KEYPAD_UP, GPIO_KEYPAD_DOWN, GPIO_KEYPAD_LEFT, GPIO_KEYPAD_RIGHT, 
                      GPIO_KEYPAD_START, GPIO_KEYPAD_SELECT,
                      GPIO_KEYPAD_A, GPIO_KEYPAD_B, GPIO_KEYPAD_X, GPIO_KEYPAD_Y, 
                      GPIO_KEYPAD_L1, GPIO_KEYPAD_R1,
                      GPIO_KEYPAD_L2, GPIO_KEYPAD_R2,
                      GPIO_KEYPAD_FN };

// Last state of the buttons
#if FUNC_KEY_MODE == FUNC_KEY_FN                       
char lastShiftState[REAL_BUTTON_COUNT] = {0,};
#endif
char lastButtonState[REAL_BUTTON_COUNT] = {0,};

#if USES_ANALOG_STICK
int minX, maxX;
int minY, maxY;
int anaCalMode = 0;
#endif

#if USES_CMD_SHELL
#define MAX_CMD_LEN 64
char cmdBuf[MAX_CMD_LEN+1];
int cmdBufIdx = 0;
long serialTime = 0;
#endif

#if FUNC_KEY_MODE == FUNC_KEY_FN                       
int fnOldButtonState = 0;
unsigned long lastFnPushTime = 0;
unsigned long lastFnReleaseTime = 0;
int fnMultiClickCount = 0;
#endif

int analogReadEx(int port) 
{
  int values[ANALOG_RETRY_COUNT];
  int sum = 0;
  for(int i = 0; i < ANALOG_RETRY_COUNT; i++){
    int v = analogRead(port);
    values[i] = v;
    sum += v;
  }
  int avg = sum / ANALOG_RETRY_COUNT;
  int cnt = ANALOG_RETRY_COUNT;
  for(int i = 0; i < ANALOG_RETRY_COUNT; i++){
    int v = values[i];
    if(ABS(v - avg) > 100){
      sum -= v;
      cnt --;
    }
  }
  return (cnt > 0) ? sum / cnt : 0;
}

void setButton(int idx, int state)
{
#if !USES_ANALOG_STICK
  switch(idx) {
    case 0: // UP
      Joystick.setYAxis(state ? -1 : 0);
      break;
    case 1: // DOWN
      Joystick.setYAxis(state ? 1 : 0);
      break;
    case 2 : // LEFT
      Joystick.setXAxis(state ? -1 : 0);
      break;
    case 3 : // RIGHT
      Joystick.setXAxis(state ? 1 : 0);
      break;
    case 4: // START
    case 5: // SELECT
    case 6: // A
    case 7: // B
    case 8: // X
    case 9: // Y
    case 10: // L1
    case 11: // R1
    case 12: // HOTKEY
      Joystick.setButton(idx - ARROW_COUNT, state);
      break;
  }
#else
    Joystick.setButton(idx, state);
#endif  
}

void setup() 
{
  // Initialize Button Pins
  for (int i = 0; i < REAL_BUTTON_COUNT+1; i++){
    pinMode(buttonPort[i], INPUT_PULLUP);
  }

#if USES_ANALOG_STICK
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
#endif

  // Initialize Joystick Library
  Joystick.begin();
#if USES_ANALOG_STICK
  Joystick.setXAxisRange(minX, maxX);
  Joystick.setYAxisRange(minY, maxY);
#else
  Joystick.setXAxisRange(-1, 1);
  Joystick.setYAxisRange(-1, 1);
#endif  
#if USES_CMD_SHELL
  Serial.begin(9600);
#endif
}


void loop() {
#if FUNC_KEY_MODE == FUNC_KEY_FN
  int fnButtonState = !digitalRead(KEYPAD_FN);
  unsigned long fnTime = millis();
#endif

#if USES_ANALOG_STICK
#if !ANALOG_X_REVERSE
  int anaX = analogReadEx(KEYPAD_ANALOG_X);
#else  
  int anaX = 1023 - analogReadEx(KEYPAD_ANALOG_X);
#endif
#if !ANALOG_Y_REVERSE
  int anaY = analogReadEx(KEYPAD_ANALOG_Y);
#else
  int anaY = 1023 - analogReadEx(KEYPAD_ANALOG_Y);
#endif
#endif

#if USES_CMD_SHELL
  int availCnt = Serial.available();
  if(availCnt > 0){
    if(millis() - serialTime > 1000){
      cmdBufIdx = 0;
    }
    
    while(availCnt > 0){
      char ch = Serial.read();
      availCnt--;
      if(ch == '\n'){
        cmdBuf[cmdBufIdx] = '\0';
        cmdBufIdx = 0;
  
        String cmdStr = String(cmdBuf);
        cmdStr.trim();
        //Serial.println("> input: " + cmdStr);
        if(cmdStr == "help"){
          Serial.println("Suc:Help.....");
        } else if(cmdStr == "version"){
          Serial.print("Suc:"); Serial.println(VERSION);
        }
#if USES_BATTERY_CHECK    
        else if(cmdStr == "battery"){
          int batteryLvl = analogReadEx(BATTERY_CHECK);
          float v = batteryLvl * BATTERY_MUL / (1024.0 - 1);
          Serial.print("Suc:"); Serial.println(v, 2);
        }
#endif
        else {
          Serial.println("Err:Known Command");
        }
      } else {
        if(cmdBufIdx >= MAX_CMD_LEN){
          cmdBufIdx = 0;
        }
        cmdBuf[cmdBufIdx++] = ch;
      }
    }
    serialTime = millis();
  }
#endif

#if FUNC_KEY_MODE == FUNC_KEY_FN
  if(fnButtonState != fnOldButtonState){
#if USES_ANALOG_STICK
    if(!anaCalMode){
      if(fnButtonState && (fnTime - lastFnReleaseTime) < MULTICLICK_THRESHOLD_TIME && fnMultiClickCount >= 2){
        minX = maxX = anaX;
        minY = maxY = anaY;
        anaCalMode = 1;
      }
    } else {
      Joystick.setXAxisRange(minX, maxX);
      Joystick.setYAxisRange(minY, maxY);

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
#endif

#if FUNC_KEY_MODE == FUNC_KEY_FN
    if(fnButtonState){
      if(fnTime - lastFnReleaseTime < MULTICLICK_THRESHOLD_TIME){
        fnMultiClickCount ++;
      } else {
        fnMultiClickCount = 0;
      }
      lastFnPushTime = fnTime;
    } else {
      if(fnTime - lastFnPushTime > MULTICLICK_THRESHOLD_TIME){
        fnMultiClickCount = 0;
      }
      lastFnReleaseTime = fnTime;
    }
    fnOldButtonState = fnButtonState;
#endif
    
#if USES_ANALOG_STICK
  } else {
    if(anaCalMode) {
      if(anaX < minX) minX = anaX;
      if(anaX > maxX) maxX = anaX;
      if(anaY < minY) minY = anaY;
      if(anaY > maxY) maxY = anaY;

      return;
    }
#endif    
  }
#endif

  // Read pin values
  for (int i = 0; i < REAL_BUTTON_COUNT; i++)
  {
    int currentButtonState = !digitalRead(buttonPort[i]);
    if (currentButtonState != lastButtonState[i])
    {
      int idx = -1;
      switch (i) {
#if !USES_ANALOG_STICK
        case 0: // UP
        case 1: // DOWN
        case 2: // LEFT
        case 3: // RIGHT
          idx = i;
          break;
#else
        case 0: // UP
        case 1: // DOWN
        case 2: // LEFT
        case 3: // RIGHT
#endif          
        case 4: // START
        case 5: // SELECT
        case 6: // A
        case 7: // B
        case 8: // X
        case 9: // Y
        case 10: // L1
        case 11: // R1
#if USES_ANALOG_STICK
          idx = i;
#else
          idx = i - ARROW_COUNT;
#endif          
          break;
      }
      
      if(idx >= 0){
#if FUNC_KEY_MODE == FUNC_KEY_FN                       
        if(currentButtonState){
          if(fnButtonState){
#if USES_ANALOG_STICK
            idx += REAL_BUTTON_COUNT;
#else
            if(idx < BUTTON_COUNT) idx += ARROW_COUNT+BUTTON_COUNT;
#endif            
          }
        } else {
          if(lastShiftState[i]){
#if USES_ANALOG_STICK
            idx += REAL_BUTTON_COUNT;
#else
            if(idx < BUTTON_COUNT) idx += ARROW_COUNT+BUTTON_COUNT;
#endif            
          }
        }
#endif        
        setButton(idx, currentButtonState);
      }

#if FUNC_KEY_MODE == FUNC_KEY_FN                       
      lastShiftState[i] = fnButtonState;
#endif      
      lastButtonState[i] = currentButtonState;
    }
  }

#if USES_ANALOG_STICK
  if(anaX < minX) anaX = minX;
  if(anaX > maxX) anaX = maxX;
  if(anaY < minY) anaY = minY;
  if(anaY > maxY) anaY = maxY;
  Joystick.setXAxis(anaX);
  Joystick.setYAxis(anaY);
#endif
  
  delay(10);
}

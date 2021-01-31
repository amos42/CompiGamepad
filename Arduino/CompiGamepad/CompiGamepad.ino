#include <Joystick.h>
#include <EEPROM.h>


#define ON  (1)
#define OFF (0)


//================= 사용자 설정 (시작) =================

// 버전
#define VERSION ("0.2.0")

// feature 설정
#define USES_ANALOG_STICK   (OFF)   /// 아날로그 스틱 사용 여부 (4방향 버튼은 아날로그 스틱 사용 시 그냥 버튼, 사용하지 않을 시 스틱으로 처리)
#define USES_CMD_SHELL      (OFF)   /// 시리얼 커맨드 쉘 사용 여부 (battery 전압값 얻기 위해서는 필수)
#define USES_BATTERY_CHECK  (OFF)   /// battery 전압값 얻기 여부
#define USES_BUTTON_L2R2    (OFF)   /// L2, R2 버튼 사용 여부

// 확장키 모드
#define EXTRA_KEY_MODE_OFF     (0)   /// 확장키 사용하지 않음
#define EXTRA_KEY_MODE_FN      (1)   /// 확장키를 펑션키로 사용
#define EXTRA_KEY_MODE_HOTKEY  (2)   /// 확장키를 핫키로 사용
#define EXTRA_KEY_MODE         EXTRA_KEY_MODE_HOTKEY

// GPIO 매핑
#if USES_ANALOG_STICK
#define GPIO_KEYPAD_ANALOG_X  (A0)
#define GPIO_KEYPAD_ANALOG_Y  (A1)
#else
#define GPIO_KEYPAD_UP        (10)
#define GPIO_KEYPAD_DOWN      (16)
#define GPIO_KEYPAD_LEFT      (14)
#define GPIO_KEYPAD_RIGHT     (15)
#endif
#define GPIO_KEYPAD_START     (2)
#define GPIO_KEYPAD_SELECT    (3)
#define GPIO_KEYPAD_A         (4)
#define GPIO_KEYPAD_B         (5)
#define GPIO_KEYPAD_X         (6)
#define GPIO_KEYPAD_Y         (7)
#define GPIO_KEYPAD_L1        (8)
#define GPIO_KEYPAD_R1        (9)
#if USES_BUTTON_L2R2
#define GPIO_KEYPAD_L2        (A0)
#define GPIO_KEYPAD_R2        (A1)
#endif
#if EXTRA_KEY_MODE != EXTRA_KEY_MODE_OFF
#define GPIO_KEYPAD_EXTRA     (A2)
#endif
#if USES_BATTERY_CHECK
#define GPIO_BATTERY_CHECK    (A3)
#endif

//================= 사용자 설정 (끝) =================


#if USES_ANALOG_STICK
// 아날로그 스틱용 설정
#define ANALOG_X_REVERSE          (OFF)
#define ANALOG_Y_REVERSE          (ON)
#define ANALOG_RETRY_COUNT        (5)
#define MULTICLICK_THRESHOLD_TIME (700)
#endif

#if USES_BATTERY_CHECK
#define BATTERY_MUL         (4.6)  /// on Arduino Pro Micro
#endif

// 버튼 갯수 관련 상수
#define ARROW_COUNT         (4)

#if EXTRA_KEY_MODE == EXTRA_KEY_MODE_HOTKEY
#define BUTTON_COUNT        (2+4+2+1)
#else
#define BUTTON_COUNT        (2+4+2)
#endif

#if USES_BUTTON_L2R2
#define L2R2_BUTTON_COUNT   (2)
#else
#define L2R2_BUTTON_COUNT   (0)
#endif

#define REAL_BUTTON_COUNT   (ARROW_COUNT + BUTTON_COUNT + L2R2_BUTTON_COUNT)

#if EXTRA_KEY_MODE == EXTRA_KEY_MODE_FN
#define VIRTU_BUTTON_COUNT      (ARROW_COUNT + (BUTTON_COUNT + L2R2_BUTTON_COUNT) * 2)
#else
#define VIRTU_BUTTON_COUNT      (ARROW_COUNT + BUTTON_COUNT + L2R2_BUTTON_COUNT)
#endif

#if EXTRA_KEY_MODE == EXTRA_KEY_MODE_FN
#define GPIO_COUNT              (REAL_BUTTON_COUNT + 1)
#else
#define GPIO_COUNT              (REAL_BUTTON_COUNT)
#endif

// 매크로 함수 선언
#define ABS(a) (((a) >= 0)? (a) : -(a))


Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_GAMEPAD,
    VIRTU_BUTTON_COUNT,    // Button Count
    0,                     // Hat Switch Count
    true, true, false,     // X and Y, but no Z Axis
    false, false, false,   // No Rx, Ry, or Rz
    false, false,          // No rudder or throttle
    false, false, false    // No accelerator, or brake, steering
);

static int buttonPort[GPIO_COUNT] = {
    GPIO_KEYPAD_UP, GPIO_KEYPAD_DOWN, GPIO_KEYPAD_LEFT, GPIO_KEYPAD_RIGHT,
    GPIO_KEYPAD_START, GPIO_KEYPAD_SELECT,
    GPIO_KEYPAD_A, GPIO_KEYPAD_B, GPIO_KEYPAD_X, GPIO_KEYPAD_Y,
    GPIO_KEYPAD_L1, GPIO_KEYPAD_R1
#if USES_BUTTON_L2R2                      
    , GPIO_KEYPAD_L2, GPIO_KEYPAD_R2
#endif                      
#if EXTRA_KEY_MODE != EXTRA_KEY_MODE_OFF
    , GPIO_KEYPAD_EXTRA
#endif                      
};

// Last state of the buttons
#if EXTRA_KEY_MODE == EXTRA_KEY_MODE_FN                       
char lastShiftState[REAL_BUTTON_COUNT] = { 0, };
#endif
char lastButtonState[REAL_BUTTON_COUNT] = { 0, };

#if USES_ANALOG_STICK
// 아날로그 스틱용 전역변수
int minX, maxX;
int minY, maxY;
int anaCalMode = 0;
#endif

#if USES_CMD_SHELL
// Command Shell용 전역변수
#define MAX_CMD_LEN 64
char cmdBuf[MAX_CMD_LEN + 1];
int cmdBufIdx = 0;
long serialTime = 0;
#endif

#if EXTRA_KEY_MODE == EXTRA_KEY_MODE_FN                       
// Fn키 처리용 전역변수
int fnOldButtonState = 0;
unsigned long lastFnPushTime = 0;
unsigned long lastFnReleaseTime = 0;
int fnMultiClickCount = 0;
#endif

#if USES_ANALOG_STICK || USES_BATTERY_CHECK
// 아날로그 값을 읽는다
int analogReadEx(int port)
{
    int values[ANALOG_RETRY_COUNT];
    int sum = 0;
    for (int i = 0; i < ANALOG_RETRY_COUNT; i++) {
        int v = analogRead(port);
        values[i] = v;
        sum += v;
    }
    int avg = sum / ANALOG_RETRY_COUNT;
    int cnt = ANALOG_RETRY_COUNT;
    for (int i = 0; i < ANALOG_RETRY_COUNT; i++) {
        int v = values[i];
        if (ABS(v - avg) > 100) {
            sum -= v;
            cnt--;
        }
    }
    return (cnt > 0) ? sum / cnt : 0;
}
#endif

// 버튼 상태 세팅
void setButton(int idx, int state)
{
#if !USES_ANALOG_STICK
    switch (idx) {
        case 0: // UP
            Joystick.setYAxis(state ? -1 : 0);
            break;
        case 1: // DOWN
            Joystick.setYAxis(state ? 1 : 0);
            break;
        case 2: // LEFT
            Joystick.setXAxis(state ? -1 : 0);
            break;
        case 3: // RIGHT
            Joystick.setXAxis(state ? 1 : 0);
            break;
        default:
        case 4: // START
        case 5: // SELECT
        case 6: // A
        case 7: // B
        case 8: // X
        case 9: // Y
        case 10: // L1
        case 11: // R1
#if USAGE_BUTTON_L2R2      
        case 12: // L2
        case 13: // R2
#if EXTRA_KEY_MODE == EXTRA_KEY_MODE_HOTKEY
        case 14: // HOTKEY
#endif      
#else
#if EXTRA_KEY_MODE == EXTRA_KEY_MODE_HOTKEY
        case 12: // HOTKEY
#endif      
#endif
            Joystick.setButton(idx - ARROW_COUNT, state);
            break;
    }
#else
    Joystick.setButton(idx, state);
#endif  
}


#if USES_ANALOG_STICK
// EEPROM에서 아날로그 캘리브레이션 값 읽기
void readCalibFromEEPROM()
{
    // read eeprom
    char sig[4];
    for (int i = 0; i < 4; i++) {
        sig[i] = EEPROM.read(i);
    }
    if (sig[0] == 'C' && sig[1] == 'A' && sig[2] == 'L' && sig[3] == 'I') {
        minX = ((int)EEPROM.read(4)) << 8 | (int)EEPROM.read(5);
        maxX = ((int)EEPROM.read(6)) << 8 | (int)EEPROM.read(7);
        minY = ((int)EEPROM.read(8)) << 8 | (int)EEPROM.read(9);
        maxY = ((int)EEPROM.read(10)) << 8 | (int)EEPROM.read(11);
    }
    else {
        minX = 0;
        maxX = 1023;
        minY = 0;
        maxY = 1023;
    }
}

//EEPROM에 아날로그 클래브레이션 값 저장
void updateCalibtoEEPROM()
{
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
}
#endif

#if USES_CMD_SHELL
// Command Shell을 수행
void processCmdShell()
{
    int availCnt = Serial.available();
    if (availCnt > 0) {
        if (millis() - serialTime > 1000) {
            cmdBufIdx = 0;
        }

        while (availCnt > 0) {
            char ch = Serial.read();
            availCnt--;
            if (ch == '\n') {
                cmdBuf[cmdBufIdx] = '\0';
                cmdBufIdx = 0;

                String cmdStr = String(cmdBuf);
                cmdStr.trim();
                //Serial.println("> input: " + cmdStr);
                if (cmdStr == "help") {
                    Serial.print("Suc:");
                    Serial.print("help");
                    Serial.print(",version");
#if USES_BATTERY_CHECK    
                    Serial.print(",battery");
#endif
                    Serial.println("");
                }
                else if (cmdStr == "version") {
                    Serial.print("Suc:"); Serial.println(VERSION);
                }
#if USES_BATTERY_CHECK    
                else if (cmdStr == "battery") {
                    int batteryLvl = analogReadEx(BATTERY_CHECK);
                    float v = batteryLvl * BATTERY_MUL / (1024.0 - 1);
                    Serial.print("Suc:"); Serial.println(v, 2);
                }
#endif
                else {
                    Serial.println("Err:Known Command");
                }
            }
            else {
                if (cmdBufIdx >= MAX_CMD_LEN) {
                    cmdBufIdx = 0;
                }
                cmdBuf[cmdBufIdx++] = ch;
            }
        }
        serialTime = millis();
    }
}
#endif

// 초기화
void setup()
{
    // Initialize Button Pins
    for (int i = 0; i < GPIO_COUNT; i++) {
        pinMode(buttonPort[i], INPUT_PULLUP);
    }

#if USES_ANALOG_STICK
    // read eeprom
    readCalibFromEEPROM();
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


// 메인루프
void loop() {
#if EXTRA_KEY_MODE == EXTRA_KEY_MODE_FN
    int fnButtonState = !digitalRead(GPIO_KEYPAD_EXTRA);
    unsigned long fnTime = millis();
#endif

#if USES_ANALOG_STICK
    int anaX = analogReadEx(GPIO_KEYPAD_ANALOG_X);
#if ANALOG_X_REVERSE
    anaX = 1023 - anaX;
#endif
    int anaY = analogReadEx(GPIO_KEYPAD_ANALOG_Y);
#if ANALOG_Y_REVERSE
    anaY = 1023 - anaY;
#endif
#endif

#if USES_CMD_SHELL
    processCmdShell();
#endif

#if EXTRA_KEY_MODE == EXTRA_KEY_MODE_FN && USES_ANALOG_STICK
    if (fnButtonState != fnOldButtonState) {
        if (!anaCalMode) {
            if (fnButtonState && (fnTime - lastFnReleaseTime) < MULTICLICK_THRESHOLD_TIME && fnMultiClickCount >= 2) {
                minX = maxX = anaX;
                minY = maxY = anaY;
                anaCalMode = 1;
            }
        }
        else {
            updateCalibtoEEPROM();
            anaCalMode = 0;
        }

        if (fnButtonState) {
            if (fnTime - lastFnReleaseTime < MULTICLICK_THRESHOLD_TIME) {
                fnMultiClickCount++;
            }
            else {
                fnMultiClickCount = 0;
            }
            lastFnPushTime = fnTime;
        }
        else {
            if (fnTime - lastFnPushTime > MULTICLICK_THRESHOLD_TIME) {
                fnMultiClickCount = 0;
            }
            lastFnReleaseTime = fnTime;
        }
        fnOldButtonState = fnButtonState;
    }
    else {
        if (anaCalMode) {
            if (anaX < minX) minX = anaX;
            if (anaX > maxX) maxX = anaX;
            if (anaY < minY) minY = anaY;
            if (anaY > maxY) maxY = anaY;
            return;
        }
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
                    //idx = i - ARROW_COUNT;
                    idx = i;
#endif          
                    break;
            }

            if (idx >= 0) {
#if EXTRA_KEY_MODE == EXTRA_KEY_MODE_FN
                if (currentButtonState) {
                    if (fnButtonState) {
#if USES_ANALOG_STICK
                        idx += REAL_BUTTON_COUNT;
#else
                        if (idx < BUTTON_COUNT) idx += ARROW_COUNT + BUTTON_COUNT;
#endif            
                    }
                }
                else {
                    if (lastShiftState[i]) {
#if USES_ANALOG_STICK
                        idx += REAL_BUTTON_COUNT;
#else
                        if (idx < BUTTON_COUNT) idx += ARROW_COUNT + BUTTON_COUNT;
#endif            
                    }
                }
#endif        
                setButton(idx, currentButtonState);
            }

#if EXTRA_KEY_MODE == EXTRA_KEY_MODE_FN
            lastShiftState[i] = fnButtonState;
#endif      
            lastButtonState[i] = currentButtonState;
        }
    }

#if USES_ANALOG_STICK
    if (anaX < minX) anaX = minX;
    if (anaX > maxX) anaX = maxX;
    if (anaY < minY) anaY = minY;
    if (anaY > maxY) anaY = maxY;
    Joystick.setXAxis(anaX);
    Joystick.setYAxis(anaY);
#endif

    delay(10);
}

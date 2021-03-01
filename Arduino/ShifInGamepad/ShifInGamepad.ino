/*
The MIT License (MIT)
Copyright (c) <year> <copyright holders>
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
ShiftInGamepad.ino : 컴파이용 게임패드 펌
Made by : Amos42
*/

#include <Joystick.h>


#define ON  (1)
#define OFF (0)
#define HIGH  (1)
#define LOW (0)


//================= 사용자 설정 (시작) =================

// 버전
#define VERSION ("0.1.0")

// feature 설정
#define USES_CMD_SHELL      (OFF)   /// 시리얼 커맨드 쉘 사용 여부 (battery 전압값 얻기 위해서는 필수)
#define USES_BATTERY_CHECK  (OFF)   /// battery 전압값 얻기 여부

// 키패드 갯수
#define KEYPAD_COUNT          (2)

// 버튼 갯수
//#define KEYPAD_BUTTON_COUNT   (4+2+4+2)

// GPIO 매핑
#define GPIO_SCLK             (15)
#define GPIO_MISO             (14)
#define GPIO_MOSI             (16)

#if USES_BATTERY_CHECK
#define GPIO_BATTERY_CHECK    (A3)
#endif

//================= 사용자 설정 (끝) =================

#define PULSE_WIDTH_USEC    (5)

#if USES_BATTERY_CHECK
#define BATTERY_MUL         (4.6)  /// on Arduino Pro Micro
#endif

// 버튼 갯수 관련 상수
#define ARROW_COUNT         (4)
#define BUTTON_COUNT        (2+4+2)

#define ALL_BUTTON_COUNT   (ARROW_COUNT + BUTTON_COUNT)

// 매크로 함수 선언
#define ABS(a) (((a) >= 0)? (a) : -(a))

Joystick_ *Joysticks[KEYPAD_COUNT];

// Last state of the buttons
char lastButtonState[KEYPAD_COUNT][ALL_BUTTON_COUNT] = { 0, };

#if USES_CMD_SHELL
// Command Shell용 전역변수
#define MAX_CMD_LEN 64
char cmdBuf[MAX_CMD_LEN + 1];
int cmdBufIdx = 0;
long serialTime = 0;
#endif

#if USES_BATTERY_CHECK
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
void setButton(int id, int idx, int state)
{
/*    
    switch (idx) {
        case 0: // UP
            Joysticks[id]->setYAxis(state ? -1 : 0);
            break;
        case 1: // DOWN
            Joysticks[id]->setYAxis(state ? 1 : 0);
            break;
        case 2: // LEFT
            Joysticks[id]->setXAxis(state ? -1 : 0);
            break;
        case 3: // RIGHT
            Joysticks[id]->setXAxis(state ? 1 : 0);
            break;
        case 4: // START
        case 5: // SELECT
        case 6: // A
        case 7: // B
        case 8: // X
        case 9: // Y
        case 10: // L1
        case 11: // R1
            Joysticks[id]->setButton(idx - ARROW_COUNT, state);
            break;
    }
*/  
    // 월광보합 케이스에 맞는 키배치  
    int btnIdx = -1;
    switch (idx) {
        case 9: // UP
            Joysticks[id]->setYAxis(state ? -1 : 0);
            break;
        case 8: // DOWN
            Joysticks[id]->setYAxis(state ? 1 : 0);
            break;
        case 7: // LEFT
            Joysticks[id]->setXAxis(state ? -1 : 0);
            break;
        case 6: // RIGHT
            Joysticks[id]->setXAxis(state ? 1 : 0);
            break;
        case 10: // START
            btnIdx = 0;
            break;
        case 11: // SELECT
            btnIdx = 1;
            break;
        case 0: // A
            btnIdx = 2;
            break;
        case 1: // B
            btnIdx = 3;
            break;
        case 3: // X
            btnIdx = 4;
            break;
        case 4: // Y
            btnIdx = 5;
            break;
        case 5: // L1
            btnIdx = 6;
            break;
        case 2: // R1
            btnIdx = 7;
            break;
    }
    if(btnIdx >= 0) Joysticks[id]->setButton(btnIdx, state);
}


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
    pinMode(GPIO_SCLK, OUTPUT);
    pinMode(GPIO_MOSI, OUTPUT);
    pinMode(GPIO_MISO, INPUT);

    // // Initialize Joystick Library
    for(int i = 0; i < KEYPAD_COUNT; i++)
    {
        Joysticks[i] = new Joystick_(JOYSTICK_DEFAULT_REPORT_ID + i, JOYSTICK_TYPE_GAMEPAD,
            BUTTON_COUNT,          // Button Count
            0,                     // Hat Switch Count
            true, true, false,     // X and Y, but no Z Axis
            false, false, false,   // No Rx, Ry, or Rz
            false, false,          // No rudder or throttle
            false, false, false    // No accelerator, or brake, steering
        );

        Joysticks[i]->begin();
        Joysticks[i]->setXAxisRange(-1, 1);
        Joysticks[i]->setYAxisRange(-1, 1);
    }

#if USES_CMD_SHELL
    Serial.begin(9600);
#endif
}


// 메인루프
void loop() {
#if USES_CMD_SHELL
    processCmdShell();
#endif

    byte values[KEYPAD_COUNT * ALL_BUTTON_COUNT];
    digitalWrite(GPIO_MOSI, LOW);
    delayMicroseconds(PULSE_WIDTH_USEC);
    digitalWrite(GPIO_MOSI, HIGH);
    for(int i = 0; i < (KEYPAD_COUNT * ALL_BUTTON_COUNT); i++) 
    {
        values[i/8*8 + 7-i%8] = digitalRead(GPIO_MISO);

        digitalWrite(GPIO_SCLK, HIGH);
        delayMicroseconds(PULSE_WIDTH_USEC);
        digitalWrite(GPIO_SCLK, LOW);
    }

    for(int id = 0; id < KEYPAD_COUNT; id++){
        int stIdx = id * ALL_BUTTON_COUNT;
        // Read pin values
        for (int i = 0; i < ALL_BUTTON_COUNT; i++)
        {
            int currentButtonState = !values[stIdx + i];
            if (currentButtonState != lastButtonState[id][i])
            {
                setButton(id, i, currentButtonState);

                lastButtonState[id][i] = currentButtonState;
            }
        }
    }

    delay(10);
}

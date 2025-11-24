#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SH1107_SEEED_128X128_1_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);

const uint8_t PIN_ENC_CLK = 2;
const uint8_t PIN_ENC_DT = 3;
const uint8_t PIN_ENC_SW = 4;

const uint8_t PIN_ROW_1 = 5;
const uint8_t PIN_ROW_2 = 6;
const uint8_t PIN_ROW_3 = 7;

const uint8_t PIN_COL_1 = 8;
const uint8_t PIN_LED = 9;
const uint8_t PIN_COL_2 = 10;
const uint8_t PIN_COL_3 = 11;

const uint8_t EXT_D12 = 12;
const uint8_t EXT_D13 = 13;
const uint8_t EXT_A0 = A0;
const uint8_t EXT_A1 = A1;
const uint8_t EXT_A2 = A2;
const uint8_t EXT_A3 = A3;
const uint8_t EXT_A6 = A6;
const uint8_t EXT_A7 = A7;

volatile long encoderValue = 0;
long lastEncoderValue = 0;
volatile int lastCLKState;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;
bool ledState = false;
String currentAction = "Ready";

const int rows[] = {PIN_ROW_1, PIN_ROW_2, PIN_ROW_3};
const int cols[] = {PIN_COL_1, PIN_COL_2, PIN_COL_3};

void handleEncoder()
{
    int clkState = digitalRead(PIN_ENC_CLK);
    int dtState = digitalRead(PIN_ENC_DT);

    if (clkState != lastCLKState)
    {
        if (dtState != clkState)
        {
            encoderValue++;
        }
        else
        {
            encoderValue--;
        }
        lastCLKState = clkState;
    }
}

void drawScreen()
{
    u8g2.firstPage();
    do
    {
        u8g2.setFont(u8g2_font_ncenB08_tr);
        u8g2.drawStr(0, 10, "CONSOLE DECK");

        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(0, 30, "Encoder Pos:");
        u8g2.setCursor(80, 30);
        u8g2.print(encoderValue);

        u8g2.drawStr(0, 50, "Action:");
        u8g2.setFont(u8g2_font_ncenB10_tr);
        u8g2.setCursor(0, 70);
        u8g2.print(currentAction);

        u8g2.setFont(u8g2_font_6x10_tr);
        u8g2.drawStr(0, 90, "LED Status:");
        u8g2.drawStr(70, 90, ledState ? "ON" : "OFF");

    } while (u8g2.nextPage());
}

void setup()
{
    u8g2.begin();
    u8g2.setFontMode(1);

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    pinMode(PIN_ENC_CLK, INPUT);
    pinMode(PIN_ENC_DT, INPUT);
    pinMode(PIN_ENC_SW, INPUT_PULLUP);

    for (int i = 0; i < 3; i++)
    {
        pinMode(rows[i], OUTPUT);
        digitalWrite(rows[i], HIGH);
        pinMode(cols[i], INPUT_PULLUP);
    }

    lastCLKState = digitalRead(PIN_ENC_CLK);
    attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), handleEncoder, CHANGE);

    drawScreen();
}

void loop()
{
    bool needsUpdate = false;

    if (encoderValue != lastEncoderValue)
    {
        if (encoderValue > lastEncoderValue)
        {
            currentAction = "Enc Right";
        }
        else
        {
            currentAction = "Enc Left";
        }
        lastEncoderValue = encoderValue;
        needsUpdate = true;
    }

    if (digitalRead(PIN_ENC_SW) == LOW)
    {
        if (millis() - lastDebounceTime > debounceDelay)
        {
            ledState = !ledState;
            digitalWrite(PIN_LED, ledState);
            currentAction = "Enc Click";
            needsUpdate = true;
            lastDebounceTime = millis();
        }
    }

    for (int r = 0; r < 3; r++)
    {
        digitalWrite(rows[r], LOW);

        for (int c = 0; c < 3; c++)
        {
            if (digitalRead(cols[c]) == LOW)
            {
                if (millis() - lastDebounceTime > debounceDelay)
                {

                    int btnNum = 0;
                    if (r == 0)
                    { // Row 1: SW1, SW4, SW7
                        if (c == 0)
                            btnNum = 1;
                        if (c == 1)
                            btnNum = 4;
                        if (c == 2)
                            btnNum = 7;
                    }
                    else if (r == 1)
                    { // Row 2: SW2, SW5, SW8
                        if (c == 0)
                            btnNum = 2;
                        if (c == 1)
                            btnNum = 5;
                        if (c == 2)
                            btnNum = 8;
                    }
                    else if (r == 2)
                    { // Row 3: SW3, SW6, SW9
                        if (c == 0)
                            btnNum = 3;
                        if (c == 1)
                            btnNum = 6;
                        if (c == 2)
                            btnNum = 9;
                    }

                    currentAction = "BTN " + String(btnNum);
                    digitalWrite(PIN_LED, HIGH);
                    ledState = true;
                    needsUpdate = true;
                    lastDebounceTime = millis();
                }
            }
        }
        digitalWrite(rows[r], HIGH);
    }

    if (needsUpdate)
    {
        drawScreen();
    }
}
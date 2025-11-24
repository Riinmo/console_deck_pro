#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "dashboard_graphics.h"

// --- ENUMS & CONFIG ---
enum ModuleType
{
  MOD_NONE,
  MOD_SLIDERS,
  MOD_EXT_BTNS,
  MOD_KNOBS
};
ModuleType currentModule = MOD_SLIDERS;

enum ScreenState
{
  STATE_DEFAULT,
  STATE_MENU,
  STATE_SUBMENU
};
ScreenState currentState = STATE_DEFAULT;

enum HomeMode
{
  HOME_BASIC,
  HOME_ADVANCE,
  HOME_SPLASH,
  HOME_TESTING
};
HomeMode currentHomeMode = HOME_TESTING;

bool backlightEnabled = true;
const unsigned long LONG_PRESS_MS = 800;

// Costruttore Display
U8G2_SH1107_SEEED_128X128_1_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);

// --- DUMMY DATA ---
int cpuUsage = 45;
int gpuUsage = 32;
float memFree = 8.4;
float totalRAM = 16.0;
int pcTemp = 62;
int tempGPU = 58;
float netDown = 12.5;
float netUp = 4.2;

// --- PIN DEFINITIONS ---
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

const uint8_t EXT_A0 = A0;
const uint8_t EXT_A1 = A1;
const uint8_t EXT_A2 = A2;
const uint8_t EXT_A3 = A3;

// --- VARS ---
volatile long encoderValue = 0;
long lastEncoderValue = 0;
volatile int lastCLKState;
unsigned long lastActionTime = 0;

unsigned long btnPressStartTime = 0;
bool btnPressed = false;
bool longPressTriggered = false;

int menuIndex = 0;
const int MENU_ITEMS = 3;
int submenuIndex = 0;
int submenuMaxItems = 0;

// Stringhe Submenu (in PROGMEM per risparmiare RAM se servisse, ma qui sono poche)
const char *subItemsHome[] = {"PC INFO BASIC", "PC INFO ADVANCE", "SPLASH IMAGE", "TESTING"};
const char *subItemsModules[] = {"NONE", "BUTTONS", "SLIDERS", "KNOBS"};
const char *subItemsBacklight[] = {"YES", "NO"};

// System Action
char mainAction[16] = "Ready"; // char array invece di String per efficienza
char extAction[24] = "";
const int rows[] = {PIN_ROW_1, PIN_ROW_2, PIN_ROW_3};
const int cols[] = {PIN_COL_1, PIN_COL_2, PIN_COL_3};
const int NUM_READINGS = 16;
bool moduleConnected = false;
unsigned long disconnectTimer = 0;

// --- DRAWING HELPERS ---
void drawHomeIcon(int x, int y)
{
  u8g2.drawTriangle(x, y - 28, x - 26, y, x + 26, y);
  u8g2.drawFrame(x - 20, y, 40, 24);
  u8g2.drawBox(x - 7, y + 9, 14, 15);
}

void drawModulesIcon(int x, int y)
{
  u8g2.drawRFrame(x - 28, y - 22, 56, 44, 4);
  u8g2.drawLine(x - 14, y - 15, x - 14, y + 15);
  u8g2.drawBox(x - 19, y - 4, 10, 8);
  u8g2.drawBox(x + 6, y - 10, 6, 6);
  u8g2.drawBox(x + 18, y - 10, 6, 6);
  u8g2.drawBox(x + 6, y + 4, 6, 6);
  u8g2.drawBox(x + 18, y + 4, 6, 6);
}

void drawBacklightIcon(int x, int y)
{
  u8g2.drawCircle(x, y, 16);
  u8g2.drawBox(x - 9, y + 14, 18, 5);
  u8g2.drawHLine(x - 7, y + 20, 14);
  u8g2.drawLine(x, y - 20, x, y - 28);
  u8g2.drawLine(x - 20, y, x - 28, y);
  u8g2.drawLine(x + 20, y, x + 28, y);
  u8g2.drawLine(x - 14, y - 14, x - 20, y - 20);
  u8g2.drawLine(x + 14, y - 14, x + 20, y - 20);
}

// --- HARDWARE ---
void setupExtModule()
{
  pinMode(EXT_A0, INPUT);
  pinMode(EXT_A1, INPUT);
  pinMode(EXT_A2, INPUT);
  pinMode(EXT_A3, INPUT);

  if (currentModule == MOD_NONE)
  {
    moduleConnected = false;
    extAction[0] = '\0';
  }
  else if (currentModule == MOD_SLIDERS || currentModule == MOD_KNOBS)
  {
    pinMode(EXT_A0, INPUT_PULLUP);
    pinMode(EXT_A1, INPUT_PULLUP);
  }
  else if (currentModule == MOD_EXT_BTNS)
  {
    pinMode(EXT_A0, INPUT_PULLUP);
    pinMode(EXT_A1, INPUT_PULLUP);
    pinMode(EXT_A2, OUTPUT);
    digitalWrite(EXT_A2, HIGH);
    pinMode(EXT_A3, OUTPUT);
    digitalWrite(EXT_A3, HIGH);
  }
}

int readSmooth(int pin)
{
  long sum = 0;
  for (int i = 0; i < NUM_READINGS; i++)
  {
    sum += analogRead(pin);
    delayMicroseconds(20);
  }
  return sum / NUM_READINGS;
}

int rawToPercent(int raw)
{
  if (raw < 15)
    return 0;
  if (raw > 1015)
    return 100;
  return map(raw, 15, 1015, 0, 100);
}

void handleEncoder()
{
  int clk = digitalRead(PIN_ENC_CLK);
  int dt = digitalRead(PIN_ENC_DT);
  if (clk != lastCLKState)
  {
    if (dt != clk)
      encoderValue++;
    else
      encoderValue--;
    lastCLKState = clk;
  }
}

// --- DISPLAY LOGIC ---
void drawDefaultScreen()
{
  if (currentHomeMode == HOME_BASIC)
  {
    drawSimpleLayout(u8g2, cpuUsage, pcTemp, memFree);
  }
  else if (currentHomeMode == HOME_ADVANCE)
  {
    drawAdvancedLayout(u8g2, cpuUsage, gpuUsage, memFree, totalRAM, pcTemp, tempGPU, netDown, netUp);
  }
  else if (currentHomeMode == HOME_SPLASH)
  {
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(20, 60, "SPLASH");
  }
  else
  { // TESTING
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 10, "DECK [TEST]");

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 24, "Mod: ");
    if (currentModule == MOD_NONE)
      u8g2.drawStr(30, 24, "NONE");
    else if (!moduleConnected)
      u8g2.drawStr(30, 24, "DISCONNECTED");
    else if (currentModule == MOD_SLIDERS)
      u8g2.drawStr(30, 24, "SLIDERS");
    else if (currentModule == MOD_KNOBS)
      u8g2.drawStr(30, 24, "KNOBS");
    else
      u8g2.drawStr(30, 24, "BUTTONS");

    u8g2.drawStr(0, 40, "Enc: ");
    u8g2.setCursor(30, 40);
    u8g2.print(encoderValue);
    u8g2.drawStr(0, 55, "Main:");

    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(0, 70, mainAction);

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 90, "Ext Data:");
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 105, (currentModule != MOD_NONE && moduleConnected) ? extAction : "- - -");
  }
}

void drawMenuScreen()
{
  int centerX = 64;
  int iconY = 50;
  int textY = 110;
  u8g2.setFont(u8g2_font_ncenB10_tr); // Font Medio

  if (menuIndex == 0)
  {
    drawHomeIcon(centerX, iconY);
    u8g2.drawStr((128 - u8g2.getStrWidth("HOME")) / 2, textY, "HOME");
  }
  else if (menuIndex == 1)
  {
    drawModulesIcon(centerX, iconY);
    u8g2.drawStr((128 - u8g2.getStrWidth("MODULES")) / 2, textY, "MODULES");
  }
  else if (menuIndex == 2)
  {
    drawBacklightIcon(centerX, iconY);
    u8g2.drawStr((128 - u8g2.getStrWidth("BACKLIGHT")) / 2, textY, "BACKLIGHT");
  }

  u8g2.setFont(u8g2_font_open_iconic_arrow_2x_t);
  if (menuIndex > 0)
    u8g2.drawGlyph(6, iconY + 5, 60);
  if (menuIndex < MENU_ITEMS - 1)
    u8g2.drawGlyph(112, iconY + 5, 61);
}

void drawSubmenuScreen()
{
  u8g2.setFont(u8g2_font_ncenB10_tr); // Font Medio
  int startY = 20;
  int lineHeight = 16;
  const char **items;
  if (menuIndex == 0)
    items = subItemsHome;
  else if (menuIndex == 1)
    items = subItemsModules;
  else
    items = subItemsBacklight;

  for (int i = 0; i < submenuMaxItems; i++)
  {
    int y = startY + (i * lineHeight);
    if (i == submenuIndex)
      u8g2.drawStr(5, y, ">");
    u8g2.drawStr(20, y, items[i]);
  }
}

void updateDisplay()
{
  u8g2.firstPage();
  do
  {
    if (currentState == STATE_DEFAULT)
      drawDefaultScreen();
    else if (currentState == STATE_MENU)
      drawMenuScreen();
    else if (currentState == STATE_SUBMENU)
      drawSubmenuScreen();
  } while (u8g2.nextPage());
}

void readExtModule()
{
  if (currentModule == MOD_NONE)
    return;

  if (currentModule == MOD_SLIDERS || currentModule == MOD_KNOBS)
  {
    int raw1 = readSmooth(EXT_A0);
    int raw2 = readSmooth(EXT_A1);

    if (raw1 > 1018 && raw2 > 1018)
    {
      if (disconnectTimer == 0)
        disconnectTimer = millis();
      else if (millis() - disconnectTimer > 500)
      {
        moduleConnected = false;
        extAction[0] = '\0';
      }
      return;
    }
    else
    {
      disconnectTimer = 0;
      moduleConnected = true;
    }

    if (!moduleConnected)
      return;
    int p1 = rawToPercent(raw1);
    int p2 = rawToPercent(raw2);
    static int lp1 = -1, lp2 = -1;
    if (p1 != lp1 || p2 != lp2 || extAction[0] == '\0')
    {
      // Uso sprintf leggero o costruzione manuale
      sprintf(extAction, "S1:%d%% S2:%d%%", p1, p2);
      lp1 = p1;
      lp2 = p2;
    }
  }
  else if (currentModule == MOD_EXT_BTNS)
  {
    moduleConnected = true;
    extAction[0] = '\0';
    digitalWrite(EXT_A2, LOW);
    if (!digitalRead(EXT_A0))
      strcpy(extAction, "Ext Btn 1");
    if (!digitalRead(EXT_A1))
      strcpy(extAction, "Ext Btn 2");
    digitalWrite(EXT_A2, HIGH);
    digitalWrite(EXT_A3, LOW);
    if (!digitalRead(EXT_A0))
      strcpy(extAction, "Ext Btn 3");
    if (!digitalRead(EXT_A1))
      strcpy(extAction, "Ext Btn 4");
    digitalWrite(EXT_A3, HIGH);
  }
}

// --- MAIN LOOP ---
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
  setupExtModule();
  lastCLKState = digitalRead(PIN_ENC_CLK);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_CLK), handleEncoder, CHANGE);
  updateDisplay();
}

void loop()
{
  bool update = false;
  unsigned long now = millis();

  // BUTTON
  if (digitalRead(PIN_ENC_SW) == LOW)
  {
    if (!btnPressed)
    {
      btnPressed = true;
      btnPressStartTime = now;
      longPressTriggered = false;
    }
    else if (!longPressTriggered && (now - btnPressStartTime > LONG_PRESS_MS))
    {
      longPressTriggered = true;
      currentState = (currentState == STATE_DEFAULT) ? STATE_MENU : STATE_DEFAULT;
      menuIndex = 0;
      update = true;
    }
  }
  else
  {
    if (btnPressed)
    {
      if (!longPressTriggered)
      {
        if (currentState == STATE_DEFAULT)
        {
          strcpy(mainAction, "Enc Click");
          lastActionTime = now;
          update = true;
        }
        else if (currentState == STATE_MENU)
        {
          currentState = STATE_SUBMENU;
          submenuIndex = 0;
          if (menuIndex == 0)
            submenuMaxItems = 4;
          else if (menuIndex == 1)
            submenuMaxItems = 4;
          else
            submenuMaxItems = 2;
          update = true;
        }
        else if (currentState == STATE_SUBMENU)
        {
          if (menuIndex == 0)
            currentHomeMode = (HomeMode)submenuIndex;
          else if (menuIndex == 1)
          {
            currentModule = (ModuleType)submenuIndex;
            setupExtModule();
          }
          else if (menuIndex == 2)
          {
            backlightEnabled = (submenuIndex == 0);
            u8g2.setPowerSave(!backlightEnabled);
          }
          currentState = STATE_MENU;
          update = true;
        }
      }
      btnPressed = false;
    }
  }

  // ENCODER
  long diff = encoderValue - lastEncoderValue;
  if (diff != 0)
  {
    if (currentState == STATE_DEFAULT)
    {
      strcpy(mainAction, (diff > 0) ? "Enc Right" : "Enc Left");
      lastActionTime = now;
    }
    else if (currentState == STATE_MENU)
    {
      if (diff > 0)
      {
        menuIndex++;
        if (menuIndex >= MENU_ITEMS)
          menuIndex = 0;
      }
      else
      {
        menuIndex--;
        if (menuIndex < 0)
          menuIndex = MENU_ITEMS - 1;
      }
    }
    else
    {
      if (diff > 0)
      {
        submenuIndex++;
        if (submenuIndex >= submenuMaxItems)
          submenuIndex = 0;
      }
      else
      {
        submenuIndex--;
        if (submenuIndex < 0)
          submenuIndex = submenuMaxItems - 1;
      }
    }
    lastEncoderValue = encoderValue;
    update = true;
  }

  // DEFAULT MODE LOGIC
  if (currentState == STATE_DEFAULT)
  {
    char prevExt[24];
    strcpy(prevExt, extAction);
    bool prevConn = moduleConnected;
    readExtModule();
    if (strcmp(prevExt, extAction) != 0 || prevConn != moduleConnected)
      update = true;

    bool active = false;
    for (int r = 0; r < 3; r++)
    {
      digitalWrite(rows[r], LOW);
      for (int c = 0; c < 3; c++)
      {
        if (digitalRead(cols[c]) == LOW)
        {
          sprintf(mainAction, "BTN %d", (c * 3) + r + 1);
          lastActionTime = now;
          active = true;
          digitalWrite(PIN_LED, HIGH);
          update = true;
        }
      }
      digitalWrite(rows[r], HIGH);
    }

    if (!active && strcmp(mainAction, "Ready") != 0)
    {
      if (now - lastActionTime > 500)
      {
        strcpy(mainAction, "Ready");
        update = true;
      }
      if (digitalRead(PIN_LED))
        digitalWrite(PIN_LED, LOW);
    }
  }

  if (update)
    updateDisplay();
}
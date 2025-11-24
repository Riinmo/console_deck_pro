#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// --- ENUMS & CONFIGURAZIONE ---
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

// Opzioni HOME
enum HomeMode
{
  HOME_BASIC,
  HOME_ADVANCE,
  HOME_SPLASH,
  HOME_TESTING
};
HomeMode currentHomeMode = HOME_BASIC;

// Opzioni BACKLIGHT
bool backlightEnabled = true;

const unsigned long LONG_PRESS_MS = 800;

U8G2_SH1107_SEEED_128X128_1_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);

// --- PIN ---
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

// --- VARIABILI GLOBALI ---
volatile long encoderValue = 0;
long lastEncoderValue = 0;
volatile int lastCLKState;
unsigned long lastActionTime = 0;

unsigned long btnPressStartTime = 0;
bool btnPressed = false;
bool longPressTriggered = false;

// MENU & SUBMENU NAVIGATION
int menuIndex = 0;
const int MENU_ITEMS = 3;

int submenuIndex = 0;
int submenuMaxItems = 0;

// DATI SUBMENU
const char *subItemsHome[] = {"PC INFO BASIC", "PC INFO ADVANCE", "SPLASH IMAGE", "TESTING"};
const char *subItemsModules[] = {"NONE", "BUTTONS", "SLIDERS", "KNOBS"};
const char *subItemsBacklight[] = {"YES", "NO"};

// DATI SYSTEM
String mainAction = "Ready";
String extAction = "";
String lastDrawnAction = "";
const int rows[] = {PIN_ROW_1, PIN_ROW_2, PIN_ROW_3};
const int cols[] = {PIN_COL_1, PIN_COL_2, PIN_COL_3};
const int NUM_READINGS = 16;
bool moduleConnected = false;
unsigned long disconnectTimer = 0;

// --- FUNZIONI GRAFICHE ICONE ---

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
  int btnSize = 6;
  int gap = 12;
  int startX = x + 6;
  int startY = y - 10;
  u8g2.drawBox(startX, startY, btnSize, btnSize);
  u8g2.drawBox(startX + gap, startY, btnSize, btnSize);
  u8g2.drawBox(startX, startY + gap + 4, btnSize, btnSize);
  u8g2.drawBox(startX + gap, startY + gap + 4, btnSize, btnSize);
}

void drawBacklightIcon(int x, int y)
{
  int radius = 16;
  u8g2.drawCircle(x, y, radius);
  u8g2.drawBox(x - 9, y + 14, 18, 5);
  u8g2.drawHLine(x - 7, y + 20, 14);
  u8g2.drawHLine(x - 5, y + 22, 10);
  int rStart = radius + 4;
  int rLen = 8;
  u8g2.drawLine(x, y - rStart, x, y - rStart - rLen);
  u8g2.drawLine(x - rStart, y, x - rStart - rLen, y);
  u8g2.drawLine(x + rStart, y, x + rStart + rLen, y);
  int dOff = 14;
  int dLen = 6;
  u8g2.drawLine(x - dOff, y - dOff, x - dOff - dLen, y - dOff - dLen);
  u8g2.drawLine(x + dOff, y - dOff, x + dOff + dLen, y - dOff - dLen);
  u8g2.drawLine(x - dOff, y + dOff, x - dOff - dLen, y + dOff + dLen);
  u8g2.drawLine(x + dOff, y + dOff, x + dOff + dLen, y + dOff + dLen);
}

// --- LOGICA SYSTEM ---

void setupExtModule()
{
  // Reset tutti i pin come INPUT (disconnessi/safe)
  pinMode(EXT_A0, INPUT);
  pinMode(EXT_A1, INPUT);
  pinMode(EXT_A2, INPUT);
  pinMode(EXT_A3, INPUT);

  if (currentModule == MOD_NONE)
  {
    // Non fare nulla, lascia input flottanti o input semplici
    moduleConnected = false;
    extAction = "";
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
    delayMicroseconds(50);
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
  int clkState = digitalRead(PIN_ENC_CLK);
  int dtState = digitalRead(PIN_ENC_DT);

  if (clkState != lastCLKState)
  {
    if (dtState != clkState)
      encoderValue++;
    else
      encoderValue--;
    lastCLKState = clkState;
  }
}

// --- DRAWING SCREENS ---

void drawDefaultScreen()
{
  u8g2.setFont(u8g2_font_ncenB08_tr);

  if (currentHomeMode == HOME_BASIC)
    u8g2.drawStr(0, 10, "DECK [BASIC]");
  else if (currentHomeMode == HOME_ADVANCE)
    u8g2.drawStr(0, 10, "DECK [ADV]");
  else if (currentHomeMode == HOME_SPLASH)
    u8g2.drawStr(0, 10, "DECK [SPLASH]");
  else if (currentHomeMode == HOME_TESTING)
    u8g2.drawStr(0, 10, "DECK [TEST]");
  else
    u8g2.drawStr(0, 10, "CONSOLE DECK");

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 24, "Mod: ");
  if (currentModule == MOD_NONE)
  {
    u8g2.drawStr(30, 24, "NONE");
  }
  else if (!moduleConnected)
  {
    u8g2.drawStr(30, 24, "DISCONNECTED");
  }
  else
  {
    if (currentModule == MOD_SLIDERS)
      u8g2.drawStr(30, 24, "SLIDERS");
    else if (currentModule == MOD_KNOBS)
      u8g2.drawStr(30, 24, "KNOBS");
    else if (currentModule == MOD_EXT_BTNS)
      u8g2.drawStr(30, 24, "BUTTONS");
  }

  u8g2.drawStr(0, 40, "Enc: ");
  u8g2.setCursor(30, 40);
  u8g2.print(encoderValue);

  u8g2.drawStr(0, 55, "Main:");
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.setCursor(0, 70);
  u8g2.print(mainAction);

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 90, "Ext Data:");
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.setCursor(0, 105);

  if (currentModule != MOD_NONE && moduleConnected)
  {
    u8g2.print(extAction);
  }
  else
  {
    u8g2.print("- - -");
  }
}

void drawMenuScreen()
{
  int centerX = 64;
  int iconY = 50;
  int textY = 110;

  u8g2.setFont(u8g2_font_ncenB08_tr);

  if (menuIndex == 0)
  {
    drawHomeIcon(centerX, iconY);
    int w = u8g2.getStrWidth("HOME");
    u8g2.drawStr((128 - w) / 2, textY, "HOME");
  }
  else if (menuIndex == 1)
  {
    drawModulesIcon(centerX, iconY);
    int w = u8g2.getStrWidth("MODULES");
    u8g2.drawStr((128 - w) / 2, textY, "MODULES");
  }
  else if (menuIndex == 2)
  {
    drawBacklightIcon(centerX, iconY);
    int w = u8g2.getStrWidth("BACKLIGHT");
    u8g2.drawStr((128 - w) / 2, textY, "BACKLIGHT");
  }

  u8g2.setFont(u8g2_font_open_iconic_arrow_2x_t);
  if (menuIndex > 0)
    u8g2.drawGlyph(6, iconY + 5, 60);
  if (menuIndex < MENU_ITEMS - 1)
    u8g2.drawGlyph(112, iconY + 5, 61);
}

void drawSubmenuScreen()
{
  u8g2.setFont(u8g2_font_ncenB08_tr);

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

    // Cursore
    if (i == submenuIndex)
    {
      u8g2.drawStr(5, y, ">");
    }

    // Testo
    u8g2.drawStr(20, y, items[i]);
  }
}

void updateDisplay()
{
  u8g2.firstPage();
  do
  {
    if (currentState == STATE_DEFAULT)
    {
      drawDefaultScreen();
    }
    else if (currentState == STATE_MENU)
    {
      drawMenuScreen();
    }
    else if (currentState == STATE_SUBMENU)
    {
      drawSubmenuScreen();
    }
  } while (u8g2.nextPage());
}

void readExtModule()
{
  if (currentModule == MOD_NONE)
  {
    moduleConnected = false;
    extAction = "";
    return;
  }

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
        extAction = "";
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
    int pct1 = rawToPercent(raw1);
    int pct2 = rawToPercent(raw2);
    static int lastPct1 = -1;
    static int lastPct2 = -1;
    if (pct1 != lastPct1 || pct2 != lastPct2 || extAction == "")
    {
      extAction = "S1:" + String(pct1) + "% S2:" + String(pct2) + "%";
      lastPct1 = pct1;
      lastPct2 = pct2;
    }
  }
  else if (currentModule == MOD_EXT_BTNS)
  {
    moduleConnected = true;
    String tempAction = "";
    digitalWrite(EXT_A2, LOW);
    if (digitalRead(EXT_A0) == LOW)
      tempAction = "Ext Btn 1";
    if (digitalRead(EXT_A1) == LOW)
      tempAction = "Ext Btn 2";
    digitalWrite(EXT_A2, HIGH);
    digitalWrite(EXT_A3, LOW);
    if (digitalRead(EXT_A0) == LOW)
      tempAction = "Ext Btn 3";
    if (digitalRead(EXT_A1) == LOW)
      tempAction = "Ext Btn 4";
    digitalWrite(EXT_A3, HIGH);
    extAction = tempAction;
  }
}

// --- SETUP & LOOP ---

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
  bool needsUpdate = false;
  unsigned long now = millis();

  int btnState = digitalRead(PIN_ENC_SW);

  if (btnState == LOW)
  {
    if (!btnPressed)
    {
      btnPressed = true;
      btnPressStartTime = now;
      longPressTriggered = false;
    }
    else
    {
      if (!longPressTriggered && (now - btnPressStartTime > LONG_PRESS_MS))
      {
        longPressTriggered = true;
        if (currentState == STATE_DEFAULT)
        {
          currentState = STATE_MENU;
          menuIndex = 0;
        }
        else
        {
          currentState = STATE_DEFAULT;
        }
        needsUpdate = true;
      }
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
          mainAction = "Enc Click";
          lastActionTime = now;
          needsUpdate = true;
        }
        else if (currentState == STATE_MENU)
        {
          currentState = STATE_SUBMENU;
          submenuIndex = 0;
          if (menuIndex == 0)
            submenuMaxItems = 4;
          else if (menuIndex == 1)
            submenuMaxItems = 4; // Ora sono 4 (None, Btn, Sld, Knob)
          else
            submenuMaxItems = 2;
          needsUpdate = true;
        }
        else if (currentState == STATE_SUBMENU)
        {

          if (menuIndex == 0)
          {
            if (submenuIndex == 0)
              currentHomeMode = HOME_BASIC;
            if (submenuIndex == 1)
              currentHomeMode = HOME_ADVANCE;
            if (submenuIndex == 2)
              currentHomeMode = HOME_SPLASH;
            if (submenuIndex == 3)
              currentHomeMode = HOME_TESTING;
          }
          else if (menuIndex == 1)
          { // Modules
            if (submenuIndex == 0)
              currentModule = MOD_NONE;
            if (submenuIndex == 1)
              currentModule = MOD_EXT_BTNS;
            if (submenuIndex == 2)
              currentModule = MOD_SLIDERS;
            if (submenuIndex == 3)
              currentModule = MOD_KNOBS;
            setupExtModule();
          }
          else if (menuIndex == 2)
          {
            if (submenuIndex == 0)
            {
              backlightEnabled = true;
              u8g2.setPowerSave(0);
            }
            if (submenuIndex == 1)
            {
              backlightEnabled = false;
              u8g2.setPowerSave(1);
            }
          }

          currentState = STATE_MENU;
          needsUpdate = true;
        }
      }
      btnPressed = false;
    }
  }

  long deltaEncoder = encoderValue - lastEncoderValue;
  if (deltaEncoder != 0)
  {
    if (currentState == STATE_DEFAULT)
    {
      mainAction = (deltaEncoder > 0) ? "Enc Right" : "Enc Left";
      lastActionTime = now;
    }
    else if (currentState == STATE_MENU)
    {
      if (deltaEncoder > 0)
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
    else if (currentState == STATE_SUBMENU)
    {
      if (deltaEncoder > 0)
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
    needsUpdate = true;
  }

  if (currentState == STATE_DEFAULT)
  {
    String prevExt = extAction;
    bool prevConn = moduleConnected;
    readExtModule();
    if (extAction != prevExt || moduleConnected != prevConn)
      needsUpdate = true;

    bool matrixActive = false;
    for (int r = 0; r < 3; r++)
    {
      digitalWrite(rows[r], LOW);
      for (int c = 0; c < 3; c++)
      {
        if (digitalRead(cols[c]) == LOW)
        {
          int btnNum = (c * 3) + r + 1;
          mainAction = "BTN " + String(btnNum);
          lastActionTime = now;
          matrixActive = true;
          digitalWrite(PIN_LED, HIGH);
          needsUpdate = true;
        }
      }
      digitalWrite(rows[r], HIGH);
    }

    if (!matrixActive && mainAction != "Ready")
    {
      if (now - lastActionTime > 500)
      {
        mainAction = "Ready";
        needsUpdate = true;
      }
      if (digitalRead(PIN_LED) == HIGH && btnState == HIGH)
      {
        digitalWrite(PIN_LED, LOW);
      }
    }
  }

  if (needsUpdate)
  {
    updateDisplay();
  }
}
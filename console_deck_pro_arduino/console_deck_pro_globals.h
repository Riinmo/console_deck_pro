#ifndef CONSOLE_DECK_PRO_GLOBALS_H
#define CONSOLE_DECK_PRO_GLOBALS_H

#include <Arduino.h>
#include <U8g2lib.h>

// --- Firmware ---
const char FIRMWARE_VERSION[] = "v1.2.0";

// --- Enums ---
enum ModuleType
{
  MOD_NONE,
  MOD_EXT_BTNS,
  MOD_SLIDERS,
  MOD_KNOBS
};

enum ScreenState
{
  STATE_DEFAULT,
  STATE_MENU,
  STATE_SUBMENU,
  STATE_CALIBRATION
};

enum HomeMode
{
  HOME_PC_STATS,
  HOME_SPLASH,
  HOME_TESTING
};

// --- Timing / UI ---
const unsigned long LONG_PRESS_MS = 800;
const int MENU_ENC_SENSITIVITY = 2;
const unsigned long DISPLAY_UPDATE_INTERVAL_MS = 33;

// --- EEPROM ---
const int EEPROM_ADDR_MAGIC = 0;
const int EEPROM_ADDR_HOME = 1;
const int EEPROM_ADDR_MOD = 2;
const int EEPROM_ADDR_LIGHT = 3;
const int EEPROM_ADDR_SCREENSAVER = 4;
const int EEPROM_ADDR_MAP = 10;
const byte MAGIC_NUMBER = 0x42;

// Screen saver: 0=10s, 1=30s, 2=1min, 3=5min, 4=10min, 5=never
const unsigned long SCREEN_SAVER_DELAY_MS[] = { 10000, 30000, 60000, 300000, 600000, 0 };

// --- Pins ---
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
const uint8_t EXT_A6 = A6;
const uint8_t EXT_A7 = A7;

const int rows[] = {PIN_ROW_1, PIN_ROW_2, PIN_ROW_3};
const int cols[] = {PIN_COL_1, PIN_COL_2, PIN_COL_3};

// --- Menu ---
const int MENU_ITEMS = 6;

// --- Display: defined in .ino (constructor) ---
extern U8G2_SH1107_SEEED_128X128_1_HW_I2C u8g2;
const bool ENABLE_DISPLAY = true;  // set to false to fully disable OLED rendering for latency tests
const bool DISPLAY_PERFORMANCE_MODE = true;       // reduce OLED refresh while user is actively interacting
const unsigned long DISPLAY_ACTIVE_INTERVAL_MS = 220; // max refresh during input bursts
const unsigned long DISPLAY_IDLE_INTERVAL_MS = 50;    // normal refresh when idle
const unsigned long DISPLAY_MENU_INTERVAL_MS = 20;    // faster refresh for menu navigation
const unsigned long DISPLAY_INPUT_BURST_WINDOW_MS = 300;
const bool DISPLAY_STRICT_INPUT_PRIORITY = true;       // if true, skip OLED rendering completely during input bursts
const unsigned long DISPLAY_HARD_BLOCK_WINDOW_MS = 450;
const unsigned long MODULE_READ_INTERVAL_MS = 12;

// --- State ---
ModuleType currentModule = MOD_SLIDERS;
ScreenState currentState = STATE_DEFAULT;
HomeMode currentHomeMode = HOME_PC_STATS;
bool backlightEnabled = true;

// --- Calibration ---
uint8_t btnMap[9] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
int calibStep = 0;
bool calibWaitingRelease = false;

// --- Serial buffer ---
char serialBuf[48];
int serialBufIdx = 0;

// --- Stats (home) ---
char timeStr[6] = "--:--";
char dateStr[6] = "--/--";
int cpuUsage = 45;
int gpuUsage = 32;
int ramUsage = 50;
int cpuFreqMHz = 3600;
int gpuTempC = 58;
unsigned long lastStatsTime = 0;
bool pcConnected = false;

// --- Encoder / buttons ---
volatile long encoderValue = 0;
long lastEncoderValue = 0;
volatile int lastCLKState;
unsigned long lastActionTime = 0;
unsigned long btnPressStartTime = 0;
bool btnPressed = false;
bool longPressTriggered = false;
int menuIndex = 1;
int submenuIndex = 0;
int submenuMaxItems = 0;

// --- Menu strings (PROGMEM) ---
static const char str_pc[] PROGMEM = "PC STATS";
static const char str_splash[] PROGMEM = "SPLASH IMAGE";
static const char str_test[] PROGMEM = "TESTING";
static const char* const subItemsHome[] PROGMEM = {str_pc, str_splash, str_test};

static const char str_none[] PROGMEM = "NONE";
static const char str_btns[] PROGMEM = "BUTTONS";
static const char str_sliders[] PROGMEM = "SLIDERS";
static const char str_knobs[] PROGMEM = "KNOBS";
static const char* const subItemsModules[] PROGMEM = {str_none, str_btns, str_sliders, str_knobs};

static const char str_yes[] PROGMEM = "YES";
static const char str_no[] PROGMEM = "NO";
static const char* const subItemsBacklight[] PROGMEM = {str_yes, str_no};

static const char str_10s[] PROGMEM = "10 SEC";
static const char str_30[] PROGMEM = "30 SEC";
static const char str_1m[] PROGMEM = "1 MIN";
static const char str_5m[] PROGMEM = "5 MIN";
static const char str_10m[] PROGMEM = "10 MIN";
static const char str_never[] PROGMEM = "NEVER";
static const char* const subItemsScreenSaver[] PROGMEM = {str_10s, str_30, str_1m, str_5m, str_10m, str_never};

// --- Screen saver ---
int screenSaverDelayIndex = 5;  // default: never
unsigned long lastActivityTime = 0;
bool screenSaverActive = false;

// --- Actions / module ---
char mainAction[12] = "Ready";
char extAction[20] = "";
bool moduleConnected = false;
unsigned long disconnectTimer = 0;
bool matrixActive = false;

// --- Serial data ---
int mainBtnStates[9] = {0};
int extBtnStates[6] = {0};
int serVal1 = 0;
int serVal2 = 0;

#endif

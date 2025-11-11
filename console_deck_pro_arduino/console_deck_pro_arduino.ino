/*
 * Console Deck - v1.0.0
 * OLED I2C + Encoder HW-040 + LED D7
 * Arduino Nano: SDA=A4, SCL=A5
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Encoder HW-040 ---
// Pin aggiornati alla tua configurazione
const uint8_t PIN_SW = 2;  // Pulsante
const uint8_t PIN_DT = 3;  // Canale B
const uint8_t PIN_CLK = 4; // Canale A (interrupt)

// --- LED ---
const uint8_t LED_PIN = 7;
bool ledOn = false;

volatile long encoderValue = 0;
volatile int lastCLKState;
unsigned long lastBtnTime = 0;
const unsigned long debounceMs = 200;

// Interrupt Encoder
void handleEncoder()
{
  int clkState = digitalRead(PIN_CLK);
  int dtState = digitalRead(PIN_DT);

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

void drawHeader()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(F("Console Deck"));

  display.setTextSize(1);
  display.setCursor(0, 22);
  display.println(F("v1.0.0"));

  display.display();
}

void drawStatus()
{
  display.fillRect(0, 34, SCREEN_WIDTH, 30, SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(0, 36);
  display.print(F("Encoder: "));
  display.println(encoderValue);

  display.setCursor(0, 48);
  display.print(F("LED D7: "));
  display.println(ledOn ? F("ON") : F("OFF"));

  display.display();
}

void setup()
{
  // LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Encoder
  pinMode(PIN_CLK, INPUT);
  pinMode(PIN_DT, INPUT);
  pinMode(PIN_SW, INPUT_PULLUP);
  lastCLKState = digitalRead(PIN_CLK);
  attachInterrupt(digitalPinToInterrupt(PIN_CLK), handleEncoder, CHANGE);

  // OLED + I2C settings
  Wire.begin();
  Wire.setClock(100000); // clock stabile

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
  {
    while (true)
    {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(250);
    }
  }

  display.clearDisplay();
  display.invertDisplay(false);
  display.setRotation(0);

  drawHeader();
  drawStatus();
}

void loop()
{
  // Pulsante encoder → toggle LED
  if (digitalRead(PIN_SW) == LOW)
  {
    unsigned long now = millis();
    if (now - lastBtnTime > debounceMs)
    {
      ledOn = !ledOn;
      digitalWrite(LED_PIN, ledOn ? HIGH : LOW);
      drawStatus();
      lastBtnTime = now;
    }
  }

  // Se l’encoder cambia → aggiorna display
  static long lastShown = 0;
  if (encoderValue != lastShown)
  {
    drawStatus();
    lastShown = encoderValue;
  }
}

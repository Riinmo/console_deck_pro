/*
 * Flexible Test Script for Arduino Nano
 * Display: 1.5" 128x128 (SH1107 - "SEEED" Model)
 * Toggles between animation and a static frame.
 */

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// Correct constructor for this hardware (fixes X-axis offset)
U8G2_SH1107_SEEED_128X128_1_HW_I2C u8g2(U8G2_R2, /* reset=*/U8X8_PIN_NONE);

// --- MAIN TOGGLE SWITCH ---
// Set to 'true' for the bouncing square animation
// Set to 'false' for the static border frame
bool showAnimation = true;

// Static text variables
const char *text1 = "Arduino Nano";
const char *text2 = "Display OLED";
const char *text3 = "Test Switch";

// Animation variables
int square_x = 0;
int square_size = 10;
int square_speed = 2;
int square_direction = 1;

void setup(void)
{
  u8g2.begin();
}

void loop(void)
{
  u8g2.firstPage();
  do
  {

    // --- Draw Static Text (Always visible) ---
    int x_pos;
    int larghezza;

    u8g2.setFont(u8g2_font_ncenB10_tr);
    larghezza = u8g2.getStrWidth(text1);
    x_pos = (u8g2.getDisplayWidth() - larghezza) / 2;
    u8g2.drawStr(x_pos, 25, text1);

    u8g2.setFont(u8g2_font_ncenB14_tr);
    larghezza = u8g2.getStrWidth(text2);
    x_pos = (u8g2.getDisplayWidth() - larghezza) / 2;
    u8g2.drawStr(x_pos, 60, text2);

    u8g2.setFont(u8g2_font_7x13_tr);
    larghezza = u8g2.getStrWidth(text3);
    x_pos = (u8g2.getDisplayWidth() - larghezza) / 2;
    u8g2.drawStr(x_pos, 95, text3);

    // --- Conditional Logic: Draw Animation OR Frame ---
    if (showAnimation)
    {
      // Draw the moving square
      u8g2.drawBox(square_x, 110, square_size, square_size);
    }
    else
    {
      // Draw the static frame
      u8g2.drawFrame(0, 0, 128, 128);
    }

  } while (u8g2.nextPage());

  // --- Conditional Logic: Update Animation State ---

  // Only update the square's position if animation is active
  if (showAnimation)
  {
    // Move the square
    square_x += square_speed * square_direction;

    // Check for boundaries (screen edges)
    if (square_x + square_size >= u8g2.getDisplayWidth())
    {
      square_direction = -1; // Move left
    }
    if (square_x <= 0)
    {
      square_direction = 1; // Move right
    }
  }

  // delay(10); // Uncomment to slow down the animation if needed
}
/*
 * Tutti i metodi relativi alla grafica dello schermo (secondo file .ino)
 * Icone, layout PC Stats, schermate menu/default/calibrazione, updateDisplay.
 */
#include <avr/pgmspace.h>

// --- Helper icons for drawStatsLayout (from dashboard_graphics) ---
static void drawSmallCpuIcon(U8G2 &u8g2, int x, int y)
{
    u8g2.drawFrame(x, y, 10, 10);
    u8g2.drawBox(x + 2, y + 2, 6, 6);
}

static void drawSmallGpuIcon(U8G2 &u8g2, int x, int y)
{
    u8g2.drawFrame(x, y, 10, 8);
    u8g2.drawFrame(x + 6, y + 8, 4, 2);
}

static void drawSmallRamIcon(U8G2 &u8g2, int x, int y)
{
    u8g2.drawFrame(x, y, 10, 6);
    // RAM "teeth" (pins)
    u8g2.drawVLine(x + 1, y + 6, 2);
    u8g2.drawVLine(x + 3, y + 6, 2);
    u8g2.drawVLine(x + 5, y + 6, 2);
    u8g2.drawVLine(x + 7, y + 6, 2);
}

static void drawSmallTempIcon(U8G2 &u8g2, int x, int y)
{
    u8g2.drawFrame(x + 3, y, 3, 7);
    u8g2.drawDisc(x + 4, y + 8, 2);
}

static void drawSmallFreqIcon(U8G2 &u8g2, int x, int y)
{
    // Tiny waveform icon for CPU frequency
    u8g2.drawFrame(x, y + 1, 10, 8);
    u8g2.drawLine(x + 1, y + 7, x + 3, y + 5);
    u8g2.drawLine(x + 3, y + 5, x + 5, y + 8);
    u8g2.drawLine(x + 5, y + 8, x + 7, y + 3);
    u8g2.drawLine(x + 7, y + 3, x + 9, y + 6);
}

static void drawProgressBar(U8G2 &u8g2, int x, int y, int width, int height, int percentage)
{
    int filled = (percentage * width) / 100;
    u8g2.drawFrame(x, y, width, height);
    u8g2.drawBox(x, y, filled, height);
}

void drawStatsLayout(U8G2 &u8g2, int cpu, int gpu, int ram, int cpuFreqMHz, int gpuTempC)
{
    u8g2.setFont(u8g2_font_6x10_tr);

    const int H = 26;
    int y = 28;  // shifted down for better vertical centering
    int x_bar = 42;
    int x_val = 98;

    drawSmallCpuIcon(u8g2, 3, y - 9);
    u8g2.drawStr(16, y, "CPU");
    drawProgressBar(u8g2, x_bar, y - 8, 48, 8, cpu);
    u8g2.setCursor(x_val, y);
    u8g2.print(cpu);
    u8g2.print("%");

    y += H;
    drawSmallGpuIcon(u8g2, 3, y - 9);
    u8g2.drawStr(16, y, "GPU");
    if (gpu >= 0) {
        drawProgressBar(u8g2, x_bar, y - 8, 48, 8, gpu);
        u8g2.setCursor(x_val, y);
        u8g2.print(gpu);
        u8g2.print("%");
    } else {
        u8g2.drawStr(x_bar, y, "N/A");
    }

    y += H;
    drawSmallRamIcon(u8g2, 3, y - 9);
    u8g2.drawStr(16, y, "RAM");
    drawProgressBar(u8g2, x_bar, y - 8, 48, 8, ram);
    u8g2.setCursor(x_val, y);
    u8g2.print(ram);
    u8g2.print("%");

    y += H;
    drawSmallFreqIcon(u8g2, 3, y - 9);
    u8g2.setCursor(16, y);
    if (cpuFreqMHz > 0) {
        int mhz = cpuFreqMHz; // Python sends CPU frequency in MHz
        u8g2.print("F:");
        u8g2.print(mhz / 1000);
        u8g2.print(".");
        u8g2.print((mhz % 1000) / 100);
        u8g2.print("G");
    } else {
        u8g2.print("F:");
        u8g2.print("N/A");
    }
    drawSmallTempIcon(u8g2, 58, y - 9);
    u8g2.setCursor(70, y);
    u8g2.print("GPU:");
    if (gpuTempC >= 0) {
        u8g2.print(gpuTempC);
        u8g2.print("C");
    } else {
        u8g2.print("N/A");
    }
}

// --- Menu icons ---
void drawBackIcon(int x, int y)
{
  u8g2.drawBox(x - 14, y - 16, 14, 32);
  u8g2.drawFrame(x + 2, y - 16, 14, 32);
  u8g2.drawLine(x + 5, y, x + 10, y);
  u8g2.drawTriangle(x + 12, y, x + 9, y - 3, x + 9, y + 3);
}

void drawHomeIcon(int x, int y)
{
  // Roof: symmetric triangle (45° slopes) to avoid pixel stepping
  u8g2.drawTriangle(x, y - 24, x - 24, y, x + 24, y);
  u8g2.drawBox(x - 18, y, 36, 20);
  u8g2.setColorIndex(0);
  u8g2.drawBox(x - 6, y + 8, 12, 12);
  u8g2.setColorIndex(1);
}

void drawModulesIcon(int x, int y)
{
  u8g2.drawRFrame(x - 28, y - 22, 56, 44, 4);
  u8g2.drawLine(x - 14, y - 15, x - 14, y + 15);
  u8g2.drawBox(x - 19, y - 4, 10, 8);
  int btnSize = 6;
  int gap = 12;
  int startX = x + 4;
  int startY = y - 10;
  u8g2.drawBox(startX, startY, btnSize, btnSize);
  u8g2.drawBox(startX + gap, startY, btnSize, btnSize);
  u8g2.drawBox(startX, startY + gap + 4, btnSize, btnSize);
  u8g2.drawBox(startX + gap, startY + gap + 4, btnSize, btnSize);
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

void drawMonitorIcon(int x, int y)
{
  int y0 = y + 16;  // lowered to align with other menu icons
  int w = 44;
  int h = 28;
  u8g2.drawFrame(x - w/2, y0 - h - 6, w, h + 4);
  u8g2.drawFrame(x - w/2 + 2, y0 - h - 4, w - 4, h - 2);
  u8g2.drawBox(x - 4, y0 - 2, 8, 4);
  u8g2.drawHLine(x - 12, y0 + 6, 24);
  u8g2.drawVLine(x - 12, y0 + 2, 4);
  u8g2.drawVLine(x + 12, y0 + 2, 4);
}

void drawSettingsIcon(int x, int y)
{
  int r = 15;
  u8g2.drawDisc(x, y, r);
  u8g2.drawBox(x - 20, y - 4, 40, 8);
  u8g2.drawBox(x - 4, y - 20, 8, 40);
  u8g2.drawLine(x - 14, y - 14, x + 14, y + 14);
  u8g2.drawLine(x - 15, y - 14, x + 13, y + 14);
  u8g2.drawLine(x - 13, y - 14, x + 15, y + 14);
  u8g2.drawLine(x + 14, y - 14, x - 14, y + 14);
  u8g2.drawLine(x + 15, y - 14, x - 13, y + 14);
  u8g2.drawLine(x + 13, y - 14, x - 15, y + 14);
  u8g2.setColorIndex(0);
  u8g2.drawDisc(x, y, 8);
  u8g2.setColorIndex(1);
}

void drawCenteredStr(int y, const char *str)
{
  int w = u8g2.getStrWidth(str);
  u8g2.drawStr((128 - w) / 2, y, str);
}

// --- Screens ---
void drawDefaultScreen()
{
  if (currentHomeMode == HOME_SPLASH)
  {
      u8g2.setFont(u8g2_font_ncenB10_tr);
      drawCenteredStr(55, "CONSOLE");
      drawCenteredStr(75, "DECK PRO");
      u8g2.setFont(u8g2_font_6x10_tr);
      drawCenteredStr(92, FIRMWARE_VERSION);
      return;
  }

  if (currentHomeMode == HOME_PC_STATS && !pcConnected)
  {
      u8g2.setFont(u8g2_font_ncenB10_tr);
      drawCenteredStr(60, "WAITING FOR");
      drawCenteredStr(80, "PC DATA...");
      u8g2.setFont(u8g2_font_6x10_tr);
      drawCenteredStr(100, "(Run Python App)");
      return;
  }

  if (currentHomeMode == HOME_PC_STATS)
  {
    drawStatsLayout(u8g2, cpuUsage, gpuUsage, ramUsage, cpuFreqMHz, gpuTempC);
  }
  else
  {
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.setCursor(45, 30); u8g2.print("Enc: "); u8g2.print(encoderValue);

    if (currentModule == MOD_NONE)
      drawCenteredStr(65, "Mod: NONE");
    else if (!moduleConnected)
      drawCenteredStr(65, "Mod: DISCONN.");
    else if (currentModule == MOD_SLIDERS)
      drawCenteredStr(65, "Mod: SLIDERS");
    else if (currentModule == MOD_KNOBS)
      drawCenteredStr(65, "Mod: KNOBS");
    else
      drawCenteredStr(65, "Mod: BUTTONS");

    drawCenteredStr(85, mainAction);
    u8g2.drawHLine(44, 95, 40);
    if (currentModule != MOD_NONE && moduleConnected)
      drawCenteredStr(110, extAction);
    else
      drawCenteredStr(110, "- NO DATA -");
  }
}

void drawMenuScreen()
{
  int centerX = 64;
  int iconY = 50;
  int textY = 110;
  u8g2.setFont(u8g2_font_ncenB10_tr);

  if (menuIndex == 0)
  {
    drawBackIcon(centerX, iconY);
    drawCenteredStr(textY, "EXIT MENU");
  }
  else if (menuIndex == 1)
  {
    drawHomeIcon(centerX, iconY);
    drawCenteredStr(textY, "HOME");
  }
  else if (menuIndex == 2)
  {
    drawModulesIcon(centerX, iconY);
    drawCenteredStr(textY, "MODULES");
  }
  else if (menuIndex == 3)
  {
    drawBacklightIcon(centerX, iconY);
    drawCenteredStr(textY, "BACKLIGHT");
  }
  else if (menuIndex == 4)
  {
    drawMonitorIcon(centerX, iconY);
    drawCenteredStr(textY, "SCREEN SAVER");
  }
  else if (menuIndex == 5)
  {
    drawSettingsIcon(centerX, iconY);
    drawCenteredStr(textY, "CALIBRATION");
  }
  u8g2.setFont(u8g2_font_open_iconic_arrow_2x_t);
  if (menuIndex > 0)
    u8g2.drawGlyph(6, iconY + 5, 60);
  if (menuIndex < MENU_ITEMS - 1)
    u8g2.drawGlyph(112, iconY + 5, 61);
}

void drawSubmenuScreen()
{
  u8g2.setFont(u8g2_font_6x10_tr);
  int startY = 20;
  int lineHeight = 16;
  const char* const* items;
  if (menuIndex == 1)
    items = subItemsHome;
  else if (menuIndex == 2)
    items = subItemsModules;
  else if (menuIndex == 3)
    items = subItemsBacklight;
  else
    items = subItemsScreenSaver;
  char buf[14];
  for (int i = 0; i < submenuMaxItems; i++)
  {
    int y = startY + (i * lineHeight);
    if (i == submenuIndex)
      u8g2.drawStr(5, y, ">");
    strcpy_P(buf, (const char*)pgm_read_word(&items[i]));
    u8g2.drawStr(20, y, buf);
  }
}

void drawCalibrationScreen()
{
    u8g2.setFont(u8g2_font_ncenB10_tr);
    drawCenteredStr(30, "CALIBRATION");
    u8g2.setCursor(20, 70);
    u8g2.print("PRESS BTN ");
    u8g2.print(calibStep + 1);
    u8g2.setFont(u8g2_font_6x10_tr);
    drawCenteredStr(100, "(Scan Matrix)");
}

void updateDisplay()
{
  u8g2.firstPage();
  do
  {
    if (screenSaverActive && currentState == STATE_DEFAULT)
    {
      u8g2.setDrawColor(0);
      u8g2.drawBox(0, 0, 128, 128);
      u8g2.setDrawColor(1);
    }
    else if (currentState == STATE_DEFAULT)
      drawDefaultScreen();
    else if (currentState == STATE_MENU)
      drawMenuScreen();
    else if (currentState == STATE_SUBMENU)
      drawSubmenuScreen();
    else if (currentState == STATE_CALIBRATION)
      drawCalibrationScreen();
  } while (u8g2.nextPage());
}

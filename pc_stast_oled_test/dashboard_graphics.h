/*
 * PC STATS DASHBOARD - GRAPHICS LIBRARY
 * (File: dashboard_graphics.h)
 */

#ifndef DASHBOARD_GRAPHICS_H
#define DASHBOARD_GRAPHICS_H

// Questo file ha bisogno di U8g2lib.h per sapere cos'è "U8G2"
#include <U8g2lib.h>

// --- Buffer (locali alle funzioni) ---
// Non abbiamo più bisogno di buffer globali, li dichiariamo
// dove servono per mantenere il file pulito.

// --- SIMPLE Layout Definitions ---
const int START_X_SIMPLE = 5;
const int ICON_PADDING_SIMPLE = 9;
const int CPU_ICON_SIZE = 16;
const int TEMP_ICON_SIZE = 20;
const int RAM_ICON_SIZE = 16;

// --- CUSTOM Icons (for Simple Mode) ---

/* Draws a 16x16 CPU icon with small pins */
static void drawCpuIcon(U8G2 &u8g2, int x, int y)
{
    u8g2.drawFrame(x, y, CPU_ICON_SIZE, CPU_ICON_SIZE); // Main body
    u8g2.drawBox(x + 4, y + 4, 8, 8);                   // Core
    // Left side pins
    u8g2.drawVLine(x - 2, y + 3, 2);
    u8g2.drawVLine(x - 2, y + 6, 2);
    u8g2.drawVLine(x - 2, y + 9, 2);
    u8g2.drawVLine(x - 2, y + 12, 2);
    // Right side pins
    u8g2.drawVLine(x + CPU_ICON_SIZE + 1, y + 3, 2);
    u8g2.drawVLine(x + CPU_ICON_SIZE + 1, y + 6, 2);
    u8g2.drawVLine(x + CPU_ICON_SIZE + 1, y + 9, 2);
    u8g2.drawVLine(x + CPU_ICON_SIZE + 1, y + 12, 2);
    // Top side pins
    u8g2.drawHLine(x + 3, y - 2, 2);
    u8g2.drawHLine(x + 6, y - 2, 2);
    u8g2.drawHLine(x + 9, y - 2, 2);
    u8g2.drawHLine(x + 12, y - 2, 2);
    // Bottom side pins
    u8g2.drawHLine(x + 3, y + CPU_ICON_SIZE + 1, 2);
    u8g2.drawHLine(x + 6, y + CPU_ICON_SIZE + 1, 2);
    u8g2.drawHLine(x + 9, y + CPU_ICON_SIZE + 1, 2);
    u8g2.drawHLine(x + 12, y + CPU_ICON_SIZE + 1, 2);
}

/* Draws a 20x20 Thermometer icon */
static void drawTempIcon(U8G2 &u8g2, int x, int y)
{
    int center_x = x + TEMP_ICON_SIZE / 2;
    center_x = center_x - 1;
    int bulb_radius = 4;
    u8g2.drawDisc(center_x, y + TEMP_ICON_SIZE - bulb_radius - 1, bulb_radius);
    int tube_width = 5;
    int tube_x = x + (TEMP_ICON_SIZE - tube_width) / 2;
    u8g2.drawFrame(tube_x, y, tube_width, 13);
    int liquid_width = 3;
    int liquid_x = x + (TEMP_ICON_SIZE - liquid_width) / 2;
    u8g2.drawBox(liquid_x, y + 8, liquid_width, 8);
}

/* Draws a 16x16 RAM icon */
static void drawRamIcon(U8G2 &u8g2, int x, int y)
{
    u8g2.drawFrame(x, y + 2, RAM_ICON_SIZE, 10);
    u8g2.drawVLine(x + 2, y + 13, 3);
    u8g2.drawVLine(x + 4, y + 13, 3);
    u8g2.drawVLine(x + 6, y + 13, 3);
    u8g2.drawVLine(x + 8, y + 13, 3);
    u8g2.drawVLine(x + 10, y + 13, 3);
    u8g2.drawVLine(x + 12, y + 13, 3);
    u8g2.drawVLine(x + 14, y + 13, 3);
}

// --- CUSTOM Icons (for Advanced Mode) ---

/* Draws a small CPU icon (10x10) with 4 EQUALLY SPACED pins per side */
static void drawSmallCpuIcon(U8G2 &u8g2, int x, int y)
{
    u8g2.drawFrame(x, y, 10, 10);     // Main body
    u8g2.drawBox(x + 2, y + 2, 6, 6); // Core
    // Left side pins
    u8g2.drawVLine(x - 1, y, 1);
    u8g2.drawVLine(x - 1, y + 3, 1);
    u8g2.drawVLine(x - 1, y + 6, 1);
    u8g2.drawVLine(x - 1, y + 9, 1);
    // Right side pins
    u8g2.drawVLine(x + 10, y, 1);
    u8g2.drawVLine(x + 10, y + 3, 1);
    u8g2.drawVLine(x + 10, y + 6, 1);
    u8g2.drawVLine(x + 10, y + 9, 1);
    // Top side pins
    u8g2.drawHLine(x, y - 1, 1);
    u8g2.drawHLine(x + 3, y - 1, 1);
    u8g2.drawHLine(x + 6, y - 1, 1);
    u8g2.drawHLine(x + 9, y - 1, 1);
    // Bottom side pins
    u8g2.drawHLine(x, y + 10, 1);
    u8g2.drawHLine(x + 3, y + 10, 1);
    u8g2.drawHLine(x + 6, y + 10, 1);
    u8g2.drawHLine(x + 9, y + 10, 1);
}

/* Draws a small GPU icon (10x10-ish) with a slightly wider bracket */
static void drawSmallGpuIcon(U8G2 &u8g2, int x, int y)
{
    u8g2.drawFrame(x, y, 10, 8);        // Main body (10 wide, 8 high)
    u8g2.drawFrame(x + 6, y + 8, 4, 2); // Staffa I/O
}

/* Draws a small RAM icon (10x10) */
static void drawSmallRamIcon(U8G2 &u8g2, int x, int y)
{
    u8g2.drawFrame(x, y, 10, 6);
    u8g2.drawVLine(x + 2, y + 7, 2);
    u8g2.drawVLine(x + 4, y + 7, 2);
    u8g2.drawVLine(x + 6, y + 7, 2);
    u8g2.drawVLine(x + 8, y + 7, 2);
}

/* Disegna un'icona Termometro piccola (10x10) */
static void drawSmallTempIcon(U8G2 &u8g2, int x, int y)
{
    u8g2.drawFrame(x + 3, y, 3, 7); // Tubo
    u8g2.drawDisc(x + 4, y + 8, 2); // Bulbo (raggio 2)
}

/* Disegna una freccia GIÙ piccola (10x10) */
static void drawSmallDownArrowIcon(U8G2 &u8g2, int x, int y)
{
    u8g2.drawVLine(x + 4, y, 8);               // Asta
    u8g2.drawLine(x + 4, y + 8, x + 1, y + 5); // Testa sx
    u8g2.drawLine(x + 4, y + 8, x + 7, y + 5); // Testa dx
}

/* Disegna una freccia SU piccola (10x10) */
static void drawSmallUpArrowIcon(U8G2 &u8g2, int x, int y)
{
    u8g2.drawVLine(x + 4, y + 2, 8);           // Asta
    u8g2.drawLine(x + 4, y + 2, x + 1, y + 5); // Testa sx
    u8g2.drawLine(x + 4, y + 2, x + 7, y + 5); // Testa dx
}

/* Helper function to draw a progress bar */
static void drawProgressBar(U8G2 &u8g2, int x, int y, int width, int height, int percentage)
{
    int filled_width = map(percentage, 0, 100, 0, width);
    u8g2.drawFrame(x, y, width, height);
    u8g2.drawBox(x, y, filled_width, height);
}

// --- LAYOUT DRAWING FUNCTIONS ---

/* Draws the SIMPLE layout */
void drawSimpleLayout(U8G2 &u8g2, int cpuUsage, int pcTemp, float memFree)
{
    char stringBuffer[20];
    char floatBuffer[10];
    int text_x_pos;

    text_x_pos = START_X_SIMPLE + CPU_ICON_SIZE + ICON_PADDING_SIMPLE;
    u8g2.setFont(u8g2_font_ncenB18_tr);
    sprintf(stringBuffer, "%d %%", cpuUsage);
    drawCpuIcon(u8g2, START_X_SIMPLE, 15);
    u8g2.drawStr(text_x_pos, 32, stringBuffer);

    text_x_pos = START_X_SIMPLE + TEMP_ICON_SIZE + ICON_PADDING_SIMPLE;
    u8g2.setFont(u8g2_font_ncenB18_tr);
    sprintf(stringBuffer, "%d\xb0"
                          " C",
            pcTemp);
    drawTempIcon(u8g2, START_X_SIMPLE - 1, 51);
    u8g2.drawStr(text_x_pos - 5, 70, stringBuffer);

    text_x_pos = START_X_SIMPLE + RAM_ICON_SIZE + ICON_PADDING_SIMPLE;
    u8g2.setFont(u8g2_font_ncenB18_tr);
    dtostrf(memFree, 3, 1, floatBuffer);
    sprintf(stringBuffer, "%s GB", floatBuffer);
    drawRamIcon(u8g2, START_X_SIMPLE, 91);
    u8g2.drawStr(text_x_pos, 108, stringBuffer);
}

/* Draws the ADVANCED layout (Vertically Centered & Parametric X) */
void drawAdvancedLayout(U8G2 &u8g2,
                        int cpuUsage, int gpuUsage,
                        float memFree, float totalRAM,
                        int pcTemp, int tempGPU,
                        float netDown, float netUp)
{
    // Buffers locali
    char stringBuffer[20];
    char floatBuffer[10];

    // --- Definizioni per il layout ---
    const int ROW_SPACING = 22;
    const int START_Y_BASELINE = 24;
    const int START_X_ADVANCED = 3;

    const int COL_ICON = START_X_ADVANCED;
    const int COL_LABEL = START_X_ADVANCED + 16;
    const int COL_BAR_1 = START_X_ADVANCED + 43;
    const int COL_VALUE_1 = START_X_ADVANCED + 103;

    const int COL_BAR_3 = START_X_ADVANCED + 43;
    const int COL_VALUE_3 = START_X_ADVANCED + 78;

    const int COL_ICON_L = START_X_ADVANCED;
    const int COL_TEXT_L = START_X_ADVANCED + 12;
    const int COL_ICON_R = START_X_ADVANCED + 64;
    const int COL_TEXT_R = START_X_ADVANCED + 64 + 12;
    // --- Fine definizioni ---

    u8g2.setFont(u8g2_font_profont12_tr);
    int y_pos;

    // --- Riga 1: CPU ---
    y_pos = START_Y_BASELINE;
    drawSmallCpuIcon(u8g2, COL_ICON, y_pos - 9);
    u8g2.setFont(u8g2_font_profont12_tr);
    u8g2.drawStr(COL_LABEL, y_pos, "CPU");
    sprintf(stringBuffer, "%d%%", cpuUsage);
    drawProgressBar(u8g2, COL_BAR_1, y_pos - 8, 57, 10, cpuUsage);
    u8g2.drawStr(COL_VALUE_1, y_pos, stringBuffer);

    // --- Riga 2: GPU ---
    y_pos = START_Y_BASELINE + (1 * ROW_SPACING);
    drawSmallGpuIcon(u8g2, COL_ICON, y_pos - 9);
    u8g2.setFont(u8g2_font_profont12_tr);
    u8g2.drawStr(COL_LABEL, y_pos, "GPU");
    sprintf(stringBuffer, "%d%%", gpuUsage);
    drawProgressBar(u8g2, COL_BAR_1, y_pos - 8, 57, 10, gpuUsage);
    u8g2.drawStr(COL_VALUE_1, y_pos, stringBuffer);

    // --- Riga 3: RAM ---
    y_pos = START_Y_BASELINE + (2 * ROW_SPACING);
    int ramPercent = (memFree / totalRAM) * 100.0;
    drawSmallRamIcon(u8g2, COL_ICON, y_pos - 9);
    u8g2.setFont(u8g2_font_profont12_tr);
    u8g2.drawStr(COL_LABEL, y_pos, "RAM");
    dtostrf(memFree, 3, 1, floatBuffer);
    sprintf(stringBuffer, "%s/%dGB", floatBuffer, (int)totalRAM);
    drawProgressBar(u8g2, COL_BAR_3, y_pos - 8, 32, 10, ramPercent);
    u8g2.drawStr(COL_VALUE_3, y_pos, stringBuffer);

    // --- Riga 4: Temperature (CPU / GPU) ---
    y_pos = START_Y_BASELINE + (3 * ROW_SPACING);

    // Blocco 1: CPU Temp
    drawSmallTempIcon(u8g2, COL_ICON_L, y_pos - 9);
    u8g2.setFont(u8g2_font_profont12_tr);
    sprintf(stringBuffer, "CPU %d\xb0"
                          "C",
            pcTemp);
    u8g2.drawStr(COL_TEXT_L, y_pos, stringBuffer);

    // Blocco 2: GPU Temp
    // --- BUG FIX (corretto da drawSmallTempIcon) ---
    drawSmallTempIcon(u8g2, COL_ICON_R, y_pos - 9);
    u8g2.setFont(u8g2_font_profont12_tr);
    sprintf(stringBuffer, "GPU %d\xb0"
                          "C",
            tempGPU);
    u8g2.drawStr(COL_TEXT_R, y_pos, stringBuffer);

    // --- Riga 5: Rete (Down / Up) ---
    y_pos = START_Y_BASELINE + (4 * ROW_SPACING);

    // Blocco 1: Download
    drawSmallDownArrowIcon(u8g2, COL_ICON_L, y_pos - 9);
    u8g2.setFont(u8g2_font_profont12_tr);
    dtostrf(netDown, 4, 1, floatBuffer);
    sprintf(stringBuffer, "%sMB/s", floatBuffer);
    u8g2.drawStr(COL_TEXT_L, y_pos, stringBuffer);

    // Blocco 2: Upload
    drawSmallUpArrowIcon(u8g2, COL_ICON_R, y_pos - 9);
    u8g2.setFont(u8g2_font_profont12_tr);
    dtostrf(netUp, 3, 1, floatBuffer);
    sprintf(stringBuffer, "%sMB/s", floatBuffer);
    u8g2.drawStr(COL_TEXT_R, y_pos, stringBuffer);
}

#endif // DASHBOARD_GRAPHICS_H
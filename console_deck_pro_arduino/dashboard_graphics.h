/*
 * PC STATS DASHBOARD - GRAPHICS LIBRARY (OPTIMIZED)
 * (File: dashboard_graphics.h)
 */

#ifndef DASHBOARD_GRAPHICS_H
#define DASHBOARD_GRAPHICS_H

#include <U8g2lib.h>

// --- Font condivisi per risparmiare memoria ---
// Usiamo quelli definiti nel file principale, passati via u8g2
// Assumiamo che u8g2_font_6x10_tr (Piccolo) e u8g2_font_ncenB10_tr (Medio) siano usati.

const int START_X_SIMPLE = 5;
const int ICON_PADDING_SIMPLE = 9;
const int CPU_ICON_SIZE = 16;
const int TEMP_ICON_SIZE = 20;
const int RAM_ICON_SIZE = 16;

// --- CUSTOM ICONS (Disegnate a codice per risparmiare flash rispetto alle bitmap) ---

static void drawCpuIcon(U8G2 &u8g2, int x, int y)
{
    u8g2.drawFrame(x, y, CPU_ICON_SIZE, CPU_ICON_SIZE);
    u8g2.drawBox(x + 4, y + 4, 8, 8);
    // Pins semplificati (meno chiamate = meno spazio)
    u8g2.drawVLine(x - 2, y + 3, 10);
    u8g2.drawVLine(x + CPU_ICON_SIZE + 1, y + 3, 10);
    u8g2.drawHLine(x + 3, y - 2, 10);
    u8g2.drawHLine(x + 3, y + CPU_ICON_SIZE + 1, 10);
}

static void drawTempIcon(U8G2 &u8g2, int x, int y)
{
    u8g2.drawDisc(x + 9, y + 15, 4);
    u8g2.drawFrame(x + 7, y, 5, 13);
    u8g2.drawBox(x + 8, y + 8, 3, 8);
}

static void drawRamIcon(U8G2 &u8g2, int x, int y)
{
    u8g2.drawFrame(x, y + 2, RAM_ICON_SIZE, 10);
    for (int i = 2; i <= 14; i += 2)
        u8g2.drawVLine(x + i, y + 13, 3);
}

// --- ICONE PICCOLE (Optimized) ---

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
    u8g2.drawVLine(x + 5, y + 7, 2);
}

static void drawSmallTempIcon(U8G2 &u8g2, int x, int y)
{
    u8g2.drawFrame(x + 3, y, 3, 7);
    u8g2.drawDisc(x + 4, y + 8, 2);
}

static void drawSmallArrow(U8G2 &u8g2, int x, int y, bool up)
{
    u8g2.drawVLine(x + 4, y + (up ? 2 : 0), 8);
    if (up)
    {
        u8g2.drawLine(x + 4, y + 2, x + 1, y + 5);
        u8g2.drawLine(x + 4, y + 2, x + 7, y + 5);
    }
    else
    {
        u8g2.drawLine(x + 4, y + 8, x + 1, y + 5);
        u8g2.drawLine(x + 4, y + 8, x + 7, y + 5);
    }
}

static void drawProgressBar(U8G2 &u8g2, int x, int y, int width, int height, int percentage)
{
    int filled = (percentage * width) / 100;
    u8g2.drawFrame(x, y, width, height);
    u8g2.drawBox(x, y, filled, height);
}

// --- LAYOUTS (Senza sprintf per risparmiare memoria) ---

void drawSimpleLayout(U8G2 &u8g2, int cpuUsage, int pcTemp, float memFree)
{
    // Usiamo il font Medio (già caricato nel main) invece del B18 che è enorme
    u8g2.setFont(u8g2_font_ncenB10_tr);

    // CPU
    drawCpuIcon(u8g2, START_X_SIMPLE, 15);
    u8g2.setCursor(START_X_SIMPLE + 25, 28);
    u8g2.print(cpuUsage);
    u8g2.print(F("%"));

    // TEMP
    drawTempIcon(u8g2, START_X_SIMPLE, 45);
    u8g2.setCursor(START_X_SIMPLE + 25, 60);
    u8g2.print(pcTemp);
    u8g2.print(F("\xb0C"));

    // RAM
    drawRamIcon(u8g2, START_X_SIMPLE, 80);
    u8g2.setCursor(START_X_SIMPLE + 25, 92);
    u8g2.print(memFree, 1);
    u8g2.print(F(" GB"));
}

void drawAdvancedLayout(U8G2 &u8g2, int cpu, int gpu, float mem, float totRam, int tempC, int tempG, float netD, float netU)
{
    // Usiamo il font Piccolo standard (6x10) invece del profont per non caricarne due simili
    u8g2.setFont(u8g2_font_6x10_tr);

    const int H = 22; // Altezza riga
    int y = 24;
    int x_bar = 45;
    int x_val = 100;

    // CPU
    drawSmallCpuIcon(u8g2, 3, y - 9);
    u8g2.drawStr(18, y, "CPU");
    drawProgressBar(u8g2, x_bar, y - 8, 50, 8, cpu);
    u8g2.setCursor(x_val, y);
    u8g2.print(cpu);
    u8g2.print("%");

    // GPU
    y += H;
    drawSmallGpuIcon(u8g2, 3, y - 9);
    u8g2.drawStr(18, y, "GPU");
    drawProgressBar(u8g2, x_bar, y - 8, 50, 8, gpu);
    u8g2.setCursor(x_val, y);
    u8g2.print(gpu);
    u8g2.print("%");

    // RAM
    y += H;
    drawSmallRamIcon(u8g2, 3, y - 9);
    u8g2.drawStr(18, y, "RAM");
    drawProgressBar(u8g2, x_bar, y - 8, 25, 8, (int)((mem / totRam) * 100));
    u8g2.setCursor(75, y);
    u8g2.print(mem, 1);
    u8g2.print("/");
    u8g2.print((int)totRam);

    // TEMPS
    y += H;
    drawSmallTempIcon(u8g2, 3, y - 9);
    u8g2.setCursor(15, y);
    u8g2.print("C:");
    u8g2.print(tempC);
    u8g2.print("\xb0");

    drawSmallTempIcon(u8g2, 64, y - 9);
    u8g2.setCursor(76, y);
    u8g2.print("G:");
    u8g2.print(tempG);
    u8g2.print("\xb0");

    // NET
    y += H;
    drawSmallArrow(u8g2, 3, y - 9, false); // Down
    u8g2.setCursor(15, y);
    u8g2.print(netD, 1);

    drawSmallArrow(u8g2, 64, y - 9, true); // Up
    u8g2.setCursor(76, y);
    u8g2.print(netU, 1);
}

#endif
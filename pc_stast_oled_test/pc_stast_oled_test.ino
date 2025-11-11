/*
 * PC STATS DASHBOARD - MAIN LOGIC
 * (File .ino principale)
 */

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "dashboard_graphics.h" // <-- Includi il tuo nuovo file grafico

// The correct constructor for this display
U8G2_SH1107_SEEED_128X128_1_HW_I2C u8g2(U8G2_R2, /* reset=*/U8X8_PIN_NONE);

// --- MAIN TOGGLE SWITCH ---
bool isAdvance = true; // true = Advanced, false = Simple

// --- Dummy Data (placeholders) ---
float cpuUsage = 75.2;
float gpuUsage = 42.4;
float pcTemp = 68.1;
float memFree = 8.6;
float totalRAM = 16.0;
float tempGPU = 72.4;
float netDown = 15.4;
float netUp = 2.1;

void setup(void)
{
    u8g2.begin();
    u8g2.setFontMode(1); // Use transparent font mode
}

void loop(void)
{
    // 1. Qui è dove leggerai i dati dalla porta seriale e
    //    aggiornerai le variabili qui sopra.
    //    (Per ora, usiamo i dati fittizi)

    // 2. Inizia il disegno
    u8g2.firstPage();
    do
    {
        // Controlla quale layout disegnare
        if (isAdvance)
        {
            // Passa tutti i dati necessari alla funzione di disegno
            drawAdvancedLayout(
                u8g2,
                cpuUsage, gpuUsage,
                memFree, totalRAM,
                pcTemp, tempGPU,
                netDown, netUp);
        }
        else
        {
            // Passa i dati per il layout semplice
            drawSimpleLayout(
                u8g2,
                cpuUsage, pcTemp, memFree);
        }

    } while (u8g2.nextPage());

    delay(1000); // Refresh rate
}
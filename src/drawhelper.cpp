#include <Arduino.h>
#include "drawhelper.h"
#include "tft.h"
#include "log.h"

void drawErrorMessage(String message)
{
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 20);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.println(message);
}
  
/*
void printDeviceAddress()
{

    const uint8_t *point = esp_bt_dev_get_address();

    for (int i = 0; i < 6; i++)
    {

        char str[3];

        sprintf(str, "%02X", (int)point[i]);
        //Serial.print(str);
        tft.print(str);

        if (i < 5)
        {
            // Serial.print(":");
            tft.print(":");
        }
    }
}
*/
  
void drawSingleButton(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, uint32_t outline, String label) 
{

    //Draw the button
    uint8_t r = min(w, h) / 4; // Corner radius
    tft.fillRoundRect(x, y, w, h, r, color);
    tft.drawRoundRect(x, y, w, h, r, outline);

    //Print the label
    tft.setTextColor(TFT_WHITE,color);
    tft.setTextSize(2);  
    uint8_t tempdatum = tft.getTextDatum();
    tft.setTextDatum(MC_DATUM);
    uint16_t tempPadding = tft.getTextPadding();
    tft.setTextPadding(0);

    tft.drawString(label, x + (w/2), y + (h/2));
    tft.setTextDatum(tempdatum);
    tft.setTextPadding(tempPadding);
}
#include <Arduino.h>
#include <TFT_eSPI.h> // The TFT_eSPI library
#include "filesystem.h"
#include "tft.h"
#include "bmp.h"
#include "log.h"

static uint16_t read16(fs::File &f)
{
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

static uint32_t read32(fs::File &f)
{
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

unsigned long convertHTMLtoRGB888(char *html)
{
  char *hex = html + 1; // remove the #
  unsigned long rgb = strtoul(hex, NULL, 16);
  return rgb;
}

unsigned int convertRGB888ToRGB565(unsigned long rgb)
{
  return (((rgb & 0xf80000) >> 8) | ((rgb & 0xfc00) >> 5) | ((rgb & 0xf8) >> 3));
}

void drawBmpTransparent(const char *filename, int16_t x, int16_t y)
{

  if ((x >= tft.width()) || (y >= tft.height()))
    return;

  fs::File bmpFS;

  bmpFS = FILESYSTEM.open(filename, "r");

  if (!bmpFS)
  {
    logError("Bitmap not found: %s\n", filename);
    filename = "/logos/question.bmp";
    bmpFS = FILESYSTEM.open(filename, "r");
  }
  logDebug("Loading: %s\n", filename);

  uint32_t seekOffset;
  uint16_t w, h, row;
  uint8_t r, g, b;

  if (read16(bmpFS) == 0x4D42)
  {
    read32(bmpFS);
    read32(bmpFS);
    seekOffset = read32(bmpFS);
    read32(bmpFS);
    w = read32(bmpFS);
    h = read32(bmpFS);

    if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0))
    {
      y += h - 1;

      bool oldSwapBytes = tft.getSwapBytes();
      tft.setSwapBytes(true);
      bmpFS.seek(seekOffset);

      uint16_t padding = (4 - ((w * 3) & 3)) & 3;
      uint8_t lineBuffer[w * 3 + padding];

      for (row = 0; row < h; row++)
      {
        bmpFS.read(lineBuffer, sizeof(lineBuffer));
        uint8_t *bptr = lineBuffer;
        uint16_t *tptr = (uint16_t *)lineBuffer;
        // Convert 24 to 16 bit colours
        for (uint16_t col = 0; col < w; col++)
        {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
          *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        // Push the pixel row to screen, pushImage will crop the line if needed
        // y is decremented as the BMP image is drawn bottom up
        tft.pushImage(x, y--, w, 1, (uint16_t *)lineBuffer, TFT_BLACK);
      }
      tft.setSwapBytes(oldSwapBytes);
    }
    else
      Serial.println("BMP format not recognized.");
  }
  bmpFS.close();
}

/**
* @brief This function draws a BMP on the TFT screen according
         to the given x and y coordinates. 
*
* @param  *filename
* @param x int16_t 
* @param y int16_t 
*
* @return none
*
* @note In contradiction to drawBmpTransparent() this does draw black pixels.
*/
void drawBmp(const char *filename, int16_t x, int16_t y)
{
  if ((x >= tft.width()) || (y >= tft.height())) {
    logWarn("BMP %s invalid position: %d,%d (%d,%d)\n", filename, x, y, tft.width(), tft.height());
    return;
  }

  fs::File bmpFS;
  bmpFS = FILESYSTEM.open(filename, "r");

  if (!bmpFS)
  {
    logError("File Not Found: %s\n", filename);
    return;
  }

  uint32_t seekOffset;
  uint16_t w, h, row;
  uint8_t r, g, b;

  uint16_t magic = read16(bmpFS);
  if (magic == 0x4D42)
  {
    read32(bmpFS);
    read32(bmpFS);
    seekOffset = read32(bmpFS);
    read32(bmpFS);
    w = read32(bmpFS);
    h = read32(bmpFS);

    if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0))
    {
      y += h - 1;

      bool oldSwapBytes = tft.getSwapBytes();
      tft.setSwapBytes(true);
      bmpFS.seek(seekOffset);

      uint16_t padding = (4 - ((w * 3) & 3)) & 3;
      uint8_t lineBuffer[w * 3 + padding];

      for (row = 0; row < h; row++)
      {

        bmpFS.read(lineBuffer, sizeof(lineBuffer));
        uint8_t *bptr = lineBuffer;
        uint16_t *tptr = (uint16_t *)lineBuffer;
        // Convert 24 to 16 bit colours
        for (uint16_t col = 0; col < w; col++)
        {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
          *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        // Push the pixel row to screen, pushImage will crop the line if needed
        // y is decremented as the BMP image is drawn bottom up
        tft.pushImage(x, y--, w, 1, (uint16_t *)lineBuffer);
      }
      tft.setSwapBytes(oldSwapBytes);
    }
    else {
      logError("%s: BMP format not recognized\n", filename);
    }
  } else {
      logError("%s: expected magic 0x4D42, got 0x%X\n", filename, magic);
  }
  bmpFS.close();
}
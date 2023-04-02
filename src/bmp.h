#ifndef _H_BMP_
#define _H_BMP_

unsigned long convertHTMLtoRGB888(char *html);
unsigned int convertRGB888ToRGB565(unsigned long rgb);
void drawBmpTransparent(const char *filename, int16_t x, int16_t y);
void drawBmp(const char *filename, int16_t x, int16_t y);

#endif // _H_BMP_
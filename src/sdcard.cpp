#include <SPI.h>
#include <SD.h>
#include "log.h"
#include "sdcard.h"

void beginSDCard()
{
#if 0
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    SD.begin(SD_CS);
    logInfo("beginSDCard\n");

    auto cardType = SD.cardType();

    const char *typ = "UNKNOWN";
    switch(cardType) {
        case CARD_NONE: typ = "None"; break;
        case CARD_MMC: typ="MMC"; break;
        case CARD_SD: typ="SD"; break;
        case CARD_SDHC: typ="SDHC"; break;
    }
    uint64_t cardSize = cardType == CARD_NONE ? 0 : SD.cardSize() / (1024 * 1024);

    logInfo("Card Type: %s, (%d MB)\n", typ, cardSize);
#endif
}
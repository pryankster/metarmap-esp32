#include <Arduino.h>
// #include <SPI.h>

#include <sys/dirent.h>
#include <sys/unistd.h>

#include "filesystem.h"
#include "log.h"
#include "esp_spiffs.h"
#include "esp_vfs_fat.h"

bool beginFilesystem()
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    switch(ret) {
        case ESP_FAIL:
            logError("Failed to mount filesystem\n");
            break;
        case ESP_ERR_NOT_FOUND:
            logError("Failed to find SPIFFS partition\n");
            break;
        case ESP_OK:
            logInfo("SPIFFS mounted on /spiffs\n");
            break;
        default:
            logError("Unknown error %d mounting SPIFFS", ret);
            break;
    }

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,  // from espressif example
    };

    sdmmc_card_t *card;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = (gpio_num_t) SPI_MOSI,
        .miso_io_num = (gpio_num_t) SPI_MISO,
        .sclk_io_num = (gpio_num_t) SPI_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000
    };

    esp_log_level_set("*", ESP_LOG_DEBUG);

    ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        logError("Failed to initialize SD SPI bus: 0x%x (%d)\n", ret, ret);
        return false;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = (gpio_num_t) SD_CS;
    slot_config.host_id = SPI2_HOST;

    // now mount SD card
    logInfo("Mounting FS\n");
    ret = esp_vfs_fat_sdspi_mount("/sd", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        logInfo("VFS Mount RC: %d\n", ret);
        return false;
    }
    logInfo("SD card mounted\n");

    DIR *d = opendir("/sd/");
    if (!d) {
        logError("Failed to opendir /sd/: %s\n", strerror(errno));
        return false;
    } 

    struct dirent *entry;
    logInfo("--- FILES in /sd ---\n");
    while ((entry = readdir(d)) != NULL) {
        logInfo("%s: %s\n",(entry->d_type == DT_REG ? "file" : entry->d_type == DT_DIR ? "dir" : "unknown"), entry->d_name);
    }
    logInfo("--- END OF FILES---\n");
    closedir(d);

    return true;
}

char *data_path(const char *path)
{
    static char buffer[256];
    strcpy(buffer,DATA_PATH_BASE);
    if (path[0] != '/')
        strcat(buffer,"/");
    strcat(buffer,path);
    return buffer;
}
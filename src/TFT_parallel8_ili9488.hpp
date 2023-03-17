#pragma once

#define LGFX_USE_V1

#include <LovyanGFX.hpp>

// Setting example when using LovyanGFX with original settings on ESP32

/*
Duplicate this file, give it a new name, and change the settings to match your environment.
It becomes available by including the created file from the user program.

You can put the duplicated file in the lgfx_user folder of the library and use it,
In that case, please note that it may be deleted when updating the library.

If you want to operate safely, make a backup or put it in the user project folder.
//*/


/// Create a class that does your own settings, derived from LGFX_Device.
class LGFX_Parallel8_ILI9488 : public lgfx::LGFX_Device
{
/*
 You can change the class name from "LGFX" to something else.
 When using with AUTODETECT, "LGFX" is used, so change it to something other than LGFX.
 Also, when using multiple panels at the same time, give each a different name.
 * When changing the class name, it is necessary to change the name of the constructor to the same name as well.

 You can decide how to name it freely, but assuming that the number of settings increases,
 For example, when setting ILI9341 for SPI connection with ESP32 DevKit-C,
  LGFX_DevKitC_SPI_ILI9341
 By using a name such as , and matching the file name and class name, it will be difficult to get lost when using it.
//*/


  lgfx::Panel_ILI9488 _panel_instance;

  lgfx::Bus_Parallel8 _bus_instance; // 8-bit parallel bus instance (ESP32 only)

  lgfx::Light_PWM _light_instance;

public:

  // Create a constructor and set various settings here.
  // If you change the class name, specify the same name for the constructor.
  LGFX_Parallel8_ILI9488(void)
  {
    { // Configure bus control settings.
      auto cfg = _bus_instance.config(); // Get the structure for bus configuration.
      // 8-bit parallel bus settings  
      cfg.i2s_port = I2S_NUM_0; // Select the I2S port to use (I2S_NUM_0 or I2S_NUM_1) (Use ESP32's I2S LCD mode)
      cfg.freq_write = 20000000; // Transmit clock (maximum 20MHz, rounded to 80MHz divided by an integer)
      cfg.pin_rd = TFT_PIN_RD; // pin number connecting RD       (5-pin BLACK)
      cfg.pin_wr = TFT_PIN_WR; // pin number connecting WR       (5-pin WHITE)
      cfg.pin_rs = TFT_PIN_RS; // Pin number connecting RS(D/C) (5-pin GREY) -- this pin is also used for touch screen
      cfg.pin_d0 = TFT_PIN_D0; // pin number connecting D0      (8-pin BLACK)
      cfg.pin_d1 = TFT_PIN_D1; // pin number connecting D1      (8-pin BROWN)
      cfg.pin_d2 = TFT_PIN_D2; // pin number connecting D2      (8-pin RED)
      cfg.pin_d3 = TFT_PIN_D3; // pin number connecting D3      (8-pin ORANGE)
      cfg.pin_d4 = TFT_PIN_D4; // pin number connecting D4      (8-pin YELLOW)
      cfg.pin_d5 = TFT_PIN_D5; // pin number connecting D5      (8-pin GREEN)
      cfg.pin_d6 = TFT_PIN_D6; // pin number connecting D6      (8-pin BLUE)
      cfg.pin_d7 = TFT_PIN_D7; // pin number connecting D7      (8-pin VIOLET)

      _bus_instance.config(cfg); // Apply the settings to the bus.
      _panel_instance.setBus(&_bus_instance); // Sets the bus to the panel.
    }

    { // Set display panel control.
      auto cfg = _panel_instance.config(); // Get the structure for display panel settings.

      // Pin32 (CS) is also used as an analog input to read the touch screen.
      cfg.pin_cs = TFT_PIN_CS; // pin number where CS is connected (-1 = disable) (5pin - VIOLET) (WAS 14 -- conflicts with D7)
      cfg.pin_rst = TFT_PIN_RST; // pin number where RST is connected (-1 = disable) (5pin - BLUE)
      cfg.pin_busy = TFT_PIN_BUSY; // pin number to which BUSY is connected (-1 = disable)

      // * The following setting values are general initial values for each panel, so please try commenting out any unknown items.

      cfg.panel_width = LV_HOR_RES_MAX; // actual displayable width
      cfg.panel_height = LV_VER_RES_MAX; // actual displayable height
      cfg.offset_x = 0; // Panel offset in X direction
      cfg.offset_y = 0; // Panel offset in Y direction
      cfg.offset_rotation = 0; // Rotation value offset 0~7 (4~7 are upside down)
      cfg.dummy_read_pixel = 8; // number of dummy read bits before pixel read
      cfg.dummy_read_bits = 1; // number of dummy read bits before non-pixel data read
      cfg.readable = true; // set to true if data can be read
      cfg.invert = false; // Set to true if panel light/dark is inverted
      cfg.rgb_order = false; // set to true if red and blue on the panel are swapped
      cfg.dlen_16bit = false; // Set to true for panels that transmit data length in 16-bit units with 16-bit parallel or SPI
      cfg.bus_shared = false; // Set to true when the bus is shared with the SD card (bus control with drawJpgFile etc.)

// Please set the following only when the display shifts with a driver with a variable number of pixels such as ST7735 or ILI9163.
// cfg.memory_width = 240; // Maximum width supported by driver IC
// cfg.memory_height = 320; // Maximum height supported by driver IC

      _panel_instance.config(cfg);
    }

/*
    { // Set backlight control. (delete if not necessary)
      auto cfg = _light_instance.config(); // Get the structure for backlight configuration.

      cfg.pin_bl = 32; // pin number to which the backlight is connected
      cfg.invert = false; // true to invert backlight brightness
      cfg.freq = 44100; // backlight PWM frequency
      cfg.pwm_channel = 7; // PWM channel number to use

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance); // Sets the backlight to the panel.
    } 
/*/

/*
    { // Configure settings for touch screen control. (delete if not necessary)
      auto cfg = _touch_instance.config();

      cfg.x_min = 0; // Minimum X value (raw value) obtained from the touchscreen
      cfg.x_max = 239; // Maximum X value (raw value) obtained from the touchscreen
      cfg.y_min = 0; // Minimum Y value obtained from touchscreen (raw value)
      cfg.y_max = 319; // Maximum Y value (raw value) obtained from touchscreen
      cfg.pin_int = 38; // pin number where INT is connected
      cfg.bus_shared = true; // Set true when using a common bus with the screen
      cfg.offset_rotation = 0;// Adjustment when display and touch direction do not match Set with a value of 0 to 7

// for SPI connection
      cfg.spi_host = VSPI_HOST;// Select SPI to use (HSPI_HOST or VSPI_HOST)
      cfg.freq = 1000000; // set SPI clock
      cfg.pin_sclk = 18; // pin number to which SCLK is connected
      cfg.pin_mosi = 23; // pin number where MOSI is connected
      cfg.pin_miso = 19; // pin number where MISO is connected
      cfg.pin_cs = 5; // pin number where CS is connected

// For I2C connection
      cfg.i2c_port = 1; // Select I2C to use (0 or 1)
      cfg.i2c_addr = 0x38; // I2C device address number
      cfg.pin_sda = 23; // pin number to which SDA is connected
      cfg.pin_scl = 32; // pin number to which SCL is connected
      cfg.freq = 400000; // set I2C clock

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance); // Set the touchscreen to the panel.
    }
/*/

    setPanel(&_panel_instance); // Sets the panel to use.
  }
};

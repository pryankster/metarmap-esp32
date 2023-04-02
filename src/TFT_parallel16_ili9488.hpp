#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include <driver/i2c.h>

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ILI9488 _panel_instance;
    lgfx::Bus_Parallel16 _bus_instance;

public:
    LGFX(void)
    {
        {                                      
            auto cfg = _bus_instance.config();

            cfg.port = 0;
            cfg.freq_write = 20000000;
            cfg.pin_wr = TFT_PIN_WR;           // WR 
            cfg.pin_rd = TFT_PIN_RD;           // RD 
            cfg.pin_rs = TFT_PIN_RS;           // RS(D/C)

            cfg.pin_d0 = TFT_PIN_D0;
            cfg.pin_d1 = TFT_PIN_D1;
            cfg.pin_d2 = TFT_PIN_D2;
            cfg.pin_d3 = TFT_PIN_D3;
            cfg.pin_d4 = TFT_PIN_D4;
            cfg.pin_d5 = TFT_PIN_D5;
            cfg.pin_d6 = TFT_PIN_D6;
            cfg.pin_d7 = TFT_PIN_D7;
            cfg.pin_d8 = TFT_PIN_D8;
            cfg.pin_d9 = TFT_PIN_D9;
            cfg.pin_d10 = TFT_PIN_D10;
            cfg.pin_d11 = TFT_PIN_D11;
            cfg.pin_d12 = TFT_PIN_D12;
            cfg.pin_d13 = TFT_PIN_D13;
            cfg.pin_d14 = TFT_PIN_D14;
            cfg.pin_d15 = TFT_PIN_D15;

            _bus_instance.config(cfg);              
            _panel_instance.setBus(&_bus_instance);
        }

        {                                        
            auto cfg = _panel_instance.config();

            cfg.pin_cs = -1;
            cfg.pin_rst = -1;
            cfg.pin_busy = -1;

            cfg.memory_width = LV_HOR_RES_MAX;   
            cfg.memory_height = LV_VER_RES_MAX;  
            cfg.panel_width = LV_HOR_RES_MAX;    
            cfg.panel_height = LV_VER_RES_MAX;   
            cfg.offset_x = 0;         
            cfg.offset_y = 0;         
            cfg.offset_rotation = 0;  
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;  
            cfg.readable = true;      
            cfg.invert = false;       
            cfg.rgb_order = false;    
            cfg.dlen_16bit = true;    
            cfg.bus_shared = true;    

            _panel_instance.config(cfg);
        }

        setPanel(&_panel_instance);
    }
};
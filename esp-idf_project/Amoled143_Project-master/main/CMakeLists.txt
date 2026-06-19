idf_component_register(SRCS "mcpwm_frequency.c" "pcnt_frequency.c" "main.cpp" "bsp.c"
                    INCLUDE_DIRS "."
                    
    PRIV_REQUIRES lvgl touch_bsp read_lcd_id_bsp UI)

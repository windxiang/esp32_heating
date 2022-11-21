set(srcs_rotary
    "rotary/rotary.cpp"
    "rotary/OneButton.cpp"
)

set(srcs_oled
    "oled/oled.cpp"
)

set(srcs_render
    "render/render_main.cpp"
    "render/render_menu.cpp"
    "render/render_screensavers.cpp"
    "render/menuobj.cpp"
    "render/bitmap.cpp"
    "render/ExternDraw.cpp"
)

set(srcs_tools
    "tools/mathFun.cpp"
    "tools/ntc.cpp"
)

set(srcs_shell
    "shell/shell_task.cpp"
) 

set(srcs_nvs
    "nvs/nvs.cpp"
)

set(srcs_logic
    "logic/logic_task.cpp"
    "logic/pidCtl.cpp"
)

set(srcs_pwm
    "pwm/pwm.cpp"
)

set(srcs_ws28122
    "ws2812/ws2812.cpp"
)

set(srcs_max6675
    "max6675/max6675.cpp"
)

set(srcs_adc
    "adc/adc_task.cpp"
)

set(srcs_beep
    "beep/beep.cpp"
)

idf_component_register(SRCS "app_main.cpp" "${srcs_rotary}" "${srcs_oled}" "${srcs_render}" "${srcs_tools}" "${srcs_shell}" "${srcs_nvs}" "${srcs_logic}" "${srcs_pwm}" "${srcs_ws28122}" "${srcs_max6675}" "${srcs_adc}" "${srcs_beep}"
                    INCLUDE_DIRS "." "rotary" "oled" "render" "tools" "shell" "nvs" "logic" "pwm" "ws2812" "max6675" "adc" "beep"
                    REQUIRES spi_flash u8g2 console vfs nvs_flash PIDv1 esp_adc_cal)

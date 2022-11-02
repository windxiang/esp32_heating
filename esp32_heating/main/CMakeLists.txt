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
    "render/menuobj.cpp"
    "render/bitmap.cpp"
    "render/ExternDraw.cpp"
)

set(srcs_tools
    "tools/mathFun.cpp"
)

set(srcs_event
    "event/event.cpp"
)

set(srcs_shell
    "shell/shell_task.cpp"
) 

set(srcs_nvs
    "nvs/nvs.cpp"
)

set(srcs_logic
    "logic/logic_task.cpp"
)

set(srcs_pwm
    "pwm/pwm.cpp"
)

idf_component_register(SRCS "app_main.cpp" "${srcs_rotary}" "${srcs_oled}" "${srcs_render}" "${srcs_tools}" "${srcs_event}" "${srcs_shell}" "${srcs_nvs}" "${srcs_logic}" "${srcs_pwm}"
                    INCLUDE_DIRS "." "rotary" "oled" "render" "tools" "event" "shell" "nvs" "logic" "pwm"
                    REQUIRES spi_flash u8g2 console vfs nvs_flash PIDv1)

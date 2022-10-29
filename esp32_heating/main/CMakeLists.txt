set(srcs_rotary
    "rotary/rotary.cpp"
    "rotary/OneButton.cpp"
)

set(srcs_oled
    "oled/oled.cpp"
)

set(srcs_render
    "render/menuobj.cpp"
    "render/bitmap.cpp"
    "render/render_task.cpp"
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

idf_component_register(SRCS "app_main.cpp" "${srcs_rotary}" "${srcs_oled}" "${srcs_render}" "${srcs_tools}" "${srcs_event}" "${srcs_shell}" "${srcs_nvs}"
                    INCLUDE_DIRS "." "rotary" "oled" "render" "tools" "event" "shell" "nvs"
                    REQUIRES spi_flash u8g2 console vfs nvs_flash)

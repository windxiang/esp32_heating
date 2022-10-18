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

idf_component_register(SRCS "app_main.cpp" "${srcs_rotary}" "${srcs_oled}" "${srcs_render}" "${srcs_tools}" "${srcs_event}"
                    INCLUDE_DIRS "." "rotary" "oled" "render" "tools" "event"
                    REQUIRES spi_flash u8g2)

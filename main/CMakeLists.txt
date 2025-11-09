# ESP32-S3 USB Host Automator - Main Component CMakeLists.txt
# Project: USB Host Mode Automation for Michael Steinmann
# Date: November 2025

idf_component_register(
    SRCS
        "main.c"
        "board_pins.c"
        "led_control.c"
        "filesystem.c"
        "usb_host.c"
        "usb_device.c"
        "usb_host_automator.c"
        "internal_storage.c"
        "workflow.c"
        "button.c"
        "usb_mode_manager.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        "fatfs"
        "driver"
        "esp_rom"
        "usb"
        "spiffs"
        "esp_timer"
)

# Add main directory to include path for tinyusb config
idf_build_set_property(INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" APPEND)


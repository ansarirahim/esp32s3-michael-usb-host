set(priv_requires fatfs console nvs_flash)

if(CONFIG_EXAMPLE_STORAGE_MEDIA_SPIFLASH)
    list(APPEND priv_requires wear_levelling esp_partition)
endif()

idf_component_register(
    SRCS 
        "cdc_acm_vcp_lpc21isp_main.cpp"
        "tusb_msc_main.c"
        "fatfs_example.cpp"
        "rgb_led.cpp"
        "led_strip_encoder.c"
    INCLUDE_DIRS "."
    REQUIRES lpc21isp fatfs wear_levelling driver nvs_flash espressif__esp_tinyusb esp_psram
    PRIV_REQUIRES "${priv_requires}"
    )

# Embed the README.md file as binary data (accessible as _binary_README_md_start/_end)
target_add_binary_data(${COMPONENT_LIB} "${CMAKE_SOURCE_DIR}/README.md" TEXT)

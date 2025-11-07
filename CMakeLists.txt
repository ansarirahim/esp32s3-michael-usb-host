# ESP32-S3 USB Host Automator - Root CMakeLists.txt
# Project: USB Host Mode Automation for Michael Steinmann
# Date: November 2025

cmake_minimum_required(VERSION 3.16)

# Project configuration
set(SUPPORTED_TARGETS esp32s3)

# Include ESP-IDF CMake functions
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Project name and version
project(esp32s3_michael_usb_host)


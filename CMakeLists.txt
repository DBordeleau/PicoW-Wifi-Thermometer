cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH "C:/Users/13435/Desktop/pico-sdk")
set(PICO_EXTRAS_PATH "C:/Users/13435/Desktop/pico-extras")
set(PICO_BOARD pico_w)

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include(${PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

project(pico-w-wifi-thermometer C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

file(GLOB U8G2_SRCS "${CMAKE_CURRENT_LIST_DIR}/external/u8g2/csrc/*.c")

add_executable(wifi_thermometer
    src/main.cpp
    src/dht11.cpp
    src/u8g2_pico.c
    src/http_server.cpp
    fs/fsdata.c
    ${U8G2_SRCS}
)

target_include_directories(wifi_thermometer PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/external/u8g2/csrc
    ${PICO_SDK_PATH}/src/boards/include
)

# custom lwipopts.h
target_compile_definitions(wifi_thermometer PRIVATE
    PICO_LWIPOPTS_PATH="${CMAKE_CURRENT_LIST_DIR}/src/lwipopts.h"
)

target_link_libraries(wifi_thermometer
    pico_stdlib
    hardware_i2c
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_http
)

pico_enable_stdio_usb(wifi_thermometer 1)
pico_enable_stdio_uart(wifi_thermometer 0)

pico_add_extra_outputs(wifi_thermometer)
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/cyw43_arch.h"
#include "dht11.h"
#include "u8g2.h"
#include "http_server.h"
#include <cstdio>
#include <cstring>

// OLED setup
#define I2C_PORT i2c1
#define SDA_PIN 14
#define SCL_PIN 15
#define OLED_ADDR 0x3C

// Wi-Fi credentials
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

// Pico SDK wrappers for u8g2 implemented in u8g2_pico.c
extern void u8g2_sh1106_init(u8g2_t *u8g2);

// Global u8g2 object used by OLED helper functions
static u8g2_t u8g2;

// DHT temp/humidity sensor is on GPIO 16
#define DHT_PIN 16

// Push button that toggles C/F on the OLED is on GPIO 15
#define BUTTON_PIN 15

// Temperature unit toggle
static bool use_celsius = true;

// For button debounce
static int last_button_state = 0;
static unsigned long last_press_time = 0;
const unsigned long debounce_delay = 50;

// Loading animation frames (32x32 bitmaps)
#define FRAME_WIDTH 32
#define FRAME_HEIGHT 32
#define FRAME_COUNT 28

// Animation frames for connecting to wifi animation
// Generated with https://javl.github.io/image2cpp/
static const unsigned char frames[FRAME_COUNT][128] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63, 252, 0, 1, 224, 7, 128, 7, 0, 0, 224, 28, 3, 192, 56, 48, 63, 252, 12, 97, 224, 7, 134, 195, 0, 0, 195, 108, 7, 224, 54, 56, 62, 124, 28, 16, 224, 7, 8, 1, 128, 1, 128, 3, 7, 224, 192, 3, 28, 56, 192, 1, 240, 15, 128, 0, 192, 3, 0, 0, 7, 224, 0, 0, 12, 48, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63, 252, 0, 1, 224, 7, 128, 7, 0, 0, 224, 28, 3, 192, 56, 48, 63, 252, 12, 97, 224, 7, 134, 195, 0, 0, 195, 108, 7, 224, 54, 56, 62, 124, 28, 16, 224, 7, 8, 1, 128, 1, 128, 3, 7, 224, 192, 3, 28, 56, 192, 1, 240, 15, 128, 0, 192, 3, 0, 0, 7, 224, 0, 0, 12, 48, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63, 252, 0, 1, 224, 7, 128, 7, 0, 0, 224, 28, 3, 192, 56, 48, 63, 252, 12, 97, 224, 7, 134, 195, 0, 0, 195, 108, 7, 224, 54, 56, 62, 124, 28, 16, 224, 7, 8, 1, 128, 1, 128, 3, 7, 224, 192, 3, 28, 56, 192, 1, 240, 15, 128, 0, 192, 3, 0, 0, 7, 224, 0, 0, 12, 48, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63, 252, 0, 1, 224, 7, 128, 7, 0, 0, 224, 28, 3, 192, 56, 48, 63, 252, 12, 97, 224, 7, 134, 195, 0, 0, 195, 108, 7, 224, 54, 56, 62, 124, 28, 16, 224, 7, 8, 1, 128, 1, 128, 3, 7, 224, 192, 3, 28, 56, 192, 1, 240, 15, 128, 0, 192, 3, 0, 0, 7, 224, 0, 0, 12, 48, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63, 252, 0, 1, 224, 7, 128, 7, 0, 0, 224, 28, 3, 192, 56, 48, 63, 252, 12, 97, 224, 7, 134, 195, 0, 0, 195, 108, 31, 248, 54, 56, 120, 30, 28, 17, 192, 3, 136, 3, 0, 0, 192, 3, 15, 240, 192, 1, 56, 28, 128, 0, 224, 7, 0, 0, 3, 193, 0, 0, 15, 240, 0, 0, 24, 24, 0, 0, 8, 16, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31, 248, 0, 0, 248, 31, 0, 3, 128, 1, 192, 14, 0, 0, 112, 24, 31, 248, 24, 96, 240, 15, 6, 195, 192, 3, 195, 102, 31, 248, 102, 60, 120, 30, 60, 25, 192, 3, 152, 3, 0, 0, 192, 6, 15, 240, 96, 3, 56, 28, 192, 1, 224, 7, 128, 0, 192, 1, 0, 0, 7, 224, 0, 0, 24, 24, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63, 252, 0, 1, 224, 7, 128, 7, 0, 0, 224, 28, 0, 0, 56, 48, 63, 252, 12, 96, 224, 7, 6, 195, 128, 1, 195, 102, 7, 224, 102, 56, 62, 124, 28, 16, 224, 7, 8, 1, 128, 1, 128, 6, 7, 224, 96, 3, 28, 56, 192, 1, 240, 15, 128, 0, 192, 3, 0, 0, 0, 0, 0, 0, 7, 224, 0, 0, 12, 48, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 2, 64, 0, 0, 1, 128, 0, 0, 0, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 31, 248, 0, 0, 248, 31, 0, 3, 128, 1, 192, 12, 0, 0, 48, 56, 31, 248, 28, 96, 248, 31, 6, 195, 128, 1, 195, 198, 0, 0, 99, 124, 0, 0, 62, 48, 0, 0, 12, 0, 0, 0, 0, 0, 3, 192, 0, 0, 31, 248, 0, 0, 112, 14, 0, 0, 192, 3, 0, 1, 143, 241, 128, 0, 248, 31, 0, 0, 96, 6, 0, 0, 7, 224, 0, 0, 28, 56, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63, 252, 0, 0, 224, 7, 0, 3, 131, 193, 192, 6, 63, 252, 96, 12, 96, 6, 48, 5, 128, 1, 160, 3, 0, 0, 192, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 13, 176, 0, 0, 31, 248, 0, 0, 24, 24, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 128, 0, 0, 8, 16, 0, 0, 36, 36, 0, 0, 23, 232, 0, 0, 12, 56, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 128, 0, 0, 15, 240, 0, 0, 24, 24, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 224, 0, 0, 12, 48, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 224, 0, 0, 12, 48, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 224, 0, 0, 12, 48, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 224, 0, 0, 12, 48, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 224, 0, 0, 12, 48, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 224, 0, 0, 12, 48, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 224, 0, 0, 12, 48, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 192, 0, 0, 15, 240, 0, 0, 31, 248, 0, 0, 24, 24, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 240, 0, 0, 60, 60, 0, 0, 195, 195, 0, 1, 159, 249, 128, 3, 48, 12, 192, 1, 192, 3, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 240, 0, 0, 28, 56, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 240, 0, 0, 252, 63, 0, 3, 192, 3, 192, 6, 1, 128, 96, 28, 63, 252, 56, 48, 224, 7, 12, 51, 128, 1, 204, 30, 0, 0, 120, 12, 0, 0, 48, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 192, 0, 0, 30, 120, 0, 0, 48, 12, 0, 0, 103, 230, 0, 0, 56, 28, 0, 0, 15, 240, 0, 0, 24, 24, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 31, 248, 0, 0, 248, 31, 0, 3, 128, 1, 192, 12, 0, 0, 48, 56, 31, 248, 28, 96, 248, 31, 6, 195, 128, 1, 195, 198, 0, 0, 99, 124, 0, 0, 62, 48, 0, 0, 12, 0, 15, 240, 0, 0, 60, 60, 0, 0, 224, 7, 0, 1, 129, 129, 128, 1, 15, 240, 128, 1, 184, 29, 128, 0, 224, 7, 0, 0, 0, 0, 0, 0, 7, 224, 0, 0, 28, 56, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63, 252, 0, 1, 224, 7, 128, 7, 0, 0, 224, 28, 0, 0, 56, 48, 63, 252, 12, 96, 224, 7, 6, 195, 128, 1, 195, 102, 7, 224, 102, 56, 62, 124, 28, 16, 224, 7, 8, 1, 128, 1, 128, 6, 7, 224, 96, 3, 28, 56, 192, 1, 240, 15, 128, 0, 192, 3, 0, 0, 0, 0, 0, 0, 7, 224, 0, 0, 12, 48, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 2, 64, 0, 0, 1, 128, 0, 0, 0, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31, 248, 0, 0, 248, 31, 0, 3, 128, 1, 192, 14, 0, 0, 112, 24, 31, 248, 24, 96, 240, 15, 6, 195, 192, 3, 195, 102, 31, 248, 102, 60, 120, 30, 60, 25, 192, 3, 152, 3, 0, 0, 192, 6, 15, 240, 96, 3, 56, 28, 192, 1, 224, 7, 128, 0, 192, 1, 0, 0, 7, 224, 0, 0, 24, 24, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63, 252, 0, 1, 224, 7, 128, 7, 0, 0, 224, 28, 3, 192, 56, 48, 63, 252, 12, 97, 224, 7, 134, 195, 0, 0, 195, 108, 31, 248, 54, 56, 120, 30, 28, 17, 192, 3, 136, 3, 0, 0, 192, 3, 15, 240, 192, 1, 56, 28, 128, 0, 224, 7, 0, 0, 3, 193, 0, 0, 15, 240, 0, 0, 24, 24, 0, 0, 8, 16, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63, 252, 0, 1, 224, 7, 128, 7, 0, 0, 224, 28, 3, 192, 56, 48, 63, 252, 12, 97, 224, 7, 134, 195, 0, 0, 195, 108, 7, 224, 54, 56, 62, 124, 28, 16, 224, 7, 8, 1, 128, 1, 128, 3, 7, 224, 192, 3, 28, 56, 192, 1, 240, 15, 128, 0, 192, 3, 0, 0, 7, 224, 0, 0, 12, 48, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63, 252, 0, 1, 224, 7, 128, 7, 0, 0, 224, 28, 3, 192, 56, 48, 63, 252, 12, 97, 224, 7, 134, 195, 0, 0, 195, 108, 7, 224, 54, 56, 62, 124, 28, 16, 224, 7, 8, 1, 128, 1, 128, 3, 7, 224, 192, 3, 28, 56, 192, 1, 240, 15, 128, 0, 192, 3, 0, 0, 7, 224, 0, 0, 12, 48, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 63, 252, 0, 1, 224, 7, 128, 7, 0, 0, 224, 28, 3, 192, 56, 48, 63, 252, 12, 97, 224, 7, 134, 195, 0, 0, 195, 108, 7, 224, 54, 56, 62, 124, 28, 16, 224, 7, 8, 1, 128, 1, 128, 3, 7, 224, 192, 3, 28, 56, 192, 1, 240, 15, 128, 0, 192, 3, 0, 0, 7, 224, 0, 0, 12, 48, 0, 0, 24, 24, 0, 0, 12, 48, 0, 0, 6, 96, 0, 0, 3, 192, 0, 0, 1, 128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

// Writes to OLED command register via I2C
void oled_write_cmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};
    i2c_write_blocking(I2C_PORT, OLED_ADDR, buf, 2, false);
}

static void display_init(void)
{
    u8g2_sh1106_init(&u8g2);
}

static void display_clear(void)
{
    u8g2_ClearBuffer(&u8g2);
    u8g2_SendBuffer(&u8g2);
}

// Displays a splash screen at startup: "Created by Dillon Bordeleau"
static void display_splash_screen(void)
{
    u8g2_ClearBuffer(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_7x14_tr); // Larger font

    // Center the text
    const char *text = "Created by";
    int width = u8g2_GetStrWidth(&u8g2, text);
    u8g2_DrawStr(&u8g2, (128 - width) / 2, 25, text);

    const char *name = "Dillon Bordeleau";
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
    width = u8g2_GetStrWidth(&u8g2, name);
    u8g2_DrawStr(&u8g2, (128 - width) / 2, 40, name);

    u8g2_SendBuffer(&u8g2);
    sleep_ms(2000); // Show for 2 seconds
}

// Draw one line into the buffer at page 'line'
// Each page is 8px high and the display is 128x64 so there are 8 lines (0-7)
static void display_print_line(const char *msg, int line)
{
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tr); // small fixed width font
    // Each line is 10px high so start at y=10, then y=20, etc.
    int y = 10 + line * 10;
    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawStr(&u8g2, 0, y, msg);
    u8g2_SendBuffer(&u8g2);
}

// Draw two lines and send once, used to update temp and humidity display simultaneously
static void display_print_two_lines(const char *l1, const char *l2, int line1, int line2)
{
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawStr(&u8g2, 0, 10 + line1 * 10, l1);
    u8g2_DrawStr(&u8g2, 0, 10 + line2 * 10, l2);
    u8g2_SendBuffer(&u8g2);
}

// Displays a loading animation while connecting to WiFi
// Shows "Connecting to wifi..." text and animated frames below
// Returns when WiFi is connected
static void display_loading_animation(void)
{
    int frame = 0;
    const uint32_t min_display_time = 2000; // minimum 2 seconds
    uint32_t start_time = to_ms_since_boot(get_absolute_time());

    while (cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA) != CYW43_LINK_UP ||
           (to_ms_since_boot(get_absolute_time()) - start_time) < min_display_time)
    {
        u8g2_ClearBuffer(&u8g2);

        // Display "Connecting to wifi..." text at the top
        u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
        u8g2_DrawStr(&u8g2, 0, 10, "Connecting to wifi...");

        // Draw the animation frame centered below the text (x=48, y=16)
        u8g2_DrawXBM(&u8g2, 48, 16, FRAME_WIDTH, FRAME_HEIGHT, frames[frame]);

        u8g2_SendBuffer(&u8g2);

        frame = (frame + 1) % FRAME_COUNT;
        sleep_ms(20); // Small delay for smooth animation
    }
}

int main()
{
    stdio_init_all();
    printf("Initializing WiFi Thermometer...\n");

    // Initialize button pin
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_down(BUTTON_PIN); // Pull down so button press reads HIGH

    display_init();
    display_splash_screen(); // Show "Created by Dillon Bordeleau"
    display_clear();

    // Initialize WiFi chip. cyw43_arch_init() returns 0 on success
    if (cyw43_arch_init())
    {
        printf("WiFi init failed\n");
        display_print_line("WiFi init failed", 1);
        return -1;
    }

    cyw43_arch_enable_sta_mode();
    printf("Connecting to Wi-Fi...\n");

    // Start WiFi connection asynchronously (non-blocking)
    cyw43_arch_wifi_connect_async(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK);

    // Show loading animation while connecting
    display_loading_animation();

    // Check if connection succeeded
    if (cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP)
    {
        printf("Wi-Fi connected.\n");
        display_clear();
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
        const char *text = "Connected!";
        int width = u8g2_GetStrWidth(&u8g2, text);
        u8g2_DrawStr(&u8g2, (128 - width) / 2, 30, text);
        u8g2_SendBuffer(&u8g2);
        sleep_ms(2000); // Show "Connected!" for 2 seconds

        // Start HTTP server
        web_server_init();

        // Print IP address to console and OLED
        char ip_str[32];
        snprintf(ip_str, sizeof(ip_str), "%s", ip4addr_ntoa(netif_ip4_addr(netif_list)));
        printf("IP: %s\n", ip_str);

        display_clear();
        u8g2_ClearBuffer(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_6x10_tr);
        u8g2_DrawStr(&u8g2, 0, 20, "IP Address:");
        u8g2_DrawStr(&u8g2, 0, 35, ip_str);
        u8g2_SendBuffer(&u8g2);
        sleep_ms(3000); // Show IP for 3 seconds
    }
    else
    {
        printf("Wi-Fi connect failed\n");
        display_print_line("WiFi connect fail", 1);
    }

    // Initialize DHT11 sensor, delay 2 seconds to ensure sensor is ready
    DHT11 dht(DHT_PIN);
    sleep_ms(2000);

    // Main loop: read DHT11 and update OLED display every 3 seconds
    // Then attempt to upload reading to HTTP server
    while (true)
    {
        // Handle button press for temperature unit toggle
        int current_button_state = gpio_get(BUTTON_PIN);
        uint32_t current_time = to_ms_since_boot(get_absolute_time());

        // Toggle temp unit on button press with debounce
        if (current_button_state == 1 && last_button_state == 0 &&
            (current_time - last_press_time) > debounce_delay)
        {
            use_celsius = !use_celsius;
            last_press_time = current_time;
            printf("Temperature unit toggled to %s\n", use_celsius ? "Celsius" : "Fahrenheit");
        }
        last_button_state = current_button_state;

        float temp, humidity;
        if (dht.read(&temp, &humidity))
        {
            printf("Temp: %.2fC, Hum: %.2f%%\n", temp, humidity);
            char line1[32];

            if (use_celsius)
            {
                snprintf(line1, sizeof(line1), "Temp: %.2fC", temp);
            }
            else
            {
                float tempF = (temp * 9.0 / 5.0) + 32.0;
                snprintf(line1, sizeof(line1), "Temp: %.2fF", tempF);
            }

            char line2[32];
            snprintf(line2, sizeof(line2), "Hum: %.2f%%", humidity);
            display_print_two_lines(line1, line2, 1, 2);

            web_server_update_data(temp, humidity);
        }
        else
        {
            printf("DHT read error.\n");
            display_print_line("Sensor error", 1);
        }

        sleep_ms(3000);
    }
}
/*
    Pico SDK wrapper for u8g2 library.
    Provides initializer for SH1106 128x64 I2C OLED and
    implements required I2C and GPIO/delay callbacks.
    Requires submodule from https://github.com/olikraus/u8g2
*/

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "u8g2.h"
#include "u8x8.h"
#include <string.h>

#define U8G2_I2C_PORT i2c1
#define U8G2_SDA_PIN 14
#define U8G2_SCL_PIN 15
#define U8G2_ADDR 0x3C

static uint8_t i2c_buf[256];
static size_t i2c_buf_len = 0;

// handles send/init/start/end for I2C
// uses blocking I2C transfers, shouldn't be a problem with such small frame updates
uint8_t u8x8_byte_hw_i2c_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    (void)u8x8;
    switch (msg)
    {
    case U8X8_MSG_BYTE_SEND:
        if (arg_int > 0 && arg_ptr)
        {
            size_t n = (size_t)arg_int;
            if (i2c_buf_len + n <= sizeof(i2c_buf))
            {
                memcpy(i2c_buf + i2c_buf_len, arg_ptr, n);
                i2c_buf_len += n;
            }
        }
        return 1;
    case U8X8_MSG_BYTE_INIT:
        i2c_init(U8G2_I2C_PORT, 400000);
        gpio_set_function(U8G2_SDA_PIN, GPIO_FUNC_I2C);
        gpio_set_function(U8G2_SCL_PIN, GPIO_FUNC_I2C);
        gpio_pull_up(U8G2_SDA_PIN);
        gpio_pull_up(U8G2_SCL_PIN);
        return 1;
    case U8X8_MSG_BYTE_START_TRANSFER:
        i2c_buf_len = 0;
        return 1;
    case U8X8_MSG_BYTE_END_TRANSFER:
        if (i2c_buf_len)
        {
            int ret = i2c_write_blocking(U8G2_I2C_PORT, U8G2_ADDR, i2c_buf, i2c_buf_len, false);
            (void)ret;
            i2c_buf_len = 0;
        }
        return 1;
    default:
        return 0;
    }
}

// handles reset and delays
uint8_t u8x8_gpio_and_delay_pico(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    (void)u8x8;
    switch (msg)
    {
    case U8X8_MSG_GPIO_AND_DELAY_INIT:
        // not used
        return 1;
    case U8X8_MSG_DELAY_MILLI:
        sleep_ms(arg_int);
        return 1;
    case U8X8_MSG_DELAY_10MICRO:
        sleep_us(arg_int * 10);
        return 1;
    case U8X8_MSG_GPIO_RESET:
        // Reset pin not used
        return 1;
    default:
        return 0;
    }
}

// Initialize u8g2 structure for SH1106 128x64 I2C
void u8g2_sh1106_init(u8g2_t *u8g2)
{
    u8g2_Setup_sh1106_128x64_noname_f(u8g2, U8G2_R0, u8x8_byte_hw_i2c_pico, u8x8_gpio_and_delay_pico);
    u8g2_InitDisplay(u8g2);
    u8g2_SetPowerSave(u8g2, 0);
}
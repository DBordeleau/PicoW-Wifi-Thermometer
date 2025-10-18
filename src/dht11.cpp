/*
    DHT11 sensor driver for Raspberry Pi Pico
    Datasheet: https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf
*/

#include "dht11.h"

// Constructor: Initialize DHT11 on given GPIO pin
DHT11::DHT11(uint gpio) : gpio_(gpio)
{
    gpio_init(gpio_);
}

// Read temperature and humidity from DHT11 sensor
// Returns true on success, false on failure (e.g. checksum error)
// The DHT11 provides decimal precision, but may not be very accurate.
bool DHT11::read(float *temperature, float *humidity)
{
    uint8_t data[5] = {0}; // 5 bytes: humidity int, humidity dec, temp int, temp dec, checksum

    // Start signal, pull pin low for at least 18ms then high for 20-40us
    gpio_set_dir(gpio_, GPIO_OUT);
    gpio_put(gpio_, 0);
    sleep_ms(18);
    gpio_put(gpio_, 1);
    sleep_us(40);
    gpio_set_dir(gpio_, GPIO_IN);

    // Wait for response, DHT pulls line low for 80us then high for 80us.
    // When the line goes low again DHT is ready to send data.
    uint32_t timeout = 10000;
    while (gpio_get(gpio_) && --timeout) // Wait for DHT to pull low
        ;
    while (!gpio_get(gpio_) && --timeout) // Wait for DHT to pull high
        ;
    while (gpio_get(gpio_) && --timeout) // Wait for DHT to pull low again
        ;

    // Read 40 bits, every bit begins with 50us low pulse
    // length of following high pulse determines whether bit is 0 or 1
    // 26-28us high pulse means 0, 70us high pulse means 1
    for (int i = 0; i < 40; i++)
    {
        while (!gpio_get(gpio_)) // wait for the start of the bit's high pulse
            ;
        absolute_time_t start = get_absolute_time();
        while (gpio_get(gpio_))
            ;
        uint32_t diff = absolute_time_diff_us(start, get_absolute_time()); // calculate length of high pulse
        data[i / 8] <<= 1;                                                 // shift current byte and set LSB to 1 if length > 40us
        if (diff >= 70)
            data[i / 8] |= 1;
    }

    // Checksum
    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF))
        return false;

    *humidity = data[0] + (data[1] / 10.0);
    *temperature = data[2] + (data[3] / 10.0);
    return true;
}

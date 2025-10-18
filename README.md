# WiFi Thermometer - Raspberry Pi Pico W

A WiFi-enabled temperature and humidity monitoring system built with the Raspberry Pi Pico W. Has both an onboard OLED display as well as a web interface. Both are updated every 3 seconds with temperature and humidity readings from a DHT11 sensor. Also features an onboard push button to toggle between fahrenheit and celsius display.

[![Video Demo](https://img.youtube.com/vi/NePnPrrfUlU/0.jpg)](https://www.youtube.com/watch?v=NePnPrrfUlU)

## Features

- Real-time temperature and humidity monitoring via DHT11 sensor
- 128x64 OLED display (SH1106) showing live readings
- HTTP server that sends updates to a web interface in sync with updates to the onboard OLED display.
- Physical push button to toggle between Celsius and Fahrenheit

### Components
- Raspberry Pi Pico W
- DHT11 Temperature & Humidity Sensor
- SH1106 128x64 I2C OLED Display
- Push button
- Breadboard and jumper wires

### Wiring Diagram

![](https://i.imgur.com/5rtreW2.png)

## Building from Source

- You will need to modify the WIFI_SSID and WIFI_PASS global variables in main.cpp to match your network credentials.
- You will also need to update the PICO_SDK_PATH and PICO_EXTRAS_PATH in CMakeLists.txt to your local paths.

### Prerequisites

- CMake 3.13 or higher
- ARM GCC compiler (`arm-none-eabi-gcc`)
- [Pico SDK](https://github.com/raspberrypi/pico-sdk)
- [Pico Extras](https://github.com/raspberrypi/pico-extras)

### Setup Instructions

1. **Clone the repository with submodules:**
   ```bash
   git clone --recursive https://github.com/yourusername/PicoW-Wifi-Thermometer.git
   cd PicoW-Wifi-Thermometer
   ```
   
   **Note:** The `--recursive` flag is required to clone the u8g2 graphics library submodule.
   
   If you already cloned without `--recursive`, run:
   ```bash
   git submodule update --init --recursive
   ```

   The OLED display will not work and the project will not compile without this submodule.

2. **Set up the Pico SDK and Pico Extras:**
   
   Download and set up the Pico SDK if you haven't already:
   ```bash
   git clone https://github.com/raspberrypi/pico-sdk.git
   cd pico-sdk
   git submodule update --init
   cd ..
   
   git clone https://github.com/raspberrypi/pico-extras.git
   ```

3. **Configure WiFi credentials:**
   
   Edit `src/main.cpp` and update lines 16-17 with your WiFi credentials:
   ```cpp
   #define WIFI_SSID "YOUR_WIFI_SSID"
   #define WIFI_PASS "YOUR_WIFI_PASSWORD"
   ```

4. **Update CMakeLists.txt paths:**
   
   Edit `CMakeLists.txt` to point to your Pico SDK and Pico Extras installations:
   ```cmake
   set(PICO_SDK_PATH "/path/to/your/pico-sdk")
   set(PICO_EXTRAS_PATH "/path/to/your/pico-extras")
   ```

5. **Build the project:**
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

6. **Flash to Pico W:**
   
   Hold BOOTSEL, plug in the Pico W, then drag `build/wifi_thermometer.uf2` to the RPI-RP2 drive.

## Usage

### Web Interface

1. After flashing and connecting to WiFi, the OLED will display the Pico's IP address
2. Open a web browser and navigate to `http://<PICO_IP_ADDRESS>`. The PICO will print it's IP address in its serial output. You will need to use a serial terminal program, I use PuTTY. Select "Serial" in PuTTY and enter the COM port your Pico W is plugged into. Make sure the speed is set to 115200 and click "Open" to begin reading the serial output.
3. You'll see real-time temperature and humidity readings
4. Click the "Toggle Celsius/Fahrenheit" button to switch units

### OLED Display

- Shows current temperature (toggleable between °C and °F)
- Shows current humidity percentage
- Press the physical button (GPIO 15) to toggle temperature units

### API Endpoint

The device exposes a JSON API at `/temperature`:

```bash
curl http://<PICO_IP_ADDRESS>/temperature
```

Response:
```json
{
  "temperatureC": 23,
  "temperatureF": 73.4,
  "humidity": 45
}
```

## Customizing the Web Interface

The HTML page is embedded in the firmware. To modify it:

1. Edit `fs/index.html`
2. Run the Python script to regenerate `fsdata.c`:
   ```bash
   cd fs
   python generate_fsdata.py
   ```
3. Rebuild the project

## Project Structure

```
PicoW-Wifi-Thermometer/
├── src/
│   ├── main.cpp              # Main program logic
│   ├── dht11.cpp/h           # DHT11 sensor driver
│   ├── http_server.cpp/h     # HTTP server and CGI handlers
│   ├── u8g2_pico.c           # u8g2 OLED driver for Pico
│   └── lwipopts.h            # lwIP network stack configuration
├── fs/
│   ├── index.html            # Web interface
│   ├── generate_fsdata.py    # Script to embed HTML in firmware
│   └── fsdata.c              # Generated embedded filesystem
├── external/
│   └── u8g2/                 # u8g2 graphics library (submodule)
├── CMakeLists.txt            # Build configuration
└── README.md                 # This file
```

## Credits

- **u8g2 Library:** [olikraus/u8g2](https://github.com/olikraus/u8g2)
- **Animation frames:** Generated with [image2cpp](https://javl.github.io/image2cpp/)
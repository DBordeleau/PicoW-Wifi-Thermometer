#include "http_server.h"
#include "lwip/apps/httpd.h"
#include <cstdio>
#include <cstring>

// Required for fsdata_file structure which we use to serve the HTML page
extern "C"
{
#include "lwip/apps/fs.h"
}

static int current_temp = 0;
static int current_humidity = 0;

// CGI handler for /temperature endpoint
// Returns a JSON string with current temp in celsius and fahrenheit, and humidity
// DHT11 only provides celsius data so we calculate fahrenheit here
// index.html makes requests to this endpoint every 3 seconds
const char *temperature_cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    static char json_buffer[128];

    // Convert back from scaled integers to floats
    float tempC = current_temp / 100.0;
    float tempF = (tempC * 9.0 / 5.0) + 32.0;
    float hum = current_humidity / 100.0;

    snprintf(json_buffer, sizeof(json_buffer),
             "{\"temperatureC\":%.2f,\"temperatureF\":%.2f,\"humidity\":%.2f}",
             tempC, tempF, hum);

    return json_buffer;
}

// Registers CGI handler and starts the HTTP server
void web_server_init(void)
{
    httpd_init();

    static const tCGI cgi_handlers[] = {
        {"/temperature", temperature_cgi_handler}};
    http_set_cgi_handlers(cgi_handlers, 1);

    printf("HTTP server initialized\n");
}

void web_server_update_data(int temp, int humidity)
{
    current_temp = temp;
    current_humidity = humidity;
}
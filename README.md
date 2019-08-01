# InfoScreen
Introductory project to esp32/esp8266 programming

The project consists of three microcontrollers - one with a display, the server side, and two others with weather-related sensors, the clients.

## Server
A TTGO TS v1.2 is acting as a weather info station. The serverside utilises the display and the three buttons on the TTGO. With the buttons three different pages can be selected to be viewed on the display:

**Message page**

The server also hosts a webpage, which can be accessed from the IP viewed on the misc page. This webpage contains a simple text input box where text can be submitted and displayed on the TTGO.   


**Weather page**

Here the collected weather data (temperature, humidity and soil moisture) is displayed; both the current and as a graph. The different weather data types can be cycled through by pressing the weather button again


**Miscellaneous page**  

Different network related info is displayed here - SSID of connected network - IP of the TTGO server


## Clients

Two esp's act as clients and send their collected weather data to the TTGO over HTTP

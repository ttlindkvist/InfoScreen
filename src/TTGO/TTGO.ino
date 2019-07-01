// Board type: ESP32 dev mod
//WIFI + Server
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

const char* ssid = "WIM";
const char* password = "mercedeS24";

WebServer server(80);

//Screen
#include <SPI.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
// For the breakout, you can use any 2 or 3 pins
// These pins will also work for the 1.8" TFT shield
#define TFT_CS 16
// you can also connect this to the Arduino reset
// in which case, set this #define pin to -1!
#define TFT_RST 9
#define TFT_DC 17
#define TFT_SCLK 5
#define TFT_MOSI 23
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

/*
#include <RBD_Button.h> // https://github.com/alextaujenis/RBD_Button
#include <RBD_Timer.h>  // https://github.com/alextaujenis/RBD_Timer

// input pullup enabled by default
RBD::Button left_button(35);
RBD::Button middle_button(34);
RBD::Button right_button(39);*/


void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266!");
}

//Data for the info-screens
String localMsg;
String temp;
String hum;

//The current info screen
enum screens {MSG, WEATHER, MISC};
int currScreen = screens::WEATHER;

void handleData() {
  if(server.args() == 0) {
    server.send(200, "text/plain", "You need to include arguments with the data");
    return;
  }

  //parse the received strings to the chars[]
  if(server.arg("temp").length() > 0) {
    temp = server.arg("temp");
    Serial.print("temperature: ");
    Serial.println(temp);
  }
  if(server.arg("hum").length() > 0) {
    hum = server.arg("hum");
    Serial.print("humidity: ");
    Serial.println(hum);
  }
  if(server.arg("msg").length() > 0) {
    localMsg = server.arg("msg");
    Serial.print("Received msg: ");
    Serial.println(localMsg);
  }

  if(currScreen = screens::WEAHTER){
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextSize(2);
    tft.setCursor(0, 0);
    
    tft.print("Temp: ");
    char tempBuff[temp.length()+1];
    temp.toCharArray(tempBuff, temp.length()+1);
    tft.print(tempBuff);
    tft.print("°\n");
    
    tft.print("Fugt: ");
    char humBuff[hum.length()+1];
    hum.toCharArray(humBuff, hum.length()+1);
    tft.print(humBuff);
    tft.print("%");
  }

  
  String returnMsg = temp;
  returnMsg += ", "; 
  returnMsg += hum;
  returnMsg +=", ";
  returnMsg += localMsg;
  
  server.send(200, "text/plain", returnMsg);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}
void initServer() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  //setup different functions for different server-calls
  server.on("/", handleRoot);
  server.on("/send", handleData);
  
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  server.on("/test", [] {
    server.send(200, "text/html", "<HTML><button type=\"button\">Click Me!</button></HTML>"
               );
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void setup(void) {
  //Baud rate for serial output
  Serial.begin(115200);

  initServer();

  Serial.println("Attempting screen boot");
  
  tft.initR(INITR_144GREENTAB); // initialize a ST7735S chip, 
  Serial.println("Initialized tft");

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextWrap(true);
  tft.setTextSize(1);
  tft.setTextColor(ST7735_GREEN);
  IPAddress localAddr = WiFi.localIP();
  byte oct1 = localAddr[0];
  byte oct2 = localAddr[1];
  byte oct3 = localAddr[2];
  byte oct4 = localAddr[3];
  char s[16];  
  sprintf(s, "%d.%d.%d.%d", oct1, oct2, oct3, oct4);
  tft.print(s);

}
void loop(void) {
  server.handleClient();
}

// Board type: ESP32 dev mod
//WIFI + Server
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h> //Figure something out here
#include <vector>
#include <algorithm>
#include <math.h>

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


#include <RBD_Button.h> // https://github.com/alextaujenis/RBD_Button
#include <RBD_Timer.h>  // https://github.com/alextaujenis/RBD_Timer

RBD::Timer timer;

// input pullup enabled by default
RBD::Button leftButton(35);
RBD::Button middleButton(34);
RBD::Button rightButton(39);



void handleRoot() {
  server.send(200, "text/plain", "hello from TTGO!");
}

//Data for the info-screens
bool hasFirstData = false; // Shouldn't start storing data before the first data has been received

String localMsg;
String lastMsg;

String temp = "00.00";
String lastTemp = "00.00";
std::vector<float> tempData;

String hum = "00.00";
String lastHum = "00.00";
std::vector<float> humData;

//The current info screen
enum screens {MSG, WEATHER, MISC};
int currScreen = screens::WEATHER;

void newCursor(int s, uint16_t c, int x, int y) {
  tft.setTextSize(s);
  tft.setTextColor(c);
  tft.setCursor(x,y);
}

void updateScreen(bool manualChange) {
  if(currScreen == screens::MSG) {
    tft.fillScreen(ST77XX_BLACK);
    newCursor(2, ST7735_GREEN, 0, 0);
    tft.print("MESSAGE\n");
    tft.setTextSize(1);
    tft.setTextColor(0xFFFFFF);
    tft.print(localMsg);
    //Bottom text
    newCursor(1, ST7735_GREEN, 4, 120);
    tft.print(" msg   ");
    tft.setTextColor(0xFFFFFF);
    tft.print("vejr   misc");
  }
  else if(currScreen == screens::WEATHER) {
    tft.fillScreen(ST77XX_BLACK);
    newCursor(2, ST7735_GREEN, 0, 0);
    
    tft.print("Temp ");
    char tempBuff[temp.length()+1];
    temp.toCharArray(tempBuff, temp.length()+1);
    tft.print(tempBuff);
    tft.print("\n");
    
    tft.print("Fugt ");
    char humBuff[hum.length()+1];
    hum.toCharArray(humBuff, hum.length()+1);
    tft.print(humBuff);

    //Print the last measurements as a graph
    if(!tempData.empty()){
      /*
      float _min = *std::min_element(tempData.begin(), tempData.end());
      float _max = *std::max_element(tempData.begin(), tempData.end());
      */
      float _min = tempData[0];
      float _max = tempData[0];
      for(int i = 1; i<tempData.size(); i++)
      {
        if(_min > tempData[i]){
          _min = tempData[i];
        }
        if(_max < tempData[i]){
          _max = tempData[i];
        }
      }
      float diff = _max-_min;
      float scale = 10; //10 pixels per degree
      float minTempDelta = 6.5;
      //A new scale is calculated if the difference is larger than the minTempDelta
      if(diff > minTempDelta){
        scale = minTempDelta*scale/diff;
      }
      int lastY = 0;
      for(int i = 0; i<tempData.size(); i++){
        int x = i;
        //Calculates the position on the temperature interval and then centeres the graph
        int y = 110 - (tempData[i]-_min)*scale - (minTempDelta*10/2-diff*scale/2);
        float l = lastY - y;
        lastY = y;
        l = l<0 ? floor(l) : l;
        if(i == 0 || (int)l == 0){
          tft.drawPixel(x, y, 0xFFFFFF);  
        }
        else{
          tft.drawFastVLine(x, y, l, 0xFFFFFF);
        }
      }
      newCursor(1, ST7735_GREEN, 0, 112);
      tft.print(_min);
      tft.setCursor(0, 35);
      if(scale != 10){
        tft.print(_max);
      }
      else{
        tft.print(_min+minTempDelta);
      }
    }
    //Bottom text
    newCursor(1, 0xFFFFFF, 4, 120);
    tft.print(" msg   ");
    tft.setTextColor(ST7735_GREEN);
    tft.print("vejr   ");
    tft.setTextColor(0xFFFFFF);
    tft.print("misc");
  }
  else if(currScreen == screens::MISC) {
    tft.fillScreen(ST77XX_BLACK);
    newCursor(2, ST7735_GREEN, 0, 0);
    tft.print("MISC INFO\n");
    tft.setTextSize(1);
    tft.print("\nConnected to: ");
    tft.setTextColor(0xFFFFFF);
    tft.print(ssid);
    tft.setTextColor(ST7735_GREEN);
    tft.print("\n\nIP: ");
    tft.setTextColor(0xFFFFFF);
    tft.print(WiFi.localIP());
    tft.setTextColor(ST7735_GREEN);
    tft.print("\n\nUpdate freq: ");
    tft.setTextColor(0xFFFFFF);
    tft.print("0.2 Hz");
    tft.setTextColor(ST7735_GREEN);
    tft.print("\n\nMade by: ");
    tft.setTextColor(0xFFFFFF);
    tft.print("TTL");
    //Bottom text
    newCursor(1, 0xFFFFFF, 4, 120);
    tft.print(" msg   vejr   ");
    tft.setTextColor(ST7735_GREEN);
    tft.print("misc");
  }
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

void handleData() {
  if(server.args() == 0) {
    server.send(200, "text/plain", "You need to include arguments with the data");
    return;
  }
  //Transfer arguments to stored vars
  if(server.arg("temp").length() > 0) {
    lastTemp = temp;
    temp = server.arg("temp");
    Serial.print("temperature: ");
    Serial.println(temp);
    hasFirstData = true;
  }
  if(server.arg("hum").length() > 0) {
    lastHum = hum;
    hum = server.arg("hum");
    Serial.print("humidity: ");
    Serial.println(hum);
    hasFirstData = true;
  }
  if(server.arg("msg").length() > 0) {
    lastMsg = localMsg;
    localMsg = server.arg("msg");
    Serial.print("Received msg: ");
    Serial.println(localMsg);
    hasFirstData = true;
    updateScreen(false);
  }
  server.send(200, "text/plain", temp + String(", ") + hum + String(", ") + localMsg);
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

  //What is happening?
  if (MDNS.begin("esp32screen")) {
    Serial.println("MDNS responder started");
  }

  //Setup different functions for different server-calls
  server.on("/", handleRoot);
  server.on("/send", handleData);
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("HTTP server started");

  MDNS.addService("http", "tcp", 80);
}

void setup(void) {
  //Baud rate for serial output
  Serial.begin(115200);
  //Setup the timer to fire every 5 seconds
  timer.setTimeout(5000);
  timer.restart();

  initServer();

  //Initializing the screen
  Serial.println("Attempting screen init");
  tft.initR(INITR_144GREENTAB); // initialize a ST7735S chip, with green tab
  Serial.println("Initialized tft");

  //Setup screen basics
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(true);
  tft.setTextSize(1);
  tft.setTextColor(ST7735_GREEN);
  
  tft.print(WiFi.localIP());
}
void loop(void) {
  server.handleClient();

  if(timer.onRestart() && hasFirstData) {
    //Serial.println("5 seconds has elapsed");
    tempData.push_back(temp.toFloat());
    humData.push_back(hum.toFloat());

    if(tempData.size() > 120){
      tempData.erase(tempData.begin());
      humData.erase(humData.begin());
    }
    if(currScreen == screens::WEATHER) {
      updateScreen(false);
    }
  }

  if(leftButton.onPressed()){
    currScreen = screens::MSG;
    updateScreen(true);
  }
  else if(middleButton.onPressed()) {
    currScreen = screens::WEATHER;
    updateScreen(true);
  }
  else if(rightButton.onPressed()) {
    currScreen = screens::MISC;
    updateScreen(true);
  }
}

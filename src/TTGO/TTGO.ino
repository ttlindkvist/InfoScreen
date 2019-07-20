// Board type: ESP32 dev mod
//WIFI + Server
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <vector>
#include <algorithm>
#include <math.h>

const char* ssid = "Interwebs";
const char* password = "Internet";

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

//Screen and graph related
enum screens {MSG, WEATHER, MISC};
int currScreen = screens::WEATHER;
enum dataTypes {TEMP1, HUM1, TEMP2, HUM2, EARTH2};
int currGraph = dataTypes::TEMP2;
const String graphNames[5] = {"wTemp", "wHum", "sTemp", "sHum", "sEarth"};

float _data[5] = {0, 0, 0, 0, 0};
float _lastData[5]= {0, 0, 0, 0, 0};
std::vector<float> graphData[5];

String localMsg;
String lastMsg;

void newCursor(int s, uint16_t c, int x, int y) {
  tft.setTextSize(s);
  tft.setTextColor(c);
  tft.setCursor(x,y);
}
void writeText(String t, uint16_t c){
  tft.setTextColor(c);
  tft.print(t);
}

const int graphHeight = 65;
void printGraph(const std::vector<float> &dataPoints, float _min, float _max, uint16_t c){
  int lastY = 0;
  float diff = _max-_min;
  float scale = 10; //10 pixels per degree
  //A new scale is calculated if the difference is larger than the minTempDelta
  if(diff > graphHeight/scale){
    scale = graphHeight/diff;
  }
  for(int i = 0; i<dataPoints.size(); i++){
    int x = 124 - i;
    //Calculates the position on the temperature interval and then centeres the graph
    int y = 110 - (dataPoints[i]-_min)*scale - (graphHeight/2-diff*scale/2);
    float l = lastY - y;
    lastY = y;
    l = l<0 ? floor(l) : l;
    if(i == 0 || (int)l == 0){
      tft.drawPixel(x, y, c);  
    } else {
      tft.drawFastVLine(x, y, l, c);
    }
  }
}

void updateScreen() {
  if(currScreen == screens::MSG) {
    tft.fillScreen(ST77XX_BLACK);
    
    newCursor(2, ST7735_GREEN, 0, 0);
    tft.print("MESSAGE\n");
    tft.setTextSize(1);
    writeText(localMsg, 0xFFFFFF);
    
    //Bottom text
    newCursor(1, ST7735_GREEN, 4, 120);
    tft.print(" msg    ");
    writeText("wthr   misc", 0xFFFFFF);
  }
  else if(currScreen == screens::WEATHER) {
    tft.fillScreen(ST77XX_BLACK);
    
    //Print the last measurements as a graph
    if((currGraph == dataTypes::EARTH2) ? !graphData[currGraph].empty() : (!graphData[currGraph].empty() || !graphData[currGraph-2].empty())){
      float _min = 0;
      float _max = 0;
      //Returns -1 if two graphs should be drawn, else returns the index of the one graph that should be drawn
      int whichGraph = (currGraph == dataTypes::EARTH2) ? currGraph : (!graphData[currGraph].empty() && !graphData[currGraph-2].empty() ? -1 : (!graphData[currGraph].empty() ? currGraph : currGraph-2));
      if(whichGraph != -1){
        newCursor(2, ST7735_GREEN, 0, 0);
        tft.print("Temp ");
        tft.print((!graphData[dataTypes::TEMP2].empty()) ? _data[dataTypes::TEMP2] : _data[dataTypes::TEMP1]);
        tft.print("\n");
        
        tft.print("Hum  ");
        tft.print((!graphData[dataTypes::HUM2].empty()) ? _data[dataTypes::HUM2] : _data[dataTypes::HUM1]);
        
        _min = graphData[whichGraph][0];
        _max = graphData[whichGraph][0];
        for(int i = 1; i<graphData[whichGraph].size(); i++)
        {
          if(_min > graphData[whichGraph][i]){
            _min = graphData[whichGraph][i];
          }
          if(_max < graphData[whichGraph][i]){
            _max = graphData[whichGraph][i];
          }
        }
        printGraph(graphData[whichGraph], _min, _max, 0xFFFFFF);
        
      } else { /*DRAWING THE DOUBLE GRAPH*/
        //Print current temperatures and humidities
        newCursor(2, ST7735_GREEN, 0, 0);
        tft.print("Temp\nHum");
        newCursor(1, ST7735_GREEN, 64, 0);
        tft.print(_data[dataTypes::TEMP1]);
        newCursor(1, 0xFFFFFF, 94, 8);
        tft.print(_data[dataTypes::TEMP2]);
        newCursor(1, ST7735_GREEN, 64, 16);
        tft.print(_data[dataTypes::HUM1]);
        newCursor(1, 0xFFFFFF, 94, 24);
        tft.print(_data[dataTypes::HUM2]);

        //Find the minimum and maximum value of the two datasets by iterating through and comparing
        _min = graphData[currGraph][0];
        _max = graphData[currGraph][0];
        for(int i = 1; i<graphData[currGraph].size(); i++)
        {
          if(_min > graphData[currGraph][i]){
            _min = graphData[currGraph][i];
          }
          if(_max < graphData[currGraph][i]){
            _max = graphData[currGraph][i];
          }
        }
        for(int i = 0; i<graphData[currGraph-2].size(); i++)
        {
          if(_min > graphData[currGraph-2][i]){
            _min = graphData[currGraph-2][i];
          }
          if(_max < graphData[currGraph-2][i]){
            _max = graphData[currGraph-2][i];
          }
        }
        printGraph(graphData[currGraph], _min, _max, 0xFFFFFF);
        printGraph(graphData[currGraph-2], _min, _max, ST7735_GREEN);
      }
      
      newCursor(1, 0xFFFFFF, 0, 35);
      //If only graph is drawn, print text for one, else for two
      if(whichGraph != -1){
        writeText(String(graphNames[whichGraph]), 0xFFFFFF);
      } else {
        writeText(String(graphNames[currGraph-2][0]) + String(graphNames[currGraph-2][1]) + " ", ST7735_GREEN);
        writeText(String(graphNames[currGraph][0]) + String(graphNames[currGraph][1]), 0xFFFFFF);
      }
      //Print the found minimum and maximum value - and if the current graph is the soil humidity, throw away the decimals
      tft.setCursor(64, 35);
      writeText("max ", 0xFFFFFF);
      writeText((currGraph == dataTypes::EARTH2) ? String((int)_max) : String(_max), ST7735_GREEN);
      tft.setCursor(64, 112);
      writeText("min ", 0xFFFFFF);
      writeText((currGraph == dataTypes::EARTH2) ? String((int)_min) : String(_min), ST7735_GREEN);
    } else {
      newCursor(1, 0xFFFFFF, 0, 35);
      writeText(String(graphNames[currGraph]), ST7735_GREEN);
      tft.setCursor(64, 35);
      writeText("max NaN", 0xFFFFFF);
      tft.setCursor(64, 112);
      tft.print("min NaN");
    }
    //Bottom text
    newCursor(1, 0xFFFFFF, 4, 120);
    tft.print(" msg    ");
    writeText("wthr   ", ST7735_GREEN);
    writeText("misc", 0xFFFFFF);
  }
  else if(currScreen == screens::MISC) {
    tft.fillScreen(ST77XX_BLACK);
    
    newCursor(2, ST7735_GREEN, 0, 0);
    tft.print("MISC INFO\n");
    tft.setTextSize(1);
    tft.print("\nSSID: ");
    writeText(String(ssid), 0xFFFFFF);
    writeText("\n\nIP: ", ST7735_GREEN);
    tft.setTextColor(0xFFFFFF);
    tft.print(WiFi.localIP());
    writeText("\n\nUpdate freq: ", ST7735_GREEN);
    writeText("0.2 Hz", 0xFFFFFF);
    writeText("\n\nMade by: ", ST7735_GREEN);
    writeText("TTL", 0xFFFFFF);

    //Bottom text
    newCursor(1, 0xFFFFFF, 4, 120);
    tft.print(" msg    wthr   ");
    writeText("misc", ST7735_GREEN);
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
  for(int i = 0; i<5; i++){
    if(server.arg(graphNames[i]).length() > 0) {
      _lastData[i] = _data[dataTypes::TEMP1];
      _data[i] = server.arg(graphNames[i]).toFloat();
    }
  }
  if(server.arg("msg").length() > 0) {
    lastMsg = localMsg;
    localMsg = server.arg("msg");
    Serial.print("Received msg: ");
    Serial.println(localMsg);
    if(currScreen == screens::MSG){
      updateScreen();
    }
  }
  server.send(200, "text/plain", _data[dataTypes::TEMP1] + String(", ") + _data[dataTypes::HUM1] + String(", ") + localMsg);
}

void handleRoot() {
  //Very basic input field and button where a message can be sent to the device
  String site = "<form action=\"/send\">";
  site += "Enter a message:<br>";
  site += "<input type=\"text\" name=\"msg\" value=\"Write here\"><br>";
  site += "<input type=\"submit\" value=\"Submit\">";
  site += "</form>";
  server.send(200, "text/html", site);
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

  //Setup different functions for different server-calls
  server.on("/", handleRoot);
  server.on("/send", handleData);
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("HTTP server started");
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
  
  //Every 5 seconds the timer restarts
  if(timer.onRestart()) {
    //Save current data to the datasets
    for(int i = 0; i<5; i++){
      if(!graphData[i].empty() || _data[i] != 0){
        graphData[i].insert(graphData[i].begin(), _data[i]);  
      }
      if(graphData[i].size() > 120){
        graphData[i].erase(graphData[i].end() - 1);
      }
    }
    if(currScreen == screens::WEATHER) {
      updateScreen();
    }
  }

  if(leftButton.onPressed()){
    currScreen = screens::MSG;
    updateScreen();
  } 
  else if(middleButton.onPressed()) {
    if(currScreen == screens::WEATHER){
      currGraph = (currGraph == dataTypes::EARTH2 ? dataTypes::TEMP2 : currGraph+1);
      updateScreen();
    } else {
      currScreen = screens::WEATHER;
      updateScreen();
    }
  }
  else if(rightButton.onPressed()) {
    currScreen = screens::MISC;
    updateScreen();
  }
}

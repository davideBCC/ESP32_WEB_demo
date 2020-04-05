#include <Arduino.h>

#include <WiFi.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>

#include "ConfigManagement.h"
#include "mic_ws_streaming.h"

////////////////////////////////////////
// Global variables
////////////////////////////////////////

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 4, /* data=*/ 5, /* reset=*/ U8X8_PIN_NONE);   // ESP32 Thing, pure SW emulated I2C

AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
static tsWiFiConfig gsWiFiConfig;
static bool gbNewWiFiConfig = false;
static const char *pConfigFilename = "/SysConfig.json";  // File where the configuration is stored

////////////////////////////////////////
// Local Funcitons
////////////////////////////////////////

//get the new WiFi configuration via http
void onWiFiConfig (AsyncWebServerRequest *request);
// If WiFi station cannot connetct, then 
// start AP to set WiFi configuration
void launchConfigViaAP();
// Callback: send homepage
void onIndexRequest(AsyncWebServerRequest *request);
// Callback: on button click
void onButtonClick(AsyncWebServerRequest *request);
// Callback: receiving any WebSocket message
void onWebSocketEvent(uint8_t client_num, WStype_t type, uint8_t * payload, size_t length);

void setup() {
  Serial.begin(115200);

  u8g2.begin();
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_6x10_tf);
  int ChHeightOff = u8g2.getAscent() - u8g2.getDescent() + 1;
  int YCursor = u8g2.getAscent();
  u8g2.setCursor(0,YCursor);
  

  // Make sure we can read the file system
  if( !SPIFFS.begin())
  { //try to mount the SPIFFS
    Serial.println(F("Error mounting SPIFFS"));
    u8g2.print(F("SPIFFS error"));
    u8g2.sendBuffer();
    delay(5000);
    esp_restart();
  }
  Serial.println(F("SPIFFS Mounted"));

  //Load configuration from file
  loadConfiguration(pConfigFilename, gsWiFiConfig);
  printConfiguration(gsWiFiConfig);

  if(0)//Debug
  {
      //require a new WiFi configuration and restart
      launchConfigViaAP();

  }


  if (MDNS.begin(gsWiFiConfig.smDNSname.c_str()))
  {
    Serial.println(F("mDNS responder started"));
    Serial.print(".local host name: "); 
    Serial.println(gsWiFiConfig.smDNSname);

    MDNS.addService("http", "tcp", 80);
  }
  else
  {
    Serial.println(F("Failed to lauch mDNS responder"));
  }
  
  // u8g2.print(String((uint32_t)ESP.getEfuseMac(),HEX));
  // u8g2.sendBuffer();
  // u8g2.setCursor(0,(YCursor += ChHeightOff));

  Serial.println(F("Connecting to WiFi"));
  u8g2.print(F("Connecting to WiFi"));
  u8g2.sendBuffer();
  u8g2.setCursor(0,(YCursor += ChHeightOff));

  WiFi.mode(WIFI_STA);
  WiFi.begin(gsWiFiConfig.sSSID.c_str(), gsWiFiConfig.sPassword.c_str());
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    WiFi.disconnect();
    delay(500);
    WiFi.mode(WIFI_STA); //Try to connect a second time
    WiFi.begin(gsWiFiConfig.sSSID.c_str(), gsWiFiConfig.sPassword.c_str());
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println(F("WiFi connection Failed!"));
      u8g2.print(F("WiFi Failed!"));
      u8g2.setCursor(0,(YCursor += ChHeightOff));
      u8g2.print(F("AP mode"));
      u8g2.sendBuffer();
      //require a new WiFi configuration and restart
      launchConfigViaAP();
    }
  }

  Serial.print(F("IP Address: "));
  Serial.println(WiFi.localIP());
  u8g2.print(F("IP Address: "));
  u8g2.setCursor(0,(YCursor += ChHeightOff));
  u8g2.print(WiFi.localIP());
  u8g2.sendBuffer();
  u8g2.setCursor(0,(YCursor += ChHeightOff));

  
  server.serveStatic("/",SPIFFS,"/");
  server.on("/button_press", HTTP_POST, onButtonClick);
  server.on("/sensor_value", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(random(256)).c_str());
  });
  server.on("/api/wifi_config", HTTP_POST, onWiFiConfig);

  
  // Handle requests for pages that do not exist
  server.onNotFound([] (AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  server.begin();

  // Start WebSocket server and assign callback
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  Serial.println(F("WEB Server started."));
  u8g2.print(F("WEB Server started."));
  u8g2.sendBuffer();
  u8g2.setCursor(0,(YCursor += ChHeightOff));

  micStreamingInit();
}

////////////////////////////////////
// Loop function
////////////////////////////////////
void loop() {
  const unsigned cycleTime = 10;
  static unsigned loopCount = 0;
  static struct sMsgStayAlive {
      uint8_t id;
      uint8_t cnt;
    }msgStayAlive = {0,0};

  if(gbNewWiFiConfig) //New conviguration received by the server?
  {
    gbNewWiFiConfig = false;
    saveConfiguration(pConfigFilename,gsWiFiConfig);
    printConfiguration(gsWiFiConfig);
    Serial.println(F("New WiFi parameters received and Stored"));
    Serial.println(F("Restarting in 3 seconds"));
    delay(3000);
    ESP.restart();
  }
  // Look for and handle WebSocket data
  webSocket.loop();

  // send stay alive message to websocket
   

  if (loopCount*cycleTime%1000 == 0) //execute this code section approx every 1 second
  {
    webSocket.broadcastBIN((uint8_t*)&msgStayAlive,sizeof(msgStayAlive));
    msgStayAlive.cnt++;
    // for (unsigned i = 0; i < webSocket.connectedClients(); i++)
    // {
    //   webSocket.sendBIN(i,(uint8_t*)&msgStayAlive,sizeof(msgStayAlive));
    // }
    // msgStayAlive.cnt++;
  }

  loopCount++; 
  delay(cycleTime);
}




//get the new WiFi configuration via http
void onWiFiConfig (AsyncWebServerRequest *request) 
{

  int nParams = request->params();
  //Extract the parameters
  for(int i=0; i<nParams; i++)
  {
    AsyncWebParameter *pParameter = request->getParam(i);
    if(String("fSSID").equalsIgnoreCase(pParameter->name()))
      gsWiFiConfig.sSSID = pParameter->value();
    else if (String("fPassword").equalsIgnoreCase(pParameter->name()))
      gsWiFiConfig.sPassword = pParameter->value();
    else if (String("fmDNSname").equalsIgnoreCase(pParameter->name()))
      gsWiFiConfig.smDNSname = pParameter->value();
  }  
  
  gbNewWiFiConfig = true;
  request->send(200, "text/plain", F("Parameters received, restarting server.\n reload manually in 10 seconds"));
}

void launchConfigViaAP()
{
  //Configure AP mode
  WiFi.mode(WIFI_AP_STA);
  
  IPAddress myIP(192,168,4,1);
  IPAddress subNetIP(255,255,255,0);
  
  WiFi.softAP("ESP_Config");
  //WiFi.begin();
  delay(100); //wait 100ms while access point is started
  WiFi.config(myIP,myIP,subNetIP);  //Define a fix AP IP

  Serial.println(F("Starting AP mode for WiFi configuration"));

  AsyncWebServer serverAP(80);
  serverAP.serveStatic("/",SPIFFS,"/").setDefaultFile("WiFi_config.html");
  serverAP.on("/api/wifi_config", HTTP_POST, onWiFiConfig);

  // Handle requests for pages that do not exist
  serverAP.onNotFound([] (AsyncWebServerRequest *request) {
    request->send(404, "See Other", "404 Not found");
  });
  
  //WiFi.disconnect();
  int numberOfNetworks = WiFi.scanNetworks();
  for(int i =0; i<numberOfNetworks; i++){
 
      Serial.print("Network name: ");
      Serial.println(WiFi.SSID(i));
      Serial.print("Signal strength: ");
      Serial.println(WiFi.RSSI(i));
      Serial.println("-----------------------");
 
  }
  serverAP.begin();
  
  Serial.println(F("Started web server"));
  //Wait for the new configuration from the client
  while(1)
  {
    delay(100);
    if(gbNewWiFiConfig) //New conviguration received by the server?
    {
      gbNewWiFiConfig = false;
      break;
    }
  }
  saveConfiguration(pConfigFilename,gsWiFiConfig);
  printConfiguration(gsWiFiConfig);
  Serial.println(F("New WiFi parameters saved"));
  Serial.println(F("Restarting in 3 seconds"));
  delay(3000);
  ESP.restart();
}

// Callback: send homepage
void onIndexRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/index.htm", "text/html");
}

// Callback: on button click
void onButtonClick(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  String response;
  response.reserve(256);  //reserve enought dynamic buffer
  response =  String("Received HTTP POST request \"") + request->url() 
    + "\" from [" + remote_ip.toString() +"]"+"\n";
  //List all parameters
  int nParams = request->params();
  for(int i=0; i<nParams; i++){
    AsyncWebParameter* p = request->getParam(i);
    if(p->isFile())
    { 
      response += String("FILE[") + p->name().c_str()  + "]: "+ p->value().c_str() +", size: "+ p->size() + "\n";
    } 
    else if(p->isPost())
    {
      response += String("POST[")+ p->name().c_str() + "]: " +  p->value().c_str() + "\n" ;
    } 
    else 
    {
      response += String("GET[")+ p->name().c_str() + "]: " +   p->value().c_str() + "\n" ;
    }
  }
  //request->send(204);
  request->send(200, "text/plain", response);
  Serial.print(response);
}


// Callback: receiving any WebSocket message
void onWebSocketEvent(uint8_t client_num,
                      WStype_t type,
                      uint8_t * payload,
                      size_t length) {
  //String for ws reply message
  String out_string;

  // Figure out the type of WebSocket event
  switch(type) {
 
    // Client has disconnected
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", client_num);
      break;
 
    // New client has connected
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(client_num);
        Serial.printf("[%u] Connection from ", client_num);
        Serial.println(ip.toString());
      }
      break;
 
    // Handle text messages from client
    case WStype_TEXT:
      
      // Print out raw message
      Serial.printf("[%u] Received text: %s\n", client_num, payload);
 
      // // Toggle LED
      // if ( strcmp((char *)payload, "toggleLED") == 0 ) {
      //   led_state = led_state ? 0 : 1;
      //   Serial.printf("Toggling LED to %u\n", led_state);
      //   digitalWrite(led_pin, led_state);
 
      // // Report the state of the LED
      // } else if ( strcmp((char *)payload, "getLEDState") == 0 ) {
      //   sprintf(msg_buf, "%d", led_state);
      //   Serial.printf("Sending to [%u]: %s\n", client_num, msg_buf);
      //   webSocket.sendTXT(client_num, msg_buf);
 
      // // Message not recognized
      // } else {
      //   Serial.println("[%u] Message not recognized");
      // }


      // char array[1024];
      // for (int i = 0; i < 1023; i++)
      //   array[i] = 'a';
      // array[1023] = '\0';
      // webSocket.sendTXT(client_num,array);

      //Prepare return string
      out_string += "echo: \"";
      out_string += String((char *)payload);
      out_string += "\"\n\nThanks for the message";

      webSocket.sendTXT(client_num, out_string.c_str());
      break;

    // For everything else: do nothing
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;
  }
}
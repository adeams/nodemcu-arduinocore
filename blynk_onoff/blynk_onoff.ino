#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#define BLYNK_PRINT Serial        // Comment this out to disable prints and save space
#include "FS.h"
#include <ArduinoJson.h>
#include <Ticker.h>               //for LED status
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <BlynkSimpleEsp8266.h>
#include <EEPROM.h>

// define button set token
#define SW_PIN 0

//button Hardware
//const int btnPin0 = D0;
const int btnPin1 = D5;
const int btnPin2 = D6;
const int btnPin3 = D7;
const int btnPin4 = D9;

//button Virtual
const int AppBtn1 = D1;
const int AppBtn2 = D2;
const int AppBtn3 = D4;
const int AppBtn4 = D8;

//status output hardware
bool ledState0 = false;
bool ledState1 = false;
bool ledState2 = false;
bool ledState3 = false;
bool ledState4 = false;

//status botton config
int ledState = false;
int btnStateConfig = HIGH;

//satatus 
int btnState0 = LOW;
int btnState1 = LOW;
int btnState2 = LOW;
int btnState3 = LOW;
int btnState4 = LOW;
int btnState5 = LOW;



char mqtt_server[40];
char mqtt_port[6] = "";
char auth[] = "34c0acad7fee4dc2b2d37d555e110eb9";
char blynk_token_f[34] = "";
char blynk_token_s[34] = "";
int count = 0;
int count1 = 0;
int addr = 0;
File f;
Ticker ticker;
BlynkTimer timer;
//void checkPhysicalButton();
const char *filename = "/config.json";  // <- SD library uses 8.3 filenames

// Every time we connect to the cloud...
BLYNK_CONNECTED() {
  // Request the latest state from the server
  Blynk.syncVirtual(V0);
  Blynk.syncVirtual(V1);
  Blynk.syncVirtual(V2);
  Blynk.syncVirtual(V3);
  Blynk.syncVirtual(V4);
  // Alternatively, you could override server state using:
  //Blynk.virtualWrite(V2, ledState);
  Serial.println("BLYNK_CONNECTED");
}

// When App button is pushed - switch the state
BLYNK_WRITE(V1) {
  ledState1 = param.asInt();
  digitalWrite(AppBtn1, ledState1);
  EEPROM.write(addr+1, ledState1);
  EEPROM.commit();
  Serial.println("BLYNK_WRITEV1");
}

// When App button is pushed - switch the state
BLYNK_WRITE(V2) {
  ledState2 = param.asInt();
  digitalWrite(AppBtn2, ledState2);
  EEPROM.write(addr+2, ledState2);
  EEPROM.commit();
  Serial.println("BLYNK_WRITEV2");
}

// When App button is pushed - switch the state
BLYNK_WRITE(V3) {
  ledState3 = param.asInt();
  digitalWrite(AppBtn3, ledState3);
  EEPROM.write(addr+3, ledState3);
  EEPROM.commit();
  Serial.println("BLYNK_WRITEV3");
}

// When App button is pushed - switch the state
BLYNK_WRITE(V4) {
  ledState4 = param.asInt();
  digitalWrite(AppBtn4, ledState4);
  EEPROM.write(addr+4, ledState4);
  EEPROM.commit();
  Serial.println("BLYNK_WRITEV4");
}
void scankey(){
  checkPhysicalButton(&btnPin1,&btnState1,&ledState1,1);
  checkPhysicalButton(&btnPin2,&btnState2,&ledState2,2);
  checkPhysicalButton(&btnPin3,&btnState3,&ledState3,3);
  checkPhysicalButton(&btnPin4,&btnState4,&ledState4,4);
}
void checkPhysicalButton(const int *btnPin,int *btnState,bool *ledState,int vo){
  if (digitalRead(*btnPin) == LOW) {
    // btnState is used to avoid sequential toggles
    if (*btnState != LOW) {

      // Toggle LED state
      *ledState = !(*ledState);
      //digitalWrite(*btnPin, *ledState);
      // Update Button Widget
      Serial.printf("press button VO%d\n\r",vo);
      switch(vo){
        case 1:
           Blynk.virtualWrite(V1, *ledState);
           digitalWrite(AppBtn1, *ledState);
        break;
        case 2:
           Blynk.virtualWrite(V2, *ledState);
           digitalWrite(AppBtn2, *ledState);
        break;
        case 3:
           Blynk.virtualWrite(V3, *ledState);
           digitalWrite(AppBtn3, *ledState);
        break;
        case 4:
           Blynk.virtualWrite(V4, *ledState);
           digitalWrite(AppBtn4, *ledState);
        break;
        }
    }
    *btnState = LOW;
    //Serial.println(vo);
  } else {
    //Serial.println("HIGH");
    *btnState = HIGH;
  }
}

int countIsConfig = 5; 
void checkButtonIsConfig(){
  if (digitalRead(SW_PIN) == LOW) {
    // btnState is used to avoid sequential toggles
    if (btnStateConfig != LOW) {
      Serial.printf("Reset wifi config?:\n\r");
    }
    Serial.print(String(countIsConfig)+" "); 
    if((--countIsConfig) <= 0){
      setWifiManager(); 
      ticker.detach(); 
    }
    btnStateConfig = LOW;
  } else {
    if (btnStateConfig != HIGH) {
      Serial.printf("\n\rexit wifi config\n\r");
    }
    btnStateConfig = HIGH;
    countIsConfig = 5;
  }
}

void tick(){
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

void tick1(){
  //toggle state
  if(++count > 5){
    count = 0;
    int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
    digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
    delay(10);
    digitalWrite(BUILTIN_LED, state);     // set pin to the opposite state
  }
}

//void buttonLedWidget(const int *btnPin,boolean *btnState,int Vo){
//  // Read button
//  boolean isPressed = (digitalRead(*btnPin) == LOW);
//
//  // If state has changed...
//  if (isPressed != *btnState) {
//    if (isPressed) {
//      switch(Vo){
//        case 0:
//          led0.on();
//          EEPROM.write(addr+0, 0);
//          EEPROM.commit();
//        break;
//        case 1:
//          led1.on();
//          EEPROM.write(addr+1, 0);
//          EEPROM.commit();
//        break;
//        case 2:
//          led2.on();
//          EEPROM.write(addr+2, 0);
//          EEPROM.commit();
//        break;
//        case 3:
//          led3.on();
//          EEPROM.write(addr+3, 0);
//          EEPROM.commit();
//        break;
//        case 4:
//          led4.on();
//          EEPROM.write(addr+4, 0);
//          EEPROM.commit();
//        break;
//        case 5:
//          led5.on();
//          EEPROM.write(addr+5, 0);
//          EEPROM.commit();
//        break;
//        case 6:
//          led6.on();
//          EEPROM.write(addr+6, 0);
//          EEPROM.commit();
//        break;
//        case 7:
//          led7.on();
//          EEPROM.write(addr+7, 0);
//          EEPROM.commit();
//        break;
//        case 8:
//          led8.on();
//          EEPROM.write(addr+8, 0);
//          EEPROM.commit();
//        break;
//        case 9:
//          led9.on();
//          EEPROM.write(addr+9, 0);
//          EEPROM.commit();
//        break;
//        case 10:
//          led10.on();
//          EEPROM.write(addr+10, 0);
//          EEPROM.commit();
//        break;
//      }
//    } else {
//       switch(Vo){
//        case 0:
//          led0.off();
//          EEPROM.write(addr+0, 1);
//          EEPROM.commit();
//        break;
//        case 1:
//          led1.off();
//          EEPROM.write(addr+1, 1);
//          EEPROM.commit();
//        break;
//        case 2:
//          led2.off();
//          EEPROM.write(addr+2, 1);
//          EEPROM.commit();
//        break;
//        case 3:
//          led3.off();
//          EEPROM.write(addr+3, 1);
//          EEPROM.commit();
//        break;
//        case 4:
//          led4.off();
//          EEPROM.write(addr+4, 1);
//          EEPROM.commit();
//        break;
//        case 5:
//          led5.off();
//          EEPROM.write(addr+5, 1);
//          EEPROM.commit();
//        break;
//        case 6:
//          led6.off();
//          EEPROM.write(addr+6, 1);
//          EEPROM.commit();
//        break;
//        case 7:
//          led7.off();
//          EEPROM.write(addr+7, 1);
//          EEPROM.commit();
//        break;
//        case 8:
//          led8.off();
//          EEPROM.write(addr+8, 1);
//          EEPROM.commit();
//        break;
//        case 9:
//          led9.off();
//          EEPROM.write(addr+9, 1);
//          EEPROM.commit();
//        break;
//        case 10:
//          led10.off();
//          EEPROM.write(addr+10, 1);
//          EEPROM.commit();
//        break;
//      }
//    }
//    *btnState = isPressed;
//  }
//}

bool loadConfig(const char *filename){
//  // always use this to "mount" the filesystem
//    bool result = SPIFFS.begin();
//    Serial.println("SPIFFS opened: " + result);
    Serial.printf("loadConfig file name ->%s<\n\r",filename);
    // this opens the file "f.txt" in read-mode
     //f = SPIFFS.open("/config5.json", "r");
     f = SPIFFS.open(filename, "r");
    
    if (!f) {
      Serial.println("File doesn't exist yet. Creating it");
  
      // open the file in write mode
      //f = SPIFFS.open("/config5.json", "w");
      f = SPIFFS.open(filename, "w");
      if (!f) {
        Serial.println("file creation failed");
      }
      // Allocate the JSON document
      //
      // Inside the brackets, 200 is the RAM allocated to this document.
      // Don't forget to change this value to match your requirement.
      // Use arduinojson.org/assistant to compute the capacity.
      StaticJsonDocument<200> doc;
    
      // StaticJsonObject allocates memory on the stack, it can be
      // replaced by DynamicJsonDocument which allocates in the heap.
      //
      // DynamicJsonDocument  doc(200);
    
      // Make our document be an object
      JsonObject root = doc.to<JsonObject>();
    
      // Add values in the object
      //
      // Most of the time, you can rely on the implicit casts.
      // In other case, you can do root.set<long>("time", 1351824120);
      root["blynk_token"] = auth;
      root["mqtt_server"] = "";
      root["mqtt_port"] = "";
      // now write two lines in key/value style with  end-of-line characters
      
      // Serialize JSON to file
      if (serializeJson(doc, f) == 0) {
        Serial.println(F("Failed to write to file"));
      }
    }else {
      // we could open the file
      while(f.available()) {
        //Lets read line by line from the file
        String line = f.readStringUntil('\n');
        Serial.println(line);
        Serial.println("");
        
        // Allocate the JSON document
        //
        // Inside the brackets, 200 is the RAM allocated to this document.
        // Don't forget to change this value to match your requirement.
        // Use arduinojson.org/assistant to compute the capacity.
        StaticJsonDocument<200> doc;
        
        // Deserialize the JSON document
        DeserializationError error = deserializeJson(doc, line);
        
        // Test if parsing succeeds.
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.c_str());
          return false;
        }
        
        // Get the root object in the document
        JsonObject root = doc.as<JsonObject>();
        //const char* blynk_token_f = (const char*)root["blynk_token"];   //can not use
        //const char* blynk_token_f = root["blynk_token"];                //can not use
        //const char* blynk_token_f = root["blynk_token"];                //can not use
        strcpy(blynk_token_f, root["blynk_token"]);
        strcpy(mqtt_server, root["mqtt_server"]);
        strcpy(mqtt_port, root["mqtt_port"]);
        
        Serial.printf("blynk_token_f->%s<\n\r",blynk_token_f);
        Serial.printf("mqtt_server_f->%s<\n\r",mqtt_server);
        Serial.printf("mqtt_port_f->%s<\n\r",mqtt_port);
          
        if (!mqtt_server[0]){  //blynk_token_s == "BLYNK_TOKEN" use auth from file 
          Blynk.config(blynk_token_f);
        }else{
          char *p = mqtt_server;
          char *str;
          char *ip[5];
          int i = 0;
          while ((str = strtok_r(p, ".", &p)) != NULL){ // delimiter is the semicolon
            //ip[i] = atoi(str);
            ip[i] = str;
            Serial.printf("str->%s<ip=%s\n\r",str,ip[i]);
            i++;
          }
          if(i>2){
            Blynk.config(blynk_token_f,IPAddress(atoi(ip[0]),atoi(ip[1]),atoi(ip[2]),atoi(ip[3])),atoi(&mqtt_port[0]));   
          }else{
            strcpy(mqtt_server, root["mqtt_server"]);
            Blynk.config(blynk_token_f,mqtt_server,atoi(&mqtt_port[0]));   
          }                                     
        }
      }
    }
    f.close();
    return true;
}

bool saveConfig(const char *filename,const char* blynk_token_in,const char* mqtt_server_in,const char* mqtt_port_in){
  Serial.printf("saveConfig file name ->%s<\n\r",filename);
  //f = SPIFFS.open("/config5.json", "w");
  f = SPIFFS.open(filename, "w");
      if (!f) {
        Serial.println("file creation failed");
        return false;
      }  

      StaticJsonDocument<200> doc;
      JsonObject root = doc.to<JsonObject>();

      root["blynk_token"] = blynk_token_in;
      root["mqtt_server"] = mqtt_server_in;
      root["mqtt_port"] = mqtt_port_in;
      // now write two lines in key/value style with  end-of-line characters
      
      // Serialize JSON to file
      if (serializeJson(doc, f) == 0) {
        Serial.println(F("Failed to write to file"));
      }
      f.close();
      return true;
}

bool setWifiManager(){
  ticker.attach(0.2, tick);  // led toggle faster
  Serial.printf("<<<- setWifiManager Start ->>>\n\r");
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
 
  //reset saved settings
  if(digitalRead(SW_PIN) == LOW) // Press button
  {
    Serial.println();
    Serial.println("Reset wifi config");
    wifiManager.resetSettings(); 
  }    
  // ip 192.168.4.1
  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  WiFiManagerParameter custom_blynk_token("Blynk", "blynk token", blynk_token_s, 34);
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
  wifiManager.addParameter(&custom_blynk_token);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);

  wifiManager.autoConnect("AutoConnectAP");
  //or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();

  
  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  const char* blynk_token1 = custom_blynk_token.getValue();
  strcpy(blynk_token_s, blynk_token1);

    //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  const char* mqtt_port1 = custom_mqtt_port.getValue();
  strcpy(mqtt_port, mqtt_port1);

  Serial.printf("mqtt_server->%s<\n\r",mqtt_server);
  Serial.printf("mqtt_port->%s<\n\r",mqtt_port);
  Serial.printf("blynk_token_s->%s<\n\r",blynk_token_s);

  //if (strcmp(blynk_token_s, "BLYNK_TOKEN")){  //blynk_token_s == "BLYNK_TOKEN" use auth from file 
  if (strcmp(blynk_token_s, "")){  //blynk_token_s == "BLYNK_TOKEN" use auth from file 
  //if (blynk_token_s == ""){  //blynk_token_s == "BLYNK_TOKEN" use auth from file 
    Serial.println("blynk_token is set");
    saveConfig(filename,blynk_token_s,mqtt_server,mqtt_port);
    //Blynk.config(blynk_token_s);
     if (!mqtt_server[0]){  //blynk_token_s == "BLYNK_TOKEN" use auth from file 
        Blynk.config(blynk_token_s);
     }else{
      //IP Blynk Server = 188.166.206.43
      char *p = mqtt_server;
      char *str;
      char *ip[5];
      int i = 0;
      while ((str = strtok_r(p, ".", &p)) != NULL){ // delimiter is the semicolon
        //ip[i] = atoi(str);
        ip[i] = str;
        Serial.printf("str->%s<ip=%s\n\r",str,ip[i]);
        i++;
      }
      if(i>2){
        Blynk.config(blynk_token_s,IPAddress(atoi(ip[0]),atoi(ip[1]),atoi(ip[2]),atoi(ip[3])),atoi(&mqtt_port[0]));   
      }else{
        strcpy(mqtt_server, custom_mqtt_server.getValue());
        Serial.printf("mqtt_server_domain=%s\n\r",mqtt_server);
        Blynk.config(blynk_token_s,mqtt_server,atoi(&mqtt_port[0]));   
      }                                       
   }
  }else{
    Serial.println("blynk_token in file");  
    //Blynk.config(auth);
    loadConfig(filename);
  }
  ticker.detach(); 
  return true;
  
}


void io_refress(){
    ledState1 = EEPROM.read(addr+1);
    ledState2 = EEPROM.read(addr+2);
    ledState3 = EEPROM.read(addr+3);
    ledState4 = EEPROM.read(addr+4);
     
    digitalWrite(AppBtn1,ledState1);
    digitalWrite(AppBtn2,ledState2);
    digitalWrite(AppBtn3,ledState3);
    digitalWrite(AppBtn4,ledState4);
    //digitalWrite(D10,EEPROM.read(addr+10));
//    Serial.printf("EEPROM.read(addr+0)>%d\n\r",EEPROM.read(addr+0));
//    Serial.printf("EEPROM.read(addr+1)>%d\n\r",EEPROM.read(addr+1));
//    Serial.printf("EEPROM.read(addr+2)>%d\n\r",EEPROM.read(addr+2));
//    Serial.printf("EEPROM.read(addr+4)>%d\n\r",EEPROM.read(addr+4));  
}

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    while (!Serial) continue;
    Serial.println("");
    pinMode(SW_PIN, INPUT_PULLUP); 
    pinMode(btnPin1, INPUT_PULLUP);
    pinMode(btnPin2, INPUT_PULLUP);
    pinMode(btnPin3, INPUT_PULLUP);
    pinMode(btnPin4, INPUT_PULLUP);
    pinMode(AppBtn1, OUTPUT);
    pinMode(AppBtn2, OUTPUT);
    pinMode(AppBtn3, OUTPUT);
    pinMode(AppBtn4, OUTPUT);
    pinMode(BUILTIN_LED, OUTPUT);
    EEPROM.begin(512);
    io_refress();
    // always use this to "mount" the filesystem
    bool result = SPIFFS.begin();
    Serial.println("SPIFFS opened: " + result);
    setWifiManager();
    ticker.detach(); 
    io_refress();
  
    // Setup a function to be called every 100 ms
    //timer.setInterval(100L, checkPhysicalButton);
    timer.setInterval(1000L, checkButtonIsConfig);
    timer.setInterval(1000L, tick1);
    timer.setInterval(100L, scankey);
}

void loop() {
    // put your main code here, to run repeatedly:
    Blynk.run();
    timer.run();
}

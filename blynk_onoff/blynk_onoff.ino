#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include "FS.h"
#include <ArduinoJson.h>
//for LED status
#include <Ticker.h>
Ticker ticker;
//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <BlynkSimpleEsp8266.h>
char blynk_token_f[34] = "BLYNK_TOKEN";
char auth[] = "34c0acad7fee4dc2b2d37d555e110eb9";
char blynk_token_s[] = "BLYNK_TOKEN";
File f;
#define SW_PIN 0

void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

bool loadConfig(){
//  // always use this to "mount" the filesystem
//    bool result = SPIFFS.begin();
//    Serial.println("SPIFFS opened: " + result);
  
    // this opens the file "f.txt" in read-mode
     f = SPIFFS.open("/config5.json", "r");
    
    if (!f) {
      Serial.println("File doesn't exist yet. Creating it");
  
      // open the file in write mode
      f = SPIFFS.open("/config5.json", "w");
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
//      root["SSID"] = "PP-RD-FL4";
//      root["password"] = "ppetech1";
      root["blynk_token"] = auth;
      // now write two lines in key/value style with  end-of-line characters
      
      // Serialize JSON to file
      if (serializeJson(doc, f) == 0) {
        Serial.println(F("Failed to write to file"));
      }
      //f.println("ssid=abc");
      //f.println("password=123455secret");
    } else {
      // we could open the file
      while(f.available()) {
        //Lets read line by line from the file
        String line = f.readStringUntil('\n');
        Serial.println(line);

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
        //const char* blynk_token_f = (const char*)root["blynk_token"];
        //const char* blynk_token_f = root["blynk_token"];
        //const char* blynk_token_f = root["blynk_token"];
        strcpy(blynk_token_f, root["blynk_token"]);
        //Serial.println(blynk_token_f);
        Serial.printf("blynk_token->%s,%d<\n\r",blynk_token_f,blynk_token_f[32]);
        Serial.printf("auth->%s,%d<\n\r",auth,auth[32]);
        Blynk.config(blynk_token_f);
        //Blynk.config(auth);
        Serial.println(blynk_token_f);
        
      }
  
    }
    f.close();
    return true;
}

bool saveConfig(const char* blynk_token_in){
  ticker.attach(0.6, tick);  // led toggle faster
  f = SPIFFS.open("/config5.json", "w");
      if (!f) {
        Serial.println("file creation failed");
      }  

      StaticJsonDocument<200> doc;
      JsonObject root = doc.to<JsonObject>();

      root["blynk_token"] = blynk_token_in;
      // now write two lines in key/value style with  end-of-line characters
      
      // Serialize JSON to file
      if (serializeJson(doc, f) == 0) {
        Serial.println(F("Failed to write to file"));
      }
      f.close();
      ticker.detach();
}

bool setWifiManager(){
  ticker.attach(0.2, tick);  // led toggle faster
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

//  Serial.println("Reset wifi config?:");
//  for(int i=5; i>0; i--){
//    Serial.print(String(i)+" "); 
//    delay(1000);
//  }
//  
  //reset saved settings
  if(digitalRead(SW_PIN) == LOW) // Press button
  {
    Serial.println();
    Serial.println("Reset wifi config");
    wifiManager.resetSettings(); 
  }    
  
  //set custom ip for portal
  //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  WiFiManagerParameter custom_blynk_token("Blynk", "blynk token", blynk_token_s, 34);
  wifiManager.addParameter(&custom_blynk_token);

  wifiManager.autoConnect("AutoConnectAP");
  //or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();

  
  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  const char* blynk_token1 = custom_blynk_token.getValue();
  Serial.printf("blynk_token1->%s,%d<\n\r",blynk_token1,blynk_token1[32]);
  strcpy(blynk_token_s, blynk_token1);
  Serial.printf("blynk_token_s->%s,%d<\n\r",blynk_token_s,blynk_token_s[32]);

  if (strcmp(blynk_token_s, "BLYNK_TOKEN")){  //blynk_token_s == "BLYNK_TOKEN" use auth from file 
    Serial.println("blynk_token is set");
    Blynk.config(blynk_token_s);
    saveConfig(blynk_token_s);
  }else{
    Serial.println("blynk_token in file");  
    //Blynk.config(auth);
    loadConfig();
  }
  
}

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    while (!Serial) continue;
    pinMode(SW_PIN, INPUT_PULLUP);  

    ticker.attach(0.1, tick);

    // always use this to "mount" the filesystem
    bool result = SPIFFS.begin();
    Serial.println("SPIFFS opened: " + result);

    setWifiManager();
    
//    //WiFiManager
//    //Local intialization. Once its business is done, there is no need to keep it around
//    WiFiManager wifiManager;
//
//    Serial.println("Reset wifi config?:");
//    for(int i=5; i>0; i--){
//      Serial.print(String(i)+" "); 
//      delay(1000);
//    }
//    
//    //reset saved settings
//    if(digitalRead(SW_PIN) == LOW) // Press button
//    {
//      Serial.println();
//      Serial.println("Reset wifi config");
//      wifiManager.resetSettings(); 
//    }    
//    
//    //set custom ip for portal
//    //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
//
//    //fetches ssid and pass from eeprom and tries to connect
//    //if it does not connect it starts an access point with the specified name
//    //here  "AutoConnectAP"
//    //and goes into a blocking loop awaiting configuration
//    WiFiManagerParameter custom_blynk_token("Blynk", "blynk token", blynk_token, 34);
//    wifiManager.addParameter(&custom_blynk_token);
//  
//    wifiManager.autoConnect("AutoConnectAP");
//    //or use this for auto generated name ESP + ChipID
//    //wifiManager.autoConnect();
//
//    
//    //if you get here you have connected to the WiFi
//    Serial.println("connected...yeey :)");
//
//    const char* blynk_token1 = custom_blynk_token.getValue();
    //Blynk.begin(auth);
    //Blynk.config(custom_blynk_token.getValue());
    //Serial.println(blynk_token1);
    Serial.printf("blynk_token->%d<",blynk_token_s[0]);
    //Serial.printf("adc = %u \r\n",val); 
//    if(blynk_token_s[0]){
//      Serial.println("blynk_token is set");
//      Blynk.config(blynk_token_s);
//      saveConfig(blynk_token_s);
//    }else{
//      Serial.println("blynk_token in file");  
//      //Blynk.config(auth);
//      loadConfig();
//    }

//    if (strcmp(blynk_token_s, "BLYNK_TOKEN")){  //blynk_token_s == "BLYNK_TOKEN" use auth from file 
//      Serial.println("blynk_token is set");
//      Blynk.config(blynk_token_s);
//      saveConfig(blynk_token_s);
//    }else{
//      Serial.println("blynk_token in file");  
//      //Blynk.config(auth);
//      loadConfig();
//    }

    ticker.detach();
}

void loop() {
    // put your main code here, to run repeatedly:
    Blynk.run();
    
  //reset saved settings
  if(digitalRead(SW_PIN) == LOW){ // Press button
    delay(1000); 
    int i = 5;
    Serial.println("Reset wifi config?:");
    while(digitalRead(SW_PIN) == LOW){
      Serial.print(String(i)+" "); 
      delay(1000);
      if((--i) == 0){
        setWifiManager(); 
        ticker.detach(); 
      }
    }
    Serial.println();
    Serial.println("exit wifi config");
  } 
    
}

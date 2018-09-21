/*
 * IRremoteESP8266: IRrecvDumpV2 - dump details of IR codes with IRrecv
 * An IR detector/demodulator must be connected to the input RECV_PIN.
 *
 * Copyright 2009 Ken Shirriff, http://arcfn.com
 * Copyright 2017 David Conran
 *
 * Example circuit diagram:
 *  https://github.com/markszabo/IRremoteESP8266/wiki#ir-receiving
 *
 * Changes:
 *   Version 0.4 July, 2018
 *     - Minor improvements and more A/C unit support.
 *   Version 0.3 November, 2017
 *     - Support for A/C decoding for some protcols.
 *   Version 0.2 April, 2017
 *     - Decode from a copy of the data so we can start capturing faster thus
 *       reduce the likelihood of miscaptures.
 * Based on Ken Shirriff's IrsendDemo Version 0.1 July, 2009,
 */

#ifndef UNIT_TEST
#include <Arduino.h>
#endif
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#define  BLYNK_PRINT Serial        // Comment this out to disable prints and save space
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <IRrecv.h>
#include <IRsend.h>
#include <Ticker.h>
#include <DNSServer.h>
#include <BlynkSimpleEsp8266.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>
#include <ArduinoJson.h>
#include <FS.h>

#if DECODE_AC
#include <ir_Daikin.h>
#include <ir_Fujitsu.h>
#include <ir_Gree.h>
#include <ir_Haier.h>
#include <ir_Kelvinator.h>
#include <ir_Midea.h>
#include <ir_Toshiba.h>
#endif  // DECODE_AC

// ==================== start of TUNEABLE PARAMETERS ====================
// An IR detector/demodulator is connected to GPIO pin 14
// e.g. D5 on a NodeMCU board.
#define RECV_PIN 14
#define IR_LED 4  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
// define button set token
#define SW_PIN 0

// The Serial connection baud rate.
// i.e. Status message will be sent to the PC at this baud rate.
// Try to avoid slow speeds like 9600, as you will miss messages and
// cause other problems. 115200 (or faster) is recommended.
// NOTE: Make sure you set your Serial Monitor to the same speed.
#define BAUD_RATE 115200

// As this program is a special purpose capture/decoder, let us use a larger
// than normal buffer so we can handle Air Conditioner remote codes.
#define CAPTURE_BUFFER_SIZE 1024
#define BUFFER_SIZE 2048

// TIMEOUT is the Nr. of milli-Seconds of no-more-data before we consider a
// message ended.
// This parameter is an interesting trade-off. The longer the timeout, the more
// complex a message it can capture. e.g. Some device protocols will send
// multiple message packets in quick succession, like Air Conditioner remotes.
// Air Coniditioner protocols often have a considerable gap (20-40+ms) between
// packets.
// The downside of a large timeout value is a lot of less complex protocols
// send multiple messages when the remote's button is held down. The gap between
// them is often also around 20+ms. This can result in the raw data be 2-3+
// times larger than needed as it has captured 2-3+ messages in a single
// capture. Setting a low timeout value can resolve this.
// So, choosing the best TIMEOUT value for your use particular case is
// quite nuanced. Good luck and happy hunting.
// NOTE: Don't exceed MAX_TIMEOUT_MS. Typically 130ms.
#if DECODE_AC
#define TIMEOUT 50U  // Some A/C units have gaps in their protocols of ~40ms.
                     // e.g. Kelvinator
                     // A value this large may swallow repeats of some protocols
#else  // DECODE_AC
#define TIMEOUT 15U  // Suits most messages, while not swallowing many repeats.
#endif  // DECODE_AC
// Alternatives:
// #define TIMEOUT 90U  // Suits messages with big gaps like XMP-1 & some aircon
                        // units, but can accidentally swallow repeated messages
                        // in the rawData[] output.
// #define TIMEOUT MAX_TIMEOUT_MS  // This will set it to our currently allowed
                                   // maximum. Values this high are problematic
                                   // because it is roughly the typical boundary
                                   // where most messages repeat.
                                   // e.g. It will stop decoding a message and
                                   //   start sending it to serial at precisely
                                   //   the time when the next message is likely
                                   //   to be transmitted, and may miss it.

// Set the smallest sized "UNKNOWN" message packets we actually care about.
// This value helps reduce the false-positive detection rate of IR background
// noise as real messages. The chances of background IR noise getting detected
// as a message increases with the length of the TIMEOUT value. (See above)
// The downside of setting this message too large is you can miss some valid
// short messages for protocols that this library doesn't yet decode.
//
// Set higher if you get lots of random short UNKNOWN messages when nothing
// should be sending a message.
// Set lower if you are sure your setup is working, but it doesn't see messages
// from your device. (e.g. Other IR remotes work.)
// NOTE: Set this value very high to effectively turn off UNKNOWN detection.
#define MIN_UNKNOWN_SIZE 12
// ==================== end of TUNEABLE PARAMETERS ====================

IRsend irsend(IR_LED);  // Set the GPIO to be used to sending the message.
// Use turn on the save buffer feature for more complete capture coverage.
IRrecv irrecv(RECV_PIN, CAPTURE_BUFFER_SIZE, TIMEOUT, true);

decode_results results;  // Somewhere to store the results

File f;
Ticker ticker;
BlynkTimer timer;
const char *filename = "/config.json";  // <- SD library uses 8.3 filenames
const char *fileIRNameOn = "/dataKeyIROn.txt";  // <- SD library uses 8.3 filenames
const char *fileIRNameOff = "/dataKeyIROff.txt";  // <- SD library uses 8.3 filenames

int statusSaveIR = 3;
//status botton config
int ledState = false;
int btnStateConfig = HIGH;

//button Virtual
const int AppBtn1 = D4;

char mqtt_server[40];
char mqtt_port[6] = "";
char auth[] = "34c0acad7fee4dc2b2d37d555e110eb9";
char blynk_token_f[34] = "";
char blynk_token_s[34] = "";
int count = 0;
int count1 = 0;

// When App button is pushed - switch the state
BLYNK_WRITE(V1) {
  ledState = param.asInt();
  digitalWrite(AppBtn1, ledState);
  if(ledState){
    loadKeyRemote(fileIRNameOff);
  }else{
    loadKeyRemote(fileIRNameOn); 
  }
  Serial.println("BLYNK_WRITEV1");
}

// Display the human readable state of an A/C message if we can.
void dumpACInfo(decode_results *results) {
  String description = "";
#if DECODE_DAIKIN
  if (results->decode_type == DAIKIN) {
    IRDaikinESP ac(0);
    ac.setRaw(results->state);
    description = ac.toString();
  }
#endif  // DECODE_DAIKIN
#if DECODE_FUJITSU_AC
  if (results->decode_type == FUJITSU_AC) {
    IRFujitsuAC ac(0);
    ac.setRaw(results->state, results->bits / 8);
    description = ac.toString();
  }
#endif  // DECODE_FUJITSU_AC
#if DECODE_KELVINATOR
  if (results->decode_type == KELVINATOR) {
    IRKelvinatorAC ac(0);
    ac.setRaw(results->state);
    description = ac.toString();
  }
#endif  // DECODE_KELVINATOR
#if DECODE_TOSHIBA_AC
  if (results->decode_type == TOSHIBA_AC) {
    IRToshibaAC ac(0);
    ac.setRaw(results->state);
    description = ac.toString();
  }
#endif  // DECODE_TOSHIBA_AC
#if DECODE_GREE
  if (results->decode_type == GREE) {
    IRGreeAC ac(0);
    ac.setRaw(results->state);
    description = ac.toString();
  }
#endif  // DECODE_GREE
#if DECODE_MIDEA
  if (results->decode_type == MIDEA) {
    IRMideaAC ac(0);
    ac.setRaw(results->value);  // Midea uses value instead of state.
    description = ac.toString();
  }
#endif  // DECODE_MIDEA
#if DECODE_HAIER_AC
  if (results->decode_type == HAIER_AC) {
    IRHaierAC ac(0);
    ac.setRaw(results->state);
    description = ac.toString();
  }
#endif  // DECODE_HAIER_AC
#if DECODE_HAIER_AC_YRW02
  if (results->decode_type == HAIER_AC_YRW02) {
    IRHaierACYRW02 ac(0);
    ac.setRaw(results->state);
    description = ac.toString();
  }
#endif  // DECODE_HAIER_AC_YRW02
  // If we got a human-readable description of the message, display it.
  if (description != "")  Serial.println("Mesg Desc.: " + description);
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
    if((countIsConfig) == 3){
      ticker.attach(0.5, tick);  // led toggle faster 
      if(statusSaveIR){
        statusSaveIR = 0;
      }else{
        statusSaveIR = 3;
        ticker.detach(); 
      }
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


bool saveKeyRemote(String IRIn,const char* key){  
  
  //f = SPIFFS.open("/config5.json", "w");
//  const char *fileIRNameOn = "/dataKeyIROn.txt";  // <- SD library uses 8.3 filenames
//const char *fileIRNameOff = "/dataKeyIROff.txt";  // <- SD library uses 8.3 filenames
  const char *filename = fileIRNameOn;  
  if(key == "keyOn"){
    filename = fileIRNameOn;   
  }else{
    filename = fileIRNameOff;   
  }
  Serial.printf("saveKeyRemote file name ->%s<\n\r",filename);
  f = SPIFFS.open(filename, "w");
      if (!f) {
        Serial.println("file creation failed");
        return false;
      } 

      //codeIr.replace("uint16_t rawData[", "");
      IRIn.replace(" ", "");
      IRIn.replace("]", "");
      char str[BUFFER_SIZE];
      //char str1[1024];
      strncpy(str, IRIn.c_str(), sizeof(str));
      str[sizeof(str) - 1] = 0;
      //char str[] ="102,330,3133,76531,451:000,12,44412";
      char * pch;
      char * ptr;
      //printf ("Splitting string \"%s\" into tokens:\n",str);
      pch = strtok (str,";");
      printf ("%s\n",pch);
      ptr = strtok (pch, "[");
      printf ("%s\n",ptr);
      ptr = strtok (NULL, "[");
      
      printf ("%s\n",ptr);
      f.print(ptr);
//      strcpy(str1, ptr);
//      printf ("%s\n",str1);
//      yield();
//      StaticJsonDocument<BUFFER_SIZE> doc;
////      yield();
//      JsonObject root = doc.to<JsonObject>();
//      yield();
////
//      root["keyOn"] = "77777777777777777777777777777777777777777777";
//      yield();
//      // now write two lines in key/value style with  end-of-line characters
//      
//      // Serialize JSON to file
//      yield();
//      if (serializeJson(doc, f) == 0) {
//        Serial.println(F("Failed to write to file"));
//      }
      f.close();
      return true;
}

bool loadKeyRemote(const char *filename){
    Serial.printf("loadConfig file name ->%s<\n\r",filename);
    f = SPIFFS.open(filename, "r");
    if (!f) {
      Serial.println("File doesn't exist yet. Creating it");
      f = SPIFFS.open(filename, "w");
      if (!f) {
        Serial.println("file creation failed");
        return false;
      }
    }else {
      // we could open the file
      while(f.available()) {
        //Lets read line by line from the file
        String line = f.readStringUntil('\n');
        Serial.println(line);
        Serial.println("");
        char str[BUFFER_SIZE];
        line.replace("{", "");
        line.replace("}", "");
        strncpy(str, line.c_str(), sizeof(str));
        str[sizeof(str) - 1] = 0;
        char * pch;
        char * ptr;
        String pchOne = "";
        pch = strtok (str,"=");
        pchOne = pch; 
        int rawData_size = pchOne.toInt(); 
        uint16_t rawData[rawData_size];
        //printf ("Splitting string \"%s\" into tokens:\n",str);
        pch = strtok (NULL,",");
        int i = 0;
        while (pch != NULL)
        {
          //Serial.printf ("%s\n",pch);
          pchOne = pch; 
          //strncpy(pchOne, pch,sizeof(pchOne));
          rawData[i]= pchOne.toInt(); 
          pch = strtok (NULL, ",");
          i++;
        }
        int size = *(&rawData + 1) - rawData; 
        Serial.printf ("rawData%d size=%d\n",rawData[0],size);
        Serial.println("a rawData capture from IRrecvDumpV2");
        irsend.sendRaw(rawData, size, 38);  // Send a raw data capture at 38kHz.
      }
    }
    f.close();
    return true;
}

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
  ticker.attach(0.1, tick);  // led toggle faster
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

//bool loadKeyRemote(const char *filename){
////  // always use this to "mount" the filesystem
////    bool result = SPIFFS.begin();
////    Serial.println("SPIFFS opened: " + result);
//    Serial.printf("loadConfig file name ->%s<\n\r",filename);
//    // this opens the file "f.txt" in read-mode
//     //f = SPIFFS.open("/config5.json", "r");
//     f = SPIFFS.open(filename, "r");
//    
//    if (!f) {
//      Serial.println("File doesn't exist yet. Creating it");
//  
//      // open the file in write mode
//      //f = SPIFFS.open("/config5.json", "w");
//      f = SPIFFS.open(filename, "w");
//      if (!f) {
//        Serial.println("file creation failed");
//      }
//      // Allocate the JSON document
//      //
//      // Inside the brackets, 200 is the RAM allocated to this document.
//      // Don't forget to change this value to match your requirement.
//      // Use arduinojson.org/assistant to compute the capacity.
//      StaticJsonDocument<BUFFER_SIZE> doc;
//    
//      // StaticJsonObject allocates memory on the stack, it can be
//      // replaced by DynamicJsonDocument which allocates in the heap.
//      //
//      // DynamicJsonDocument  doc(200);
//    
//      // Make our document be an object
//      JsonObject root = doc.to<JsonObject>();
//    
//      // Add values in the object
//      //
//      // Most of the time, you can rely on the implicit casts.
//      // In other case, you can do root.set<long>("time", 1351824120);
//      root["keyOn"] = "{4558,4390,646,1598,646,1594,646,1598,620,508,646,482,646,482,646,482,672,456,642,1598,648,1596,646,1596,646,482,646,482,646,478,646,482,646,482,646,482,646,1598,646,478,650,478,620,508,620,508,646,482,646,482,648,1592,646,482,646,1598,646,1598,646,1594,646,1598,646,1598,672,1568,650,46628,4500,4446,594,1650,646,1598,646,1594,646,482,646,482,646,482,646,482,646,482,616,1624,646,1598,620,1620,652,476,648,482,646,482,646,482,646,482,646,478,644,1598,646,482,646,482,646,482,646,482,642,482,646,482,648,1598,642,486,642,1598,620,1624,646,1594,646,1598,646,1598,646,1594,650}";
//      // now write two lines in key/value style with  end-of-line characters
//      
//      // Serialize JSON to file
//      if (serializeJson(doc, f) == 0) {
//        Serial.println(F("Failed to write to file"));
//      }
//    }else {
//      // we could open the file
//      while(f.available()) {
//        //Lets read line by line from the file
//        String line = f.readStringUntil('\n');
//        Serial.println(line);
//        Serial.println("");
//        
////        // Allocate the JSON document
////        //
////        // Inside the brackets, 200 is the RAM allocated to this document.
////        // Don't forget to change this value to match your requirement.
////        // Use arduinojson.org/assistant to compute the capacity.
////        StaticJsonDocument<BUFFER_SIZE> doc;
////        
////        // Deserialize the JSON document
////        DeserializationError error = deserializeJson(doc, line);
////        
////        // Test if parsing succeeds.
////        if (error) {
////          Serial.print(F("deserializeJson() failed: "));
////          Serial.println(error.c_str());
////          return false;
////        }
////        
////        // Get the root object in the document
////        char str[1024];
////        JsonObject root = doc.as<JsonObject>();
////        strcpy(str, root["keyOn"]);
////        Serial.printf("str->%s<\n\r",str);
//      }
//    }
//    f.close();
//    return true;
//}

// The section of code run only once at start-up.
void setup() {
  Serial.begin(BAUD_RATE, SERIAL_8N1, SERIAL_TX_ONLY);
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);
  pinMode(SW_PIN, INPUT_PULLUP); 
  pinMode(AppBtn1, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);
    // always use this to "mount" the filesystem
  bool result = SPIFFS.begin();
  Serial.println("SPIFFS opened: " + result);
  Serial.println();
  Serial.print("IRrecvDumpV2 is now running and waiting for IR input on Pin ");
  Serial.println(RECV_PIN);
  irsend.begin();
  setWifiManager();
  ticker.detach(); 
//  loadKeyRemote(fileIRNameOn);
//  loadKeyRemote(fileIRNameOff);
  timer.setInterval(1000L, checkButtonIsConfig);
  timer.setInterval(1000L, tick1);
   //Format File System
//  if(SPIFFS.format())
//  {
//    Serial.println("File System Formated");
//  }
//  else
//  {
//    Serial.println("File System Formatting Error");
//  }

#if DECODE_HASH
  // Ignore messages with less than minimum on or off pulses.
  irrecv.setUnknownThreshold(MIN_UNKNOWN_SIZE);
#endif  // DECODE_HASH
  irrecv.enableIRIn();  // Start the receiver
}

// The repeating section of the code
//
void loop() {
  // Check if the IR code has been received.
  Blynk.run();
  timer.run();
  if(statusSaveIR < 2){
    if (irrecv.decode(&results)) {
      ticker.detach(); 
      ticker.attach(0.1, tick);  // led toggle faster 
      // Display a crude timestamp.
  
      if (results.overflow)
        Serial.printf("WARNING: IR code is too big for buffer (>= %d). "
                      "This result shouldn't be trusted until this is resolved. "
                      "Edit & increase CAPTURE_BUFFER_SIZE.\n",
                      CAPTURE_BUFFER_SIZE);
  
      String codeIr = resultToSourceCode(&results); 
      if(statusSaveIR == 0){
        saveKeyRemote(codeIr,"keyOn");
      }else{
        saveKeyRemote(codeIr,"keyOff");
      }
      statusSaveIR++;
      yield();
  //    int i = 0;
  //    while (ptr != NULL)
  //    {
  //      printf ("%s\n",ptr);
  //      ptr = strtok (NULL, ",");
  //    }
  //    Serial.println(codeIr);
  //    Serial.println(resultToSourceCode(&results));
      Serial.println("ok");  // Blank line between entries
      if(statusSaveIR == 1){
        ticker.detach(); 
        ticker.attach(0.8, tick);  // led toggle faster 
      }else{
        ticker.detach(); 
      }
      yield();  // Feed the WDT (again)
    }
  }
}

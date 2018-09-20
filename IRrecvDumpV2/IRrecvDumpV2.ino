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
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include "FS.h"
#include <ArduinoJson.h>
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


// Use turn on the save buffer feature for more complete capture coverage.
IRrecv irrecv(RECV_PIN, CAPTURE_BUFFER_SIZE, TIMEOUT, true);

decode_results results;  // Somewhere to store the results

File f;
const char *fileIRNameOn = "/dataKeyIROn.txt";  // <- SD library uses 8.3 filenames
const char *fileIRNameOff = "/dataKeyIROff.txt";  // <- SD library uses 8.3 filenames


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
      ptr = strtok (pch, "=");
      printf ("%s\n",ptr);
      ptr = strtok (NULL, "=");
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
      StaticJsonDocument<BUFFER_SIZE> doc;
    
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
      root["keyOn"] = "{4558,4390,646,1598,646,1594,646,1598,620,508,646,482,646,482,646,482,672,456,642,1598,648,1596,646,1596,646,482,646,482,646,478,646,482,646,482,646,482,646,1598,646,478,650,478,620,508,620,508,646,482,646,482,648,1592,646,482,646,1598,646,1598,646,1594,646,1598,646,1598,672,1568,650,46628,4500,4446,594,1650,646,1598,646,1594,646,482,646,482,646,482,646,482,646,482,616,1624,646,1598,620,1620,652,476,648,482,646,482,646,482,646,482,646,478,644,1598,646,482,646,482,646,482,646,482,642,482,646,482,648,1598,642,486,642,1598,620,1624,646,1594,646,1598,646,1598,646,1594,650}";
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
        
//        // Allocate the JSON document
//        //
//        // Inside the brackets, 200 is the RAM allocated to this document.
//        // Don't forget to change this value to match your requirement.
//        // Use arduinojson.org/assistant to compute the capacity.
//        StaticJsonDocument<BUFFER_SIZE> doc;
//        
//        // Deserialize the JSON document
//        DeserializationError error = deserializeJson(doc, line);
//        
//        // Test if parsing succeeds.
//        if (error) {
//          Serial.print(F("deserializeJson() failed: "));
//          Serial.println(error.c_str());
//          return false;
//        }
//        
//        // Get the root object in the document
//        char str[1024];
//        JsonObject root = doc.as<JsonObject>();
//        strcpy(str, root["keyOn"]);
//        Serial.printf("str->%s<\n\r",str);
      }
    }
    f.close();
    return true;
}

// The section of code run only once at start-up.
void setup() {
  Serial.begin(BAUD_RATE, SERIAL_8N1, SERIAL_TX_ONLY);
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);
    // always use this to "mount" the filesystem
  bool result = SPIFFS.begin();
  Serial.println("SPIFFS opened: " + result);
  Serial.println();
  Serial.print("IRrecvDumpV2 is now running and waiting for IR input on Pin ");
  Serial.println(RECV_PIN);
  loadKeyRemote(fileIRNameOn);
  loadKeyRemote(fileIRNameOff);
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
  if (irrecv.decode(&results)) {
    // Display a crude timestamp.
    uint32_t now = millis();
    Serial.printf("Timestamp : %06u.%03u\n", now / 1000, now % 1000);
    if (results.overflow)
      Serial.printf("WARNING: IR code is too big for buffer (>= %d). "
                    "This result shouldn't be trusted until this is resolved. "
                    "Edit & increase CAPTURE_BUFFER_SIZE.\n",
                    CAPTURE_BUFFER_SIZE);
//    // Display the basic output of what we found.
//    Serial.print(resultToHumanReadableBasic(&results));
//    dumpACInfo(&results);  // Display any extra A/C info if we have it.
//    yield();  // Feed the WDT as the text output can take a while to print.
//
//    // Display the library version the message was captured with.
//    Serial.print("Library   : v");
//    Serial.println(_IRREMOTEESP8266_VERSION_);
//    Serial.println();
//
//    // Output RAW timing info of the result.
//    Serial.println(resultToTimingInfo(&results));
//    yield();  // Feed the WDT (again)

    // Output the results as source code
    String codeIr = resultToSourceCode(&results); 
    
//    //codeIr.replace("uint16_t rawData[", "");
//    codeIr.replace(" ", "");
//    char str[1024];
//    strncpy(str, codeIr.c_str(), sizeof(str));
//    str[sizeof(str) - 1] = 0;
//    //char str[] ="102,330,3133,76531,451:000,12,44412";
//    char * pch;
//    char * ptr;
//    //printf ("Splitting string \"%s\" into tokens:\n",str);
//    pch = strtok (str,";");
//    printf ("%s\n",pch);
//    ptr = strtok (pch, "=");
//    printf ("%s\n",ptr);
//    ptr = strtok (NULL, "=");
//    printf ("%s\n",ptr);
    yield();
    saveKeyRemote(codeIr,"keyOff");
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
    yield();  // Feed the WDT (again)
  }
}

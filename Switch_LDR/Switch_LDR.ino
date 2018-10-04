/*
  AnalogReadSerial

  Reads an analog input on pin 0, prints the result to the Serial Monitor.
  Graphical representation is available using Serial Plotter (Tools > Serial Plotter menu).
  Attach the center pin of a potentiometer to pin A0, and the outside pins to +5V and ground.

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/AnalogReadSerial
*/
#define chang 0.15
int inc1 = 0;
int inc2 = 0;
int inc3 = 0;
float p;
int statusSw = 0;
int adc[100];
void sheckButton (int dataIn){
  float arv;  
  float temp = adc[0];
  for(int i = 1; i <100; i++){
    arv = ((adc[i]+temp)/2);
    temp = arv;
  }
  if(statusSw){
    //Serial.printf("arv   >%f\n\r",arv);
    //Serial.printf("dataIn>%ld\n\r",dataIn);
    statusSw = 0;
//    if(arv > dataIn){
//      p = ((arv-dataIn)/arv);
//      //Serial.printf("persen>%f\n\r",p);
//    }else{
//      p = ((dataIn-arv)/dataIn);
//      //Serial.printf("persen>%f\n\r",p);  
//    }
    p =0;
    if(dataIn > arv){
      p = ((dataIn-arv)/dataIn);
      //Serial.printf("persen>%f\n\r",p);  
    }

//    if(arv > dataIn){
//      p = ((arv-dataIn)/arv);
//      //Serial.printf("persen>%f\n\r",p);
//    }
//    Serial.printf("P>%f\n\r",p);  
    if(arv < 50){
      if(p > 0.6){
        Serial.println("L1");
        digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
      }else{
        Serial.println("H1");
        digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED on (Note that LOW is the voltage level
      }
    }else if(dataIn < 100){
      if(p > 0.5){
        Serial.println("L2");
        digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
      }else{
        Serial.println("H2");
        digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED on (Note that LOW is the voltage level
      }
    }else if(dataIn < 200){
      if(p > 0.4){
        Serial.println("L3");
        digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
      }else{
        Serial.println("H3");
        digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED on (Note that LOW is the voltage level
      }
    }else if(dataIn < 500){
      if(p > 0.3){
        Serial.println("L4");
        digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
      }else{
        Serial.println("H4");
        digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED on (Note that LOW is the voltage level
      }
    }else if(dataIn < 800){
      if(p > 0.15){
        Serial.println("L5");
        digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
      }else{
        Serial.println("H5");
        digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED on (Note that LOW is the voltage level
      }
    }else if(dataIn < 1024){
      if(p > 0.05){
        Serial.println("L6");
        digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
      }else{
        Serial.println("H6");
        digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED on (Note that LOW is the voltage level
      }
    }
  }
  
}

//void sheckButton (int dataIn){
//  float arv;  
//  float temp = adc[0];
//  for(int i = 1; i <100; i++){
//    arv = ((adc[i]+temp)/2);
//    temp = arv;
//  }
//  if(statusSw){
//    //Serial.printf("arv   >%f\n\r",arv);
//    //Serial.printf("dataIn>%ld\n\r",dataIn);
//    statusSw = 0;
//    if(arv > dataIn){
//      p = ((arv-dataIn)/arv);
//      //Serial.printf("persen>%f\n\r",p);
//    }else{
//      p = ((dataIn-arv)/dataIn);
//      //Serial.printf("persen>%f\n\r",p);  
//    }
//    Serial.printf("persen>%f\n\r",p);  
//    if(arv < 50){
//      if(p > 0.99){
//        //Serial.println("button low1");
//        digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
//      }else{
//        //Serial.println("button HIgh1");
//        digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED on (Note that LOW is the voltage level
//      }
//    }else if(dataIn < 100){
//      if(p > 0.95){
//        //Serial.println("button low2");
//        digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
//      }else{
//        //Serial.println("button HIgh2");
//        digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED on (Note that LOW is the voltage level
//      }
//    }else if(dataIn < 200){
//      if(p > 0.8){
//        //Serial.println("button low2");
//        digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
//      }else{
//        //Serial.println("button HIgh2");
//        digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED on (Note that LOW is the voltage level
//      }
//    }else if(dataIn < 500){
//      if(p > 0.6){
//        //Serial.println("button low5");
//        digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
//      }else{
//        //Serial.println("button HIgh5");
//        digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED on (Note that LOW is the voltage level
//      }
//    }else if(dataIn < 800){
//      if(p > 0.2){
//        //Serial.println("button low6");
//        digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
//      }else{
//        //Serial.println("button HIgh6");
//        digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED on (Note that LOW is the voltage level
//      }
//    }
//  }
//  
//}


// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
}


// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:
  int sensorValue = analogRead(A0);
  // print out the value you read:
  adc[inc1] = sensorValue;
  if(++inc1 > 100){
    inc1 = 0;
  }
  if(++inc2 >= 5){
    inc2 = 0;
    sheckButton (sensorValue);
  }
  if(statusSw ==0){
    if(++inc3 >= 20){
      inc3 = 0;
      statusSw = 1;
    }
  }
  Serial.println(sensorValue);
  delay(10);        // delay in between reads for stability
}

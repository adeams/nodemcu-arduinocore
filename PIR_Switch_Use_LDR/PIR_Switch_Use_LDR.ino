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
float persenChang[100]={0.030, 0.030, 0.030, 0.030, 0.030, 0.030, 0.030, 0.030, 0.030, 0.030,//<100
                        0.030, 0.030, 0.030, 0.030, 0.030, 0.030, 0.030, 0.030, 0.030, 0.030,//<200
                        0.025, 0.025, 0.025, 0.025, 0.025, 0.025, 0.025, 0.025, 0.025, 0.025,//<300
                        0.025, 0.025, 0.025, 0.025, 0.025, 0.025, 0.025, 0.025, 0.025, 0.025,//<400
                        0.025, 0.025, 0.025, 0.025, 0.025, 0.025, 0.025, 0.025, 0.025, 0.025,//<500
                        0.020, 0.020, 0.020, 0.020, 0.020, 0.020, 0.020, 0.020, 0.020, 0.020,//<600
                        0.020, 0.020, 0.020, 0.020, 0.020, 0.020, 0.020, 0.020, 0.020, 0.020,//<700
                        0.015, 0.015, 0.015, 0.015, 0.015, 0.015, 0.015, 0.015, 0.015, 0.015,//<800
                        0.010, 0.010, 0.010, 0.010, 0.010, 0.010, 0.010, 0.010, 0.010, 0.010,//<900
                        0.010, 0.010, 0.010, 0.010, 0.010, 0.010, 0.010, 0.010, 0.010, 0.010};//>1000
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

    //Serial.println(arv);
    int mod = arv/10; 
    
    //Serial.println(p);
    Serial.println(persenChang[mod]);
    if(p > persenChang[mod]){
      //Serial.println("L1");
      digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
    }else{
      //Serial.println("H1");
      digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED on (Note that LOW is the voltage level
    }
  }
  
}


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

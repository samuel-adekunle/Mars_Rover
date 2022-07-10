#include <SoftwareSerial.h>

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(Serial.available()){
      String val = Serial.readStringUntil('\n');
      Serial2.println(val);
      Serial.println("S" + val);
  }
  if(Serial2.available()){
      String val = Serial2.readStringUntil('\n');
      Serial.println("R" + val);
  }
  
}

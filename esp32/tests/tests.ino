#include <Arduino.h>
#include "ArduinoNvs.h"

bool res;
#define NAME "nvs_test"
#define NUM_TESTS 40

void setup() {  
    Serial.begin(115200); 
    NVS.begin();
}


void loop() {	
  if (Serial.available()) {
    char cmd = Serial.read();
    bool ok;
    switch (cmd) {
      case '1': {
        /*** BLOB ***/
        uint8_t blob[120]; 
      
        for (int f = 0; f < NUM_TESTS; f++) {
          char name[20];  
          sprintf(name, NAME"%d", f);
        
          for (int i=0; i < 120; i++) {
            blob[i] = random();
          }
        
          //write to flash
          unsigned long now = millis();
          res = NVS.setBlob(name, blob, sizeof(blob));
          unsigned long time = millis() - now;
          Serial.println();
          Serial.printf("%s: %d written in %d ms \r\n", name, sizeof(blob), time);
        
          // read from flash
          size_t blobLength = NVS.getBlobSize(name);    
          uint8_t blob[blobLength];
          now = millis();
          res = NVS.getBlob(name, blob, sizeof(blob));
          time = millis() - now;
          Serial.printf("%d read in %d ms \r\n", sizeof(blob), time);
        } 
      } break;
      case '?': {
        Serial.println("Commands:");
        Serial.println("1: runs a speed test on NVS flash storage");
        Serial.println("?: prints a list of commands for running tests");
      } break;
    }
  }
}

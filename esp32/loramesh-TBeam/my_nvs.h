// my_nvs.h

#include <Arduino.h>
#include <sodium/crypto_hash_sha256.h>

#include "ArduinoNvs.h"

// debug output:
#define SUMMARY_LINES 1000 // 1000

// config:
// dimensioning: with 6.5M (Heltec) we can have approx 15*200*10 packets
// dimensioning: with 2.5M (T-Beam) we can have approx 10*100*10 packets

#define MAX_FID  25  // max number of feeds
#define MAX_SEQ 200  // length of append-only log per feed
#define MAX_CNR  10  // sidechain chunks per entry

#define NAME_FEEDS "feeds.bin"

// #define NAME "nvs_test"
// #define NUM_TESTS 40

unsigned char fid_table[MAX_FID*32];
int fid_cnt;


void erase() {
  Serial.println("erasing all NVS content ...");
  int rc = NVS.eraseAll();
  Serial.printf("return code was %d\r\n", rc);

  Serial.printf(">> you must reboot now to init the erased NVS partition!\r\n");
}


void produce() {
  Serial.printf("Starting write test\r\n\r\n");

  uint8_t blob[120];
  char name[16];
  int fid, seq, cnr; // FID number, seq number, chunk number
  int n = 0, res;
  unsigned long write_time = 0;

  for (fid = 0; fid < MAX_FID; fid++)
    for (seq = 0; seq < MAX_SEQ; seq++)
      for (cnr = 0; cnr < MAX_CNR; cnr++) {
        sprintf(name, "%d.%d.%d", fid, seq, cnr);
        // Serial.printf("writing blob %s\r\n", name);
        // fill in first 64 bytes with FID and some random bytes
        memcpy(blob, fid_table + 32*fid, 32);
        esp_fill_random(blob, 32);
        // add hash at end of buffer:
        crypto_hash_sha256(blob + sizeof(blob) - crypto_hash_sha256_BYTES,
                           blob, 32+32);

        unsigned long now = millis();
        res = NVS.setBlob(name, blob, sizeof(blob));
        if (res != true) {
          Serial.printf(">> write #%d: setBlob() returned %d, aborting\r\n", n, res);
          goto end;
        }
          
        unsigned long delta = millis() - now;
        write_time += delta;
        n++;
        if ((n % SUMMARY_LINES) == 0) {
          Serial.printf("%5d: last name was '%s'\r\n", n, name);
          Serial.printf("       write time: %d msec, avg write time: %d msec, remaining heap: %d\r\n",
                        delta, write_time / n, ESP.getFreeHeap());
        }
      }

end:
  Serial.println();
  if (n != 0) {
    Serial.printf("Average write time for %d entries: %d msec\r\n", n, write_time / n);
    Serial.printf("  % entries * 120B = %d KB\r\n", n, n * 120 / 1024);
  }

  /*
        int res;
      
        for (int t = 0; t < NUM_TESTS; t++) {
          char name[15];  
          sprintf(name, NAME"%d", t);
        
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
  */
}


void consume()
{
}


void help()
{
  Serial.println("Commands:");
  Serial.println("r: reboot");
  Serial.println("w: write test");
  Serial.println("v: verify/read test");
  Serial.println("x: erase all");
  Serial.println("?: prints a list of commands for running tests");
}


void setup(void) {  
    Serial.begin(115200);
    Serial.printf("\r\n\r\n== NVS test for tinySSB\r\n\r\n");
    long a = ESP.getFreeHeap();
    Serial.printf("  heap size before NVS.Begin(): %d\r\n", a);
    NVS.begin("tinySSB", "nvs_ext");
    long b = ESP.getFreeHeap();
    Serial.printf("  heap size after NVS.Begin(): %d, delta: %d\r\n", b, a - b);
    Serial.println();

    int sz = NVS.getBlobSize(NAME_FEEDS);
    if (sz == 0) {
      fid_cnt = MAX_FID;
      esp_fill_random(fid_table, fid_cnt * 32);
      NVS.setBlob(NAME_FEEDS, fid_table, fid_cnt * 32);
      Serial.printf("created the FID table with %d feed IDs\r\n", fid_cnt);
    } else {
      int sz = NVS.getBlobSize(NAME_FEEDS);
      if (sz != MAX_FID * 32)
        Serial.printf("wrong size %d for feed table blob\r\n", sz);
      fid_cnt = sz / 32;
      NVS.getBlob(NAME_FEEDS, fid_table, sz);
      Serial.printf("loaded %d feed IDs\r\n", fid_cnt);
    }

    help();
}


void loop(void) {	
  if (Serial.available()) {
    char cmd = Serial.read();
    Serial.println();
    switch (cmd) {
    case 'r':  esp_restart();  break;
    case 'w':  produce();      break;
    case 'v':  consume();      break;
    case 'x':  erase();        break;
    default:   help();         break;
    }
    Serial.println();
  }
}


// eof

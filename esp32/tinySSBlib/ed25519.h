// tinySSBlib/ed25519.h

// tinySSB for ESP32
// 2022,2025 <christian.tschudin@unibas.ch>

#include <sodium/crypto_sign_ed25519.h>

  /* Here are some performance figures

  unsigned char sk[crypto_sign_ed25519_SECRETKEYBYTES];
  unsigned char pk[crypto_sign_ed25519_PUBLICKEYBYTES];
  
  unsigned long a = millis();
  crypto_sign_ed25519_keypair(pk, sk);
  Serial.println("ed25519 create: " + String(millis()-a,DEC));

  a = millis();
  unsigned long long siglen = crypto_sign_ed25519_BYTES;
  unsigned char s[crypto_sign_ed25519_BYTES];
  crypto_sign_ed25519_detached(s, &siglen, my_mac, sizeof(my_mac), sk);
  Serial.println("ed25519 sign: " + String(millis()-a,DEC));

  a = millis();
  int b = crypto_sign_ed25519_verify_detached(s, my_mac, sizeof(my_mac), pk);
  Serial.println("ed25519 veri: rc=" + String(b,DEC) + " " + String(millis()-a,DEC));

  Runtime results:
  
  23:28:09.086 -> ed25519 create: 22
  23:28:09.086 -> ed25519 sign: 24
  23:28:09.159 -> ed25519 veri: rc=0 54

  */
  

// the ED25519 seed is stored in the ESP32's NVS partition,
// using namespace 'tinySSB'

#define ED25519_SEED_LEN 32
#define ED25519_SK_LEN 64
#define ED25519_PK_LEN 32

typedef bool (*signing_fct)(unsigned char *sig, unsigned char *data, int len);

bool ed25519_new_seed(char *nm);
bool ed25519_del_seed(char *nm);
bool ed25519_get_seed(char *nm, unsigned char *seed);
bool ed25519_get_keypair(char *nm, unsigned char *pk, unsigned char *sk);

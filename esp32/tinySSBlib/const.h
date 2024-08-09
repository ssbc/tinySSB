// const.h

#define PEERS_INTERVAL 25000 // how often to sent pings, in msec

#define LORA_LOG // enable macro for logging received pkts
#define LORA_LOG_FILENAME    "/lora_log.txt"
#define PEERS_DATA_FILENAME  "/peer_log.txt"

#define FID_LEN         32
#define HASH_LEN        20
#define FID_HEX_LEN     (2*FID_LEN)
#define FID_B64_LEN     ((FID_LEN + 2) / 3 * 4)
// #define FEED_DIR        "/feeds"
#define MAX_FEEDS       100
#define FEED_PATH_SIZE  sizeof(FEED_DIR) + 1 + 2 * FID_LEN

#define DMX_LEN          7
#define GOSET_DMX_STR    "tinySSB-0.1 GOset 1"

#define TINYSSB_PKT_LEN  120
#define TINYSSB_SCC_LEN  (TINYSSB_PKT_LEN - HASH_LEN) // sidechain content per packet

#define tSSB_WIFI_SSID   "tinySSB"
#define tSSB_WIFI_PW     "password"
#define tSSB_UDP_ADDR    "239.5.5.8"
#define tSSB_UDP_PORT    1558

#define PKTTYPE_plain48  '\x00' // single packet with 48B payload
#define PKTTYPE_chain20  '\x01' // start of hash sidechain (pkt contains BIPF-encoded content length)

#define BLE_SERVICE_UUID           "6e400001-7646-4b5b-9a50-71becce51558"
#define BLE_CHARACTERISTIC_UUID_RX "6e400002-7646-4b5b-9a50-71becce51558"
#define BLE_CHARACTERISTIC_UUID_TX "6e400003-7646-4b5b-9a50-71becce51558"
#define BLE_CHARACTERISTIC_UUID_ST "6e400004-7646-4b5b-9a50-71becce51558"

#define BLE_RING_BUF_SIZE 3

#define NODE_ROUND_LEN   14000  // millis, for GOset, 1/2 is basis for WANT/CHNK

// eof

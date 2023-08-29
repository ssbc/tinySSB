// mgmt.h

// packet structure:
//
//     7B DMX       demultiplex (app layer)
//
//     1B TYP       message type
//     2B SRC       sender ID
//     xB PAYLOAD   message contents
//     4B FCNT      frame counter
//     4B MAC       message authentication code (computed over TYP, SRC, PAYLOAD & FCNT)
//
//     4B CRC       cyclic redundancy check (LoRa layer)
//

#include <sodium/crypto_hash_sha256.h>
#include <sodium/crypto_auth.h>

#if defined __has_include
#if __has_include ("credentials.h")
#include "credentials.h"
#else
unsigned char MGMT_KEY[crypto_auth_hmacsha512_KEYBYTES] = { 0 };
#endif
#endif

extern GOsetClass *theGOset;
extern DmxClass   *dmx;
// extern RepoClass  *repo;

#define MGMT_DMX_STR         "tinySSB-0.1 mgmt 1"
#define MGMT_ID_LEN          2
#define STATUST_SIZE         23
#define STATUST_EOL          24 * 60 * 60 * 1000 // millis (24h)
#define MGMT_FCNT_FIELD_LEN  4
#define MGMT_MAC_FIELD_LEN   4

#define MGMT_SEND_STATUS_INTERVAL  5*60*1000 // millis (5 minutes) // TODO how often should this be sent during deployment?
#define MGMT_SEND_BEACON_INTERVAL     5*1000 // millis (5 seconds) // TODO turn off after 1h?

#define MGMT_FCNT_LOG_FILENAME        "/mgmt_fcnt_log.bin"
#define MGMT_FCNT_TABLE_LOG_FILENAME  "/mgmt_fcnt_table_log.bin"
#define MGMT_FCNT_TABLE_SIZE          42

#define KEYS_DIR                      "/keys"

struct msg_key_s {
  bool op;
  unsigned char key[GOSET_KEY_LEN];
};

struct log_fcnt_s {
  unsigned char id[MGMT_ID_LEN];
  unsigned int fcnt;
};

struct msg_request_s {
  unsigned char cmd;
  unsigned char dst[MGMT_ID_LEN];
  bool all = false;
};

struct state_s {
  bool beacon;
  long voltage;
  int feeds:10;
  int entries:10;
  int chunks:10;
  int free;
  unsigned long int uptime;
  float latitude;
  float longitude;
  int altitude:14;
  int neighbors:6;
};

struct statust_s {
  unsigned char id[MGMT_ID_LEN];
  state_s state;
  unsigned long int lastSeen;
};

struct statust_local_s {
  unsigned char id[MGMT_ID_LEN];
  state_s state;
  unsigned long int lastSeen;
  unsigned long int received_on;
};

struct statust_entry_s {
  statust_local_s entry;
  statust_local_s neighbors[STATUST_SIZE];
  int neighbor_cnt;
};

#define MGMT_MSG_KEY_LEN            sizeof(struct msg_key_s)
#define MGMT_MSG_REQUEST_LEN        sizeof(struct msg_request_s)
#define MGMT_STATE_LEN              sizeof(struct state_s)
#define MGMT_STATUST_LEN            sizeof(struct statust_s)
#define MGMT_LOG_FCNT_LEN           sizeof(struct log_fcnt_s)

#define MGMT_MSG_STATUS_NEIGHBORS   (int) ((110 - MGMT_STATE_LEN) / MGMT_STATUST_LEN)

struct msg_status_s {
  state_s state;
  statust_s neighbors[MGMT_MSG_STATUS_NEIGHBORS];
};

#define MGMT_MSG_STATUS_LEN         sizeof(struct msg_status_s)



struct statust_entry_s statust[STATUST_SIZE];
int statust_cnt;
int statust_rrb;

struct log_fcnt_s mgmt_fcnt_table[MGMT_FCNT_TABLE_SIZE];
int mgmt_fcnt_table_cnt;

File mgmt_fcnt_log;
File mgmt_fcnt_table_log;

unsigned int mgmt_fcnt = 0;

unsigned char mgmt_id[MGMT_ID_LEN];

bool mgmt_beacon = false;
unsigned long int next_mgmt_send_beacon;
unsigned long int mgmt_next_send_status = MGMT_SEND_STATUS_INTERVAL;

// forward declaration
void mgmt_send_status();
void mgmt_send_beacon();


float getBattVoltage() {
#if defined(ARDUINO_TBEAM_USE_RADIO_SX1262)
#ifdef TBEAM_07
  return analogRead(35) * 0.85 * (2.0 / 1024.0); // 0.85 is voltage divider ratio measured
#else
  return axp.getBattVoltage()/1000;
#endif
#else
  return 0;
#endif
}

//------------------------------------------------------------------------------
// CREATE PAYLOADS

// fill buffer with request packet
unsigned char* _mkRequest(unsigned char cmd, unsigned char* id=NULL)
{
  static struct msg_request_s request;
  request.cmd = cmd;
  if (id == NULL) {
    request.all = true;
  } else {
    memcpy(request.dst, id, MGMT_ID_LEN);
  }
  return (unsigned char*) &request;
}

// fill buffer with status packet
unsigned char* _mkStatus()
{
  static struct msg_status_s status;
  // add self
  status.state.beacon = mgmt_beacon;
  status.state.voltage = 0;
#if defined(AXP_DEBUG)
  status.state.voltage = getBattVoltage();
#endif
  status.state.feeds = repo->rplca_cnt;
  status.state.entries = repo->entry_cnt;
  status.state.chunks = repo->chunk_cnt;
  int total = MyFS.totalBytes();
  int avail = total - MyFS.usedBytes();
  status.state.free = avail / (total/100);
  status.state.uptime = millis();
#if defined(NO_GPS)
  status.state.latitude = 0;
  status.state.longitude = 0;
  status.state.altitude = 0;
#else
  status.state.latitude = gps.location.isValid() ? (float) gps.location.lat() : 0;
  status.state.longitude = gps.location.isValid() ? (float) gps.location.lng() : 0;
  status.state.altitude = gps.location.isValid() ? (float) gps.altitude.meters() : 0;
#endif
  status.state.neighbors = statust_cnt;

  // add neighbors
  int ndxNeighbor;
  for (int i = 0; i < MGMT_MSG_STATUS_NEIGHBORS; i++) {
    if (i >= statust_cnt) { break; }
    ndxNeighbor = statust_rrb;
    statust_rrb = ++statust_rrb % statust_cnt;
    memcpy(&status.neighbors[i].id, &statust[ndxNeighbor].entry.id, MGMT_ID_LEN);
    memcpy(&status.neighbors[i].state, &statust[ndxNeighbor].entry.state, sizeof(struct state_s));
    status.neighbors[i].lastSeen = millis() - statust[ndxNeighbor].entry.received_on;
  }

  return (unsigned char*) &status;
}


// fill buffer with key packet
unsigned char* _mkKey(bool op, unsigned char* key)
{
  static struct msg_key_s key_update;
  key_update.op = op;
  memcpy(key_update.key, key, GOSET_KEY_LEN);
  return (unsigned char*) &key_update;
}



// -----------------------------------------------------------------------------
// MGMT RX

// receive key
void mgmt_rx_key(struct msg_key_s *key_update)
{
  static char p[sizeof(KEYS_DIR) + 1 + 2*GOSET_KEY_LEN];
  char *h = to_hex(key_update->key, GOSET_KEY_LEN, 0);
  strcpy(p, KEYS_DIR);
  strcat(p, "/");
  strcat(p, h);
  File k = MyFS.open(p, "r");
  unsigned int cnt = 0;
  bool changed = false;
  if (k) { // update
    k.read((unsigned char*) &cnt, sizeof(cnt));
    k.close();
    if (key_update->op && cnt % 2 == 1) { cnt++; changed = true; }
    if (!key_update->op && cnt % 2 == 0) { cnt++; changed = true; }
  } else if (not k && key_update->op) { changed = true; } // TODO probably just call goset_add here? then add cnt file to feed

  if (changed) {
    k = MyFS.open(p, "w");
    k.write((unsigned char*) &cnt, sizeof(cnt));
    k.close();
  }

  // TODO at end, probably reload repo or something like that -> interaction with goset
}

// receive request
void mgmt_rx_request(struct msg_request_s *request, int len)
{
  // beacon
  if (request->cmd == '+' || request->cmd == '-') {
    mgmt_beacon = request->cmd == '+' ? true : false;
    return;
  }

  // status
  if (request->cmd == 's') {
    mgmt_next_send_status = millis() + random(5000);
    return;
  }

  // reset
  if (request->cmd == 'r') {
    repo->reset(NULL);
    return;
  }

  // reboot
  if (request->cmd == 'x') {
    esp_restart();
    return;
  }

  // unknown
  Serial.printf("mgmt_rx received request %s ??\r\n", request->cmd);
}

// receive status
void mgmt_rx_status(unsigned char *pkt, int len, unsigned char* src) {
  unsigned long int received_on = millis();
  Serial.println(String("mgmt_rx received status update from ") + to_hex(src, 2, 0));
  // check if node is already in table
  int ndx = -1;
  for (int i = 0; i < statust_cnt; i++) {
    if (!memcmp(src, statust[i].entry.id, MGMT_ID_LEN)) {
      ndx = i;
    }
  }
  // new node, check if table not full
  if (ndx == -1) {
    if (statust_cnt < STATUST_SIZE) {
      ndx = statust_cnt++;
    } else {
      Serial.printf("%8sstatus table is full, skipping...\r\n", "");
      return;
    }
  }
  struct msg_status_s *status = (struct msg_status_s*) calloc(1, MGMT_MSG_STATUS_LEN);
  memcpy(status, pkt,  MGMT_MSG_STATUS_LEN);
  statust[ndx].entry.received_on = received_on;
  statust[ndx].entry.lastSeen = 0;
  memcpy(&statust[ndx].entry.id, src, MGMT_ID_LEN);
  memcpy(&statust[ndx].entry.state, &status->state, MGMT_STATE_LEN);

  int ndxNeighbor;
  struct statust_s *empty = (struct statust_s*) calloc(1, MGMT_STATUST_LEN);
  for (int i = 0; i < MGMT_MSG_STATUS_NEIGHBORS; i++) {
    if (!(memcmp(&status->neighbors[i], empty, MGMT_STATUST_LEN))) {
      break;
    }
    Serial.printf("%8slearned about %s\r\n", "", to_hex(status->neighbors[i].id, 2, 0));
    ndxNeighbor = -1;
    // check if node is already in table
    for (int i = 0; i < statust[ndx].neighbor_cnt; i++) {
      if (!memcmp(status->neighbors[i].id, statust[ndx].neighbors[i].id, MGMT_ID_LEN)) {
        ndxNeighbor = i;
      }
    }
    // new node, check if table not full
    if (ndxNeighbor == -1) {
      if (statust[ndx].neighbor_cnt < STATUST_SIZE) {
        ndxNeighbor = statust[ndx].neighbor_cnt++;
      } else {
	Serial.printf("%8sneighbor table of %s is full, skipping...\r\n", "", to_hex(status->neighbors[i].id, 2, 0));
	continue;
      }
    }
    memcpy(&statust[ndx].neighbors[ndxNeighbor].id, status->neighbors[i].id, MGMT_ID_LEN);
    memcpy(&statust[ndx].neighbors[ndxNeighbor].state, &status->neighbors[i].state, MGMT_STATE_LEN);
    statust[ndx].neighbors[ndxNeighbor].lastSeen = status->neighbors[i].lastSeen;
    statust[ndx].neighbors[ndxNeighbor].received_on = received_on;
  }

  free(empty);
  free(status);
}

// incoming packet with mgmt_dmx
void mgmt_rx(unsigned char *pkt, int len, unsigned char *aux, struct face_s *f)
{
  // check if face == lora   => only allow mgmt over LoRa
  if (memcmp(f->name, (char *) "lora", 4)) { return; }

  Serial.printf("mgmt_rx: message = %s\r\n", to_hex(pkt, len, 0));

  // remove DMX
  pkt += DMX_LEN;
  len -= DMX_LEN;

  // compute mac
  len -= MGMT_MAC_FIELD_LEN;
  unsigned char hash[crypto_auth_hmacsha512_BYTES];
  crypto_auth_hmacsha512(hash, pkt, len, MGMT_KEY);
  // copy received mac to computed mac
  memcpy(hash, pkt + len, MGMT_MAC_FIELD_LEN);
  // verify hmac
  if (crypto_auth_hmacsha512_verify(hash, pkt, len, MGMT_KEY) != 0) {
    Serial.printf("mgmt_rx verification of hmac failed\r\n");
    return;
  }
  // get message type
  unsigned char typ = pkt[0];
  pkt += 1;
  len -= 1;
  // get src id
  unsigned char src[MGMT_ID_LEN];
  memcpy(src, pkt, MGMT_ID_LEN);
  pkt += MGMT_ID_LEN;
  len -= MGMT_ID_LEN;
  Serial.printf("mgmt_rx got packet with type '%c' from %s\r\n", typ, to_hex(src, MGMT_ID_LEN, 0));
  // check if we sent this
  if (!memcmp(src, mgmt_id, MGMT_ID_LEN)) { return; }
  // check if node is already in fcnt table
  int ndx = -1;
  for (int i = 0; i < mgmt_fcnt_table_cnt; i++) {
    if (!memcmp(src, mgmt_fcnt_table[i].id, MGMT_ID_LEN)) {
      ndx = i;
    }
  }
  // new node, check if table not full
  if (ndx == -1) {
    if (mgmt_fcnt_table_cnt < MGMT_FCNT_TABLE_SIZE) {
      ndx = mgmt_fcnt_table_cnt++;
      memcpy(&mgmt_fcnt_table[ndx].id, src, MGMT_ID_LEN);
    } else {
      Serial.printf("mgmt_rx fcnt table is full, skipping...\r\n");
      return;
    }
  }
  // existing node -> validate fcnt
  len -= MGMT_FCNT_FIELD_LEN;
  unsigned int fcnt = (uint32_t)pkt[len+3] << 24 |
                       (uint32_t)pkt[len+2] << 16 |
                       (uint32_t)pkt[len+1] << 8  |
                       (uint32_t)pkt[len];
  Serial.printf("mgmt_rx received fcnt = %d, last fcnt = %d\r\n", fcnt, mgmt_fcnt_table[ndx].fcnt);
  if (fcnt <= mgmt_fcnt_table[ndx].fcnt) { return; }
  // update fcnt
  mgmt_fcnt_table[ndx].fcnt = fcnt;
  // write fcnt-table to file
  mgmt_fcnt_table_log = MyFS.open(MGMT_FCNT_TABLE_LOG_FILENAME, "w");
  mgmt_fcnt_table_log.write((unsigned char*) &mgmt_fcnt_table_cnt, sizeof(mgmt_fcnt_table_cnt));
  mgmt_fcnt_table_log.write((unsigned char*) &mgmt_fcnt_table, MGMT_LOG_FCNT_LEN * MGMT_FCNT_TABLE_SIZE);
  mgmt_fcnt_table_log.close();


  // receive beacon
  if (typ == 'b' && len == 0) {
    Serial.println(String("mgmt_rx received beacon from ") + to_hex(src, MGMT_ID_LEN, 0));
    return;
  }

  // receive key update
  if (typ == 'k' && len == MGMT_MSG_KEY_LEN) {
    struct msg_key_s *key_update = (struct msg_key_s*) calloc(1, MGMT_MSG_KEY_LEN);
    memcpy(key_update, pkt, MGMT_MSG_KEY_LEN);
    Serial.printf("mgmt_rx: %s key %s\r\n", key_update->op ? "allow" : "deny", to_hex(key_update->key, GOSET_KEY_LEN, 0));
    mgmt_rx_key(key_update);
    free(key_update);
    io_send(pkt - MGMT_ID_LEN - 1 - DMX_LEN, len + DMX_LEN + 1 + MGMT_ID_LEN + MGMT_MAC_FIELD_LEN + MGMT_FCNT_FIELD_LEN, NULL);
    return;
  }

  // receive status
  if (typ == 's' && len == MGMT_MSG_STATUS_LEN) {
    mgmt_rx_status(pkt, len, src);
    return;
  }

  // receive request
  if (typ == 'r' && len == MGMT_MSG_REQUEST_LEN) {
    struct msg_request_s *request = (struct msg_request_s*) calloc(1, MGMT_MSG_REQUEST_LEN);
    memcpy(request, pkt, MGMT_MSG_REQUEST_LEN);
    if (memcmp(request->dst, mgmt_id, MGMT_ID_LEN || request->all == true)) {
      io_send(pkt - MGMT_ID_LEN - 1 - DMX_LEN, len + DMX_LEN + 1 + MGMT_ID_LEN + MGMT_MAC_FIELD_LEN + MGMT_FCNT_FIELD_LEN, NULL);
    }
    if (!memcmp(request->dst, mgmt_id, MGMT_ID_LEN) || request->all == true) {
      mgmt_rx_request(request, len);
    }
    free(request);
    return;
  }

  // unknown typ
  Serial.printf("mgmt_rx t=%c ??\r\n", typ);
}



// -----------------------------------------------------------------------------
// STATUS TABLE

// print status table entry
void _print_status(state_s* state, unsigned char* id, unsigned long int received_on = NULL, unsigned char* src = NULL, unsigned long int lastSeen = NULL)
{
  // id
  Serial.printf("  %s", to_hex(id, MGMT_ID_LEN, 0));
  // src (who said this)
  Serial.printf(" | %s", src == NULL ? "self" : to_hex(src, MGMT_ID_LEN, 0));
  // when was this information received
  int r = received_on == NULL ? 0 : millis() - received_on;
  int rs = (r / 1000) % 60;
  int rm = (r / 1000 / 60) % 60;
  int rh = (r / 1000 / 60 / 60) % 24;
  Serial.printf(" | %02d:%02d:%02d", rh, rm ,rs);
  // when was the node seen for the last time
  int l = r + (lastSeen == NULL ? 0 : lastSeen);
  int ls = (l / 1000) % 60;
  int lm = (l / 1000 / 60) % 60;
  int lh = (l / 1000 / 60 / 60) % 24;
  Serial.printf(" | %02d:%02d:%02d", lh, lm ,ls);
  // beacon
  Serial.printf(" | %6s", state->beacon == true ? "on" : "off");
  // voltage
  Serial.printf(" | %6dV", state->voltage);
  // feeds, entries & chunks
  int feeds = state->feeds;
  int entries = state->entries;
  int chunks = state->chunks;
  Serial.printf(" | %5d | %7d | %6d", feeds, entries, chunks);
  // free
  Serial.printf(" | %3d%%", state->free);
  // what uptime did it report when it was last seen
  int u = state->uptime;
  int us = (u / 1000) % 60;
  int um = (u / 1000 / 60) % 60;
  int uh = (u / 1000 / 60 / 60) % 24;
  int ud = u / 1000 / 60 / 60 / 24;
  Serial.printf(" | %4dd %2dh %2dm %2ds", ud, uh, um ,us);
  // latitude
  Serial.printf(" | %10f", state->latitude);
  // longitude
  Serial.printf(" | %10f", state->longitude);
  // altitude
  Serial.printf(" | %7dm", state->altitude);
  // neighbors
  Serial.printf(" | %9d", state->neighbors);
  // newline
  Serial.printf("\r\n");
}

// print the status table
void mgmt_print_statust()
{
  // header
  Serial.println("  id   | src  | received | lastSeen | beacon | battery | feeds | entries | chunks | free | uptime            | latitude   | longitude  | altitude | neighbors");
  Serial.printf("  ");
  for (int i = 0; i < 4; i++) { Serial.printf("-"); } // id
  for (int i = 0; i < 7; i++) { Serial.printf("-"); } // src
  for (int i = 0; i < 11; i++) { Serial.printf("-"); } // received
  for (int i = 0; i < 11; i++) { Serial.printf("-"); } // lastSeen
  for (int i = 0; i < 9; i++) { Serial.printf("-"); } // beacon
  for (int i = 0; i < 10; i++) { Serial.printf("-"); } // voltage
  for (int i = 0; i < 27; i++) { Serial.printf("-"); } // FEC
  for (int i = 0; i < 7; i++) { Serial.printf("-"); } // free
  for (int i = 0; i < 20; i++) { Serial.printf("-"); } // uptime
  for (int i = 0; i < 13; i++) { Serial.printf("-"); } // latitude
  for (int i = 0; i < 13; i++) { Serial.printf("-"); } // longitude
  for (int i = 0; i < 11; i++) { Serial.printf("-"); } // altitude
  for (int i = 0; i < 12; i++) { Serial.printf("-"); } // neighbors
  Serial.printf("\r\n");

  // self
  struct state_s *own = (struct state_s*) calloc(1, MGMT_STATE_LEN);
  memcpy(own, _mkStatus(), MGMT_STATE_LEN);
  _print_status(own, mgmt_id);
  free(own);

  // table entries
  for (int i = 0; i < statust_cnt; i++) {
    _print_status(&statust[i].entry.state, statust[i].entry.id, statust[i].entry.received_on, statust[i].entry.id, statust[i].entry.lastSeen);
    // neighbors
    for (int j = 0; j < statust[i].neighbor_cnt; j++) {
      _print_status(&statust[i].neighbors[j].state, statust[i].neighbors[j].id, statust[i].neighbors[j].received_on, statust[i].entry.id, statust[i].neighbors[j].lastSeen);
    }
  }

  // glossary
  Serial.printf("\r\n");
  Serial.printf(" src:      who said this\r\n");
  Serial.printf(" received: when was this information received\r\n");
  Serial.printf(" lastSeen: last time we heard from this node\r\n");
  Serial.printf(" uptime:   reported uptime when node was last seen\r\n");
}

// remove stale entries from status table
void mgmt_statust_housekeeping()
{
  while (true) {
    int old_cnt = statust_cnt;
    for (int i = 0; i < old_cnt; i++) {
      if (millis() - statust[i].entry.received_on > STATUST_EOL) {
        if (i < old_cnt - 1) {
          memcpy(&statust[i], &statust[old_cnt - 1], sizeof(struct statust_entry_s));
	}
	statust[old_cnt - 1] = (const struct statust_entry_s) { 0 };
	statust_cnt--;
	break;
      } else {
        // same for its neighbors
        while (true) {
          int old_neighbor_cnt = statust[i].neighbor_cnt;
          for (int j = 0; j < old_neighbor_cnt; j++) {
            if (millis() - statust[i].neighbors[j].received_on + statust[i].neighbors[j].lastSeen > STATUST_EOL) {
              if (i < old_neighbor_cnt - 1) {
                memcpy(&statust[i].neighbors[j], &statust[i].neighbors[old_neighbor_cnt - 1], sizeof(struct statust_local_s));
	      }
	      statust[i].neighbors[old_neighbor_cnt - 1] = (const struct statust_local_s) { 0 };
	      statust[i].neighbor_cnt--;
	      break;
            }
          }
          if (old_neighbor_cnt == statust[i].neighbor_cnt) { break; }
        }
      }
    }
    if (old_cnt == statust_cnt) { break; }
  }
}



// -----------------------------------------------------------------------------
// MGMT TX

// send mgmt packet
void mgmt_tx(unsigned char typ, unsigned char* payload, int payload_len)
{
  int len = DMX_LEN + 1 + MGMT_ID_LEN + payload_len + MGMT_FCNT_FIELD_LEN + MGMT_MAC_FIELD_LEN;
  unsigned char message[len] = { 0 };
  int offset = 0;
  // add DMX
  memcpy(message + offset, dmx->mgmt_dmx, DMX_LEN);
  offset += DMX_LEN;
  // add typ
  int typ_len = 1;
  message[offset] = typ;
  offset += typ_len;
  // add src
  memcpy(message + offset, mgmt_id, MGMT_ID_LEN);
  offset += MGMT_ID_LEN;
  // add payload
  if (payload_len > 0) {
    memcpy(message + offset, payload, payload_len);
    offset += payload_len;
  }
  // add fcnt
  memcpy(message + offset, (unsigned char*) &++mgmt_fcnt, MGMT_FCNT_FIELD_LEN);
  offset += MGMT_FCNT_FIELD_LEN;
  // write fcnt to flash
  mgmt_fcnt_log = MyFS.open(MGMT_FCNT_LOG_FILENAME, "w");
  mgmt_fcnt_log.write((unsigned char*) &mgmt_fcnt, sizeof(mgmt_fcnt));
  mgmt_fcnt_log.close();
  // add mac
  unsigned char hash[crypto_auth_hmacsha512_BYTES];
  crypto_auth_hmacsha512(hash, message + DMX_LEN, typ_len + MGMT_ID_LEN + payload_len + MGMT_FCNT_FIELD_LEN, MGMT_KEY);
  memcpy(message + offset, hash, MGMT_MAC_FIELD_LEN);
  offset += MGMT_MAC_FIELD_LEN;
  // send
  if (offset != len) { Serial.printf("mgmt_tx: final offset and length differ\r\n"); return; }
  Serial.printf("mgmt_tx: message = %s\r\n", to_hex(message, len, 0));
  io_send(message, len, NULL);
}

// send beacon
void mgmt_send_beacon()
{
  mgmt_tx('b', NULL, 0);
}

// send key
void mgmt_send_key(bool op, unsigned char* key)
{
  mgmt_tx('k', _mkKey(op, key), MGMT_MSG_KEY_LEN);
}

// send request to specified node (all if none)
void mgmt_send_request(unsigned char cmd, unsigned char* id=NULL)
{
  mgmt_tx('r', _mkRequest(cmd,id), MGMT_MSG_REQUEST_LEN);
}

// send status response (sent periodically or after request)
void mgmt_send_status()
{
  // housekeeping
  mgmt_statust_housekeeping();
  // send status
  mgmt_tx('s', _mkStatus(), MGMT_MSG_STATUS_LEN);
  // set next event
  mgmt_next_send_status = millis() + MGMT_SEND_STATUS_INTERVAL + random(5000);
}



//------------------------------------------------------------------------------
// SETUP & TICK

// called once on boot
void mgmt_setup()
{
  // initialize mgmt_dmx
  unsigned char h[32];
  crypto_hash_sha256(h, (unsigned char*) MGMT_DMX_STR, strlen(MGMT_DMX_STR));
  memcpy(dmx->mgmt_dmx, h, DMX_LEN);
  dmx->arm_dmx(dmx->mgmt_dmx, mgmt_rx, NULL);
  Serial.printf("   DMX for MGMT is %s\r\n", to_hex(dmx->mgmt_dmx, DMX_LEN, 0));
  // get id
  for (int i = 0; i < MGMT_ID_LEN; i++) {
    mgmt_id[i] = my_mac[6 - MGMT_ID_LEN + i];
  }
  // load fcnt // TODO check if file exists
  mgmt_fcnt_log = MyFS.open(MGMT_FCNT_LOG_FILENAME, "r");
  mgmt_fcnt_log.read((unsigned char*) &mgmt_fcnt, sizeof(mgmt_fcnt));
  mgmt_fcnt_log.close();
  // load fcnt-table  // TODO check if file exists
  mgmt_fcnt_table_log = MyFS.open(MGMT_FCNT_TABLE_LOG_FILENAME, "r");
  mgmt_fcnt_table_log.read((unsigned char*) &mgmt_fcnt_table_cnt, sizeof(mgmt_fcnt_table_cnt));
  mgmt_fcnt_table_log.read((unsigned char*) &mgmt_fcnt_table, MGMT_LOG_FCNT_LEN * MGMT_FCNT_TABLE_SIZE);
  mgmt_fcnt_table_log.close();
  // create directory for goset keys
  MyFS.mkdir(KEYS_DIR);
  // dump directory contents (DEBUG)
  File kdir = MyFS.open(KEYS_DIR);
  if (kdir) {
    // Serial.printf("mgmt_setup: listing goset keys\r\n");
    File k = kdir.openNextFile();
    while (k) {
      int cnt;
      k.read((unsigned char*) &cnt, sizeof(cnt));
      // Serial.printf("    %d %s\r\n", cnt, k.name());
      k.close();
      k = kdir.openNextFile();
    }
  }
  kdir.close();

  // Serial.println("mgmt setup done");
}

// called in main loop
void mgmt_tick()
{
  // periodically send status
  if (millis() > mgmt_next_send_status) {
    mgmt_send_status();
  }
  // check if beacon is active
  if (mgmt_beacon && millis() > next_mgmt_send_beacon) {
    mgmt_send_beacon();
    next_mgmt_send_beacon = millis() + MGMT_SEND_BEACON_INTERVAL + random(2000);
  }

  // TODO have some 'hourly' broadcast so the highest goset key counter eventually reaches all nodes
  // issues:
  //   * 4B counter + 32B key
  //     128B - 4B mac - 4B fcnt - 7B dmx - 1B type - 2B src = 110B payload
  //     111B / 36B = we can send three entries per packet
  //
}

// eof

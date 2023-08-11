// cmd.h

// tinySSB for ESP32
// Aug 2022 <christian.tschudin@unibas.ch>

// --------------------------------------------------------------------------

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  int  cnt = 0;
  while (file) {
    cnt++;
    if (file.isDirectory()) {
      Serial.printf("  DIR : %s\r\n", file.name());
      if (levels)
        listDir(fs, file.path(), levels -1);
    } else
      Serial.printf("  FILE: %s\tSIZE: %d\r\n", file.name(), file.size());
    file = root.openNextFile();
  }
  if (cnt == 0)
    Serial.printf("  EMPTY\r\n");
}

// --------------------------------------------------------------------------

void cmd_rx(String cmd) {
  cmd.toLowerCase();
  cmd.trim();
  Serial.printf("CMD %s\r\n\n", cmd.c_str());
  switch(cmd[0]) {
    case '?':
      Serial.println("  ?        help");
      Serial.println("  a        add new random key");
      Serial.println("  b+[id|*] turn beacon on on id/all");
      Serial.println("  b-[id|*] turn beacon off on id/all");
      Serial.println("  c        remove log files for fcnt & fcnt-table (local dev only!)");
      Serial.println("  d        dump DMXT and CHKT");
      Serial.println("  f        list file system");
      Serial.println("  h        list heatmap file");
      Serial.println("  g        dump GOset");
      Serial.println("  i        pretty print the confIg values");
      Serial.println("  k+<key>  add new key (globally)");
      Serial.println("  k-<key>  remove key (globally)");
#if defined(LORA_LOG)
      Serial.println("  l        list log file");
      Serial.println("  m        empty log file");
#endif
      Serial.println("  r[id|*]  reset this repo to blank / request reset from id/all");
      Serial.println("  s[id|*]  status / request status from id/all");
      Serial.println("  x[id|*]  reboot / request reboot from id/all");
      Serial.println("  z[N]     zap (feed with index N) on all nodes");
      break;
    case 'a': { // inject new key
      unsigned char key[GOSET_KEY_LEN];
      for (int i=0; i < sizeof(key)/4; i++) {
        unsigned int r = esp_random();
        memcpy(key + 4*i, (unsigned char*) &r, sizeof(4));
      }
      theGOset->add(key);
      theGOset->dump();
      break;
    }
    case 'b': // beacon
      if (!(cmd[1] == '+' or cmd[1] == '-')) { Serial.printf("invalid command: %s\n", cmd[1]); break; }
      if (cmd[2] == '*') {
        Serial.printf("sending request to turn %s beacon to all nodes\r\n", cmd[1] == '+' ? "on" : "off");
        mgmt_send_request(cmd[1]);
      } else if (cmd.length() == 2 * MGMT_ID_LEN + 2) {
	char idHex[2 * MGMT_ID_LEN];
	for (int i = 0; i < 2 * MGMT_ID_LEN; i++) { idHex[i] = cmd[i+2]; }
	unsigned char *id = from_hex(idHex, MGMT_ID_LEN);
        Serial.printf("sending request to turn %s beacon to %s\r\n", cmd[1] == '+' ? "on" : "off", to_hex(id, MGMT_ID_LEN, 0));
        mgmt_send_request(cmd[1], id);
      }
      break;
    case 'c': // fcnt
      Serial.println("Deleting logs of fcnt & fcnt-table...");
      MyFS.remove(MGMT_FCNT_LOG_FILENAME);
      MyFS.remove(MGMT_FCNT_TABLE_LOG_FILENAME);
      esp_restart();
      break;
    case 'd': // dump
      // goset_dump(theGOset);
      Serial.println("Installed feeds:");
      for (int i = 0; i < repo->rplca_cnt; i++) {
        unsigned char *key = theGOset->get_key(i);
        Serial.printf("  %d %s, next_seq=%d\r\n", i, to_hex(key, 32, 0), repo->fid2replica(key)->get_next_seq(NULL));
      }
      Serial.printf("DMX table: (%d entries)\r\n", dmx->dmxt_cnt);
      for (int i = 0; i < dmx->dmxt_cnt; i++) {
        unsigned char *dmx_val = dmx->dmxt[i].dmx;
        Serial.printf("  %s", to_hex(dmx_val, DMX_LEN));
        unsigned char *fid = dmx->dmxt[i].fid;
        if (fid != NULL) {
          int ndx = theGOset->_key_index(fid);
          if (ndx >= 0)
            Serial.printf("  %d.%d\r\n", ndx, dmx->dmxt[i].seq);
        } else {
          char *d = "?";
          if (!memcmp(dmx_val, dmx->goset_dmx, DMX_LEN))
            d = "<GOset>";
          else if (!memcmp(dmx_val, dmx->want_dmx, DMX_LEN))
            d = "<WANT>";
          else if (!memcmp(dmx_val, dmx->chnk_dmx, DMX_LEN))
            d = "<CHNK>";
          else if (!memcmp(dmx_val, dmx->mgmt_dmx, DMX_LEN))
            d = "<MGMT>";
          Serial.printf("  %s\r\n", d);
        }
      }
      Serial.printf("CHUNK table: (%d entries)\r\n", dmx->blbt_cnt);
      for (int i = 0; i < dmx->blbt_cnt; i++) {
        struct blb_s *bp = dmx->blbt + i;
        Serial.printf("  %s ", to_hex(bp->h, HASH_LEN, 0));
        for (struct chain_s *tp = bp->front; tp; tp = tp->next) {
          int ndx = theGOset->_key_index(tp->fid);
          Serial.printf(" %d.%d.%d", ndx, tp->seq, tp->cnr);
        }
        Serial.printf("\r\n");
      }
      break;
    case 'f': // Directory dump
      Serial.printf("File system: %d total bytes, %d used\r\n",
                    MyFS.totalBytes(), MyFS.usedBytes());
      listDir(MyFS, "/", 2); // FEED_DIR, 2);
      break;
    case 'g': // GOset dump
      Serial.printf("GOset: %d entries\r\n", theGOset->goset_len);
      for (int i = 0; i < theGOset->goset_len; i++)
        Serial.printf("%2d %s\r\n", i,
                      to_hex(theGOset->get_key(i), GOSET_KEY_LEN, 0));
      break;
    case 'i': { // config values
      // FIXME: we should not print the mgmt signing key to the console ?
      String s = bipf2String(the_config, "\r\n", 0);
      Serial.printf("Configuration values:\r\n%s\r\n", s.c_str());
      break;
    }
    case 'k': { // allow/deny key
      if (cmd.length() != 2 * GOSET_KEY_LEN + 2) { Serial.printf("invalid key length\r\n"); break; }
      if (!(cmd[1] == '+' or cmd[1] == '-')) { Serial.printf("invalid command: %s\r\n", cmd[1]); break; }
      char keyHex[2 * GOSET_KEY_LEN];
      for (int i = 0; i < 2 * GOSET_KEY_LEN; i++) { keyHex[i] = cmd[i+2]; }
      unsigned char *key = from_hex(keyHex, GOSET_KEY_LEN);
      Serial.printf("sending request to %s key %s globally\r\n", cmd[1] == '+' ? "allow" : "deny", to_hex(key, GOSET_KEY_LEN, 0));
      mgmt_send_key(cmd[1] == '+' ? true : false, key);
      break;
    }
#if defined(LORA_LOG)
  case 'h': // list heatmap file
      hm_log.close();
      hm_log = MyFS.open(HEATMAP_FILENAME, FILE_READ);
      while (hm_log.available()) {
        Serial.write(hm_log.read());
      }
      hm_log.close();
      hm_log = MyFS.open(HEATMAP_FILENAME, FILE_APPEND);
      break;
  case 'l': // list Log file
      lora_log.close();
      lora_log = MyFS.open(LORA_LOG_FILENAME, FILE_READ);
      while (lora_log.available()) {
        Serial.write(lora_log.read());
      }
      lora_log.close();
      lora_log = MyFS.open(LORA_LOG_FILENAME, FILE_APPEND);
      break;
  case 'm': // empty Log file
      lora_log.close();
      lora_log = MyFS.open(LORA_LOG_FILENAME, FILE_WRITE);
      break;
#endif
    case 'r': // reset
      if (cmd[1] == '*') {
        Serial.printf("sending reset request to all nodes\r\n");
        mgmt_send_request('r');
      } else if (cmd.length() == 2 * MGMT_ID_LEN + 1) {
	char idHex[2 * MGMT_ID_LEN];
	for (int i = 0; i < 2 * MGMT_ID_LEN; i++) { idHex[i] = cmd[i+1]; }
	unsigned char *id = from_hex(idHex, MGMT_ID_LEN);
        Serial.printf("sending reset request to %s\r\n", to_hex(id, MGMT_ID_LEN, 0));
        mgmt_send_request('r', id);
      } else {
        repo->reset(NULL);
        Serial.println("reset done");
      }
      break;
    case 's': // send status request
      if (cmd[1] == '*') {
        Serial.printf("sending status request to all nodes\r\n");
        mgmt_send_request('s');
      } else if (cmd.length() == 2 * MGMT_ID_LEN + 1) {
	char idHex[2 * MGMT_ID_LEN];
	for (int i = 0; i < 2 * MGMT_ID_LEN; i++) { idHex[i] = cmd[i+1]; }
	unsigned char *id = from_hex(idHex, MGMT_ID_LEN);
        Serial.printf("sending status request to %s\r\n", to_hex(id, MGMT_ID_LEN, 0));
        mgmt_send_request('s', id);
      } else {
        Serial.println("printing status ...\n");
        mgmt_print_statust();
      }
      break;
    case 'x': // reboot
      if (cmd[1] == '*') {
        Serial.printf("sending reboot request to all nodes\r\n");
        mgmt_send_request('x');
      } else if (cmd.length() == 2 * MGMT_ID_LEN + 1) {
	char idHex[2 * MGMT_ID_LEN];
	for (int i = 0; i < 2 * MGMT_ID_LEN; i++) { idHex[i] = cmd[i+1]; }
	unsigned char *id = from_hex(idHex, MGMT_ID_LEN);
        Serial.printf("sending reboot request to %s\r\n", to_hex(id, MGMT_ID_LEN, 0));
        mgmt_send_request('x', id);
      } else {
        lora_log.printf(">> reboot cmd\n");
        lora_log.close();
        Serial.println("rebooting ...\n");
        esp_restart();
      }
      break;
    case 'z': { // zap
      Serial.println("zap protocol started ...\n");
      int ndx = -1;
      if (cmd[1] != '\0')
        ndx = atoi(cmd.c_str()+1);
      Serial.println("zap1");
      theGOset->do_zap(ndx);
      Serial.println("zap2");
      Serial.println();
      break;
    }
    default:
      Serial.println("unknown command");
      break;
  }
  Serial.println();
}

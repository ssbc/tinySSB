// cmd.h

// tinySSB for ESP32
// 2022-2023 <christian.tschudin@unibas.ch>

// --------------------------------------------------------------------------

void cmd_rx(String cmd) {
  cmd.toLowerCase();
  cmd.trim();
  Serial.printf("CMD %s\r\n\n", cmd.c_str());
  switch(cmd[0]) {
    case '?':
      Serial.println("  ?        help");
      Serial.println("  d        dump GoSET, DMXT and CHKT");
      Serial.println("  f        list file system");
      Serial.println("  i        pretty print the confIg values");
#if defined(LORA_LOG)
      Serial.println("  h        list heatmap file");
      Serial.println("  l        list log file");
      Serial.println("  m        empty log file");
#endif
      Serial.println("  r        reset this repo to blank");
      Serial.println("  x        reboot");
      break;
    case 'd': // dump
      // goset_dump(theGOset);
      Serial.println("Installed feeds:");
      for (int i = 0; i < theRepo->rplca_cnt; i++) {
        unsigned char *key = theGOset->get_key(i);
        Serial.printf("  %d %s, next_seq=%d\r\n", i, to_hex(key, 32, 0), theRepo->fid2replica(key)->get_next_seq(NULL));
      }
      Serial.printf("DMX table: (%d entries)\r\n", theDmx->dmxt_cnt);
      for (int i = 0; i < theDmx->dmxt_cnt; i++) {
        unsigned char *dmx_val = theDmx->dmxt[i].dmx;
        Serial.printf("  %s", to_hex(dmx_val, DMX_LEN));
        unsigned char *fid = theDmx->dmxt[i].fid;
        if (fid != NULL) {
          int ndx = theGOset->_key_index(fid);
          if (ndx >= 0)
            Serial.printf("  %d.%d\r\n", ndx, theDmx->dmxt[i].seq);
        } else {
          char *d = "?";
          if (!memcmp(dmx_val, theDmx->goset_dmx, DMX_LEN))
            d = "<GOset>";
          else if (!memcmp(dmx_val, theDmx->want_dmx, DMX_LEN))
            d = "<WANT>";
          else if (!memcmp(dmx_val, theDmx->chnk_dmx, DMX_LEN))
            d = "<CHNK>";
          else if (!memcmp(dmx_val, theDmx->mgmt_dmx, DMX_LEN))
            d = "<MGMT>";
          Serial.printf("  %s\r\n", d);
        }
      }
      Serial.printf("CHUNK table: (%d entries)\r\n", theDmx->blbt_cnt);
      for (int i = 0; i < theDmx->blbt_cnt; i++) {
        struct blb_s *bp = theDmx->blbt + i;
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
    case 'i': { // config values
      // FIXME: we should not print the mgmt signing key to the console ?
      String s = bipf2String(the_config, "\r\n", 0);
      Serial.printf("Configuration values:\r\n%s\r\n", s.c_str());
      break;
    }
#if 0 && defined(LORA_LOG)
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
      theRepo->reset(NULL); // does not return
      Serial.println("reset done");
      break;
    case 'x': // reboot
      Serial.printf(">> reboot cmd\r\n");
      /*
      lora_log.close();
      Serial.println("rebooting ...\n");
      */
      esp_restart();
      break;
    default:
      Serial.println("unknown command");
      break;
  }
  Serial.println();
}

// eof

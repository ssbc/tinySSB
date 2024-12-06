package nz.scuttlebutt.tremolavossbol.tssb

import android.util.Log
import nz.scuttlebutt.tremolavossbol.MainActivity
import nz.scuttlebutt.tremolavossbol.crypto.SodiumAPI.Companion.sha256
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.DMX_LEN
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.FID_LEN
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.GOSET_DMX
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.byteArrayCmp
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.fromEncodedUByte
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toEncodedUByte
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toHex
import kotlin.experimental.xor

class Claim {
    var typ: Byte = 'c'.toByte()
    var lo = ByteArray(0) // FID_LEN)
    var hi = ByteArray(0) // FID_LEN)
    var xo = ByteArray(0) // FID_LEN)
    var sz = 0
    var wire = ByteArray(0)
}

class Novelty {
    var typ: Byte = 'n'.toByte()
    var key = ByteArray(FID_LEN)
    var wire = ByteArray(0)
}

class GOset(val context: MainActivity) {
    /* packet format:
        n 32B 32B? 32B?  // 33 bytes, in the future up to two additional keys
        c 32B 32B 32B B  // 98 bytes
          lo  hi  xor cnt(4 bytes, big endian)
        z 32B(nonce)     // 33 bytes
    */

    val NOVELTY_LEN = 33 // sizeof(struct novelty_s)
    val CLAIM_LEN = 98   // sizeof(struct claim_s)
    val ZAP_LEN = 33     // sizeof(struct zap_s)

    val GOSET_KEY_LEN   = FID_LEN
    val GOSET_MAX_KEYS  =    100 //   MAX_FEEDS
    val GOSET_ROUND_LEN = 10000L // in millis
    val MAX_PENDING     =     20 // log2(maxSetSize) + density (#neighbors)
    val NOVELTY_PER_ROUND =    1
    val ASK_PER_ROUND   =      1
    val HELP_PER_ROUND  =      2
    val ZAP_ROUND_LEN   =   4500

    val goset_dmx = GOSET_DMX.sha256().sliceArray(0..DMX_LEN-1)

    // unsigned short version; ??
    // const char* fname;
    // int goset_len; // number of set elements
    var state = ByteArray(FID_LEN)
    val keys = ArrayList<ByteArray>(0)
    var pending_claims = ArrayList<Claim>(0)
    val pending_novelty = ArrayDeque<Novelty>(0)
    var largest_claim_span = 0
    var novelty_credit = 1
    // unsigned long next_round; // FIXME: handle, or test, wraparound
    // struct zap_s zap;
    // unsigned long zap_state;
    // unsigned long zap_next;

    fun key2ndx(k: ByteArray): Int {
        return keys.indexOf(k)
    }

    fun rx(pkt: ByteArray, aux: ByteArray?) {
        // Log.d("goset", "rx ${pkt.size}")

        if (pkt.size <= DMX_LEN)
            return
        val buf = pkt.sliceArray(DMX_LEN .. pkt.lastIndex)

        if (buf.size == NOVELTY_LEN && buf[0] == 'n'.toByte()) {
            _add_key(buf.sliceArray(1..NOVELTY_LEN-1))
            return
        }
        /*
        if (pkt[0] == 'z' && len == ZAP_LEN) {
            Serial.println("ZAP message received");
            unsigned long now = millis();
            if (gp->zap_state == 0) {
                Serial.println("ZAP phase I starts");
                memcpy(&gp->zap, pkt, ZAP_LEN);
                gp->zap_state = now + ZAP_ROUND_LEN;
                gp->zap_next = now;
            }
            return;
        }
        */
        if (buf.size != CLAIM_LEN || buf[0] != 'c'.toByte())
            return;
        val cl = mkClaim(buf)

        if (cl.sz > largest_claim_span)
            largest_claim_span = cl.sz
        if (context.tinyRepo.isLoaded())
            context.wai.eval("refresh_goset_progressbar(${keys.size}, ${largest_claim_span})") // notify frontend
        // Log.d("rx", "state = ${state.toHex()}, cl.sz=${cl.sz}, k.sz=${keys.size}")
        if (cl.sz == keys.size && byteArrayCmp(state, cl.xo) == 0) {
            Log.d("goset", "seems we are synced (with at least someone), |GOset|=${keys.size}")
        } else {
            val i1 = keys.indexOf(cl.lo)
            val i2 = keys.indexOf(cl.hi)
            if (i2-i1 == 1 && cl.sz == 3) {
                for (i in cl.xo.indices) {
                    cl.xo[i] = cl.xo[i] xor cl.lo[i]
                    cl.xo[i] = cl.xo[i] xor cl.hi[i]
                }
                Log.d("goset", "computed the mussung middle key ${cl.xo.toHex()}")
                _add_key(cl.xo)
            } else {
                _add_key(cl.lo)
                _add_key(cl.hi)
                _add_pending_claim(cl)
            }
        }
        // Log.d("goset", "goset size=${keys.size}, pending_claims.size=${pending_claims.size}")
    }

    fun loop() {
        while (true) {
            beacon()
            Thread.sleep(GOSET_ROUND_LEN)
        }
    }

    fun beacon() { // called in regular intervals
        /*
        unsigned long now = millis();
        if (gp->zap_state != 0) {
            if (now > gp->zap_state + ZAP_ROUND_LEN) { // two rounds after zap started
            Serial.println("ZAP phase II ended, resetting now");
            repo_reset();
        }
            if (now < gp->zap_state && now > gp->zap_next) { // phase I
            Serial.println("sending zap message");
            io_send(_mkZap(gp), DMX_LEN + ZAP_LEN, NULL);
            gp->zap_next = now + 1000;
        }
        }
        */
        // Log.d("GOset", "beacon")
        if (keys.size == 0 || !context.tinyRepo.isLoaded()) return
        while (novelty_credit-- > 0 && pending_novelty.size > 0)
            context.tinyIO.enqueue(pending_novelty.removeFirst().wire, goset_dmx, null)
        novelty_credit = NOVELTY_PER_ROUND

        val cl = mkClaim(0, keys.size-1)
        if (byteArrayCmp(cl.xo, state) != 0) { // GOset changed
            Log.d("goset", "state change to ${cl.xo.toHex()}, |keys|=${keys.size}")
            state = cl.xo
            context.tinyDemux.set_want_dmx(state)
        }
        context.tinyIO.enqueue(cl.wire, goset_dmx, null);

        // sort pending entries, smallest first
        pending_claims.sortBy({c -> c.sz})
        var max_ask = ASK_PER_ROUND
        var max_help = HELP_PER_ROUND

        val retain = ArrayList<Claim>()
        for (c in pending_claims) {
            Log.d("goset", "pending claim loop")
            if (c.sz == 0) continue // ignore bogous claims
            var lo = keys.indexOfFirst({k -> byteArrayCmp(k,c.lo) == 0})
            var hi = keys.indexOfFirst({k -> byteArrayCmp(k,c.hi) == 0})
            if (lo == -1 || hi == -1 || lo > hi) continue // ignore bogous claims
            val partial = mkClaim(lo, hi)
            if (byteArrayCmp(partial.xo, c.xo) == 0) continue // match, hence no reason to ask others
            if (partial.sz <= c.sz) { // ask for help, starting with the smallest entry, for a limited number of questions
                if (max_ask-- > 0) {
                    // Serial.print("asking for help " + String(partial->cnt));
                    // Serial.print(String(" ") + to_hex(partial->lo,4) + String(".."));
                    // Serial.println(String(" ") + to_hex(partial->hi,4) + String(".."));
                    context.tinyIO.enqueue(partial.wire, goset_dmx, null);
                }
                if (partial.sz < c.sz) {
                    retain.add(c)
                    continue;
                }
            }
            if (max_help-- > 0) { // we have larger claim span, offer help (but limit # of claims)
                hi--; lo++
                // Log.d("goset","offer help span= " + String(partial->cnt - 2));
                // Serial.print(String(" ") + to_hex(gp->goset_keys+lo*GOSET_KEY_LEN,4) + String(".."));
                // Serial.println(String(" ") + to_hex(gp->goset_keys+hi*GOSET_KEY_LEN,4) + String(".."));
                Log.d("goset", "max_help ${lo}, ${hi}")
                if (hi <= lo)
                    context.tinyIO.enqueue(mkNovelty_from_key(keys[lo]).wire, goset_dmx, null)
                else if (hi - lo <= 2) { // span of 2 or 3
                    context.tinyIO.enqueue(mkClaim(lo, hi).wire, goset_dmx, null)
                }
                else { // split span in two intervals
                    val sz = (hi+1 - lo) / 2
                    context.tinyIO.enqueue(mkClaim(lo, lo+sz-1).wire, goset_dmx, null)
                    context.tinyIO.enqueue(mkClaim(lo+sz, hi).wire, goset_dmx, null)
                }
                continue; // only help once per claim, do not retain
            }
            retain.add(c) // we could not treat the claim in this round, hence retain
        }
        // make room for new claims
        while (retain.size >= MAX_PENDING-5)
            retain.removeLast()
        pending_claims = retain
        // Serial.println(String(gp->pending_c_cnt,DEC) + " pending claims, GOset size is " + String(gp->goset_len, DEC));
        // for (int i = 0; i < gp->pending_c_cnt; i++)
        // Serial.println("  pend claim span=" + String(gp->pending_claims[i].cnt, DEC)
        // + " " + to_hex(gp->pending_claims[i].xo,32));
    }

    fun _include_key(key: ByteArray): Boolean {
        val zero = ByteArray(GOSET_KEY_LEN)
        if (key.contentEquals(zero))
            return false
        for (k in keys)
            if (k.contentEquals(key)) {
                // Log.d("goset", "key ${key.toHex()} already exists")
                return false
            }
        if (keys.size >= GOSET_MAX_KEYS) {
            Log.d("goset", "too many keys: ${keys.size}")
            return false
        }
        Log.d("goset","_include_key ${key.toHex()}, keys.size was ${keys.size}")
        keys.add(key)
        return true
    }

    fun _add_key(key: ByteArray, trusted: Boolean = false, alias: String =""): String {
        if (!_include_key(key)) // adds key if necessary
            return ""
        //Set trusted
        context.tinyRepo.add_replica(key, trusted, alias)

        keys.sortWith({a:ByteArray,b:ByteArray -> byteArrayCmp(a,b)})
        if (keys.size >= largest_claim_span) { // only rebroadcast if we are up to date
            val n = mkNovelty_from_key(key)
            if (novelty_credit-- > 0)
                context.tinyIO.enqueue(n.wire, goset_dmx, null)
            else if (pending_novelty.size < MAX_PENDING)
                pending_novelty.add(n)
        }
        context.ble?.refreshShortNameForKey(key) // refresh shortname in devices overview
        //Set
        Log.d("goset", "added key ${key.toHex()}, |keys|=${keys.size}")
        if (!trusted) {
            //add entry to json file with the hex
            context.saveContactToJSONFile(key.toHex(), 24)
        }
        return key.toHex()
    }

    fun remove_key(key: ByteArray) {
        for (k in keys) {
            if (k.contentEquals(key)) {
                keys.remove(k)
                Log.d("goset", "removed key ${key.toHex()}, |keys|=${keys.size}")
                break
            }
        }
    }

    fun _add_pending_claim(cl: Claim) {
        for (c in pending_claims)
            if (c.sz == cl.sz && c.xo.contentEquals(cl.xo))
                return
        pending_claims.add(cl)
    }

    fun adjust_state() {
        keys.sortWith({a:ByteArray,b:ByteArray -> byteArrayCmp(a,b)})
        if (keys.size > 0) {
            val cl = mkClaim(0, keys.size-1)
            state = cl.xo
        } else
            state = ByteArray(FID_LEN)
        Log.d("goset", "adjust state for ${keys.size} keys, resulted in ${state.toHex()}")
        context.tinyDemux.set_want_dmx(state)
    }

    fun mkClaim(pkt: ByteArray): Claim {
        val cl = Claim()
        cl.lo = pkt.sliceArray(1..32)
        cl.hi = pkt.sliceArray(33..64)
        cl.xo = pkt.sliceArray(65..96)
        cl.sz = pkt[97].fromEncodedUByte()
        cl.wire = pkt
        return cl
    }

    fun mkClaim(lo: Int, hi: Int): Claim {
        val cl = Claim()
        cl.lo = keys[lo]
        cl.hi = keys[hi]
        cl.xo = _xor(lo, hi)
        cl.sz = hi - lo + 1
        val b = cl.sz.toEncodedUByte()
        cl.wire = byteArrayOf(cl.typ) + cl.lo + cl.hi + cl.xo + byteArrayOf(b)
        return cl
    }

    fun mkNovelty(pkt: ByteArray): Novelty {
        val n = Novelty()
        n.key = pkt.sliceArray(1..33)
        n.wire = pkt
        return n
    }

    fun mkNovelty_from_key(key: ByteArray): Novelty {
        val n = Novelty()
        n.wire = byteArrayOf('n'.toByte()) + key
        n.key = key
        return n
    }

    fun _xor(lo: Int, hi: Int): ByteArray {
        val xor = keys[lo].clone()
        for (k in keys.subList(lo+1,hi+1)) // argh, in Kotlin the toIndex is EXCLUSIVE, unlike in a range
            for (i in 0..FID_LEN-1)
                xor[i] = xor[i] xor k[i]
        // Log.d("xor", "lo=${keys[lo].toHex()}, cnt=${keys.size}")
        // Log.d("xor", "hi=${keys[hi].toHex()}")
        // Log.d("xor", "xo=${xor.toHex()}")
        return xor
    }

}
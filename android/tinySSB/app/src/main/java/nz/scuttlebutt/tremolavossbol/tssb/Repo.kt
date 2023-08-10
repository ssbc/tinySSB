package nz.scuttlebutt.tremolavossbol.tssb

import android.content.Context.MODE_PRIVATE
import android.util.Log
import nz.scuttlebutt.tremolavossbol.MainActivity
import nz.scuttlebutt.tremolavossbol.crypto.SodiumAPI.Companion.sha256
import nz.scuttlebutt.tremolavossbol.crypto.SodiumAPI.Companion.signDetached
import nz.scuttlebutt.tremolavossbol.crypto.SodiumAPI.Companion.verifySignDetached
import nz.scuttlebutt.tremolavossbol.utils.Bipf.Companion.varint_decode
import nz.scuttlebutt.tremolavossbol.utils.Bipf.Companion.varint_encode
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.DMX_LEN
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.DMX_PFX
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.FID_LEN
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.HASH_LEN
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.PKTTYPE_chain20
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.PKTTYPE_plain48
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_DIR
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_PKT_LEN
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.decodeHex
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toBase64
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toByteArray
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toHex
import java.io.File
import java.io.RandomAccessFile

class Feed(fid: ByteArray) {
    val fid = fid
    // var seq = 0
    var next_seq = 1
    var prev_hash = fid.sliceArray(0..HASH_LEN-1)
    var max_prev_seq = 0
}

// tremola: [fid,mid,xrf,app,dat]
// raw       32B,20B,20B,2B,X
// enc       64B,40B,40B,2B,X
// msg       body=BIPF(XRF~20B,APP~2B,??~DAT)
class LogTinyEntry(fid: ByteArray, seq: Int, mid: ByteArray, body: ByteArray) {
    val fid = fid // author
    val seq = seq
    val mid = mid // msg hash
    val body = body // Bipf(APP,XRF,OTHERDATA)
}

class Repo(val context: MainActivity) {
    val FEED_DIR = "feeds"
    val feeds = ArrayList<Feed>()

    fun _feed_index(fid: ByteArray): Int {
        var i = 0
        for (f in feeds) {
            if (f.fid.contentEquals(fid)) return i
            i++
        }
        return -1
    }

    fun fid2rec(fid: ByteArray, createIfNeeded: Boolean =false): Feed? {
        for (f in feeds)
            if (f.fid.contentEquals(fid)) {
                // Log.d("repo", "use feed record ${f} next_seq=${f.next_seq}")
                return f
            }
        if (!createIfNeeded)
            return null
        // Log.d("repo", "create new feed ${fid.toHex()}")
        val frec = Feed(fid)
        feeds.add(frec)
        // Log.d("repo", "use NEW feed record ${frec}")
        return frec
    }

    fun _openFile(fid: ByteArray, mode: String, seq: Int = -1, blob: Int = -1): RandomAccessFile {
        // blob=-1: create filename for MID hash, blob=1: incomplete sidechain, blob=0: full sidechain
        val feeds_dir = File(context.getDir(TINYSSB_DIR, MODE_PRIVATE), FEED_DIR)
        val fdir = File(feeds_dir, fid.toHex())
        fdir.mkdirs()
        var fn: String
        if (blob == -1) {
            if (seq >= 0)
                fn = "mid"
            else
                fn = "log"
        } else if (blob > 0)
            fn = "!" + seq.toString()
        else
            fn = "-" + seq.toString()
        val f = File(fdir, fn)
        if (!f.exists()) {
            f.createNewFile()
            if (fn == "log")
                File(fdir, "mid").createNewFile()
        }
        return RandomAccessFile(f, mode)
    }

    fun delete_feed(fid: ByteArray) {
        val feeds_dir = File(context.getDir(TINYSSB_DIR, MODE_PRIVATE), FEED_DIR)
        val feed = File(feeds_dir, fid.toHex())
        if (feed.exists() && feed.isDirectory)
            for (f in feed.listFiles())
                f.deleteRecursively()
        val rec = fid2rec(fid)
        rec?.next_seq = 1
        rec?.prev_hash = fid.sliceArray(0 until HASH_LEN)
    }

    fun repo_clean(dir: File) {
        for (f in dir.listFiles()) {
            if (f.isDirectory)
                repo_clean(File(dir, f.name))
            f.delete()
        }
    }

    fun repo_reset() {
        repo_clean(context.getDir(TINYSSB_DIR, MODE_PRIVATE));
    }

    fun repo_load() {
        val fdir = File(context.getDir(TINYSSB_DIR, MODE_PRIVATE), FEED_DIR)
        fdir.mkdirs()
        for (f in fdir.listFiles()) {
            if (!f.isDirectory || f.name.length != 2* FID_LEN) continue
            val fid = f.name.decodeHex()
            var ndx = _feed_index(fid)
            if (ndx < 0) {
                ndx = feeds.size
                feeds.add(Feed(fid))
            }
            val frec = feeds[ndx] // feed record
            frec.next_seq = feed_len(fid) + 1
            for (g in f.listFiles()) { // move away from old style filename
                // (files starting with dot cannot be selected for uploading)
                if (g.name[0] == '.')
                    g.renameTo(File(f,"-" + g.name.substring(1..g.name.lastIndex)))
            }
            val m = File(f,"mid")
            if (!m.exists())
                m.createNewFile()
            if (m.length() == 0L && frec.next_seq != 1)
                Log.d("repo", "FIXME: must repair messageID file!")
            else if (m.length() >= HASH_LEN) {
                val raf = RandomAccessFile(m, "r")
                raf.seek(raf.length() - HASH_LEN)
                raf.read(frec.prev_hash)
            }
        }
        // FIXME: check next_seq for each feed, whether the log file (no suffix) has more log entries
        // and therefore we need to compute a new message id, create a new file with a seq nr suffix
        // ... while ((feeds[ndx].max_prev_seq+1) < feeds[ndx].next_seq) .. compute missing prev values, cycle files

        for (f in feeds) {
            Log.d("repo", "loaded feed ${f.fid.toHex()}")
            context.tinyGoset._include_key(f.fid)
        }
        context.tinyGoset.adjust_state()
    }

    fun new_feed(fid: ByteArray) {
        Log.d("repo", "create empty log")
        val fdir = File(context.getDir(TINYSSB_DIR, MODE_PRIVATE), FEED_DIR)
        val ldir = File(fdir, fid.toHex())
        ldir.mkdirs()
        try {
            repo_clean(ldir)
        } catch (e: Exception) {}
        // context.getDir(p, MODE_PRIVATE) // creates this dir if necessary
        File(ldir, "log").createNewFile() // create empty log file
        File(ldir, "mid").createNewFile() // create empty log file
        feeds.add(Feed(fid))
        if(context.isWaiInitialized())
            context.wai.eval("b2f_new_contact(\"@${fid.toBase64()}.ed25519\")") // notify frontend
    }

    fun feed_read_mid(fid: ByteArray, seq: Int): ByteArray? {
        if (seq < 1) return null
        val fdir = File(context.getDir(TINYSSB_DIR, MODE_PRIVATE), FEED_DIR)
        val f = RandomAccessFile(File(File(fdir, fid.toHex()), "mid"), "r")
        if (f.length()/HASH_LEN < seq) return null
        f.seek((HASH_LEN * (seq-1)).toLong())
        val buf = ByteArray(HASH_LEN)
        val sz = f.read(buf, 0, buf.size)
        f.close()
        return if (sz == buf.size) buf else null
    }

    fun feed_read_pkt(fid: ByteArray, seq: Int): ByteArray? {
        if (seq < 1) return null
        val f = _openFile(fid, "r", -1)
        // Log.d("repo", "read_pkt: file length ${f.length()}")
        if (f.length()/TINYSSB_PKT_LEN < seq) {
            f.close()
            return null
        }
        f.seek((TINYSSB_PKT_LEN * (seq-1)).toLong())
        val buf = ByteArray(TINYSSB_PKT_LEN)
        val sz = f.read(buf, 0, buf.size)
        f.close()
        if (sz != buf.size)
            Log.d("repo","could not read packet #${seq}")
        return if (sz == buf.size) buf else null
    }

    fun feed_read_chunk(fid: ByteArray, seq: Int, cnr: Int): ByteArray? {
        var f: RandomAccessFile? = null
        try {
            f = _openFile(fid, "r", seq, 0)
        } catch (e: Exception) {
            try {
                f = _openFile(fid, "r", seq, 1)
            } catch (e: Exception) {}
        }
        if (f == null) return null
        f.seek((TINYSSB_PKT_LEN * cnr).toLong())
        val buf = ByteArray(TINYSSB_PKT_LEN)
        val sz = f.read(buf, 0, buf.size)
        f.close()
        return if (sz == buf.size) buf else null
    }

    fun feed_read_content(fid: ByteArray, seq: Int): Pair<ByteArray?,ByteArray?> {
        val logEntry = feed_read_pkt(fid, seq)
        if (logEntry == null) return null to null
        val mid = feed_read_mid(fid, seq)
        if (mid == null) return null to null
        if (logEntry[DMX_LEN].toInt() == PKTTYPE_plain48)
            return logEntry.sliceArray(DMX_LEN+1..DMX_LEN+1+48-1) to mid
        if (logEntry[DMX_LEN].toInt() != PKTTYPE_chain20) return null to null
        val (sz, len) = varint_decode(logEntry, DMX_LEN+1, DMX_LEN+4)
        if (sz <= (28-len))
            return logEntry.sliceArray(DMX_LEN+1+len..DMX_LEN+1+len+sz-1) to mid
        var content = logEntry.sliceArray(DMX_LEN+1+len..DMX_LEN+1+28-1)
        val sidechain = _openFile(fid, "r", seq, 0)
        while (true) {
            val buf = ByteArray(TINYSSB_PKT_LEN)
            if (sidechain.read(buf) != TINYSSB_PKT_LEN)
                break
            content += buf.sliceArray(0..TINYSSB_PKT_LEN - HASH_LEN - 1)
        }
        sidechain.close()
        if (content.size < sz)
            return null to null
        if (content.size == sz)
            return content to mid
        Log.d("repo", "recovered sidechain content is ${content.sliceArray(0..sz-1).toHex()}")
        return content.sliceArray(0..sz-1) to mid
    }

    fun feed_len(fid: ByteArray): Int {
        val fdir = File(context.getDir(TINYSSB_DIR, MODE_PRIVATE), FEED_DIR)
        val f = File(File(fdir, fid.toHex()), "log")
        val cnt = f.length().toInt()/TINYSSB_PKT_LEN
        return cnt
    }

    fun mk_contentLogEntry(content: ByteArray): ByteArray? {
        val fid = context.idStore.identity.verifyKey
        val frec = fid2rec(fid, false)
        if (frec == null) {
            Log.d("node","unknown fid")
            return null
        }
        Log.d("repo", "creating content of length ${content.size} ${content.toHex()}")
        val sz_enc = varint_encode(content.size)
        val intro: ByteArray
        var ptr: ByteArray
        Log.d("repo", "varint size: ${sz_enc.size}")
        if (sz_enc.size + content.size <= 28) {
            intro = sz_enc + content + ByteArray(28 - sz_enc.size - content.size)
            ptr = ByteArray(HASH_LEN)
        } else {
            val i = 28 - sz_enc.size
            intro = sz_enc + content.sliceArray(0..i-1)
            var remaining: ByteArray? = content.sliceArray(i..content.lastIndex)
            val chunks = ArrayList<ByteArray>(0)
            ptr = ByteArray(HASH_LEN)
            while (remaining != null) {
                var len = remaining.size % 100
                if (len == 0) len = 100
                var pkt = remaining.sliceArray(remaining.size - len..remaining.lastIndex)
                pkt += ByteArray(100 - len) + ptr
                chunks.add(pkt)
                ptr = pkt.sha256().sliceArray(0..HASH_LEN - 1)
                if (len >= remaining.size)
                    remaining = null
                else
                    remaining = remaining.sliceArray(0..remaining.lastIndex - len)
            }
            Log.d("repo", "number of chunks= ${chunks.size}")
            if (chunks.size > 0) {
                val chain = _openFile(fid, "rw", frec.next_seq, 0)
                for (b in chunks.reversed())
                    chain.write(b)
                chain.close()
            }
        }
        Log.d("repo", "mkLogEntry |intro|=${intro.size} |ptr|=${ptr.size}")
        Log.d("repo", "mkLogEnty intro=${intro.toHex()}" )
        val nm0 = fid + frec.next_seq.toByteArray() + frec.prev_hash
        val dmx = context.tinyDemux.compute_dmx(nm0)
        val msg = dmx + ByteArray(1) {PKTTYPE_chain20.toByte()} + intro + ptr
        val me = context.idStore.identity
        // don't append here (although we presisted the chunks)
        return msg + signDetached(DMX_PFX + nm0 + msg, me.signingKey!!)
    }

    fun feed_append(fid: ByteArray, pkt: ByteArray): Boolean {
        var ndx = _feed_index(fid)
        if (ndx < 0) {
            Log.d("repo", "no such feed ${fid.toHex()}")
            return false
        }
        // check dmx
        val seq = feeds[ndx].next_seq
        Log.d("repo", "append seq=${seq}, pkt= ${pkt.toHex()}")
        val nm0 = fid + seq.toByteArray() + feeds[ndx].prev_hash
        val dmx = context.tinyDemux.compute_dmx(nm0)
        if (!pkt.sliceArray(0..DMX_LEN-1).contentEquals(dmx)) { // wrong dmx field
            Log.d("repo", " DMX mismatch")
            return false
        }
        // check signature
        val buf = DMX_PFX + nm0 + pkt
        // pkt + 56, buf, strlen(DMX_PFX) + FID_LEN + 4 + HASH_LEN + 56, fid);
        if (!verifySignDetached(pkt.sliceArray(56..pkt.lastIndex),
                buf.sliceArray(0..buf.lastIndex-64), fid)) {
            Log.d("repo", "ed25519 signature verification failed")
            return false
        }
        Log.d("repo", "  writing pkt to log (seq=${seq})")
        var f = _openFile(fid, "rw")
        f.seek(f.length())
        f.write(pkt)
        f.close()
        // prev hash data
        val h = buf.sha256().sliceArray(0..HASH_LEN-1)
        feeds[ndx].prev_hash = h
        if (feeds[ndx].next_seq >= 1) { // should always be the case
            f = _openFile(fid, "rw", 1) //
            f.seek(f.length())
            f.write(h)
            f.close()
        }
        val d = File(File(context.getDir(TINYSSB_DIR, MODE_PRIVATE), FEED_DIR),fid.toHex())
        if (feeds[ndx].next_seq >= 2) { // above replaces old MID file
            File(d, "+" + (feeds[ndx].next_seq - 1).toString()).delete()
        }
        //xx
        feeds[ndx].next_seq++
        Log.d("repo", "append next_seq now ${feeds[ndx].next_seq}")


        if (pkt[DMX_LEN].toInt() == PKTTYPE_plain48) {
            Log.d("repo", "plain 48")
            val e = LogTinyEntry(fid, seq, h, pkt.sliceArray(DMX_LEN + 1..DMX_LEN + 1 + 48 - 1))
            context.wai.sendTinyEventToFrontend(e)
        }
        if (pkt[DMX_LEN].toInt() == PKTTYPE_chain20) {
            val (sz, len) = varint_decode(pkt, DMX_LEN + 1, DMX_LEN + 4)
            // Log.d("repo", "load sidechain of length ${sz}B")
            Log.d("repo", "sz: ${sz}")
            Log.d("repo", "len: ${len}")
            if (sz <= 28 - len) {
                val content = pkt.sliceArray(DMX_LEN + 1 + len..DMX_LEN + 1 + len + sz - 1)
                val e = LogTinyEntry(fid, seq, h, content)
                Log.d("repo", "Logentry content:${content.toHex()} ")
                context.wai.sendTinyEventToFrontend(e)
                // File(d,"-"+seq.toString()).createNewFile()
            } else {
                if (File(d, "-" + seq.toString()).exists()) {
                    val (content,mid) = feed_read_content(fid, seq)
                    if (content != null) {
                        Log.d("repo", "Logentry content > 48:${content} ")
                        val e = LogTinyEntry(fid, seq, mid!!, content)
                        context.wai.sendTinyEventToFrontend(e)
                    }
                } else {
                    File(d, "!" + seq.toString()).createNewFile()
                    Log.d("repo", "Logentry wait for sidechain ")
                    // and wait until the sidechain has been loaded to announce it to the frontend
                }
            }
        }

        context.tinyDemux.arm_dmx(pkt.sliceArray(0..DMX_LEN-1)); // remove old DMX handler
        // install handler for next pkt:
        val new_dmx = context.tinyDemux.compute_dmx(fid + feeds[ndx].next_seq.toByteArray()
                                                    + feeds[ndx].prev_hash)
        val fct = { buf: ByteArray, fid: ByteArray?, _: String? -> context.tinyNode.incoming_pkt(buf,fid!!) }
        context.tinyDemux.arm_dmx(new_dmx, fct, fid)

        return true
    }

    fun sidechain_append(buf: ByteArray, blbt_ndx: Int) {
        Log.d("repo", "sidechain append")
        val b = context.tinyDemux.chkt[blbt_ndx]
        val f = _openFile(b.fid!!, "rw", b.seq,1)
        f.seek(f.length())
        f.write(buf)
        f.close()
        var i = TINYSSB_PKT_LEN - HASH_LEN - 1 // are we at the end of the chain_?
        while (i < TINYSSB_PKT_LEN) // check for a all-zero-hash
            if (buf[i++].toInt() != 0)
                break
        if (i == TINYSSB_PKT_LEN) { // end of chain reached, rename file
            val ldir = File(File(context.getDir(TINYSSB_DIR, MODE_PRIVATE), FEED_DIR), b.fid!!.toHex())
            val g = File(ldir, "!" + b.seq.toString())
            g.renameTo(File(ldir, "-" + b.seq.toString())) // new naming style
            val (content,mid) = feed_read_content(b.fid!!, b.seq)
            if (content != null) {
                val e = LogTinyEntry(b.fid!!, b.seq, mid!!, content)
                context.wai.sendTinyEventToFrontend(e)
            }
        } else { // chain extends, install next chunk handler
            val h = buf.sliceArray(TINYSSB_PKT_LEN - HASH_LEN..buf.lastIndex)
            val fct = { chunk: ByteArray, b_ndx: Int -> context.tinyNode.incoming_chunk(chunk,b_ndx) }
            context.tinyDemux.arm_blb(h, fct, b.fid, b.seq, b.bnr + 1)
        }
        context.tinyDemux.arm_blb(b.h!!) // remove old BLB handler for this packet
    }

    fun listFeeds(): Array<ByteArray> {
        val kset = context.tinyGoset.keys
        val a = Array<ByteArray>(kset.size) { ByteArray(0) }
        for (i in 0..a.size-1)
            a[i] = kset[i]
        return a
    }
}

// eof

package nz.scuttlebutt.tremolavossbol.tssb

import android.util.AtomicFile
import android.util.Log
import nz.scuttlebutt.tremolavossbol.MainActivity
import nz.scuttlebutt.tremolavossbol.crypto.SodiumAPI.Companion.sha256
import nz.scuttlebutt.tremolavossbol.crypto.SodiumAPI.Companion.signDetached
import nz.scuttlebutt.tremolavossbol.crypto.SodiumAPI.Companion.verifySignDetached
import nz.scuttlebutt.tremolavossbol.utils.Bipf
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.DMX_LEN
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.DMX_PFX
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.HASH_LEN
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.PKTTYPE_chain20
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.PKTTYPE_plain48
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_PKT_LEN
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toByteArray
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toHex
import java.io.File
import java.io.RandomAccessFile
import kotlin.math.min

class Pending(var cnr: Int, var rem: Int, var hptr: ByteArray, var pos: Int) {

    fun toList(): ArrayList<Any> {
       return arrayListOf(cnr, rem, hptr, pos)
    }

    companion object {
        fun fromList(lst: ArrayList<Any>): Pending {
            val cnr = lst[0] as Int
            val rem = lst[1] as Int
            val hptr = lst[2] as ByteArray
            val pos = lst[3] as Int
            return Pending(cnr, rem, hptr, pos)
        }
    }


}

class State {

    var max_seq = 0
    var max_pos = 0
    var prev = ByteArray(20)
    var pend_sc = mutableMapOf<Int, Pending>()

    private fun toDict(): MutableMap<String, *> {
        val dict = mutableMapOf<String, Any?>()
        dict["max_seq"] = max_seq
        dict["max_pos"] = max_pos
        dict["prev"] = prev
        val it = pend_sc.mapValues { it.value.toList() }
        dict["pend_sc"] = pend_sc.mapValues { it.value.toList() }
        return dict
    }

    fun toWire(): ByteArray {
        val wire = toDict()
        Log.d("to_wire", "wire_maxpos: ${Bipf.bipf_loads(Bipf.encode(Bipf.mkDict(toDict()))!!)!!.get()}")
        return Bipf.encode(Bipf.mkDict(toDict()))!!
    }

    fun loadFromWire(wire: ByteArray) {
        val dict = Bipf.bipf_loads(wire)?.getDict() as MutableMap<String, Any?>
        max_seq = dict["max_seq"] as Int
        max_pos = dict["max_pos"] as Int
        prev = dict["prev"] as ByteArray
        pend_sc = (dict["pend_sc"] as Map<Int, ArrayList<Any>>).mapValues { Pending.fromList(it.value) }.toMutableMap()
    }
}

class Replica(val context: MainActivity, val datapath: File, val fid: ByteArray) {

    private val path = File(datapath, fid.toHex())
    private val log = File(path, "log.bin")
    private val fnt = AtomicFile(File(path, "frontier.bin"))
    private val mid = File(path, "mid.bin")

    var state = State()

    init {
        path.mkdirs()
        log.createNewFile()
        mid.createNewFile()
        if (fnt.baseFile.createNewFile()) {
            persist_frontier(0, 0, fid.sliceArray(0 until 20))
        }
        state.loadFromWire(fnt.readFully())
        Log.d("replica", "loaded prev: ${state.prev.toHex()} (fid: ${fid.toHex()}), length: ${log.length()}, max pos: ${state.max_pos}")
        while (log.length() > state.max_pos) {
            RandomAccessFile(log, "rwd").use { f ->
                var pos = state.max_pos
                Log.d("replica", "init pos: $pos")
                f.seek(pos.toLong())
                val pkt = ByteArray(TINYSSB_PKT_LEN)
                f.read(pkt)
                val seq = state.max_seq + 1
                val nam = DMX_PFX + fid + seq.toByteArray() + state.prev
                val dmx = context.tinyDemux.compute_dmx(nam)
                if (!dmx.contentEquals(pkt.sliceArray(0 until DMX_LEN))) { // TODO isAuthor check (like in the simplepub implementation)
                    f.setLength(pos.toLong())
                    return@use
                }

                var chunk_cnt = 0
                if (pkt[7].toInt() == PKTTYPE_chain20) {
                    var (sz, len) = Bipf.varint_decode(pkt, DMX_LEN + 1, DMX_LEN + 4)
                    len -= 48 - 20 - sz
                    val ptr = pkt.sliceArray(36 until 56)
                    chunk_cnt = (len + 99) / 100
                    if (chunk_cnt > 0) {
                        state.pend_sc[seq] = Pending(0, chunk_cnt, ptr, pos + TINYSSB_PKT_LEN)
                    }

                }
                while (chunk_cnt > 0) {
                    f.write(ByteArray(TINYSSB_PKT_LEN))
                    chunk_cnt--
                }
                f.write(pos.toByteArray())
                pos = f.filePointer.toInt()
                persist_frontier(seq, pos, (nam + pkt).sha256().sliceArray(0 until HASH_LEN))
            }
        }
    }

    fun persist_frontier(seq: Int, pos: Int, prev: ByteArray) {
        state.max_seq = seq
        state.max_pos = pos
        Log.d("persist_frontier", "pos: $pos")
        state.prev = prev
        val f = fnt.startWrite()
        f.write(state.toWire())
        fnt.finishWrite(f)
    }

    fun ingest_entry_pkt(pkt: ByteArray, seq: Int): Boolean {
        Log.d("Replica", "ingest_entry_pkt")
        var sendToFront = false
        if (pkt.size != TINYSSB_PKT_LEN) return false
        if (seq != state.max_seq + 1) {
            Log.d("Replica", "ingest_entry: wrong seq nr")
            return false
        }
        val nam = DMX_PFX + fid + seq.toByteArray() + state.prev
        val dmx = context.tinyDemux.compute_dmx(nam)
        if (!dmx.contentEquals(pkt.sliceArray(0 until DMX_LEN))) {
            Log.d("Replica", "ingest_entry: dmx mismatch")
            return false
        }
        // TODO
        if (!verifySignDetached(
                pkt.sliceArray(56..pkt.lastIndex),
                nam + pkt.sliceArray(0 until 56),
                fid
            )
        ) {
            Log.d("Replica", "ingest_entry: signature verify failed")
            return false
        }
        Log.d("Replica", "DEBUG REP")
        var chunk_cnt = 0
        if (pkt[7].toInt() == PKTTYPE_chain20) {
            var (len, sz) = Bipf.varint_decode(pkt, DMX_LEN + 1, DMX_LEN + 4)
            len -= 48 - 20 - sz
            chunk_cnt = (len + 99) / 100
        }
        Log.d("Replica", "DEBUG REP 2")
        var log_entry = pkt + ByteArray(chunk_cnt * TINYSSB_PKT_LEN)
        log_entry += state.max_pos.toByteArray()
        log.appendBytes(log_entry)
        Log.d("Replica", "DEBUG REP 3, chunk_cnt: $chunk_cnt")
        if (chunk_cnt > 0) {
            val ptr = pkt.sliceArray(36 until 56)
            state.pend_sc[seq] = Pending(0, chunk_cnt, ptr, state.max_pos + TINYSSB_PKT_LEN)
            val chunk_fct = { chunk: ByteArray, fid: ByteArray?, seq: Int -> context.tinyNode.incoming_chunk(chunk,fid,seq) }
            context.tinyDemux.arm_blb(ptr, chunk_fct, fid, seq, 0)
        } else { // no sidechain, entry is complete
            sendToFront = true
        }

        mid.appendBytes((nam + pkt).sha256().sliceArray(0 until HASH_LEN))
        val pos = state.max_pos + log_entry.size
        persist_frontier(seq, pos, (nam + pkt).sha256().sliceArray(0 until HASH_LEN))

        context.tinyDemux.arm_dmx(dmx) // remove old dmx handler
        // arm dmx handler for next entry
        val new_nam = DMX_PFX + fid + (seq + 1).toByteArray() + (nam + pkt).sha256().sliceArray(0 until HASH_LEN)
        val new_dmx = context.tinyDemux.compute_dmx(new_nam)
        val fct = { buf: ByteArray, fid: ByteArray?, _: String? -> context.tinyNode.incoming_pkt(buf,fid!!) }
        context.tinyDemux.arm_dmx(new_dmx, fct, fid)

        if(sendToFront)
            context.wai.sendTinyEventToFrontend(fid, seq, (nam + pkt).sha256().sliceArray(0 until HASH_LEN), read(seq)!!)
        return true
    }

    fun ingest_chunk_pkt(pkt: ByteArray, seq: Int): Boolean {
        Log.d("replica", "ingest_chunk-pkt")
        if (pkt.size != TINYSSB_PKT_LEN) return false
        val pend: Pending
        var pos: Int
        try {
            pend = state.pend_sc[seq]!!
            if(pend.hptr.contentEquals(pkt.sha256().sliceArray(0 until HASH_LEN)))
                return false
            RandomAccessFile(log, "rwd").use { f ->
                f.seek(pend.pos.toLong())
                f.write(pkt)
                pos = f.filePointer.toInt()
            }
            context.tinyDemux.arm_blb(pend.hptr) // remove old chnk handler
        } catch (e: Exception) {
            return false
        }
        if (pend.rem <= 1) { //sidechain is complete
            state.pend_sc.remove(seq)
            val content = read(seq)
            val message_id = get_mid(seq)
            Log.d("ingest_chnk", "sidechain complete, content: ${content?.toHex()}, mid: ${message_id?.toHex()}")
            if (message_id != null && content != null) {
                context.wai.sendTinyEventToFrontend(fid, seq, message_id, content)
            }
        } else {
            pend.cnr++
            pend.rem--
            pend.hptr = pkt.sliceArray(pkt.size - 20..pkt.lastIndex)
            pend.pos = pos
            Log.d("ingest_chnk", "arm new chnk dmx, cnr: ${pend.cnr}, rem: ${pend.rem}")
            val chunk_fct = { chunk: ByteArray, fid: ByteArray?, seq: Int -> context.tinyNode.incoming_chunk(chunk,fid,seq) }
            context.tinyDemux.arm_blb(pend.hptr, chunk_fct, fid, seq, pend.cnr)
        }
        val f = fnt.startWrite()
        f.write(state.toWire())
        fnt.finishWrite(f)


        return true
    }

    fun get_mid(seq: Int): ByteArray? {
        Log.d("get_mid", "for seq: $seq")
        if(seq < 1 || seq > state.max_seq)
            return null
        val pos = (seq - 1) * HASH_LEN
        if (pos >= mid.length())
            return null
        val buf = ByteArray(HASH_LEN)

        RandomAccessFile(mid, "r").use {f ->
            f.seek(pos.toLong())
            f.read(buf)
        }
        return buf

    }

    fun get_next_seq(): Pair<Int, ByteArray> {
        val seq = state.max_seq + 1
        val nam = DMX_PFX + fid + seq.toByteArray() + state.prev
        return Pair(seq, nam.sha256().sliceArray(0 until DMX_LEN))
    }

    fun get_open_chains(cursor: Int = 0): MutableMap<Int, Pending> { // FIXME: Unused Parameter cursor
        return state.pend_sc
    }

    fun get_entry_pkt(seq: Int): ByteArray? {
        try {
            Log.d("get_entry", "seq: $seq, max: ${state.max_seq}")
            if(seq < 1 || seq > state.max_seq)
                return null
            var pos = log.length()
            var cnt = state.max_seq - seq + 1
            RandomAccessFile(log, "rwd").use { f ->
                while (cnt > 0) {
                    f.seek(pos - 4)
                    pos = f.readInt().toLong()
                    cnt--
                }
                f.seek(pos)
                val buf = ByteArray(TINYSSB_PKT_LEN)
                f.read(buf)
                return buf
            }

        } catch (e: Exception) {
            return null
        }
    }

    fun get_content_len(seq: Int): Pair<Int, Int>? {
        val pkt = get_entry_pkt(seq)
        if (pkt == null)
            return null
        if (pkt[7].toInt() == PKTTYPE_plain48)
            return Pair(48, 48)
        if (pkt[7].toInt() == PKTTYPE_chain20) {
            val (len, sz) = Bipf.varint_decode(pkt, DMX_LEN + 1, DMX_LEN + 4)
            if (!state.pend_sc.containsKey(seq))
                return Pair(len, len)
            val available = (48 - 20 - sz) + 100 * state.pend_sc[seq]!!.cnr
            return Pair(available, len)
        }
        return null
    }

    fun get_chunk_pkt(seq: Int, cnr: Int): ByteArray? {
        try {
            if(seq < 1 || seq > state.max_seq)
                return null
            if (state.pend_sc.containsKey(seq)) {
                if (cnr >= state.pend_sc[seq]!!.cnr)
                    return null
            }
            var pos = log.length()
            var cnt = state.max_seq - seq + 1
            var lim = pos
            RandomAccessFile(log, "rwd").use { f ->
                while (cnt > 0) {
                    f.seek(pos - 4)
                    lim = pos
                    pos = f.readInt().toLong()
                    cnt--
                }
                pos += TINYSSB_PKT_LEN * (cnr + 1)
                if (pos > lim - 120)
                    return null
                f.seek(pos)
                val buf = ByteArray(TINYSSB_PKT_LEN)
                f.read(buf)
                return buf
            }
        } catch (e: Exception) {
            return null
        }
    }

    fun read(seq: Int): ByteArray? {
        Log.d("read", "DEBUG, max seq: ${state.max_seq}, seq: $seq")
        if (state.max_seq < seq || seq < 1)
            return null
        Log.d("read", "DEBUG1.1")
        var pos = log.length()
        Log.d("read", "DEBUG1.2, pos: $pos")
        var cnt = state.max_seq - seq + 1
        RandomAccessFile(log, "rwd").use { f ->
            while (cnt > 0) {
                f.seek(pos - 4)
                pos = f.readInt().toLong()
                cnt--
            }
            Log.d("read", "DEBUG2")
            f.seek(pos)
            val pkt = ByteArray(TINYSSB_PKT_LEN)
            f.read(pkt)
            Log.d("read", "DEBUG3")
            if (pkt[7].toInt() == PKTTYPE_plain48)
                return pkt.sliceArray(8 until 56)
            if (pkt[7].toInt() == PKTTYPE_chain20) { //read whole sidechain
                Log.d("read", "DEBUG4")
                val (chain_len, sz) = Bipf.varint_decode(pkt, DMX_LEN + 1, DMX_LEN + 4)
                var content = pkt.sliceArray(8 + sz until 36)
                var blocks = (chain_len - content.size + 99) / 100
                while (blocks > 0) {
                    f.read(pkt)
                    content += pkt.sliceArray(0 until 100)
                    blocks--
                }
                return content
            }
            return null // unknown pkt type

        }
    }


    // TODO implement PKTTYPE_plain48
    fun write48(c: ByteArray): Int {
        var content = c
        if(log.length().toInt() != state.max_pos)
            return -1
        if (content.size < 48) {
            content += ByteArray(48-content.size) //TODO: Is this correct? (compare to simplepub code l. 237)
        } else {
            content = content.sliceArray(0 until 48)
        }
        val seq = state.max_seq + 1
        val nam = DMX_PFX + fid + seq.toByteArray() + state.prev
        val dmx = context.tinyDemux.compute_dmx(nam)
        val msg = dmx + ByteArray(1) { PKTTYPE_plain48.toByte()} + content
        val wire = msg + signDetached(nam + msg, context.idStore.identity.signingKey!!)
        if(wire.size != TINYSSB_PKT_LEN)
            return -1
        if(!verifySignDetached(
            wire.sliceArray(56..wire.lastIndex),
            nam + wire.sliceArray(0 until 56),
            fid
        ))
            return -1
        log.appendBytes(wire + state.max_pos.toByteArray())
        persist_frontier(seq, wire.size + 4, (nam +wire).sha256().sliceArray(0 until HASH_LEN))
        return seq
    }

    fun write(c: ByteArray): Int {
        var content = c
        if(log.length().toInt() != state.max_pos)
            return -1//
        val chunks = ArrayList<ByteArray>()
        val seq = state.max_seq + 1
        val sz = Bipf.varint_encode(content.size)
        var payload = sz + content.sliceArray(0 until min(28 - sz.size, content.size - 1))
        if (payload.size < 28) { //TODO: compare with simplepub code l. 260
            payload += ByteArray(28 - payload.size)
        }
        content = content.sliceArray(28 - sz.size .. content.lastIndex)
        var i = content.size % 100
        if (i > 0)
            content += ByteArray(100-i)
        var ptr = ByteArray(HASH_LEN)
        while (content.size > 0) {
            val buf = content.sliceArray(content.size - 100 .. content.lastIndex) + ptr
            chunks.add(buf)
            ptr = buf.sha256().sliceArray(0 until HASH_LEN)
            content = content.sliceArray(0 .. content.lastIndex - 100)
        }
        chunks.reverse()
        payload += ptr
        val nam = DMX_PFX + fid + seq.toByteArray() + state.prev
        val dmx = context.tinyDemux.compute_dmx(nam)

        Log.d("replica write", "dmx is ${dmx.toHex()}, chnk_cnt: ${chunks.size}")
        val msg = dmx + ByteArray(1) { PKTTYPE_chain20.toByte()} + payload
        var wire = msg + signDetached(nam + msg, context.idStore.identity.signingKey!!)
        if(wire.size != TINYSSB_PKT_LEN)
            return -1
        if(!verifySignDetached(
            wire.sliceArray(56..wire.lastIndex),
            nam + wire.sliceArray(0 until 56),
            fid
        ))
            return -1
        chunks.add(0, wire)
        var log_entry = chunks.reduce {acc, it -> acc + it}
        log_entry += state.max_pos.toByteArray()
        Log.d("size", "size before: ${log.length()}")
        log.appendBytes(log_entry)
        // save mid
        mid.appendBytes((nam + wire).sha256().sliceArray(0 until HASH_LEN))
        //Log.d("sizes", "sizes: dmx: ${dmx.size}, +1, payload: ${payload.size}, sign: ${signDetached(nam + msg, context.idStore.identity.signingKey!!).size}")
        persist_frontier(seq, state.max_pos + log_entry.size, (nam + wire).sha256().sliceArray(0 until HASH_LEN))
        Log.d("write", "new max pos: ${state.max_pos}")
        context.wai.sendTinyEventToFrontend(fid, seq, (nam + wire).sha256().sliceArray(0 until HASH_LEN), c)
        Log.d("replica", "write success, len: ${log_entry.size}")
        return seq
    }
}
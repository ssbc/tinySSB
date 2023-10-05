package nz.scuttlebutt.tremolavossbol.tssb

import android.content.Context.MODE_PRIVATE
import android.util.Log
import nz.scuttlebutt.tremolavossbol.MainActivity
import nz.scuttlebutt.tremolavossbol.utils.Bipf
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.DMX_PFX
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.FID_LEN
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.decodeHex
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toByteArray
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toHex
import java.io.File
import kotlin.random.Random

class Repo(val context: MainActivity) {
    val TINYSSB_DIR = "tinyssb"
    val FEED_DIR = "feeds"
    private val replicas = ArrayList<Replica>()
    private var want_is_valid = false
    private var chnk_is_valid = false
    private var want_offs = 0
    private var chnk_offs = 0

    private fun clean(dir: File) {
        for (f in dir.listFiles() ?: emptyArray()) {
            if (f.isDirectory)
                clean(File(dir, f.name))
            f.delete()
        }
    }

    fun delete_feed(fid: ByteArray) {
        clean(File(File(context.getDir(TINYSSB_DIR, MODE_PRIVATE), FEED_DIR), fid.toHex()))
    }

    fun reset() {
        val fdir = File(context.getDir(TINYSSB_DIR, MODE_PRIVATE), FEED_DIR)
        clean(fdir);
    }

    fun load() {
        val fdir = File(context.getDir(TINYSSB_DIR, MODE_PRIVATE), FEED_DIR)
        fdir.mkdirs()

        for (f in fdir.listFiles { file -> file.isDirectory && file.name.length == 2 * FID_LEN} ?: emptyArray()) {
            add_replica(f.name.decodeHex())
        }

    }

    fun add_replica(fid: ByteArray) {
        if (replicas.any { it.fid.contentEquals(fid) })
            return
        val new_r = Replica(context, File(context.getDir(TINYSSB_DIR, MODE_PRIVATE), FEED_DIR), fid)
        replicas.add(new_r)
        val seq = new_r.state.max_seq + 1
        val nam = DMX_PFX + fid + seq.toByteArray() + new_r.state.prev
        val dmx = context.tinyDemux.compute_dmx(nam)
        val fct = { buf: ByteArray, fid: ByteArray?, _: String? -> context.tinyNode.incoming_pkt(buf,fid!!) }
        context.tinyDemux.arm_dmx(dmx, fct, fid)

        val chains = new_r.get_open_chains()
        val chunk_fct = { chunk: ByteArray, fid: ByteArray?, seq: Int -> context.tinyNode.incoming_chunk(chunk,fid,seq) }
        for ((seq, p) in chains) {
            context.tinyDemux.arm_blb(p.hptr, chunk_fct,fid,seq, p.cnr)
        }

        if (context.tinyGoset.keys.size > 1) {
            want_offs = Random.nextInt(0, context.tinyGoset.keys.size - 1)
            chnk_offs = Random.nextInt(0, context.tinyGoset.keys.size - 1)
        }

        want_is_valid = false
        chnk_is_valid = false

    }

    fun fid2replica(fid: ByteArray): Replica? {
        return replicas.find { it.fid.contentEquals(fid) }
    }

    fun feed_read_pkt(fid: ByteArray, seq: Int): ByteArray? {
        val r = fid2replica(fid)
        if (r == null)
            return null
        return r.get_entry_pkt(seq)
    }

    fun feed_read_chunk(fid: ByteArray, seq: Int, cnr: Int): ByteArray? {
        val r = fid2replica(fid)
        if (r == null)
            return null
        return r.get_chunk_pkt(seq, cnr)
    }

    fun feed_append(fid: ByteArray, buf: ByteArray): Boolean {
        val r = fid2replica(fid)
        if (r == null) {
            return false
        }

        val success = r.ingest_entry_pkt(buf, r.state.max_seq + 1)
        if (success)
            want_is_valid = false
        return success
    }

    fun mk_logEntry(buf: ByteArray): Int {
        val r = fid2replica(context.idStore.identity.verifyKey)
        if (r == null)
            return -1
        return r.write(buf)
    }

    fun sidechain_append(buf: ByteArray, fid:ByteArray, seq: Int): Boolean {
        val r = fid2replica(fid)
        if (r == null)
            return false
        return r.ingest_chunk_pkt(buf, seq)
    }

    fun mk_want_vect(): ByteArray? {
        want_is_valid = false // not implemented yet
        if (want_is_valid) return null

        val lst = ArrayList<Int>()
        var v = ""
        var encoding_len = Bipf.encodingLength(want_offs)

        lst.add(want_offs)
        var i = 0
        while (i < context.tinyGoset.keys.size) {
            val ndx = (want_offs + i) % context.tinyGoset.keys.size
            val fid = context.tinyGoset.keys[ndx]
            val r = fid2replica(fid)
            if (r == null) {
                i++
                continue
            }
            val (ns, ndmx) = r.get_next_seq()
            encoding_len += Bipf.encode(Bipf.mkInt(ns))!!.size
            lst.add(ns)
            v += (if (v.length == 0) "[ " else " ") + "$ndx.$ns"
            i++
            if (encoding_len > 100)
                break
        }
        want_offs = (want_offs + i + 1) % context.tinyGoset.keys.size
        want_is_valid = true
        if(lst.size > 1) {
            return Bipf.encode(Bipf.mkList(lst))
        }
        return null
    }

    fun mk_chnk_vect(): ByteArray? {
        chnk_is_valid = false // not implemented yet
        if (chnk_is_valid) return null

        val lst = ArrayList<Any>()
        var v = ""
        var encoding_len = Bipf.encodingLength(chnk_offs)

        //lst.add(chnk_offs)
        var i = 0
        while (i < context.tinyGoset.keys.size) {
            val ndx = (chnk_offs + i) % context.tinyGoset.keys.size
            val fid = context.tinyGoset.keys[ndx]
            val r = fid2replica(fid)
            val pending = r?.get_open_chains()
            if (pending == null || pending.isEmpty()) {
                i++
                continue
            }
            for ((seq, p) in pending) {
                val c = arrayListOf(ndx, seq, p.cnr)
                lst.add(c)
                Log.d("mk_chnk", "calc encoding len")
                encoding_len += Bipf.encodingLength(c)
                v += (if (v.length == 0) "[ " else " ") + "$ndx.$seq.${p.cnr}"
                if (encoding_len > 100)
                    break
            }
            i++
            if (encoding_len > 100)
                break
            Log.d("mk_chnk", "new feed")
            chnk_offs = (chnk_offs + i + 1) % context.tinyGoset.keys.size

        }
        chnk_is_valid = true
        if (lst.size > 0) {
            return Bipf.encode(Bipf.mkList(lst))
        }
        return null
    }

    fun listFeeds(): List<ByteArray> {
        return replicas.map { it -> it.fid }
    }

    fun feed_read_content(fid: ByteArray, seq: Int): ByteArray? {
        return fid2replica(fid)?.read(seq)
    }
}

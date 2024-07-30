package nz.scuttlebutt.tremolavossbol.tssb

import android.content.Context.MODE_PRIVATE
import android.util.Log
import nz.scuttlebutt.tremolavossbol.MainActivity
import nz.scuttlebutt.tremolavossbol.crypto.SodiumAPI.Companion.sha256
import nz.scuttlebutt.tremolavossbol.utils.Bipf
import nz.scuttlebutt.tremolavossbol.utils.Constants
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.DMX_PFX
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.FID_LEN
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_PKT_LEN
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.decodeHex
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toBase64
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toByteArray
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toHex
import java.io.File
import java.io.RandomAccessFile
import java.nio.file.Files
import java.nio.file.StandardCopyOption
import kotlin.random.Random

class Repo(val context: MainActivity) {
    val TINYSSB_DIR = "tinyssb"
    val FEED_DIR = "feeds"
    private var loadingFinished = false // indicates whether all replicas have already been loaded into the repo.
    private val replicas = ArrayList<Replica>()
    private var want_is_valid = false
    private var chnk_is_valid = false
    private var want_offs = 0
    private var chnk_offs = 0
    private var numberOfPendingChunks = 0

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
            context.tinyGoset._add_key(f.name.decodeHex())
        }

        loadingFinished = true

    }

    fun add_replica(fid: ByteArray) {
        if (replicas.any { it.fid.contentEquals(fid) })
            return
        val new_r = Replica(context, File(context.getDir(TINYSSB_DIR, MODE_PRIVATE), FEED_DIR), fid)
        replicas.add(new_r)
        val seq = new_r.state.max_seq + 1
        val nam = DMX_PFX + fid + seq.toByteArray() + new_r.state.prev
        val dmx = nam.sha256().sliceArray(0 until Constants.DMX_LEN)
        val fct = { buf: ByteArray, fid: ByteArray?, _: String? -> context.tinyNode.incoming_pkt(buf,fid!!) }
        context.tinyDemux.arm_dmx(dmx, fct, fid)

        val chains = new_r.get_open_chains()
        val chunk_fct = { chunk: ByteArray, fid: ByteArray?, seq: Int -> context.tinyNode.incoming_chunk(chunk,fid,seq) }
        for ((seq, p) in chains) {
            context.tinyDemux.arm_blb(p.hptr, chunk_fct,fid,seq, p.cnr)
            addNumberOfPendingChunks(p.rem)
        }



        if (context.tinyGoset.keys.size > 1) {
            want_offs = Random.nextInt(0, context.tinyGoset.keys.size - 1)
            chnk_offs = Random.nextInt(0, context.tinyGoset.keys.size - 1)
        }

        if(context.wai.frontend_ready) // was: isWaiInitialized()
            context.wai.eval("b2f_new_contact(\"@${fid.toBase64()}.ed25519\")") // notify frontend

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
        want_is_valid = false // TODO optimization not implemented yet
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




        if(lst.size > 1) {
            // notify frontend
            var vec = lst.slice(1 .. lst.lastIndex) // want_vector without offset
            val front = vec.subList(vec.size - lst[0], vec.size)
            val back = vec.subList(0, vec.size - lst[0])
            vec = front + back
            context.tinyNode.update_progress(vec, "me")

            return Bipf.encode(Bipf.mkList(lst))
        }

        want_offs = (want_offs + i + 1) % context.tinyGoset.keys.size
        want_is_valid = true
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
                encoding_len += Bipf.encodingLength(c)
                v += (if (v.length == 0) "[ " else " ") + "$ndx.$seq.${p.cnr}"
                if (encoding_len > 100)
                    break
            }
            i++
            if (encoding_len > 100)
                break
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
        return fid2replica(fid)?.read_content(seq)
    }

    /**
     * Converts the old repo filesystem (used in version v0.1.5-alpha) to the current filesystem.
     * This method is crash resistant, e.g it only overwrites/deletes old files, if the upgrade was successful.
     *
     * It is assumed that the previous filesystem was correctly stored.
     *
     */
    fun upgrade_repo() {
        val dir = File(context.getDir(TINYSSB_DIR, MODE_PRIVATE), FEED_DIR)
        feediterate@ for (f in dir.listFiles()) {
            if (!f.isDirectory || f.name.length != 2* FID_LEN)
                continue
            val feed_dir = File(dir, f.name)
            val feed_files = feed_dir.listFiles()
            if(feed_files == null) { // no upgrade needed or unknown file system
                continue
            }
            // crashed after writing log file to disk, which can lead to missing mid.bin file
            if(feed_files.any { it.name == "log.bin" }) {
                if(!feed_files.any { it.name == "mid.bin" }) {
                    Files.move(File(feed_dir, "mid").toPath(), File(feed_dir, "mid.bin").toPath(), StandardCopyOption.ATOMIC_MOVE)
                    val sidechains = feed_files.filter { it.name.startsWith("!") || it.name.startsWith("-") }
                    sidechains.forEach { it.delete() }
                }
                continue // frontier, mid and log file are already successfully upgraded
            }
            val old_log = RandomAccessFile(File(feed_dir, "log"), "r")
            var buffer = ByteArray(TINYSSB_PKT_LEN)
            var pos = 0
            val new_log = File(feed_dir, "log.bin.tmp")

            if(new_log.exists()) // remove unsuccessful upgrade attempt and restart the upgrade
                new_log.delete()
            new_log.createNewFile()

            val pend = mutableMapOf<Int, Pending>()

            while(old_log.read(buffer) == TINYSSB_PKT_LEN) {
                pos += TINYSSB_PKT_LEN
                var chunk_cnt = 0
                val curr_seq = pos / TINYSSB_PKT_LEN
                if (buffer[7].toInt() == Constants.PKTTYPE_chain20) {
                    var (len, sz) = Bipf.varint_decode(buffer, Constants.DMX_LEN + 1, Constants.DMX_LEN + 4)
                    len -= 48 - 20 - sz
                    chunk_cnt = (len + 99) / 100
                }
                var logentry = buffer
                if (chunk_cnt > 0) {
                    val finishedSidechain = File(feed_dir, "-$curr_seq")
                    val openSidechain = File(feed_dir, "!$curr_seq")
                    if(finishedSidechain.exists()) {
                        logentry += finishedSidechain.readBytes()
                    } else if(openSidechain.exists()) {
                        val num_missing = chunk_cnt - (openSidechain.length() % TINYSSB_PKT_LEN).toInt()
                        val chain = openSidechain.readBytes()
                        logentry += chain + ByteArray((num_missing * TINYSSB_PKT_LEN))
                        pend[curr_seq] = Pending((openSidechain.length() % TINYSSB_PKT_LEN).toInt(), num_missing, chain.sliceArray(chain.size - 20 .. chain.lastIndex), new_log.length().toInt() + TINYSSB_PKT_LEN + openSidechain.length().toInt())
                    } else {
                        continue@feediterate //TODO handle corrupted filesystems (e.g. clear corrupted feeds)
                    }
                }
                logentry += new_log.length().toInt().toByteArray()
                new_log.appendBytes(logentry)
            }

            val new_state = State()
            val old_mid = File(feed_dir, "mid")

            new_state.max_pos = new_log.length().toInt()
            new_state.pend_sc = pend
            if (old_mid.exists() && old_mid.length() > 0)
                new_state.prev = old_mid.readBytes().sliceArray(old_mid.length().toInt() - 20 until old_mid.length().toInt())
            new_state.max_seq  = File(feed_dir, "log").length().toInt() / 120



            // log file successfully upgraded, persist changes
            val frontier = File(feed_dir, "frontier.bin")
            if (frontier.exists())
                frontier.delete()
            frontier.createNewFile()
            frontier.appendBytes(new_state.toWire())
            context.wai.frontend_frontier.edit().putInt(f.name, new_state.max_pos + 1).apply()

            Files.move(new_log.toPath(), File(feed_dir, "log.bin").toPath(), StandardCopyOption.ATOMIC_MOVE)
            File(feed_dir, "log").delete()
            Files.move(old_mid.toPath(), File(feed_dir, "mid.bin").toPath(), StandardCopyOption.ATOMIC_MOVE)


            val sidechains = feed_files.filter { it.name.startsWith("!") || it.name.startsWith("-") }
            sidechains.forEach { it.delete() }
        }

        // file containing the version number of the current repo filesystem
        val version_file = File(dir, "version")
        if(version_file.exists())
            version_file.delete()

        version_file.writeText("3fpf-0.0.1")

    }

    fun isLoaded(): Boolean {
        return loadingFinished
    }

    fun addNumberOfPendingChunks(amount: Int) {
        numberOfPendingChunks += amount
        context.wai.eval("refresh_chunk_progressbar($numberOfPendingChunks)")
    }

    fun getNumberOfPendingCHunks(): Int {
        return numberOfPendingChunks
    }

}

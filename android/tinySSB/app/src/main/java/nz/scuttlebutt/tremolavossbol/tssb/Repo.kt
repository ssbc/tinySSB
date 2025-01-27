package nz.scuttlebutt.tremolavossbol.tssb

import android.content.Context.MODE_PRIVATE
import android.content.Intent
import android.os.Build
import android.util.Log
import androidx.annotation.RequiresApi
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import nz.scuttlebutt.tremolavossbol.MainActivity
import nz.scuttlebutt.tremolavossbol.crypto.SodiumAPI.Companion.sha256
import nz.scuttlebutt.tremolavossbol.tssb.ble.BleForegroundService
import nz.scuttlebutt.tremolavossbol.tssb.ble.ForegroundNotificationType
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

class Repo(val service: BleForegroundService) {
    val TINYSSB_DIR = "tinyssb"
    val FEED_DIR = "feeds"
    private var loadingFinished = false // indicates whether all replicas have already been loaded into the repo.
    private val replicas = ArrayList<Replica>()
    // private var want_is_valid = false
    // private var chnk_is_valid = false
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
        clean(File(File(service.getDir(TINYSSB_DIR, MODE_PRIVATE), FEED_DIR), fid.toHex()))
    }

    fun reset() {
        val fdir = File(service.getDir(TINYSSB_DIR, MODE_PRIVATE), FEED_DIR)
        clean(fdir);
    }

    fun load() {
        try {
            val fdir = File(service.getDir(TINYSSB_DIR, MODE_PRIVATE), FEED_DIR)
            fdir.mkdirs()
            Log.d("Repo", "Loading repo from ${fdir.absolutePath}")
            for (f in fdir.listFiles { file -> file.isDirectory && file.name.length == 2 * FID_LEN} ?: emptyArray()) {
                Log.d("Repo", "Loading feed ${f.name}")
                add_replica(f.name.decodeHex()) // TODO this happens just once
                try {
                    BleForegroundService.getTinyGoset()!!._add_key(f.name.decodeHex())
                } catch (e: Exception) {
                    Log.e("Repo", "Error getting TinyGotset and adding key: ${e.message}")
                }
            }
            Log.d("Repo", "Repo successfully loaded!")

            loadingFinished = true
        } catch (e: Exception) {
            Log.e("Repo", "Error loading repo: ${e.message}")
        }
    }

    fun add_replica(fid: ByteArray) {
        if (replicas.any { it.fid.contentEquals(fid) })
            return
        Log.d("Repo", "Adding replica for ${fid.toBase64()}")
        val new_r : Replica;
        try {
            new_r = Replica(service, File(service.getDir(TINYSSB_DIR, MODE_PRIVATE), FEED_DIR), fid)
        } catch (e: Exception) {
            Log.e("Repo", "Error creating replica: ${e.message}")
            return
        }
        Log.d("Repo", "About to call replicas.add(${new_r.datapath})")
        replicas.add(new_r)
        val seq = new_r.state.max_seq + 1
        val nam = DMX_PFX + fid + seq.toByteArray() + new_r.state.prev
        val dmx = nam.sha256().sliceArray(0 until Constants.DMX_LEN)
        val fct = { buf: ByteArray, fid: ByteArray?, _: String? -> BleForegroundService.getTinyNode()!!.incoming_pkt(buf,fid!!) }
        BleForegroundService.getTinyDemux()!!.arm_dmx(dmx, fct, fid)
        Log.d("Repo", "Demux armed for ${fid.toBase64()}")

        val chains = new_r.get_open_chains()
        val chunk_fct = { chunk: ByteArray, fid: ByteArray?, seq: Int -> BleForegroundService.getTinyNode()!!.incoming_chunk(chunk,fid,seq) }
        for ((seq, p) in chains) {
            BleForegroundService.getTinyDemux()!!.arm_blb(p.hptr, chunk_fct,fid,seq, p.cnr)
            addNumberOfPendingChunks(p.rem)
        }
        Log.d("Repo", "Replica added for ${fid.toBase64()}")


        if (BleForegroundService.getTinyGoset()!!.keys.size > 1) {
            want_offs = Random.nextInt(0, BleForegroundService.getTinyGoset()!!.keys.size - 1)
            chnk_offs = Random.nextInt(0, BleForegroundService.getTinyGoset()!!.keys.size - 1)
        }

        Log.d("Repo", "About to enter if for frontend_ready")
        if(BleForegroundService.getFrontendReady()) { // was: isWaiInitialized()
            //context.wai.eval("b2f_new_contact(\"@${fid.toBase64()}.ed25519\")") // notify frontend
            try {
                val intent = Intent(ForegroundNotificationType.EVALUATION.value)
                intent.putExtra("message", "b2f_new_contact(\"@${fid.toBase64()}.ed25519\")")
                LocalBroadcastManager.getInstance(service).sendBroadcast(intent)
            } catch (e: Exception) {
                Log.e("Repo", "Error notifying frontend: ${e.message}")
                Log.e("Repo", "b2f_new_contact(\"@${fid.toBase64()}.ed25519\")")
            }
        }
        // want_is_valid = false
        // chnk_is_valid = false
    }

    fun fid2replica(fid: ByteArray): Replica? {
        try {
            //Log.d("Repo", "Finding replica for ${fid.toBase64()}")
            //Log.d("Repo", replicas.toString())
            return replicas.find { it.fid.contentEquals(fid) }
        } catch (e: Exception) {
            Log.e("Repo", "Error finding replica: ${e.message}")
            return null
        }
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
        // if (success)
        //     want_is_valid = false
        return success
    }

    fun mk_logEntry(buf: ByteArray): Int {
        try {
            Log.d("Repo", BleForegroundService.getTinyIdStore()!!.identity.verifyKey.toBase64())
            val r = fid2replica(BleForegroundService.getTinyIdStore()!!.identity.verifyKey)
            Log.d("Repo", "Retrieved replica?")
            if (r == null) {
                Log.d(
                    "Repo",
                    "Retrieved replica is null"
                )
                return -1
            }
            Log.d("Repo", "Writing log entry")
            return r.write(buf)
        } catch (e: Exception) {
            Log.e(
                "Repo",
                "Error creating log entry: ${e.message}"
            )
            return -1
        }
    }

    fun sidechain_append(buf: ByteArray, fid:ByteArray, seq: Int): Boolean {
        val r = fid2replica(fid)
        if (r == null)
            return false
        return r.ingest_chunk_pkt(buf, seq)
    }

    fun mk_want_vect(): ByteArray? {
        // want_is_valid = false // TODO optimization not implemented yet
        // if (want_is_valid) return null

        val lst = ArrayList<Int>()
        var v = ""
        var encoding_len = Bipf.encodingLength(want_offs)

        lst.add(want_offs)
        var new_want_offs = want_offs + 1
        var i = 0
        while (i < BleForegroundService.getTinyGoset()!!.keys.size) {
            val ndx = (want_offs + i) % BleForegroundService.getTinyGoset()!!.keys.size
            val fid = BleForegroundService.getTinyGoset()!!.keys[ndx]
            val r = fid2replica(fid)
            if (r == null) {
                i++
                continue
            }
            new_want_offs++
            val (ns, ndmx) = r.get_next_seq()
            encoding_len += Bipf.encodingLength(ns) // Bipf.encode(Bipf.mkInt(ns))!!.size
            lst.add(ns)
            v += (if (v.length == 0) "[" else " ") + "$ndx.$ns"
            if (encoding_len > 100)
                break
            i++
        }
        want_offs = new_want_offs % BleForegroundService.getTinyGoset()!!.keys.size
        // want_is_valid = true
        Log.d("repo", "mk_want offs=${want_offs}, vector=${v}]")

        if (lst.size > 1) {
            // notify frontend
            var vec = lst.slice(1 .. lst.lastIndex) // want_vector without offset
            val front = vec.subList(vec.size - lst[0], vec.size)
            val back = vec.subList(0, vec.size - lst[0])
            vec = front + back
            BleForegroundService.getTinyNode()!!.update_progress(vec, "me")

            return Bipf.encode(Bipf.mkList(lst))
        }
        return null
    }

    fun mk_chnk_vect(): ByteArray? {
        // chnk_is_valid = false // not implemented yet
        // if (chnk_is_valid) return null

        val lst = ArrayList<Any>()
        var v = ""
        var encoding_len = 0

        var i = 0
        var new_chnk_offs = chnk_offs + 1
        while (i < BleForegroundService.getTinyGoset()!!.keys.size) {
            val ndx = (chnk_offs + i) % BleForegroundService.getTinyGoset()!!.keys.size
            val fid = BleForegroundService.getTinyGoset()!!.keys[ndx]
            val r = fid2replica(fid)
            val pending = r?.get_open_chains()
            if (pending == null || pending.isEmpty()) {
                i++
                continue
            }
            new_chnk_offs++
            var lim = 3 // limit # of chunks per feed that we ask for
            for ((seq, p) in pending) {
                val c = arrayListOf(ndx, seq, p.cnr)
                lst.add(c)
                // val cl = Bipf.encodingLength(c)
                // Log.d("repo", "mk_chunk_vect - enc_len ${cl}")
                encoding_len += Bipf.encodingLength(c)
                v += (if (v.length == 0) "[" else " ") + "$ndx.$seq.${p.cnr}"
                if (encoding_len > 100)
                    break
                lim--
                if (lim <= 0)
                    break
            }
            if (encoding_len > 100)
                break
            i++
        }
        chnk_offs = new_chnk_offs % BleForegroundService.getTinyGoset()!!.keys.size
        // chnk_is_valid = true

        if (lst.size > 0) {
            Log.d("repo", "mk_chnk offs=${chnk_offs}, vector=${v}]")
            val lst_e = Bipf.encode(Bipf.mkList(lst))
            return lst_e
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
    @RequiresApi(
        Build.VERSION_CODES.O
    )
    fun upgrade_repo() {
        try {
            val dir = File(service.getDir(TINYSSB_DIR, MODE_PRIVATE), FEED_DIR)
            Log.d("repo", "Upgrading repo filesystem: ${dir?.absolutePath}")
            feediterate@ for (f in dir.listFiles()) {
                if (!f.isDirectory || f.name.length != 2* FID_LEN) {
                    Log.d("Repo", "Skipping file: ${f.name}")
                    continue
                }
                Log.d("Repo", "Upgrading feed: ${f.name}")
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
                //context.wai.frontend_frontier.edit().putInt(f.name, new_state.max_pos + 1).apply()
                sendEditFrontierToMainActivity(f.name, new_state.max_pos + 1)

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
        } catch (e: Exception) {
            Log.e("Repo", "Error upgrading repo filesystem: ${e.message}")
        }
    }

    /**
     * Sends a broadcast to the MainActivity to update the frontier of a feed.
     */
    private fun sendEditFrontierToMainActivity(name: String, value: Int) {
        val intent = Intent(ForegroundNotificationType.EDIT_FRONTEND_FRONTIER.value)
        intent.putExtra("name", name)
        intent.putExtra("value", value)
        LocalBroadcastManager.getInstance(service).sendBroadcast(intent)
    }

    fun isLoaded(): Boolean {
        return loadingFinished
    }

    fun addNumberOfPendingChunks(amount: Int) {
        numberOfPendingChunks += amount
        val intent = Intent(ForegroundNotificationType.EVALUATION.value)
        intent.putExtra("message", "refresh_chunk_progressbar($numberOfPendingChunks)")
        LocalBroadcastManager.getInstance(service).sendBroadcast(intent)
        //context.wai.eval("refresh_chunk_progressbar($numberOfPendingChunks)")
    }

    fun getNumberOfPendingCHunks(): Int {
        return numberOfPendingChunks
    }

}

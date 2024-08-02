package nz.scuttlebutt.tremolavossbol.tssb

import android.os.Build
import android.util.Log
import androidx.annotation.RequiresApi
import nz.scuttlebutt.tremolavossbol.MainActivity
import nz.scuttlebutt.tremolavossbol.WebAppInterface
import nz.scuttlebutt.tremolavossbol.utils.Constants
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.byteArrayCmp
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toHex
import java.net.DatagramPacket
import java.util.concurrent.locks.ReentrantLock
import java.util.zip.CRC32

/*
            byte[] bytes = msg.getBytes(StandardCharsets.UTF_8);
            DatagramPacket hi = new DatagramPacket(bytes, bytes.length,
                mc_group, Constants.MC_PORT);
            mc_socket.send(hi);
            // get their responses!
            byte[] buf = new byte[1000];
            DatagramPacket recv = new DatagramPacket(buf, buf.length);
            mc_socket.receive(recv);
            ...
            // OK, I'm done talking - leave the group...
            mc_socket.leaveGroup(mc_group);
*/

class IO(val context: MainActivity, val wai: WebAppInterface?) {
    val IO_MAX_QUEUE_LEN = 10
    val outqueue = ArrayDeque<ByteArray>()
    val lck = ReentrantLock()

    fun enqueue(buf: ByteArray, dmx: ByteArray? =null, aux: ByteArray? =null) {
        // Log.d("tinyIO", "enqueue ${buf.size} bytes")
        if (outqueue.size >= IO_MAX_QUEUE_LEN)
            return
        val pkt = if (dmx == null) buf else dmx + buf
        lck.lock()
        for (p in outqueue)
            if (byteArrayCmp(p, pkt) == 0) {
                lck.unlock()
                Log.d("IO", "not sending duplicate paket (${buf.size} bytes)")
                return
            }
        outqueue.add(pkt)
        lck.unlock()
    }

    fun appendCRC(buf: ByteArray) : ByteArray {
        val crc32 = CRC32()
        crc32.update(buf)
        return buf + byteArrayOf(((crc32.value shr 24) and 0xff).toByte(),
            ((crc32.value shr 16) and 0xff).toByte(),
            ((crc32.value shr 8) and 0xff).toByte(),
            (crc32.value and 0xff).toByte())
    }

    fun checkCRC(buf: ByteArray, crcBuf: ByteArray): Boolean {
        val crc32 = CRC32()
        crc32.update(buf)
        val x = byteArrayOf(((crc32.value shr 24) and 0xff).toByte(),
            ((crc32.value shr 16) and 0xff).toByte(),
            ((crc32.value shr 8) and 0xff).toByte(),
            (crc32.value and 0xff).toByte())
        return x.contentEquals(crcBuf)
    }

    @RequiresApi(Build.VERSION_CODES.O)
    fun senderLoop() {
        while (true) {
            if (outqueue.size > 0) {
                lck.lock()
                val buf = outqueue.removeFirst()
                // Log.d("tinyIO", "send loop: buf size is ${buf.size} bytes")
                lck.unlock()
                if (context.mc_socket != null) { // WiFi, add CRC
                    val msg = appendCRC(buf)
                    val dgram = DatagramPacket(
                        msg, msg.size,
                        context.mc_group, Constants.SSB_VOSSBOL_MC_PORT
                    );
                    try {
                        Log.d("tinyIO", "send ${msg.size} bytes via UDP mc")
                        context.mc_socket?.send(dgram);
                    } catch (e: Exception) {
                        Log.d("WiFi sender exc", e.toString())
                    }
                }
                try { // BLE, no CRC added
                    if (context.ble != null && context.ble!!.peers.size > 0) { // && context.ble!!.peers.size > 0
                        Log.d("tinyIO", "send ${buf.size} bytes via BLE")
                        context.ble!!.write(buf)
                        //context.ble!!.advertise(buf)
                    }
                } catch (e: Exception) {
                    Log.d("BLE sender exc", e.toString())
                }
                // websocket
                if (context.websocket != null) {
                    context.websocket!!.send(buf)
                }
            }
            Thread.sleep(1000) // slow pace
        }
    }

    fun mcReceiverLoop(lck: ReentrantLock) {
        val pkt = ByteArray(256)
        val ingram = DatagramPacket(pkt, pkt.size)
        while (true) {
            // val s = context.broadcast_socket
            // blocks?? Log.d("listen", "${s}, ports=${s?.port}/${s?.localPort} closed=${s?.isClosed} bound=${s?.isBound}")
            try {
                if (context.mc_socket == null) {
                    Thread.sleep(Constants.UDP_BROADCAST_INTERVAL) // wait for better conditions
                    continue
                }
                context.mc_socket?.receive(ingram)
            } catch (e: Exception) {
                Log.d("MC mcReceiverLoop", "e=${e}, bsock=${context.mc_socket}")
                Thread.sleep(Constants.UDP_BROADCAST_INTERVAL) // wait for better conditions
                continue
            }
            val buf = ingram.data.sliceArray(0..ingram.length-5)
            val crc = ingram.data.sliceArray(ingram.length-4..ingram.length-1)
            // Log.d("MC received", "${ingram.length} ${(buf+crc).toHex()} crc:${checkCRC(buf, crc)}")
            if (checkCRC(buf,crc)) {
                lck.lock()
                if (!context.tinyDemux.on_rx(buf)) {
                    Log.d("io", "no handler for ${buf.size} Bytes ${buf.toHex()}")
                    // Log.d("demux", "- GOset state ${context.tinyGoset.state.toHex()}")
                    // Log.d("demux", "- WANT dmx is ${context.tinyDemux.want_dmx!!.toHex()}")
                    // Log.d("demux", "- BLOB dmx is ${context.tinyDemux.chnk_dmx!!.toHex()}")
                }
                lck.unlock()
            }
        }
    }
}
package nz.scuttlebutt.tremolavossbol.tssb.iroh

import android.util.Base64
import android.util.Log
import nz.scuttlebutt.tremolavossbol.MainActivity
import nz.scuttlebutt.tremolavossbol.utils.Constants
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toHex
import org.json.JSONObject

class IrohPeers(val act: MainActivity) {
    // val neighbors = mutableMapOf<String, String>

    init {
        IrohBridge.registerListener(object : IrohMessageListener {
            override fun onIrohMessage(data: ByteArray) {
                // 1) UTF-8 JSON
                val raw = data.toString(Charsets.UTF_8)
                Log.d("IrohRecv", "raw JSON: ${raw.take(80)}…")
                val js = JSONObject(raw)
                if (js.getString("type") != "message") {
                    Log.d("IrohEvent", raw)
                    return
                }
                // 2) Base64 → wire
                val decoded = Base64.decode(js.getString("payload_b64"), Base64.NO_WRAP)
                val handled = act.tinyDemux.on_rx(decoded, null)
                Log.d("Iroh", "onIrohMessage received ${decoded.size} bytes, handled: $handled")
            }
        })

        val secretKey32ByteHex =
            act.idStore.identity.signingKey?.sliceArray(0 until 32)?.toHex()
        // Make sure toHex() returns a UTF-8 compatible String!
        // Log.d("IrohPeers", "Secret key = $secretKey32ByteHex")

        val started = secretKey32ByteHex?.let { IrohBridge.start_node(it) }
        Log.d("IrohPeers", "start_node returned $started")
        if (started != null) {
            require(started)
        }

        val nodeId = IrohBridge.getNodeId()
        Log.d("IrohPeers", "my iroh node ID is $nodeId")
    }

    fun start() {
        Log.d("IrohPeers", "start()")
        // act.iroh = null
    }

    fun stop() {
        Log.d("IrohPeers", "stop()")
        // act.iroh = null
    }

    fun write(buf: ByteArray) {
        // for (p in peers) {
        val sent = IrohBridge.sendPacket(buf)
        if (sent) {
            Log.d("IrohPeers", "sent ${buf.size} bytes via IrohBridge, sent=${sent}")
        } else {
            Log.d("IrohPeers", "failed to send ${buf.size} bytes via IrohBridge")
        }
    }

    fun reconnectToIrohNodes() {
        Log.d("IrohPeers", "Reconnecting to all known iroh nodes")
        // Get all known contacts from the tinyGoset
        val contacts = act.tinyRepo.listFeeds()
        for (contact in contacts) {
            // Discard own contact
            if (contact.contentEquals(act.idStore.identity.verifyKey)) {
                continue
            }
            val contactHex = contact.toHex()
            Log.d("IrohPeers", "Reconnecting to peer: [$contactHex]")
            IrohBridge.connect(contactHex)
        }
    }

    fun connectToIrohNode(contactID: String) {
        Log.d("IrohPeers", "About to connect to peer: [$contactID]")
        val connected = IrohBridge.connect(contactID)
        Log.d("IrohPeers", "connect() returned $connected")
    }

    fun loop() {
        while (true) {
            try {
                Thread.sleep(Constants.UDP_BROADCAST_INTERVAL)
                continue
            } catch (e: Exception) {
                Log.d("IROH loop", "e=${e}")
                continue
            }
            Thread.sleep(Constants.UDP_BROADCAST_INTERVAL)
        }
    }
}
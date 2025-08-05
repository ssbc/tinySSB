package nz.scuttlebutt.tremolavossbol

interface IrohMessageListener {
    fun onIrohMessage(data: ByteArray)
}

object IrohBridge {
    init {
        System.loadLibrary("iroh_kotlin_bridge")
    }

    /** @see Rust: Java_nz_scuttlebutt_tremolavossbol_IrohBridge_start_1node */
    external fun start_node(secretKeyHex: String): Boolean

    /** @see Rust: Java_nz_scuttlebutt_tremolavossbol_IrohBridge_connectAndChat */
    external fun connectAndChat(peerHex: String): Boolean

    /** @see Rust: Java_nz_scuttlebutt_tremolavossbol_IrohBridge_tryRecv */
    external fun tryRecv(): String

    /** @see Rust: Java_nz_scuttlebutt_tremolavossbol_IrohBridge_getNodeId */
    external fun getNodeId(): String

    /** @see Rust: Java_nz_scuttlebutt_tremolavossbol_IrohBridge_sendMessage */
    external fun sendMessage(msgBytes: ByteArray): Boolean

    /** @see Rust: Java_nz_scuttlebutt_tremolavossbol_IrohBridge_registerListener */
    external fun registerListener(listener: IrohMessageListener): Boolean

    external fun sendPacket(data: ByteArray): Boolean

    /** Kotlin‐side callback holder */
    var messageListener: IrohMessageListener? = null
}

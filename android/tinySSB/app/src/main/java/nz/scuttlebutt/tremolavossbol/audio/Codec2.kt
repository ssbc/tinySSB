package nz.scuttlebutt.tremolavossbol.audio

object Codec2 {
    // supported modes for Codec2 v1.0.4
    const val CODEC2_MODE_3200    = 0
    const val CODEC2_MODE_2400    = 1
    const val CODEC2_MODE_1600    = 2
    const val CODEC2_MODE_1400    = 3
    const val CODEC2_MODE_1300    = 4
    const val CODEC2_MODE_1200    = 5
    // const val CODEC2_MODE_700  = 6
    // const val CODEC2_MODE_700B = 7
    const val CODEC2_MODE_700C    = 8
    // const val CODEC2_MODE_WB   = 9
    const val CODEC2_MODE_450    = 10
    // const val CODEC2_MODE_450PWB = 11

    const val CODEC2_FILE_HEADER_SIZE = 7

    fun makeHeader(mode: Int): ByteArray {
        val a = ByteArray(CODEC2_FILE_HEADER_SIZE)
        a[0] = 0xc0 - 256
        a[1] = 0xde - 256
        a[2] = 0xc2 - 256
        a[3] = 1 // version_major
        a[4] = 0 // version_minor
        a[5] = mode.toByte()
        a[6] = 0 // flags
        return a
    }

    fun extractMode(a: ByteArray): Int { // return -1 if header is invalid, else compression mode
        if (a.size < 8 || a[0] != (0xc0-256).toByte() || a[1] != (0xde-256).toByte() ||
            a[2] != (0xc2-256).toByte() || a[3].toInt() != 1 || a[4].toInt() != 0)
                return -1
        return a[5].toInt()
    }

    external fun create(mode: Int): Long
    external fun getSamplesPerFrame(con: Long): Int
    external fun getBitsSize(con: Long): Int
    external fun destroy(con: Long): Int
    external fun encode(con: Long, buf: ShortArray?, bits: ByteArray?): Long
    /**
     * Decode one frame from codec2.
     *
     * @param con pointer long, as from the create method
     * @param bits input buffer containing one frame of audio
     * @param outputBuffer buffer which will be filled with raw PCM audio decoded
     *
     * @return 0 on successful completion
     */
    external fun decode(con: Long, bits: ByteArray?, outputBuffer: ShortArray?): Long

     fun pcm_to_codec2(mode: Int, pcm: ShortArray): ByteArray {
        val c2 = create(mode)
        val spf = getSamplesPerFrame(c2)
        val bpf = getBitsSize(c2) // bytes per frame
        var outBuf = ByteArray(0)
        var pos = 0
        while ((pos + spf) <= pcm.size) {
            var bits = ByteArray(bpf)
            encode(c2, pcm.sliceArray(pos..pos+spf-1), bits)
            outBuf += bits
            pos += spf
        }
        destroy(c2)
        return outBuf
    }

    fun codec2_to_pcm(c2mode: Int, c2data: ByteArray): ShortArray {
        var c2 = Codec2.create(c2mode)
        var fps = Codec2.getSamplesPerFrame(c2) // PCM frames per sample
        var bps = Codec2.getBitsSize(c2) // encoded bytes per fps
        var pos = 0
        var pcm = ShortArray(0)
        while ((pos+bps) <= c2data.size) {
            val c2bytes = c2data.sliceArray(pos .. pos+bps-1)
            val pcmShorts = ShortArray(fps)
            Codec2.decode(c2, c2bytes, pcmShorts)
            pcm = pcm + pcmShorts
            pos += bps
        }
        /*
        rate: 8000 Hz,
        AudioFormat.CHANNEL_OUT_MONO,
        AudioFormat.ENCODING_PCM_16BIT
        framecount: pcm.size
        */
        return pcm
    }

    init {
        System.loadLibrary("codec2")
    }
}
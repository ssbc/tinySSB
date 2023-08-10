package nz.scuttlebutt.tremolavossbol.audio

import android.util.Log
import kotlin.experimental.and

class Wav {

    fun le4B(i: Int): ByteArray {
        val b = ByteArray(4)
        var v = i
        for (i in 0..3) {
            b[i] = (v and 0xff).toByte()
            v = v shr 8
        }
        return b
    }

    fun pcm_to_wav(pcm: ShortArray): ByteArray {
        /* assumes
        rate: 8000 Hz,
        AudioFormat.CHANNEL_OUT_MONO,
        AudioFormat.ENCODING_PCM_16BIT
        framecount: pcm.size
        */
        var buf = ByteArray(0)
        buf += "RIFF".encodeToByteArray()
        buf += le4B(44 + 2*pcm.size - 8)
        buf += "WAVEfmt ".encodeToByteArray()
        buf += le4B(16)
        buf += le4B(0x00010001) // PCM, one channel
        buf += le4B(8000)
        buf += le4B(8000*16*1/8)
        buf += le4B(0x00100002) // bps*chan/8, bitspersample
        buf += "data".encodeToByteArray()
        buf += le4B(2*pcm.size)

        Log.d("Codec2", "WAV header size is ${buf.size}, should be 44")
        for (s in pcm)
            buf += byteArrayOf((s and 0xff).toByte(), ((s.toInt() shr 8) and 0xff).toByte())
        return buf
    }
    /*
#define WAV_WRITE_LE16(b,v) { (b)[0] = (unsigned char)((unsigned short)(v) & 0xFF); (b)[1] = (unsigned char)((unsigned short)(v) >> 8); }
#define WAV_WRITE_LE32(b,v) { (b)[0] = (unsigned char)((unsigned int)(v) & 0xFF); (b)[1] = (unsigned char)(((unsigned int)(v) >> 8) & 0xFF); (b)[2] = (unsigned char)(((unsigned int)(v) >> 16) & 0xFF); (b)[3] = (unsigned char)((unsigned int)(v) >> 24); }
WAV_WRITE_LE32(hdr +  0, 0x46464952U);                                 //"RIFF"
WAV_WRITE_LE32(hdr +  4, WAV_HEADER_SIZE - 4 - 4 + pcm_data_size);     //chunk size
WAV_WRITE_LE32(hdr +  8, 0x45564157U);                                 //"WAVE"
WAV_WRITE_LE32(hdr + 12, 0x20746d66U);                                 //subchunk 1 id "fmt "
WAV_WRITE_LE32(hdr + 16, 16);                                          //subchunk 1 size
WAV_WRITE_LE16(hdr + 20, 1);                                           //audio format
WAV_WRITE_LE16(hdr + 22, channels);                                    //num of chan
WAV_WRITE_LE32(hdr + 24, sample_rate);                                 //samples per sec
WAV_WRITE_LE32(hdr + 28, sample_rate * info.channels * sizeof(short)); //bytes per sec
WAV_WRITE_LE16(hdr + 32, channels * sizeof(short));                    //block align
WAV_WRITE_LE16(hdr + 34, sizeof(short) * 8);                           //bits per sample
WAV_WRITE_LE32(hdr + 36, 0x61746164U);                                 //subchunk 2 id "data"
WAV_WRITE_LE32(hdr + 40, pcm_data_size);                               //subchunk 2 size
#undef WAV_WRITE_LE16
#undef WAV_WRITE_LE32
    */
}
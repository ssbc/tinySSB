package nz.scuttlebutt.tremolavossbol.audio

import android.annotation.SuppressLint
import android.media.AudioFormat
import android.media.AudioRecord
import android.util.Log
import java.io.File
import java.nio.ByteBuffer
import java.nio.ByteOrder


class PCMRecorder(val audioSrc: Int) {
    var audioRecorder: AudioRecord? = null
    var maxAmplitude: Short = 0

    @SuppressLint(
        "MissingPermission"
    )
    fun start() {
        // var bufSize = 2 * AudioRecord.getMinBufferSize(8000,
        //    AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT);
        val bufSize = 3*1024 // 4096 // 40*4096 // 2*640
        Log.d("PCM", "start " + bufSize.toString())
        audioRecorder = AudioRecord(
        audioSrc, 8000, AudioFormat.CHANNEL_IN_MONO,
        AudioFormat.ENCODING_PCM_16BIT, bufSize)
        audioRecorder!!.startRecording()
    }

    fun read(buf: ShortArray, pos: Int): Int {
        val cnt = audioRecorder!!.read(buf, pos, buf.size)
        maxAmplitude = 0
        for (i in pos .. pos + cnt-1) {
            val a = buf[i]
            if (a > maxAmplitude || -a > maxAmplitude)
                maxAmplitude = a
        }
        return cnt
    }

    fun stop() {
        Log.d("PCM", "stop " + audioRecorder.toString())
        audioRecorder!!.stop()
        audioRecorder = null
    }

    fun writeToFile(path: String, pcmData: ShortArray) {
        val wav = File(path)
        for (s in make_wav_header(pcmData!!.size * 2) + pcmData!!) {
            val us = s.toUShort()
            val b = ByteArray(2)
            b[0] = (us % 256u).toByte()
            b[1] = ((us / 256u) % 256u).toByte()
            wav.appendBytes(b)
        }
    }

    private fun make_wav_header(byteCnt: Int): ShortArray {
        val b = ByteBuffer.allocate(44).order(ByteOrder.LITTLE_ENDIAN)
        b.put('R'.toByte()); b.put('I'.toByte()); b.put('F'.toByte()); b.put('F'.toByte())
        b.putInt(byteCnt - 8)
        b.put('W'.toByte()); b.put('A'.toByte()); b.put('V'.toByte()); b.put('E'.toByte())
        b.put('f'.toByte()); b.put('m'.toByte()); b.put('t'.toByte()); b.put(' '.toByte())
        b.put(16.toByte()); b.put(0.toByte()); b.put(0.toByte()); b.put(0.toByte())
        b.put(1.toByte()); b.put(0.toByte()) // audio format
        b.putShort(1) // channels
        b.putInt(8000) // sampleRate
        b.putInt(8000 * 1 * (16/2))
        b.putShort(1 * (16/8))
        b.putShort(16)
        b.put('d'.toByte()); b.put('a'.toByte()); b.put('t'.toByte()); b.put('a'.toByte())
        b.putInt(byteCnt - 44)
        val s = ShortArray(22)
        for (i in 0..21) {
            val u: UInt =  b[2*i].toUInt() + 256u * b[2*i+1].toUInt()
            s[i] = u.toShort()
        }
        return s
    }
}
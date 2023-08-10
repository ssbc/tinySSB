package nz.scuttlebutt.tremolavossbol.audio

import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import android.media.MediaPlayer
import android.util.Log


class Codec2Player(val c2mode: Int, val c2data: ByteArray): MediaPlayer()  {
    var frameCount = 0
    val Hz = 8000
    var isAtPlay = false
    lateinit var track: AudioTrack
    var c2 = Codec2.create(c2mode)
    var fps = Codec2.getSamplesPerFrame(c2) // PCM frames per sample
    var bps = Codec2.getBitsSize(c2) // encoded bytes per fps
    var headOffset = 1
    var pcm : ShortArray? = null

    override fun isPlaying(): Boolean {
        return isAtPlay
    }

    fun duration(): Int { // in millis
        return frameCount * 1000 / Hz
    }

    fun currentPosition(): Int { // in millis
        var pos = headOffset + track.playbackHeadPosition
        if (pos >= frameCount - 1)
            pause()
        pos = pos * 1000 / Hz
        return pos
    }

    override fun seekTo(pos: Int) {
        headOffset = pos * Hz / 1000
        if (headOffset < 0) headOffset = 0
        if (headOffset >= frameCount) headOffset = frameCount-1
        track.reloadStaticData()
        track.playbackHeadPosition = headOffset
    }

    override fun prepare() {
        Log.d("C2P", "prepare " + c2data.size.toString() + " " + fps.toString() + " " + bps.toString())
        var pos = 0
        pcm = ShortArray(0)
        while ((pos+bps) <= c2data.size) {
            val c2bytes = c2data.sliceArray(pos .. pos+bps-1)
            val pcmShorts = ShortArray(fps)
            Codec2.decode(c2, c2bytes, pcmShorts)
            pcm = pcm!! + pcmShorts
            pos += bps
        }
        Codec2.destroy(c2)
        c2 = 0L
        frameCount = pcm!!.size
        Log.d("C2P", "decoded! " + pcm!!.size.toString())

        track = AudioTrack(AudioManager.STREAM_MUSIC, 8000,
            AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT,
            2*pcm!!.size, AudioTrack.MODE_STATIC)
        track.write(pcm!!, 0, pcm!!.size)
        track.setNotificationMarkerPosition(frameCount-1)
    }

    override fun start() {
        Log.d("C2P", "start")
        isAtPlay = true
        track.play()
    }
    override fun pause() {
        track.pause()
        Log.d("C2P", "pause")
        isAtPlay = false
    }
    override fun stop() {
        Log.d("C2P", "stop")
        isAtPlay = false
        track.stop()
    }
    override fun release() {
        track.release()
    }

    fun getAmplitude(): Int {
        val width = 300
        var pos = currentPosition() * 8000 / 1000 - width/2
        if (pos + width/2 > pcm!!.size)
            pos = pcm!!.size - width
        var amp = 0
        for (i in pos .. pos+width-1) {
            var v = pcm!![pos].toInt()
            if (v < 0) v = -v
            if (v > amp) amp = v
        }
        return amp
    }

    init {
        Log.d("C2P", "init")
    }
}
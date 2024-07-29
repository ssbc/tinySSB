package nz.scuttlebutt.tremolavossbol

// import kotlinx.android.synthetic.main.activity_player.*

import android.content.ContentValues
import android.os.*
import android.provider.MediaStore
import android.util.Log
import android.view.View
import android.view.View.VISIBLE
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.res.ResourcesCompat
import nz.scuttlebutt.tremolavossbol.audio.Codec2
import nz.scuttlebutt.tremolavossbol.audio.Codec2Player
import nz.scuttlebutt.tremolavossbol.audio.Wav
import tremolavossbol.R
import tremolavossbol.databinding.ActivityPlayBinding
import java.io.ByteArrayInputStream
import java.io.File
import java.io.FileOutputStream
import java.io.OutputStream
import java.text.SimpleDateFormat
import java.util.*

class PlayActivity: AppCompatActivity(), AdapterView.OnItemSelectedListener {

    private lateinit var binding: ActivityPlayBinding
    private val refreshRate = 60L
    private lateinit var runnable : Runnable
    private lateinit var handler : Handler
    private lateinit var mediaPlayer : Codec2Player
    private var c2mode = Codec2.CODEC2_MODE_1300
    private var c2data : ByteArray? = null
    private var pcmData : ShortArray? = null
    private var pos2mode : IntArray = intArrayOf(0,1,2,3,4,5,8,10)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityPlayBinding.inflate(layoutInflater)
        val view = binding.root
        // setContentView(R.layout.activity_play)
        setContentView(view)

        setSupportActionBar(binding.toolbar)
        supportActionBar?.setDisplayHomeAsUpEnabled(true)
        supportActionBar?.setDisplayShowHomeEnabled(true)
        binding.toolbar.setNavigationOnClickListener {
            onBackPressed()
        }

        val mModeSelect = findViewById<Spinner>(R.id.c2modeSelect)
        mModeSelect.onItemSelectedListener = this
        val dataAdapter = ArrayAdapter<String>(this,
            android.R.layout.simple_spinner_item,
            resources.getStringArray(R.array.c2modes)
        )
        dataAdapter.setDropDownViewResource(R.layout.c2mode_spinner_item)
        mModeSelect.adapter = dataAdapter

        c2data = intent.getByteArrayExtra("c2Data")
        if (c2data != null) {
            c2mode = c2data!![5].toInt()
            c2data = c2data!!.sliceArray(7.. c2data!!.size-1)
            // fileNameView.text = intent.getStringExtra("fileName")
            // btnSave.visibility = GONE
            mModeSelect.setEnabled(false);
            mModeSelect.setSelection(pos2mode.indexOf(c2mode)) // this will launch the player
        } else {
            pcmData = intent.getShortArrayExtra("pcmData")
            c2data = Codec2.pcm_to_codec2(c2mode, pcmData!!)
            // mModeSelect.setSelection(dataAdapter.getPosition("450 (compact)"))
            mModeSelect.setSelection(dataAdapter.getPosition("700C"))
            // launch_player()
        }
        var from = intent.getStringExtra("from")
        var date = intent.getStringExtra("date")
        binding.playerFrom.text = if (from == null) "" else from
        binding.playerWhen.text = if (date == null) "" else date

        handler = Handler(Looper.getMainLooper())

        binding.btnPlay.setOnClickListener {
            playPausePlayer()
        }

        binding.btnForward.setOnClickListener {
            if (mediaPlayer.isPlaying) freeze()
            mediaPlayer.seekTo(mediaPlayer.currentPosition() + 1000)
            binding.seekBar.progress = mediaPlayer.currentPosition()
        }

        binding.btnBackward.setOnClickListener {
            if (mediaPlayer.isPlaying) freeze()
            mediaPlayer.seekTo(mediaPlayer.currentPosition() - 1000)
            binding.seekBar.progress = mediaPlayer.currentPosition()
        }

        binding.seekBar.setOnSeekBarChangeListener(object: SeekBar.OnSeekBarChangeListener{
            override fun onProgressChanged(p0: SeekBar?, p1: Int, p2: Boolean) {}
            override fun onStartTrackingTouch(p0: SeekBar?) {
                if (mediaPlayer.isPlaying) freeze()
            }
            override fun onStopTrackingTouch(p0: SeekBar?) {
                mediaPlayer.seekTo(binding.seekBar.progress)
            }
        })

        fun storeMedia(fileName: String, content: ByteArray, mimeType: String): String? {
            var fname = fileName
            try {
                var outputStream: OutputStream?
                if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
                    // val d = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS)
                    val f = File(getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS), fname)
                    // val d = "/sdcard/Download" // Environment.getExternalStorageDirectory().absolutePath + "/Download"
                    // val f = File(d /* getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS)*/ , fileName)
                    f.createNewFile()
                    outputStream = FileOutputStream(f, false)
                    fname = f.absolutePath
                } else {
                    val contentValues = ContentValues().apply {
                        put(MediaStore.MediaColumns.DISPLAY_NAME, fname)
                        put(MediaStore.MediaColumns.MIME_TYPE, mimeType)
                        put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_DOWNLOADS)
                    }
                    val resolver = this.contentResolver
                    val uri = resolver.insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, contentValues)
                    outputStream = resolver.openOutputStream(uri!!)
                }
                ByteArrayInputStream(content).use { input ->
                    outputStream.use { output ->
                        input.copyTo(output!!, DEFAULT_BUFFER_SIZE)
                    }
                }
            } catch(e: Exception) {
                Log.d("exception", e.toString())
                return null
            }
            return fname
        }

        binding.btnSave.setOnClickListener {
            /*
            Log.d("player", "done")
            val voice = Codec2.makeHeader(c2mode) + c2data!!
            val _result = intent // Intent(this, MainActivity::class.java) // intent
            _result.putExtra("codec2", voice)
            // startActivity(_result)
            setResult(RESULT_OK, _result)
            finish()
            // finishActivity(808)
            onBackPressed()
            */
            val pattern = "yyyymmdd_HHMMSS"

            var fileName = "tremola-${SimpleDateFormat(pattern).format(Date())}.c2"
            var path = storeMedia(fileName, Codec2.makeHeader(c2mode) + c2data!!, "audio/vnd.codec2")
            var text = if (path != null) "Stored as '${path}'" else "Saving C2 audio file failed"
            Toast.makeText(applicationContext, text, Toast.LENGTH_SHORT).show()

            fileName = "tremola-${SimpleDateFormat(pattern).format(Date())}.wav"
            val pcm = Codec2.codec2_to_pcm(c2mode, c2data!!)
            path = storeMedia(fileName, Wav().pcm_to_wav(pcm), "audio/vnd.wav")
            text = if (path != null) "Stored as '${path}'" else "Saving WAV audio file failed"
            Log.d("play", text)
            Toast.makeText(applicationContext, text, Toast.LENGTH_SHORT).show()
        }
    }

    override fun onNothingSelected(p0: AdapterView<*>) {}
    override fun onItemSelected(p0: AdapterView<*>, p1: View, p2: Int, p3: Long) {
        if (pcmData != null) {
            if (this::mediaPlayer.isInitialized && mediaPlayer.isPlaying)
                freeze()
            c2mode = pos2mode[p2]
            c2data = Codec2.pcm_to_codec2(c2mode, pcmData!!)
        }
        launch_player()
    }

    private fun launch_player() {
        if (this::mediaPlayer.isInitialized) {
            mediaPlayer.stop()
            mediaPlayer.release()
        }
        mediaPlayer = Codec2Player(c2mode, c2data!!)
        mediaPlayer?.prepare()

        val millis =  mediaPlayer.duration()
        binding.seekBar.max = millis
        binding.totalTime.text = String.format("%02d:%02d.%1d",
            millis/60/1000, (millis/1000)%60, (millis/100)%10)
        if (c2data!!.size >= 1000)
            binding.totalBytes.text = String.format("%.1f KB", c2data!!.size/1024.0)
        else
            binding.totalBytes.text = String.format("%d B", c2data!!.size)
    }

    private fun freeze() {
        handler.removeCallbacks(runnable)
        mediaPlayer.pause()
        binding.btnPlay.background = ResourcesCompat.getDrawable(resources, R.drawable.ic_play_circle, theme)
    }

    private fun playPausePlayer(){
        if (mediaPlayer.isPlaying)
            freeze()
        else {
            if (mediaPlayer.currentPosition() >= mediaPlayer.duration()) {
                mediaPlayer.seekTo(0)
                binding.seekBar.progress = 0
            }
            mediaPlayer.start()
            binding.btnPlay.background = ResourcesCompat.getDrawable(resources,
                R.drawable.ic_pause_circle, theme)

            runnable = Runnable {
                var progress = mediaPlayer.currentPosition()
                binding.seekBar.progress = progress
                if (progress >= mediaPlayer.duration()) {
                    binding.btnPlay.background =
                        ResourcesCompat.getDrawable(resources, R.drawable.ic_play_circle, theme)
                } else {
                    val amp = mediaPlayer.getAmplitude()
                    binding.playerView.updateAmps(amp)
                    handler.postDelayed(runnable, refreshRate)
                }
            }
            handler.postDelayed(runnable, refreshRate)
        }
    }

    override fun onBackPressed() {
        if (::runnable.isInitialized)
            handler.removeCallbacks(runnable)
        mediaPlayer.stop()
        mediaPlayer.release()

        binding.btnSave.visibility = VISIBLE
        // findViewById<Spinner>(R.id.c2modeSpinner).isClickable = true
        (findViewById<Spinner>(R.id.c2modeSelect) as Spinner).setEnabled(true)
        binding.fileNameView.text = ""

        super.onBackPressed()
    }
}
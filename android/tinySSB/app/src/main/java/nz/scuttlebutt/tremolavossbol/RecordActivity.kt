package nz.scuttlebutt.tremolavossbol

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.media.MediaRecorder
import android.net.Uri
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.provider.MediaStore
import android.text.method.ScrollingMovementMethod
import android.util.Log
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import kotlinx.android.synthetic.main.activity_record.*
import nz.scuttlebutt.tremolavossbol.audio.Codec2
import nz.scuttlebutt.tremolavossbol.audio.PCMRecorder
import nz.scuttlebutt.tremolavossbol.audio.Timer
import tremolavossbol.R
import java.io.ByteArrayOutputStream


class RecordActivity : AppCompatActivity(), Timer.OnTimerUpdateListener {
    private val PERMISSION_REQUEST = 789
    private val CHOOSEFILE_REQUEST = 987

    private var recorder: PCMRecorder? = null
    private var recording = false
    private var refreshRate : Long = 60
    private lateinit var timer: Timer
    private var recThread: Thread? = null
    private var pcmData = ShortArray(0)
    private lateinit var handler: Handler

    private var permissionToRecordAccepted = false
    private var permissionToReadAccepted = false
    private var permissions: Array<String> = arrayOf(Manifest.permission.RECORD_AUDIO,
        Manifest.permission.READ_EXTERNAL_STORAGE)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_record)

        aboutView.setMovementMethod(ScrollingMovementMethod())
        aboutView.text = """
-- About Codec2 Recorder --

This app records voice and compresses it using Codec2: You can listen to the compression result before saving the file. This app also permits to play back Codec2 files.

(c) 2022-07-17, Christian Tschudin, MIT license, see https://github.com/...

Uses the Codec2 library of David Grant Rowe, v1.0.3, LGPL license 2.1, see https://github.com/drowe67/codec2

Android UI code based on exRivalis' AudioRecorder, see https://github.com/exRivalis/AudioRecorder

Load icon by Alice Rizzo, CC Attribution License https://creativecommons.org/licenses/by/4.0/, converted to VectorDraw

Save icon by Zwoelf, http://www.zwoelf.hu/, free for commercial use license, converted to VectorDraw

"""

        // Record to the external cache directory for visibility
        ActivityCompat.requestPermissions(this, permissions, PERMISSION_REQUEST)

        handler = Handler(Looper.myLooper()!!)

        titleTextView.setOnClickListener {
            mainLayout.visibility = View.GONE
            aboutView.visibility = View.VISIBLE
        }

        recordBtn.setOnClickListener {
            if (recording)
                stopRecording()
            else
                startRecording()
        }

        btnLoad.setOnClickListener {
            if (recording)
                stopRecording()
            reset()
            load_file()
        }

        doneBtn.setOnClickListener {
            stopRecording()
            val _result = intent // Intent(this, MainActivity::class.java) // intent
            val c2mode = Codec2.CODEC2_MODE_1300
            val c2data = Codec2.makeHeader(c2mode) + Codec2.pcm_to_codec2(c2mode, pcmData)
            _result.putExtra("codec2", c2data)
            setResult(RESULT_OK, _result)
            finish()
            // passToPlayer()
            // reset()
        }

        deleteBtn.setOnClickListener {
            stopRecording()
            reset()
        }

        timer = Timer(this)
        timerView.text = "00:00.0"

        Log.d("recorder", "start recording now")
    }

    override fun onResume() {
        super.onResume()
        startRecording()
    }

    private fun reset() {
        pcmData = ShortArray(0)

        doneBtn.visibility = View.INVISIBLE
        doneBtn.isClickable = false
        deleteBtn.visibility = View.INVISIBLE
        deleteBtn.isClickable = false
        recordBtn.setImageResource(R.drawable.ic_record)

        try {
            timer.stop()
        } catch (e: Exception){}
        timer = Timer(this)
        timerView.text = "00:00.0"
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode != PERMISSION_REQUEST)
            return
        for (i in 0 .. permissions.size-1) {
            if (permissions[i].equals(Manifest.permission.RECORD_AUDIO))
                permissionToRecordAccepted = grantResults[i] == PackageManager.PERMISSION_GRANTED
            if (permissions[i].equals(Manifest.permission.READ_EXTERNAL_STORAGE))
                permissionToReadAccepted = grantResults[i] == PackageManager.PERMISSION_GRANTED
        }
    }

    private fun startRecording() {
        if (!permissionToRecordAccepted){
            ActivityCompat.requestPermissions(this, permissions, PERMISSION_REQUEST)
            return
        }

        doneBtn.visibility = View.VISIBLE
        doneBtn.isClickable = true
        deleteBtn.visibility = View.VISIBLE
        deleteBtn.isClickable = true
        recordBtn.setImageResource(R.drawable.ic_pause)

        recording = true
        timer.start()

        recorder = PCMRecorder(MediaRecorder.AudioSource.MIC)
        recThread = Thread {
            recorder!!.start()
            while (recording) {
                val buf = ShortArray(1024)
                val cnt = recorder!!.read(buf, 0)
                if (cnt > 0)
                    pcmData = pcmData!! + buf.slice(0..cnt-1)
            }
        }
        recThread?.start()

        animatePlayerView()
    }

    private fun animatePlayerView(){
        if(recording){
            var amp = recorder!!.maxAmplitude.toInt()
            recorderView.updateAmps(amp/4)
            handler.postDelayed(
                Runnable {
                    kotlin.run { animatePlayerView() }
                }, refreshRate
            )
        }
    }

    private fun stopRecording(){
        recording = false
        recorder?.apply {
            stop()
            // release()
        }
        recorder = null
        recordBtn.setImageResource(R.drawable.ic_record)

        recorderView.reset()
        timer.pause()
    }

    private fun passToPlayer() {
        val intent = Intent(this, PlayActivity::class.java)
        intent.putExtra("pcmData", pcmData) //data", c2data)
        startActivityForResult(intent, 809)
    }

    private fun load_file() {
        if (!permissionToReadAccepted){
            ActivityCompat.requestPermissions(this, permissions, PERMISSION_REQUEST)
            return
        }
        var chooseFile = Intent(Intent.ACTION_GET_CONTENT)
        chooseFile.type = "*/*"
        chooseFile = Intent.createChooser(chooseFile, "Choose a file")
        startActivityForResult(chooseFile, CHOOSEFILE_REQUEST)
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        Log.d("recorder", "activity result ${requestCode} ${resultCode}")
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == 809) {
            Log.d("recorder", "onActRes data=${data}")
            if (data != null && data.getByteArrayExtra("codec2") != null) {
                // Log.d("recorder", "sending back result ${data.getStringExtra("codec2")}")
                val _result = intent // Intent(this, MainActivity::class.java) // intent
                _result.putExtra("codec2", data.getByteArrayExtra("codec2"))
                setResult(RESULT_OK, _result)
            }
            finish()
            return
        }
        /* no file loading in tinyTremola
        if (requestCode != CHOOSEFILE_REQUEST) return
        if (resultCode != RESULT_OK || data == null) {
            Toast.makeText(applicationContext, "No file loaded", Toast.LENGTH_SHORT).show()
            return
        }
        val uri = data!!.getData() as Uri
        val segs = uri.path!!.split('/')
        val fileName = segs[segs.size - 1]
        val outStream = ByteArrayOutputStream()
        try {
            val inStream = contentResolver.openInputStream(uri)
            var readCnt = 0
            val buf = ByteArray(1024)
            while (inStream!!.read(buf).also({ readCnt = it }) != -1) {
                outStream.write(buf, 0, readCnt)
            }
        } catch(e: Exception){
            Toast.makeText(applicationContext, "Could not load file", Toast.LENGTH_SHORT).show()
            return
        }
        val c2data = outStream.toByteArray()
        val c2mode = Codec2.extractMode(c2data)
        if (c2mode < 0) {
            Toast.makeText(applicationContext, "Input file has invalid Codec2 header", Toast.LENGTH_SHORT).show()
            return
        }
        val intent = Intent(this, PlayActivity::class.java)
        intent.putExtra("c2Data", c2data)
        intent.putExtra("fileName", fileName)
        intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK
        startActivityForResult(intent, 808)
        */
    }

    private fun getPath(uri: Uri): String? {
        var path : String? = null;
        val projection = MediaStore.Files.FileColumns.DATA as Array<String>?
        val cursor = getContentResolver().query(uri, projection, null, null, null);
        if (cursor == null)
            path = uri.getPath()
        else {
            cursor.moveToFirst();
            val column_index = cursor.getColumnIndexOrThrow(projection!![0]);
            path = cursor.getString(column_index);
            cursor.close();
        }
        if (path == null || path.isEmpty())
            return uri.getPath()
        return path
    }

    override fun onTimerUpdate(duration: String) {
        runOnUiThread {
            if (recording)
                timerView.text = duration.subSequence(0..6)
        }
    }

    override fun onBackPressed() {
        if (aboutView.visibility == View.VISIBLE) {
            aboutView.visibility = View.GONE
            mainLayout.visibility = View.VISIBLE
            return
        }
        super.onBackPressed()
    }
}
package nz.scuttlebutt.tremolavossbol

// import android.R

// import nz.scuttlebutt.tremolavossbol.tssb.ble.BlePeers

import android.Manifest
import android.app.Activity
import android.app.ActivityManager
import android.bluetooth.BluetoothAdapter
import android.content.BroadcastReceiver
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.ServiceConnection
import android.content.pm.ActivityInfo
import android.content.pm.PackageManager
import android.net.*
import android.net.wifi.WifiManager
import android.os.BatteryManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.IBinder
import android.os.Messenger
import android.util.Log
import android.view.View
import android.view.Window
import android.webkit.WebSettings
import android.webkit.WebView
import android.widget.Toast
import androidx.annotation.RequiresApi
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import com.google.zxing.integration.android.IntentIntegrator
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.launch
import nz.scuttlebutt.tremolavossbol.crypto.IdStore
import nz.scuttlebutt.tremolavossbol.tssb.ble.BlePeers
import nz.scuttlebutt.tremolavossbol.tssb.*
import nz.scuttlebutt.tremolavossbol.tssb.ble.BluetoothEventListener
import nz.scuttlebutt.tremolavossbol.utils.Constants
import nz.scuttlebutt.tremolavossbol.games.common.GamesHandler
import nz.scuttlebutt.tremolavossbol.tssb.ble.ApplicationNotificationType
import nz.scuttlebutt.tremolavossbol.tssb.ble.BleForegroundService
import nz.scuttlebutt.tremolavossbol.tssb.ble.BleForegroundService.Companion.getTinyIdStore
import nz.scuttlebutt.tremolavossbol.tssb.ble.ForegroundNotificationType
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toHex
import tremolavossbol.R
import java.net.*
import java.util.Base64
import java.util.concurrent.locks.ReentrantLock
import kotlin.concurrent.thread


// import nz.scuttlebutt.tremolavossbol.MainActivity


class MainActivity : Activity() {
    // lateinit var tremolaState: TremolaState
    lateinit var wai: WebAppInterface
    var frontend_ready = true
    @Volatile var mc_group: InetAddress? = null
    @Volatile var mc_socket: MulticastSocket? = null
    var websocket: WebsocketIO? =null
    val ioLock = ReentrantLock()
    var broadcastReceiver: BroadcastReceiver? = null
    var isWifiConnected = false
    var ble_event_listener: BluetoothEventListener? = null
    var batterySave: Boolean = false

    // used for messaging
    private var serviceConnection: ServiceConnection? = null
    private var serviceMessenger: Messenger? = null
    private val coroutineScope = CoroutineScope(Dispatchers.Main + Job())

    private fun bindToService() {
        val intent = Intent(this, BleForegroundService::class.java)
        serviceConnection = object : ServiceConnection {
            override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
                serviceMessenger = Messenger(service)
            }

            override fun onServiceDisconnected(name: ComponentName?) {
                serviceMessenger = null
            }
        }
        bindService(intent, serviceConnection!!, Context.BIND_AUTO_CREATE)
    }

    /**
     *  We need to receive messages from the ForegroundService (This includes several classes within the service!).
     *  This is being split into different kinds of broadcasters, which are then forwarded to the WebAppInterface.
     */

    private val foregroundServiceReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            //handleIncomingMessage(message) // TODO unsure about this
            Log.d("MainActivity", "Received Notification from Foreground [${intent.action}]")
            when (intent.action) {
                ForegroundNotificationType.EVALUATION.value -> {
                    val message = intent.getStringExtra("message")
                    if (message != null) {
                        wai.eval(message)
                    }
                }
                ForegroundNotificationType.TINY_EVENT.value -> {
                    val fid = intent.getByteArrayExtra("fid")
                    val seq = intent.getIntExtra("seq", 0) // TODO default value of seq number!
                    val hash = intent.getByteArrayExtra("hash")
                    val body = intent.getByteArrayExtra("body")
                    if (fid != null && hash != null && body != null) {
                        coroutineScope.launch {
                            wai.sendTinyEventToFrontend(fid, seq, hash, body)
                        }
                    }
                }
                ForegroundNotificationType.INCOMPLETE_EVENT.value -> {
                    val fid = intent.getByteArrayExtra("fid")
                    val seq = intent.getIntExtra("seq", 0) // TODO default value of seq number!
                    val hash = intent.getByteArrayExtra("hash")
                    val body = intent.getByteArrayExtra("body")
                    if (fid != null && hash != null && body != null) {
                        coroutineScope.launch {
                            wai.sendIncompleteEntryToFrontend(fid, seq, hash, body)
                        }
                    }
                }
                /**
                 * Usage in: Repo.kt
                 */
                ForegroundNotificationType.EDIT_FRONTEND_FRONTIER.value -> {
                    Log.d("MainActivity", "Received EDIT_FRONTEND_FRONTIER message")
                    val name = intent.getStringExtra("name")
                    val value = intent.getIntExtra("value", 0)
                    if (name != null) {
                        wai.frontend_frontier.edit().putInt(name, value).apply()
                    }
                }
            }
        }
    }

    private val isLocationPermissionGranted
        get() = ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) ==
                PackageManager.PERMISSION_GRANTED

    /**
     * This function is used to send messages to the ForegroundService without expecting a return value.
     */
    fun sendMessageToForegroundserviceWithoutOutput(type: ApplicationNotificationType, message: Any?) {
        if (!isForegroundServiceRunning()) {
            Log.d("MainActivity", "Service is not running, message will not be sent.")
            return
        }
        Log.d("MainActivity", "Sending message to Foregroundservice: ${type.name}")
        val intent = Intent(type.type)
        when (type) { // Only special treatment for types which need parameters
            ApplicationNotificationType.ADD_NUMBER_OF_PENDING_CHUNKS -> {
                val chunks = message as Int
                intent.putExtra("chunk", chunks)
            }
            ApplicationNotificationType.FRONTEND_UP -> {
                intent.putExtra("frontend_up", message as Boolean)
            }
            ApplicationNotificationType.DELETE_FEED -> {
                val feed = message as ByteArray
                intent.putExtra("feed", feed)
            }
            ApplicationNotificationType.SET_NEW_IDENTITY -> {
                if (message != null) {
                    val identity = message as ByteArray
                    intent.putExtra("identity", identity)
                }
            }
            ApplicationNotificationType.WIPE_OTHERS -> {
                // Just a function call
            }
            ApplicationNotificationType.ADD_KEY -> {
                val key = message as ByteArray
                intent.putExtra("key", key)
            }
            ApplicationNotificationType.SET_SETTINGS -> {
                val args = message as List<String>
                intent.putExtra("setting", args[1])
                intent.putExtra("value", args[2])
            }
            ApplicationNotificationType.PUBLISH_PUBLIC_CONTENT -> {
                val content = message as ByteArray
                intent.putExtra("message", content)
            }
        }
        sendBroadcast(intent)
    }

    /**
     *  This object is used to store callbacks for messages that require a return value.
     *  The callbackId is used to identify the callback and the CompletableDeferred is used to store the result.
     *  The callbackId is generated by the system time in milliseconds. The callback is removed from the map as soon as the result is received.
     *
     *  Used for returning values from the ForegroundService to the MainActivity.
     */
    object CallbackRegistry {
        private val callbacks = mutableMapOf<String, CompletableDeferred<Any?>>()

        fun registerCallback(callbackId: String): CompletableDeferred<Any?> {
            val deferred = CompletableDeferred<Any?>()
            callbacks[callbackId] = deferred
            return deferred
        }

        fun completeCallback(callbackId: String, result: Any?) {
            callbacks.remove(callbackId)?.complete(result)
        }
    }

    fun setBatteryMode() {
        batterySave = !batterySave
        if (batterySave) {
            Toast.makeText(this, "Battery save mode enabled", Toast.LENGTH_SHORT).show()
        } else {
            Toast.makeText(this, "Battery save mode disabled.", Toast.LENGTH_SHORT).show()
        }
    }

    /**
     *  This BroadcastReceiver is used to receive results from the ForegroundService.
     *  The callbackId is used to identify the callback and the result is used to store the result.
     *  The callbackId is generated by the system time in milliseconds.
     */
    private val resultReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            try {
                val callbackId = intent.getStringExtra("CallbackID")
                val type = intent.getStringExtra("type")
                val result = intent.getStringExtra("result")
                when (type) {
                    "ByteArray" -> {
                        val decodedResult = result?.let { Base64.getDecoder().decode(it) }
                        Log.d("MainActivity", "Received bytearray result for callbackId: $callbackId, result: ${decodedResult?.size} bytes")
                        CallbackRegistry.completeCallback(callbackId!!, decodedResult)
                    }
                    "String" -> {
                        Log.d("MainActivity", "Received string result for callbackId: $callbackId, result: $result")
                        CallbackRegistry.completeCallback(callbackId!!, result)
                    }
                    "Boolean" -> {
                        val decodedResult = intent.getBooleanExtra("result", false)
                        Log.d("MainActivity", "Received boolean result for callbackId: $callbackId, result: $decodedResult")
                        CallbackRegistry.completeCallback(callbackId!!, decodedResult)
                    }
                    else -> {
                        Log.e("MainActivity", "Unknown result type received for callbackId: $callbackId")
                    }
                }
            } catch (e: Exception) {
                Log.e("MainActivity", "Error receiving result from ForegroundService", e)
            }
        }
    }

    /**
     * This function is used to send messages to the ForegroundService and expect a return value.
     * Notice that not all types require a message to be sent, which is why I left most of them to be empty.
     */
    fun sendMessageToForegroundserviceWithOutput(type: ApplicationNotificationType, message: Any?): CompletableDeferred<Any?> {
        if (!isForegroundServiceRunning()) {
            Log.d("MainActivity", "Service is not running, message will not be sent.")
            return CompletableDeferred(null)
        }
        Log.d("MainActivity", "Sending message to Foregroundservice: ${type.type}")
        val intent = Intent(type.type)
        val callbackId = System.currentTimeMillis().toString()
        Log.d("MainActivity", "CallbackID: $callbackId")
        intent.putExtra("CallbackID", callbackId)
        when (type) { // Only special treatment for types which need parameters
            ApplicationNotificationType.SET_NEW_IDENTITY -> { // There are cases where return value is needed
                val identity = intent.getByteArrayExtra("identity")
                intent.putExtra("identity", identity)
            }
            ApplicationNotificationType.GET_SETTINGS -> { // Returns String
                // This is just a normal function call, nothing needs to be done here
            }
            ApplicationNotificationType.TO_EXPORT_STRING -> { // Returns String
                // Nothing goes here, just a function call
            }
            ApplicationNotificationType.LIST_FEEDS -> { // Returns List<ByteArray>
                // Nothing goes here, just a function call
            }
            ApplicationNotificationType.IDENTITY_TO_REF -> { // Returns String
                // Nothing goes here, just a function call
            }
            ApplicationNotificationType.IS_GEO_ENABLED -> { // Returns Boolean
                // Nothing goes here, just a function call
            }
            ApplicationNotificationType.ENCRYPT_PRIV_MSG -> { // Returns ByteArray
                val bundle = message as Bundle
                val keys = bundle.getStringArrayList("keys")
                val bodyclear = bundle.getByteArray("body_clear")
                intent.putStringArrayListExtra("keys", keys)
                intent.putExtra("body_clear", bodyclear)
            }
            ApplicationNotificationType.DECRYPT_PRIV_MSG -> { // Returns ByteArray
                try {
                    val bodylist = message as ByteArray
                    intent.putExtra("bodyList", bodylist)
                    Log.d("MainActivity", "Adding bodylist to intent (${intent.action}) $bodylist")
                } catch (e: Exception) {
                    Log.e("MainActivity", "Error sending message to ForegroundService", e)
                }
            }
        }
        Log.d("MainActivity", "Sending broadcast")
        sendBroadcast(intent)
        return CallbackRegistry.registerCallback(callbackId)
    }


    /*
    var broadcast_socket: DatagramSocket? = null
    var server_socket: ServerSocket? = null
    var udp: UDPbroadcast? = null
    */
    /*
    val networkRequest = NetworkRequest.Builder()
        .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
        .build()
    private var networkCallback: ConnectivityManager.NetworkCallback? = null
    */
    // var wifiManager: WifiManager? = null
    // private var mlock: WifiManager.MulticastLock? = null
    // lateinit var currentPhotoPath: String
    // private var old_ip_addr: Int = 0 // wifiManager?.connectionInfo!!.ipAddress

    @RequiresApi(Build.VERSION_CODES.O)
    override fun onCreate(savedInstanceState: Bundle?) {
        //val app = applicationContext as TinyApplication
        //app.settings = Settings(this)

        //settings = Settings(this)
        super.onCreate(savedInstanceState)

        val filter = IntentFilter().apply {
            addAction(ForegroundNotificationType.EVALUATION.value)
            addAction(ForegroundNotificationType.TINY_EVENT.value)
            addAction(ForegroundNotificationType.INCOMPLETE_EVENT.value)
        }
        LocalBroadcastManager.getInstance(this).registerReceiver(foregroundServiceReceiver, filter)

        // Register the result receiver (ForegroundService -> MainActivity) which returns us values
        LocalBroadcastManager.getInstance(this).registerReceiver(resultReceiver, IntentFilter("RESULT_ACTION"))

        if (!isForegroundServiceRunning()) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !isLocationPermissionGranted) {
                ActivityCompat.requestPermissions(this, arrayOf(
                    Manifest.permission.ACCESS_FINE_LOCATION), 555)
            }
            val intent = Intent(this, BleForegroundService::class.java)
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                Log.d("MainActivity", "Starting BLE service")
                applicationContext.startForegroundService(intent)  // Für Android 8.0 und höher
            } else {
                Log.d("MainActivity", "Starting BLE service")
                applicationContext.startService(intent)  // Für ältere Android-Versionen
            }
        }
        bindToService() // Bind messenger to service
        Log.d("MainActivity", "Started BLE service")
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT)
        requestWindowFeature(Window.FEATURE_NO_TITLE)
        setContentView(R.layout.activity_main)
        // tremolaState = TremolaState(this)

        Log.d("MainActivity", "Initiated contentView")

        val webView = findViewById<WebView>(R.id.webView)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            webView.setLayerType(
                View.LAYER_TYPE_SOFTWARE,
                null
            ) // disable acceleration, needed for older WebViews
        }
        Log.d("MainActivity", "Initiated WebView")
        //gamesHandler = GamesHandler(idStore.identity)
        // Ich gehe davon aus, dass ich die nächste linie nicht brauche, da dies
        //BleForegroundService.getGamesHandler()?.let { webView.addJavascriptInterface(it, "GameHandler") } // TODO check difference to "GamesHandler"

        Log.d("MainActivity", "Initiating WebAppInterface...")
        //wai = BleForegroundService.getGamesHandler()?.let { WebAppInterface(this, webView, it) }!!
        wai = WebAppInterface(this, webView, null) // TODO add gameshandler
        Log.d("MainActivity", "Initiated WebAppInterface")
        webView.clearCache(true)

        /**
         * This is a workaround to prevent the WebView from crashing due to a memory issue.
         *  - webView.settings.setRenderPriority(WebSettings.RenderPriority.HIGH) // Did not work
         *
         *  - webView.settings.setAppCacheEnabled(false)  // Avoids storing extra cached data
         *    webView.settings.cacheMode = WebSettings.LOAD_NO_CACHE // Forces fresh content loading
         *    webView.settings.setSupportZoom(false) // Disables zoom for lightweight rendering
         *
         *   Optimizing the WebView for performance: (Not really changing anything)
         *  - webView.settings.setJavaScriptCanOpenWindowsAutomatically(true) // Only if necessary
         *    webView.settings.setLoadsImagesAutomatically(true) // Improves perceived load time
          */


        Log.d("MainActivity", "Initiated WebAppInterface")
        webView.addJavascriptInterface(wai, "Android")
        BleForegroundService.getGamesHandler()?.let { webView.addJavascriptInterface(it, "GamesHandler") }
        Log.d("MainActivity", "Added Javascript interfaces")

        webView.settings.javaScriptEnabled = true
        webView.settings.domStorageEnabled = true

        try {
            webView.loadUrl("file:///android_asset/web/tremola.html")
        } catch (e: OutOfMemoryError) {
            Log.e("MainActivity", "Memory issue loading WebView", e)
            webView.clearCache(true)
            webView.reload()
        }
        Log.d("MainActivity", "Finished onCreate()")
    }

    override fun onBackPressed() {
        wai.eval("onBackPressed();")
    }

    fun _onBackPressed() {
        Handler(this.getMainLooper()).post {
            super.onBackPressed()
        }
    }

    // pt 3 in https://betterprogramming.pub/5-android-webview-secrets-you-probably-didnt-know-b23f8a8b5a0c

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        Log.d("activityResult", "request code ${requestCode} ${data}")
        val result = IntentIntegrator.parseActivityResult(requestCode, resultCode, data)
        if (requestCode == 808) {
            if (data == null)
                return
            // val text = data.getStringExtra("text")
            val voice = data.getByteArrayExtra("codec2")
            if (voice != null)
                wai.return_voice(voice) // public_post_with_voice(text, voice)
            return
        }
        if (result != null) {
            Log.d("activityResult", result.toString())
            val cmd = when {
                result.contents == null -> "qr_scan_failure();"
                else -> "qr_scan_success('" + result.contents + "');"
            }
            wai.eval(cmd)
        /* disabled in tinyTremola
        }  else if (requestCode == 1001 && resultCode == RESULT_OK) { // media pick
            val pictureUri = data?.data
            val bitmap = when {
                Build.VERSION.SDK_INT < Build.VERSION_CODES.P /* 28 */ ->
                    MediaStore.Images.Media.getBitmap(this.contentResolver, pictureUri)
                else -> @RequiresApi(Build.VERSION_CODES.P) {
                    val src = ImageDecoder.createSource(this.contentResolver, pictureUri!!)
                    ImageDecoder.decodeBitmap(src)
                }
            }
            val ref = tremolaState.blobStore.storeAsBlob(bitmap)
            tremolaState.wai.eval("b2f_new_image_blob('${ref}')")
        } else if (requestCode == 1002 && resultCode == RESULT_OK) { // camera
            val ref = tremolaState.blobStore.storeAsBlob(currentPhotoPath)
            tremolaState.wai.eval("b2f_new_image_blob('${ref}')")
        */
        /* no file loading in tinyTremola
        } else if (requestCode == 808 && resultCode == RESULT_OK) { // voice
            val voice = "abc" // result!!.contents
            tremolaState.wai.eval("b2f_new_voice('${voice}')")
        */
        } else if (requestCode == 555 && resultCode == RESULT_OK) { // enable fine grained location
            // Might not be necessary anymore, as the BLE Foreground Service is started in onResume()
            //ble?.startBluetooth() // this should already happen in foreground service
        }
        super.onActivityResult(requestCode, resultCode, data)
    }

    override fun onResume() {
        Log.d("MainActivity", "onResume")
        super.onResume()

        /*
        try {
            (getSystemService(CONNECTIVITY_SERVICE) as ConnectivityManager)
                .registerNetworkCallback(networkRequest, networkCallback!!)
        } catch (e: Exception) {}
        */

        /*try {
            ble = BlePeers(this)
            ble?.startBluetooth()
        } catch (e: Exception) {
            ble = null
        }*/

        Log.d("MainActivity", "Starting websocket soon ...")

        // This is moved to ForegroundService aswell
        /*websocket = WebsocketIO(this, settings!!.getWebsocketUrl())
        websocket!!.start()*/
        Log.d("MainActivity", "Started websocket")

        //val filter = IntentFilter("MESSAGE_FROM_SERVICE")
        //LocalBroadcastManager.getInstance(this).registerReceiver(foregroundserviceReceiver, filter)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !isLocationPermissionGranted) {
            ActivityCompat.requestPermissions(this, arrayOf(
                Manifest.permission.ACCESS_FINE_LOCATION), 555)
        }
        if (isForegroundServiceRunning()) {
            sendMessageToForegroundserviceWithoutOutput(ApplicationNotificationType.FRONTEND_UP, true)
            return
        }
        val intent = Intent(this, BleForegroundService::class.java)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            Log.d("MainActivity", "Starting BLE service")
            applicationContext.startForegroundService(intent)  // Für Android 8.0 und höher
        } else {
            Log.d("MainActivity", "Starting BLE service")
            applicationContext.startService(intent)  // Für ältere Android-Versionen
        }
    }

    override fun onPause() {
        Log.d("onPause", "")
        super.onPause()
        sendMessageToForegroundserviceWithoutOutput(ApplicationNotificationType.FRONTEND_UP, false)
        //ble?.stopBluetooth() // This happens in the ForegroundService

        if (websocket != null)
            websocket!!.stop()

        /*
        try {
            (getSystemService(CONNECTIVITY_SERVICE) as ConnectivityManager)
                .unregisterNetworkCallback(networkCallback!!)
        } catch (e: Exception) {}
        */
    }

    override fun onStop() {
        Log.d("onStop", "")
        super.onStop()
        sendMessageToForegroundserviceWithoutOutput(ApplicationNotificationType.FRONTEND_UP, false)
    }
    override fun onDestroy() {
        Log.d("onDestroy", "")
        sendMessageToForegroundserviceWithoutOutput(ApplicationNotificationType.FRONTEND_UP, false)
        /*
        try { broadcast_socket?.close() } catch (e: Exception) {}
        broadcast_socket = null
        try { server_socket?.close() } catch (e: Exception) {}
        server_socket = null
        */
        super.onDestroy()
        //ble?.stopBluetooth()

        val batteryManager = getSystemService(Context.BATTERY_SERVICE) as BatteryManager
        val batteryLevel = batteryManager.getIntProperty(BatteryManager.BATTERY_PROPERTY_CAPACITY)
        if (batterySave && batteryLevel <= 20) {
            val intent = Intent(this, BleForegroundService::class.java)
            stopService(intent)
        }

        if (websocket != null) {
            websocket!!.stop()
        }

        // TODO change back if turns out to be whack in Foreground Service
        //unregisterReceiver(broadcastReceiver)
        //unregisterReceiver(ble_event_listener)
        LocalBroadcastManager.getInstance(this).unregisterReceiver(resultReceiver)
        LocalBroadcastManager.getInstance(this).unregisterReceiver(foregroundServiceReceiver)
        serviceConnection?.let {
            unbindService(it)
        }
    }

    fun mkSockets() {
        /* disable UDP advertisements in the tinyTremola version

        try { broadcast_socket?.close() } catch (e: Exception) {}
        broadcast_socket = null
        try {
            broadcast_socket = DatagramSocket(null)
            broadcast_socket?.reuseAddress = true
            broadcast_socket?.broadcast = true // really necessary ?
            val any = InetAddress.getByAddress(ByteArray(4))
            broadcast_socket?.bind(InetSocketAddress(any, Constants.SSB_IPV4_UDPPORT)) // where to listen
        } catch (e: Exception) {
            Log.d("create broadcast socket", "${e}")
            broadcast_socket = null
        }
        Log.d("new bcast sock", "${broadcast_socket}, UDP port ${broadcast_socket?.localPort}")
        */

        if(!BleForegroundService.getTinySettings()!!.isUdpMulticastEnabled())
            return

        rmSockets()
        try {
            mc_group = InetAddress.getByName(Constants.SSB_VOSSBOL_MC_ADDR)
            mc_socket= MulticastSocket(Constants.SSB_VOSSBOL_MC_PORT)
            // mc_socket?.reuseAddress = true
            // mc_socket?.broadcast = true // really necessary ?
            // val any = InetAddress.getByAddress(ByteArray(4))
            // mc_socket?.bind(InetSocketAddress(any, Constants.SSB_VOSSBOL_MC_PORT)) // where to listen
            mc_socket?.loopbackMode = true
            mc_socket?.joinGroup(mc_group)
        } catch (e: Exception) {
            Log.d("mkSockets exc", e.toString())
            rmSockets()
        }
        /* disable TCP server in the tinyTremola version

        // val wifiManager = applicationContext.getSystemService(Context.WIFI_SERVICE) as WifiManager
        if (old_ip_addr == 0 || old_ip_addr != wifiManager?.connectionInfo!!.ipAddress) {
            try {
                server_socket?.close()
            } catch (e: Exception) {
            }
            server_socket = ServerSocket(Constants.SSB_IPV4_TCPPORT)
            old_ip_addr = wifiManager?.connectionInfo!!.ipAddress
            Log.d(
                "SERVER TCP addr is new",
                "${Formatter.formatIpAddress(wifiManager?.connectionInfo!!.ipAddress)}:${server_socket!!.localPort}"
            )
        } else {
            Log.d(
                "SERVER TCP addr did not change:",
                "${Formatter.formatIpAddress(wifiManager?.connectionInfo!!.ipAddress)}:${server_socket!!.localPort}"
            )
        }
        */
    }

    fun rmSockets() {
        try { mc_socket?.leaveGroup(mc_group); mc_socket?.close() } catch (e: Exception) {}
        mc_group = null
        mc_socket = null
    }

    /**
     * Checks if the BLE foreground service is already running.
     */
    private fun isForegroundServiceRunning(): Boolean {
        val manager = getSystemService(Context.ACTIVITY_SERVICE) as ActivityManager
        for (service in manager.getRunningServices(Int.MAX_VALUE)) {
            if (BleForegroundService::class.java.name == service.service.className) {
                Log.d("MainActivity", "Service is already running.")
                return true
            }
        }
        Log.d("MainActivity", "Service was not yet running.")
        return false
    }
}
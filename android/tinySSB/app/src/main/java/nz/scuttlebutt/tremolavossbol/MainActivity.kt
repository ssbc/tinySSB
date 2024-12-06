package nz.scuttlebutt.tremolavossbol

// import android.R

// import nz.scuttlebutt.tremolavossbol.tssb.ble.BlePeers

import MyWorker
import android.app.Activity
import android.bluetooth.BluetoothAdapter
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.ActivityInfo
import android.net.NetworkInfo
import android.net.Uri
import android.net.wifi.WifiManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.util.Log
import android.view.View
import android.view.Window
import android.webkit.WebResourceRequest
import android.webkit.WebResourceResponse
import android.webkit.WebView
import androidx.annotation.RequiresApi
import androidx.appcompat.app.AppCompatDelegate
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.webkit.WebViewAssetLoader
import androidx.webkit.WebViewAssetLoader.AssetsPathHandler
import androidx.webkit.WebViewClientCompat
import androidx.work.PeriodicWorkRequestBuilder
import androidx.work.WorkManager
import com.google.zxing.integration.android.IntentIntegrator
import nz.scuttlebutt.tremolavossbol.crypto.IdStore
import nz.scuttlebutt.tremolavossbol.tssb.Demux
import nz.scuttlebutt.tremolavossbol.tssb.GOset
import nz.scuttlebutt.tremolavossbol.tssb.IO
import nz.scuttlebutt.tremolavossbol.tssb.Node
import nz.scuttlebutt.tremolavossbol.tssb.Repo
import nz.scuttlebutt.tremolavossbol.tssb.WebsocketIO
import nz.scuttlebutt.tremolavossbol.tssb.ble.BlePeers
import nz.scuttlebutt.tremolavossbol.tssb.ble.BluetoothEventListener
import nz.scuttlebutt.tremolavossbol.utils.Constants
import org.json.JSONObject
import tremolavossbol.R
import java.io.File
import java.net.InetAddress
import java.net.MulticastSocket
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.concurrent.TimeUnit
import java.util.concurrent.locks.ReentrantLock
import kotlin.concurrent.thread


// import nz.scuttlebutt.tremolavossbol.MainActivity


class MainActivity : Activity() {
    // lateinit var tremolaState: TremolaState
    lateinit var idStore: IdStore
    lateinit var wai: WebAppInterface
    lateinit var tinyIO: IO
    var frontend_ready = false
    val tinyNode = Node(this)
    val tinyRepo = Repo(this)
    val tinyDemux = Demux(this)
    val tinyGoset = GOset(this)
    var settings: Settings? = null
    @Volatile var mc_group: InetAddress? = null
    @Volatile var mc_socket: MulticastSocket? = null
    var ble: BlePeers? = null
    var websocket: WebsocketIO? =null
    val ioLock = ReentrantLock()
    var broadcastReceiver: BroadcastReceiver? = null
    var isWifiConnected = false
    var ble_event_listener: BluetoothEventListener? = null

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
        super.onCreate(savedInstanceState)

        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES)
        requestWindowFeature(Window.FEATURE_NO_TITLE)
        setRequestedOrientation (ActivityInfo.SCREEN_ORIENTATION_PORTRAIT)

        setContentView(R.layout.activity_main)

        val windowInsetsController =
            WindowCompat.getInsetsController(window, window.decorView)
        windowInsetsController!!.systemBarsBehavior =
            WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        windowInsetsController.hide(WindowInsetsCompat.Type.navigationBars())

        settings = Settings(this)
        idStore = IdStore(this)

        // wifiManager = applicationContext.getSystemService(WIFI_SERVICE) as WifiManager
        // mlock = wifiManager?.createMulticastLock("lock")
        // if (!mlock!!.isHeld) mlock!!.acquire()
        // mkSockets()

        Log.d("IDENTITY", "is ${idStore.identity.toRef()} (${idStore.identity.verifyKey})")

        val webView = findViewById<WebView>(R.id.webView)
        val assetLoader = WebViewAssetLoader.Builder()
            .addPathHandler("/assets/", AssetsPathHandler(this))
            .build()
        webView.webViewClient = object : WebViewClientCompat() {
            override fun shouldInterceptRequest(
                view: WebView,
                request: WebResourceRequest
            ): WebResourceResponse? {
                return assetLoader.shouldInterceptRequest(request.url)
            }
            @Suppress("deprecation") // for API < 21
            override fun shouldInterceptRequest(view: WebView, url: String): WebResourceResponse? {
                return assetLoader.shouldInterceptRequest(Uri.parse(url))
            }
        }

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P) {
            webView.setLayerType(
                View.LAYER_TYPE_SOFTWARE,
                null
            ) // disable acceleration, needed for older WebViews
        }

        wai = WebAppInterface(this, webView)
        // upgrades repo filesystem if necessary
        tinyRepo.upgrade_repo()
        tinyIO = IO(this, wai)
        tinyGoset._include_key(idStore.identity.verifyKey) // make sure our local key is in
        tinyRepo.load()
        tinyGoset.adjust_state()
        tinyDemux.arm_dmx(tinyGoset.goset_dmx,  {buf:ByteArray, aux:ByteArray?, _ -> tinyGoset.rx(buf,aux)}, null)
        tinyDemux.arm_dmx(tinyDemux.want_dmx!!, {buf:ByteArray, aux:ByteArray?, sender:String? -> tinyNode.incoming_want_request(buf,aux,sender)})
        tinyDemux.arm_dmx(tinyDemux.chnk_dmx!!, { buf:ByteArray, aux:ByteArray?, _ -> tinyNode.incoming_chunk_request(buf,aux)})

        webView.clearCache(true)
        webView.addJavascriptInterface(wai, "Android")

        webView.settings.javaScriptEnabled = true
        webView.settings.domStorageEnabled = true

        webView.loadUrl("https://appassets.androidplatform.net/assets/web/tremola.html")

        broadcastReceiver = object : BroadcastReceiver() {
            override fun onReceive(context: Context?, intent: Intent?) {
                val networkInfo: NetworkInfo? =
                    intent!!.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO)
                if (networkInfo == null)
                    return
                // Toast.makeText(this@MainActivity, "Wifi State Changed! connected=${networkInfo?.detailedState}", Toast.LENGTH_SHORT).show()
                if (networkInfo.detailedState == NetworkInfo.DetailedState.CONNECTED && !isWifiConnected) {
                    isWifiConnected = true
                    Handler().postDelayed({
                        mkSockets()
                        if (websocket == null)
                            websocket = WebsocketIO(this@MainActivity, settings!!.getWebsocketUrl())
                        websocket!!.start()
                        Log.d("main", "msc_sock ${mc_socket.toString()}")
                    }, 1000)
                } else if (networkInfo.detailedState == NetworkInfo.DetailedState.DISCONNECTED && isWifiConnected) {
                    rmSockets()
                    isWifiConnected = false
                    Log.d("main", "msc_sock ${mc_socket.toString()}")
                    if(websocket != null) {
                        websocket!!.stop()
                        websocket = null
                    }
                }
            }
        }
        registerReceiver(broadcastReceiver, IntentFilter(WifiManager.NETWORK_STATE_CHANGED_ACTION))

        ble_event_listener = BluetoothEventListener(this)
        registerReceiver(ble_event_listener, IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED))

        // val lck = ReentrantLock()
        /* disable TCP server and UDP advertisements in the tinyTremola version

        udp = UDPbroadcast(this, tremolaState.wai)

        val t0 = thread(isDaemon=true) {
            try {
                udp!!.beacon(tremolaState.idStore.identity.verifyKey, lck, Constants.SSB_IPV4_TCPPORT)
            } catch (e: Exception) {
                Log.d("beacon thread", "died ${e}")
            }
        }

        val t1 = thread(isDaemon=true) {
            try {
                udp!!.listen(lck)
            } catch (e: Exception) {
                Log.d("listen thread", "died ${e}")
            }
        }
        val t2 = thread(isDaemon=true)  { // accept loop, robust against reassigned server_socket
             while (true) {
                 var socket: Socket?
                 try {
                     socket = server_socket!!.accept()
                 } catch (e: Exception) {
                     sleep(3000)
                     continue
                 }
                 thread() { // one thread per connection
                     val rpcStream = RpcResponder(tremolaState, socket,
                         Constants.SSB_NETWORKIDENTIFIER)
                     rpcStream.defineServices(RpcServices(tremolaState))
                     rpcStream.startStreaming()
                 }
            }
        }
        */

        val t3 = thread(isDaemon=true) {
            try {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    tinyIO.senderLoop()
                }
            } catch (e: Exception) {
                Log.d("tssb sender thread", "died ${e}")
            }
        }
        val t4 = thread(isDaemon=true) {
            tinyIO.mcReceiverLoop(ioLock)
        }
        val t5 = thread(isDaemon=true) {
            tinyGoset.loop()
        }
        val t6 = thread(isDaemon=true) {
            tinyNode.loop(ioLock)
        }

        /*
        t0.priority = 10
        t1.priority = 10
        t2.priority = 6
        Log.d("Thread priorities", "${t0.priority} ${t1.priority} ${t2.priority}")
        */

        t3.priority = 8
        t4.priority = 10
        t5.priority = 8
        t6.priority = 8

        scheduleWorker(this)

    }

    fun scheduleWorker(context: Context) {
        val workRequest = PeriodicWorkRequestBuilder<MyWorker>(15, TimeUnit.MINUTES)
            .build()

        WorkManager.getInstance(context).enqueueUniquePeriodicWork(
            "NewRealWorker", // Unique name for the worker to prevent duplicates
            androidx.work.ExistingPeriodicWorkPolicy.KEEP, // Keep the existing worker if already scheduled
            workRequest
        )
    }

    fun removeEntryFromJsonFile(keyHex: String) {
        //get the json file with name "untrusted.json", create one if it doesn't exist
        val jsonFile = File(filesDir, "contacts.json")
        if (!jsonFile.exists()) {
            jsonFile.createNewFile()
        }

        //if the json file is empty, return
        if (jsonFile.length() == 0L) {
            return
        }
        //read the json file
        val json = jsonFile.readText()
        //parse the json string to a json object
        val jsonObject = JSONObject(json)

        //if hex key is in the json object, remove it
        if (jsonObject.has(keyHex)) {
            jsonObject.remove(keyHex)
        }

        //write the json object to the json file
        jsonFile.writeText(jsonObject.toString())
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
        }  else if (requestCode == 555 && resultCode == RESULT_OK) { // enable fine grained location
            ble?.startBluetooth()
        }

        super.onActivityResult(requestCode, resultCode, data)
    }

    fun saveContactToJSONFile(keyHex: String, hours: Int) {
        //get the json file with name "contacts.json", create one if it doesn't exist
        val jsonFile = File(filesDir, "contacts.json")
        if (!jsonFile.exists()) {
            jsonFile.createNewFile()
        }

        //get current time
        var currentTime = System.currentTimeMillis()

        //Add number of hours to the current time
        currentTime += (hours - 24) * 3600000

        val formattedTime = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault()).format(Date(currentTime))

        //if the json file is empty, add the first entry
        if (jsonFile.length() == 0L) {
            val jsonObject = JSONObject()
            jsonObject.put(keyHex, formattedTime)
            jsonFile.writeText(jsonObject.toString())
            return
        }
        //read the json file
        val json = jsonFile.readText()
        //parse the json string to a json object
        val jsonObject = JSONObject(json)

        //if hex key is not already in the json object, add it
        if (!jsonObject.has(keyHex)) {
            jsonObject.put(keyHex, formattedTime)
        }

        //write the json object to the json file
        jsonFile.writeText(jsonObject.toString())
    }

    // Function to put all untrusted contacts in the JSON file
    fun registerUntrusted() {
        val contactsTrustLevel = wai.getContactsTrustLevel();
        //contactsTrustLevel is a map<string,int>
        Log.d("contactsTrustLevel", contactsTrustLevel.toString())
        for (contact in contactsTrustLevel) {
            if (contact.value == 0) {
                saveContactToJSONFile(contact.key, 24)
            } else {
                removeEntryFromJsonFile(contact.key)
            }
        }

    }

    private fun executeThisTask() {
        // Add your task logic here
        Log.d("MyRealWorker", "Worker is working...")

        // write in a txt file the time at which the worker was executing
        val currentTime = System.currentTimeMillis()

        // Format the time into a readable string
        val formattedTime = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault()).format(Date(currentTime))

        applicationContext.openFileOutput("output.txt", Context.MODE_APPEND).use {
            it.write("$formattedTime\n".toByteArray()) // Appending the formatted time with a newline
        }

        //if json file does not exist, return
        val jsonFile = File(filesDir, "contacts.json")
        if (!jsonFile.exists()) {
            return
        }

        //prepare kill list file if it doesn't exist
        val killListFile = applicationContext.openFileOutput("killList.txt", Context.MODE_APPEND)

        //Open JSON file and check all keys (contacts) and their value (time)
        val jsonFileNew = File(filesDir, "contacts.json").bufferedReader().use { it.readText() }
        val json = JSONObject(jsonFileNew)
        val keys = json.keys()
        for (key in keys) {
            val value = json.getString(key)
            //check if the value is older than currentTime, value has following format "yyyy-MM-dd HH:mm:ss"
            if (SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault()).parse(value).time < currentTime) {
                //add key to kill list
                killListFile.write("$key\n".toByteArray())
                //remove key from contacts.json
                removeEntryFromJsonFile(key)
            }
        }
    }

    // Function to delete all contacts in the kill list
    fun deleteFromKillList() {
        val killListFile = File(filesDir, "killList.txt")
        Log.d("deleteFromKillList", "Deleting contacts")
        if (!killListFile.exists()) {
            killListFile.createNewFile()
        }
        val killList = killListFile.readLines()
        Log.d("deleteFromKillList", "Kill list is $killList")
        for (contact in killList) {
            Log.d("deleteFromKillList", "Looking at " + contact)
            //if trust level is 0, delete the contact
            val contactTrustLevel = wai.getContactTrust(contact)
            if (contactTrustLevel == 0) {
                Log.d("deleteFromKillList", "Deleting contact $contact with trust level 0")
                removeEntryFromJsonFile(contact)
                wai.deleteContact(contact)
                //remove the contact from the kill list
                killListFile.writeText(killList.filter { it != contact }.joinToString("\n"))
            }
        }
    }

    override fun onResume() {
        Log.d("onResume", "")
        registerUntrusted()
        deleteFromKillList()
        super.onResume()
        /*
        try {
            (getSystemService(CONNECTIVITY_SERVICE) as ConnectivityManager)
                .registerNetworkCallback(networkRequest, networkCallback!!)
        } catch (e: Exception) {}
        */

        try {
            ble = BlePeers(this)
            ble?.startBluetooth()
        } catch (e: Exception) {
            ble = null
        }

        websocket = WebsocketIO(this, settings!!.getWebsocketUrl())
        websocket!!.start()
    }

    override fun onPause() {
        Log.d("onPause", "")
        super.onPause()
        ble?.stopBluetooth()

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
    }
    override fun onDestroy() {
        Log.d("onDestroy", "")
        /*
        try { broadcast_socket?.close() } catch (e: Exception) {}
        broadcast_socket = null
        try { server_socket?.close() } catch (e: Exception) {}
        server_socket = null
        */
        super.onDestroy()
        ble?.stopBluetooth()

        if (websocket != null) {
            websocket!!.stop()
        }

        unregisterReceiver(broadcastReceiver)
        unregisterReceiver(ble_event_listener)
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

        if(!settings!!.isUdpMulticastEnabled())
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
}
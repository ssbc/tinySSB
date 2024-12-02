package nz.scuttlebutt.tremolavossbol.tssb.ble

import android.Manifest
import android.annotation.SuppressLint
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.PackageManager
import android.content.pm.ServiceInfo
import android.location.LocationManager
import android.net.NetworkInfo
import android.net.wifi.WifiManager
import android.os.Build
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.util.Log
import android.view.View
import android.webkit.WebView
import android.widget.Toast
import androidx.annotation.RequiresApi
import androidx.core.app.ActivityCompat
import androidx.core.app.NotificationCompat
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import nz.scuttlebutt.tremolavossbol.MainActivity
import nz.scuttlebutt.tremolavossbol.Settings
import nz.scuttlebutt.tremolavossbol.WebAppInterface
import nz.scuttlebutt.tremolavossbol.crypto.IdStore
import nz.scuttlebutt.tremolavossbol.games.common.GamesHandler
import nz.scuttlebutt.tremolavossbol.tssb.Demux
import nz.scuttlebutt.tremolavossbol.tssb.GOset
import nz.scuttlebutt.tremolavossbol.tssb.IO
import nz.scuttlebutt.tremolavossbol.tssb.Node
import nz.scuttlebutt.tremolavossbol.tssb.Repo
import nz.scuttlebutt.tremolavossbol.tssb.WebsocketIO
import nz.scuttlebutt.tremolavossbol.utils.Constants
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_DIR
import tremolavossbol.R
import java.io.File
import java.net.InetAddress
import java.net.MulticastSocket
import java.util.concurrent.Executor
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import java.util.concurrent.Future
import java.util.concurrent.TimeUnit
import java.util.concurrent.locks.ReentrantLock

/**
 * This class handles the BLE replication process in the foreground.
 * Beware: The term "foreground" means that the service is running in the background, but the user is aware of it.
 */

class BleForegroundService: Service() {

    companion object {
        // This is the only way to access the service from outside
        // There is the option to do it with intents, but this is more convenient
        // and the service is a singleton anyway.
        // I decided against using reference of ForegroundService and for the usage
        // of the individual objects to allow parallel accesses to different objects.

        private var tinyNodeLock = ReentrantLock()
        @Volatile
        private var tinyNode: Node? = null
        fun getTinyNode(): Node? {
            if (tinyNodeLock.tryLock()) {
                try {
                    return tinyNode
                } finally {
                    tinyNodeLock.unlock()
                }
            }
            return null
        }

        private var tinyRepoLock = ReentrantLock()
        @Volatile
        private var tinyRepo: Repo? = null

        fun getTinyRepo(): Repo? {
            if (tinyRepoLock.tryLock()) {
                try {
                    return tinyRepo
                } finally {
                    tinyRepoLock.unlock()
                }
            }
            return null
        }

        private var tinyGosetLock = ReentrantLock()
        @Volatile
        private var tinyGoset: GOset? = null

        fun getTinyGoset(): GOset? {
            if (tinyGosetLock.tryLock()) {
                try {
                    return tinyGoset
                } finally {
                    tinyGosetLock.unlock()
                }
            }
            return null
        }
        private var tinyIdStoreLock = ReentrantLock()
        @Volatile
        private var tinyIdStore: IdStore? = null

        fun getTinyIdStore(): IdStore? {
            if (tinyIdStoreLock.tryLock()) {
                try {
                    return tinyIdStore
                } finally {
                    tinyIdStoreLock.unlock()
                }
            }
            return null
        }
        private var gamesHandlerLock = ReentrantLock()
        @Volatile
        private var gamesHandler: GamesHandler? = null

        fun getGamesHandler(): GamesHandler? {
            if (gamesHandlerLock.tryLock()) {
                try {
                    return gamesHandler
                } finally {
                    gamesHandlerLock.unlock()
                }
            }
            return null
        }
        private var tinyDemuxLock = ReentrantLock()
        @Volatile
        private var tinyDemux: Demux? = null

        fun getTinyDemux(): Demux? {
            if (tinyDemuxLock.tryLock()) {
                try {
                    return tinyDemux
                } finally {
                    tinyDemuxLock.unlock()
                }
            }
            return null
        }
        private var tinyBleLock = ReentrantLock()
        @Volatile
        private var tinyBle: BlePeers? = null

        fun getTinyBle(): BlePeers? {
            if (tinyBleLock.tryLock()) {
                try {
                    return tinyBle
                } finally {
                    tinyBleLock.unlock()
                }
            }
            return null
        }
        private var tinyGamesHandlerLock = ReentrantLock()
        @Volatile
        private var tinyGamesHandler: GamesHandler? = null

        fun getTinyGamesHandler(): GamesHandler? {
            if (tinyGamesHandlerLock.tryLock()) {
                try {
                    return tinyGamesHandler
                } finally {
                    tinyGamesHandlerLock.unlock()
                }
            }
            return null
        }

        private var tinyIOLock = ReentrantLock()
        @Volatile
        private lateinit var tinyIO: IO
        fun getTinyIO(): IO? {
            if (tinyIOLock.tryLock()) {
                try {
                    return tinyIO
                } finally {
                    tinyIOLock.unlock()
                }
            }
            return null
        }
    }

    private val CHANNEL_ID = "BLE_FOREGROUND_SERVICE_CHANNEL"
    var frontend_ready = false
    var settings: Settings? = null
    @Volatile var mc_group: InetAddress? = null
    @Volatile var mc_socket: MulticastSocket? = null
    var ble: BlePeers? = null
    var websocket: WebsocketIO? =null
    val ioLock = ReentrantLock()
    var wifiBroadcastReceiver: BroadcastReceiver? = null
    var isWifiConnected = false
    var ble_event_listener: BluetoothEventListener? = null

    private var isRunning = true
    private val THREAD_COUNT = 6 // 5 BLE + 1 Test TODO remove test if everything seems to work
    private val executor: ExecutorService = Executors.newFixedThreadPool(THREAD_COUNT)
    private lateinit var bluetoothEventListener: BluetoothEventListener
    private var bleThreadFuture: Future<*>? = null
    private var testThreadFuture: Future<*>? = null
    private var senderloopThreadFuture: Future<*>? = null
    private var mcreceiverThreadFuture: Future<*>? = null
    private var gosetLoopThreadFuture: Future<*>? = null
    private var nodeLoopThreadFuture: Future<*>? = null

    @SuppressLint(
        "SetJavaScriptEnabled"
    )
    @RequiresApi(
        Build.VERSION_CODES.O
    )
    override fun onCreate() {
        Log.d("BleForegroundService", "Creating Service")
        settings = Settings(this)
        if (!settings!!.isBleEnabled()) {
            showToastOnMainThread("Bluetooth is disabled!")
            return
        }
        val filter = IntentFilter("MESSAGE_FROM_ACTIVITY")
        LocalBroadcastManager.getInstance(this).registerReceiver(mainActivityReceiver, filter)
        super.onCreate()
        initializeFields()
        registerReceivers()
    }

    /**
     *
     */
    private fun registerReceivers() {
        // Bluetooth Event Listener
        Log.d("BleForegroundService", "Registering Event Listeners")
        bluetoothEventListener = BluetoothEventListener(this)
        registerReceiver(bluetoothEventListener, IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED))

        // WiFi Event Listener
        wifiBroadcastReceiver = object : BroadcastReceiver() {
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
                            websocket = WebsocketIO(this@BleForegroundService, settings!!.getWebsocketUrl())
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
        registerReceiver(wifiBroadcastReceiver, IntentFilter(WifiManager.NETWORK_STATE_CHANGED_ACTION))
        Log.d("BleForegroundService", "Event Listeners registered.")
    }

    /**
     * This function is being called to initialize the fields of the BLE Foreground Service.
     * It is being called in the onCreate() method and contains mostly the content from the previous MainActivity.
     */
    @RequiresApi(
        Build.VERSION_CODES.O
    )
    private fun initializeFields() {
        try {
            Log.d("BleForegroundService", "Initializing fields")
            var tinySsbDir = File(getDir("tinyssb", MODE_PRIVATE), "feeds")
            if (tinySsbDir != null && !tinySsbDir.exists()) {
                Log.d("BleForegroundService", "Creating directory: ${tinySsbDir.absolutePath}")
            } else if (tinySsbDir != null){
                Log.d("BleForegroundService", "Directory already exists: ${tinySsbDir.absolutePath}")
            }

            tinyIdStore = IdStore(this)
            Log.d("BleForegroundService", "IdStore initialized")
            tinyRepo = Repo(this)
            tinyRepo!!.upgrade_repo()
            Log.d("BleForegroundService", "Repo initialized")
            tinyIO = IO(this)
            tinyGoset = GOset(this)
            Log.d("BleForegroundService", "GOset initialized")
            tinyGoset!!._include_key(tinyIdStore!!.identity.verifyKey) // make sure our local key is in
            Log.d("BleForegroundService", "GOset key included")
            tinyRepo!!.load()
            Log.d("BleForegroundService", "Repo loaded")
            tinyDemux = Demux(this)
            tinyGoset!!.adjust_state()
            tinyNode = Node(this)
            Log.d("BleForegroundService", "GOset state adjusted")
            tinyDemux!!.arm_dmx(tinyGoset!!.goset_dmx,  {buf:ByteArray, aux:ByteArray?, _ -> tinyGoset!!.rx(buf,aux)}, null)
            tinyDemux!!.arm_dmx(tinyDemux!!.want_dmx!!, {buf:ByteArray, aux:ByteArray?, sender:String? -> tinyNode!!.incoming_want_request(buf,aux,sender)})
            tinyDemux!!.arm_dmx(tinyDemux!!.chnk_dmx!!, { buf:ByteArray, aux:ByteArray?, _ -> tinyNode!!.incoming_chunk_request(buf,aux)})
            Log.d("IDENTITY", "is ${tinyIdStore!!.identity.toRef()} (${tinyIdStore!!.identity.verifyKey})")
            gamesHandler = GamesHandler(tinyIdStore!!.identity)
            Log.d("BleForegroundService", "Fields initialized")
        } catch (e: Exception) {
            Log.e("BleForegroundService", "Error while initializing fields: $e")
        }
    }

    /**
     * This function is being called to check if the location is enabled on the device.
     */
    private fun isLocationEnabled(): Boolean {
        val locationManager = getSystemService(Context.LOCATION_SERVICE) as LocationManager
        return locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER)
    }

    /**
     * This receiver is used to receive messages from the MainActivity, which need to be sent to the BLE device.
     */
    private val mainActivityReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val message = intent.getByteArrayExtra("message")
            if (message != null) {
                Log.d("BleForegroundService", "(1) Received message from activity: ${message.toString(Charsets.UTF_8)}")
                // TODO add here method which sends the message to the BLE device
                handleOutgoingMessage(message)
            }
        }
    }

    private fun handleOutgoingMessage(message: ByteArray) {
        Log.d("BleForegroundService", "(2) Received message from activity: $message")
        // Here, send the message over Bluetooth or process it
    }

    private fun sendMessageToActivity(message: ByteArray, actionType: ForegroundNotificationType) {
        val intent = Intent(actionType.value)
        intent.putExtra("message", message)
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent)
    }

    @RequiresApi(
        Build.VERSION_CODES.O
    )
    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        Log.d("BleForegroundService", "Service started")

        // Initialize Bluetooth Adapter and Scanner

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                CHANNEL_ID,
                "BLE Service Channel",
                NotificationManager.IMPORTANCE_HIGH
            )
            val manager = getSystemService(NotificationManager::class.java)
            manager?.createNotificationChannel(channel)
        }
        val notification = NotificationCompat.Builder(this, CHANNEL_ID)
            .setOngoing(true) // precludes the user from removing the Notification manually
            .setContentTitle("BLE Service")
            .setContentText("Continuing Replication in the Background.")
            .setSmallIcon(R.drawable.icon)
            .setPriority(NotificationCompat.PRIORITY_HIGH)
            .build()

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            Log.d("BleForegroundService", "Starting service in foreground with connected device and location type")
            startForeground(
                1,
                notification,
                ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE or ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION
            )
        } else {
            Log.d("BleForegroundService", "Starting service in foreground")
            startForeground(1, notification)
        }
        startOperations()
        return START_STICKY
    }

    override fun onDestroy() {
        shutdownOperations()
        super.onDestroy()
        Log.d("MyForegroundService", "Service destroyed")
        LocalBroadcastManager.getInstance(this).unregisterReceiver(mainActivityReceiver)
        unregisterReceiver(bluetoothEventListener)
        /*unregisterReceiver(broadcastReceiver)
        unregisterReceiver(ble_event_listener)

        websocket?.stop()
        websocket = null*/
    }

    /**
     *  This function is being called to start the operations of the BLE Foreground Service.
     *  This method could be expanded to start different kinds of operations in the foreground,
     *  but you might consider renaming things, splitting or create additional services.
     */
    @RequiresApi(
        Build.VERSION_CODES.O
    )
    public fun startOperations() {
        /**
         * The following code is for testing purposes only.
         * Notice: applicationContext as MainApplication cannot be used from Service, as it seems to cause trouble
         */
        isRunning = true
        if (testThreadFuture == null || testThreadFuture?.isDone == true || testThreadFuture?.isCancelled == true) {
            testThreadFuture = executor.submit {
                Log.d("BleForegroundService", "Attempting to start test thread.")
                try {
                    while (isRunning) {
                        val threadStates = listOf(
                            "bleThreadFuture" to bleThreadFuture,
                            "testThreadFuture" to testThreadFuture,
                            "senderloopThreadFuture" to senderloopThreadFuture,
                            "mcreceiverThreadFuture" to mcreceiverThreadFuture,
                            "gosetLoopThreadFuture" to gosetLoopThreadFuture,
                            "nodeLoopThreadFuture" to nodeLoopThreadFuture
                        )

                        threadStates.forEach { (name, future) ->
                            val state = when {
                                future == null -> "Not started"
                                future.isCancelled -> "Cancelled"
                                future.isDone -> "Completed"
                                else -> "Running"
                            }
                            Log.d("ThreadMonitor", "Thread: $name is $state")
                        }

                        // Sleep for a period to avoid spamming logs
                        Thread.sleep(60000) // 5 seconds
                    }
                    Log.d("BleForegroundService", "Test thread stopped.")
                } catch (e: Exception) {
                    Log.d("BleForegroundService", "Foreground thread 1 died $e")
                    Thread.currentThread().interrupt()
                }
            }
        }

        // Sender-Loop
        if (senderloopThreadFuture == null || senderloopThreadFuture?.isCancelled == true || senderloopThreadFuture?.isDone == true) {
            senderloopThreadFuture = executor.submit {
                Log.d("BleForegroundService", "Attempting to start Sender-Loop.")
                try {
                    getTinyIO()!!.senderLoop()
                } catch (e: Exception) {
                    Log.d("BleForegroundService", "Sender-Loop died $e")
                    Thread.currentThread().interrupt()
                }
            }
        }

        // MC-Receiver
        if (mcreceiverThreadFuture == null || mcreceiverThreadFuture?.isCancelled == true || mcreceiverThreadFuture?.isDone == true) {
            mcreceiverThreadFuture = executor.submit {
                Log.d("BleForegroundService", "Attempting to start MC-Receiver.")
                try {
                    getTinyIO()!!.mcReceiverLoop(ioLock)
                } catch (e: Exception) {
                    Log.d("BleForegroundService", "MC-Receiver died $e")
                    Thread.currentThread().interrupt()
                }
            }
        }

        // GOset-Loop
        if (gosetLoopThreadFuture == null || gosetLoopThreadFuture?.isCancelled == true || gosetLoopThreadFuture?.isDone == true) {
            gosetLoopThreadFuture = executor.submit {
                Log.d("BleForegroundService", "Attempting to start GOset-Loop.")
                try {
                    getTinyGoset()!!.loop()
                } catch (e: Exception) {
                    Log.d("BleForegroundService", "GOset-Loop died $e")
                    Thread.currentThread().interrupt()
                }
            }
        }

        // Node-Loop
        if (nodeLoopThreadFuture == null || nodeLoopThreadFuture?.isCancelled == true || nodeLoopThreadFuture?.isDone == true) {
            nodeLoopThreadFuture = executor.submit {
                Log.d("BleForegroundService", "Attempting to start Node-Loop.")
                try {
                    getTinyNode()!!.loop(ioLock)
                } catch (e: Exception) {
                    Log.e("BleForegroundService", "Node-Loop died $e")
                    Thread.currentThread().interrupt()
                }
            }
        }

        if (bleThreadFuture == null || bleThreadFuture?.isCancelled == true || bleThreadFuture?.isDone == true) {
            bleThreadFuture = executor.submit {
                Log.d("BleForegroundService", "Attempting to start BLE thread.")
                try {
                    ble = BlePeers(this)
                    ble!!.startBluetooth()
                } catch (e: Exception) {
                    Log.d("BleForegroundService", "BLE thread died $e")
                    Thread.currentThread().interrupt()
                }
            }
        }
        Log.d("BleForegroundService", "Operations started.")
    }

    /**
     *  This function is being called to stop the operations of the BLE Foreground Service.
     */
    public fun stopOperations() {
        Log.d("BleForegroundService", "Stopping operations.")
        isRunning = false
        testThreadFuture?.cancel(true)
        bleThreadFuture?.cancel(true)
        // TODO: Optionally we could shutdown threads here.
        //  But we would have some overhead of continuously having to recreate new pool
    }

    /**
     * This method gets called when the Foreground-Service is being shutdown or stopped.
     */
    public fun shutdownOperations() {
        Log.d("BleForegroundService", "Shutting down operations.")
        isRunning = false
        executor.shutdown()
        try {
            if (!executor.awaitTermination(5, TimeUnit.SECONDS)) {
                executor.shutdownNow()
            }
        } catch (e: InterruptedException) {
            executor.shutdownNow()
        }
        stopSelf()
    }

    override fun onBind(
        p0: Intent?
    ): IBinder? {
        return null
    }

    /**
     *  This function is being called to check the requirements to start the BLE Foreground Service.
     *  We need to check if the device has BLE capabilities and if the Bluetooth is enabled.
     *  If the requirements are not met, the user will be informed via a Toast message.
     *  @return Boolean
     */
    private fun checkBleForegroundRequirements(): Boolean {
        Log.d("BleForegroundService", "Checking BLE requirements")
        try {
            val bluetoothManager = getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
            val bluetoothAdapter = bluetoothManager.adapter
            if (bluetoothManager == null) {
                Log.d("BleForegroundService", "Bluetooth Manager is not null")
                return false
            }
            val context = applicationContext
            if (context == null) {
                Log.d("BleForegroundService", "Context is not null")
                return false
            }
            val pm: PackageManager = getPackageManager()
            if (!pm.hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
                showToastOnMainThread("this device does NOT have Bluetooth LE - user Wifi to sync")
                return false
            }
            if (!bluetoothAdapter.isEnabled) {
                showToastOnMainThread("Bluetooth MUST be enabled for using BlueTooth-Low-Energy sync")
                return false
            }
            if (!isLocationEnabled()) {
                showToastOnMainThread("Location MUST be enabled for using BlueTooth-Low-Energy sync, then restart")
                return false
            }
            Log.d("BleForegroundService", "BLE requirements are met.")
            return true
        } catch (e: Exception) {
            Log.d("BleForegroundService", "Error while checking BLE requirements: $e")
            return false
        }
    }

    /**
     * This function is being called to show a Toast message on the main thread,
     * because Foreground-Service is not allowed to access UI-Elements.
     */
    public fun showToastOnMainThread(message: String) {
        Handler(Looper.getMainLooper()).post {
            Toast.makeText(this, message, Toast.LENGTH_LONG).show()
        }
    }

    /**
     *      SOCKETS
     */


    fun mkSockets() {
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
    }

    fun rmSockets() {
        try { mc_socket?.leaveGroup(mc_group); mc_socket?.close() } catch (e: Exception) {}
        mc_group = null
        mc_socket = null
    }
}
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
import tremolavossbol.R
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

    private val CHANNEL_ID = "BLE_FOREGROUND_SERVICE_CHANNEL"
    lateinit var idStore: IdStore
    lateinit var tinyIO: IO
    var settings: Settings? = null
    @Volatile var mc_group: InetAddress? = null
    @Volatile var mc_socket: MulticastSocket? = null
    var ble: BlePeers? = null
    var websocket: WebsocketIO? =null
    val ioLock = ReentrantLock()
    var broadcastReceiver: BroadcastReceiver? = null
    var isWifiConnected = false
    var ble_event_listener: BluetoothEventListener? = null

    private var isRunning = true
    private val THREAD_COUNT = 2
    private val executor: ExecutorService = Executors.newFixedThreadPool(THREAD_COUNT)
    private lateinit var bluetoothEventListener: BluetoothEventListener
    private var bleThreadFuture: Future<*>? = null
    private var testThreadFuture: Future<*>? = null

    @SuppressLint(
        "SetJavaScriptEnabled"
    )
    @RequiresApi(
        Build.VERSION_CODES.O
    )
    override fun onCreate() {
        Log.d("BleForegroundService", "Creating Service")
        //registerBroadcastReceivers()
        val filter = IntentFilter("MESSAGE_FROM_ACTIVITY")
        LocalBroadcastManager.getInstance(this).registerReceiver(mainActivityReceiver, filter)
        super.onCreate()
        bluetoothEventListener = BluetoothEventListener(this)
        registerReceiver(bluetoothEventListener, IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED))
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

    private fun sendMessageToActivity(message: ByteArray) {
        val intent = Intent("MESSAGE_FROM_SERVICE")
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
            .setContentText("Continuing Replication in the background")
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
                        Log.d(
                            "BleForegroundService",
                            "Test thread running"
                        )
                        Thread.sleep(
                            10000
                        )
                    }
                    Log.d("BleForegroundService", "Test thread stopped.")
                } catch (e: Exception) {
                    Log.d("BleForegroundService", "Foreground thread 1 died $e")
                    Thread.currentThread().interrupt()
                }
            }
        }

        // BLE-Thread
        if (bleThreadFuture == null || bleThreadFuture?.isCancelled == true || bleThreadFuture?.isDone == true) {
            bleThreadFuture = executor.submit {
                Log.d("BleForegroundService", "Attempting to start BLE-Thread.")
                try {
                    if (!checkBleForegroundRequirements()) {
                        Log.e("BleForegroundService", "BLE requirements are not met.")
                        return@submit
                    }
                    while (isRunning) {
                        Log.d(
                            "BleForegroundService",
                            "BlePeers-Thread running."
                        )
                        Thread.sleep(
                            10000
                        )
                    }
                    Log.d("BleForegroundService", "BlePeers-Thread stopped.")
                } catch (e: Exception) {
                    Log.d("BleForegroundService", "Foreground Thread 2 died $e")
                    Thread.currentThread().interrupt()
                }
            }
        }
        return
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
}
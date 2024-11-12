package nz.scuttlebutt.tremolavossbol.tssb.ble

import android.annotation.SuppressLint
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.bluetooth.BluetoothAdapter
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.ServiceInfo
import android.net.NetworkInfo
import android.net.wifi.WifiManager
import android.os.Build
import android.os.Handler
import android.os.IBinder
import android.util.Log
import android.view.View
import android.webkit.WebView
import androidx.annotation.RequiresApi
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
import java.util.concurrent.Executors
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
        super.onDestroy()
        Log.d("MyForegroundService", "Service destroyed")
        LocalBroadcastManager.getInstance(this).unregisterReceiver(mainActivityReceiver)
        /*unregisterReceiver(broadcastReceiver)
        unregisterReceiver(ble_event_listener)

        websocket?.stop()
        websocket = null*/
    }

    @RequiresApi(
        Build.VERSION_CODES.O
    )
    private fun startOperations() {
        // Start the BLE operations here
        /**
         * The following code is for testing purposes only.
         * Notice: applicationContext as MainApplication cannot be used from Service, as it seems to cause trouble
         */
        val TEST = true
        if (TEST) {
            val executor = Executors.newFixedThreadPool(1)
            executor.execute {
                Log.d("BleForegroundService", "Starting test thread")
                try {
                    val test_thread = Thread {
                        while (true) {
                            Log.d(
                                "BleForegroundService",
                                "Test thread running"
                            )
                            Thread.sleep(
                                10000
                            )
                        }
                    }
                    test_thread.start()
                } catch (e: Exception) {
                    Log.d("BleForegroundService", "Test thread died ${e}")
                }
            }
            return
        }
        //val executor = Executors.newFixedThreadPool(2)

        /*executor.execute {
            Log.d("BleForegroundService", "Starting tinyIO sender loop")
            try {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    val thread = Thread {
                        tinyIO.senderLoop()
                    }
                    thread.priority = 8
                    thread.start()
                }
            } catch (e: Exception) {
                Log.d("tssb sender thread", "died ${e}")
            }
        }*/

        /*executor.execute {
            Log.d("BleForegroundService", "Starting tinyIO receiver loop")
            val thread = Thread {
                tinyIO.mcReceiverLoop(ioLock)
            }
            thread.priority = 10
        }*/

        /*executor.execute {
            Log.d("BleForegroundService", "Starting tinyGoset loop")
            val thread = Thread {
                //tinyGoset.loop()
            }
            thread.priority = 8
            thread.start()
        }

        executor.execute {
            Log.d("BleForegroundService", "Starting tinyNode loop")
            val thread = Thread {
                //tinyNode.loop(ioLock)
            }
            thread.priority = 8
            thread.start()
        }*/
    }

    override fun onBind(
        p0: Intent?
    ): IBinder? {
        return null
    }
}
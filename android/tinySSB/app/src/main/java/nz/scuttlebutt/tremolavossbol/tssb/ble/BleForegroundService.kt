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
        super.onCreate()
    }

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

        unregisterReceiver(broadcastReceiver)
        unregisterReceiver(ble_event_listener)

        websocket?.stop()
        websocket = null
    }

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
                val test_thread = Thread {
                    while (true) {
                        Log.d("BleForegroundService", "Printing Test message")
                        Thread.sleep(10000)
                    }
                }
                test_thread.start()
            }
            return
        }
        val executor = Executors.newFixedThreadPool(2)

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

        executor.execute {
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
        }
    }

    override fun onBind(
        p0: Intent?
    ): IBinder? {
        return null
    }

    private fun registerBroadcastReceivers() {
        // Register Wi-Fi state change listener
        broadcastReceiver = object : BroadcastReceiver() {
            override fun onReceive(context: Context?, intent: Intent?) {
                val networkInfo: NetworkInfo? = intent!!.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO)
                if (networkInfo == null)
                    return

                if (networkInfo.detailedState == NetworkInfo.DetailedState.CONNECTED && !isWifiConnected) {
                    isWifiConnected = true
                    Handler().postDelayed({
                        mkSockets()
                        if (websocket == null)
                            websocket = WebsocketIO(applicationContext as MainActivity, settings!!.getWebsocketUrl())
                        websocket!!.start()
                    }, 1000)
                } else if (networkInfo.detailedState == NetworkInfo.DetailedState.DISCONNECTED && isWifiConnected) {
                    rmSockets()
                    isWifiConnected = false
                    websocket?.let {
                        it.stop()
                        websocket = null
                    }
                }
            }
        }
        registerReceiver(broadcastReceiver, IntentFilter(WifiManager.NETWORK_STATE_CHANGED_ACTION))

        // Register Bluetooth state change listener
        ble_event_listener = BluetoothEventListener(applicationContext as MainActivity)
        registerReceiver(ble_event_listener, IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED))
    }

    private fun rmSockets() {
        try { mc_socket?.leaveGroup(mc_group); mc_socket?.close() } catch (e: Exception) {}
        mc_group = null
        mc_socket = null
    }


    private fun mkSockets() {
        if(!settings!!.isUdpMulticastEnabled())
            return

        rmSockets()
        try {
            mc_group = InetAddress.getByName(
                Constants.SSB_VOSSBOL_MC_ADDR)
            mc_socket= MulticastSocket(
                Constants.SSB_VOSSBOL_MC_PORT)
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

}
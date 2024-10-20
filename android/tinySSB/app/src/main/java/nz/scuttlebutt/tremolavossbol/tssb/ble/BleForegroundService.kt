package nz.scuttlebutt.tremolavossbol.tssb.ble

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder
import android.util.Log
import androidx.core.app.NotificationCompat
import tremolavossbol.R

/**
 * This class handles the BLE replication process in the foreground.
 * Beware: The term "foreground" means that the service is running in the background, but the user is aware of it.
 */

class BleForegroundService: Service() {

    private val CHANNEL_ID = "BLE_FOREGROUND_SERVICE_CHANNEL"

    override fun onCreate() {
        Log.d("BleForegroundService", "Service created")
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
        return START_STICKY
    }

    override fun onDestroy() {
        super.onDestroy()
        Log.d("MyForegroundService", "Service destroyed")
    }

    override fun onBind(
        p0: Intent?
    ): IBinder? {
        return null
    }
}
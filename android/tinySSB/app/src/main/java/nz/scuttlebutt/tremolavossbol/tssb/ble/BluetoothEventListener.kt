package nz.scuttlebutt.tremolavossbol.tssb.ble

import android.bluetooth.BluetoothAdapter
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.Build
import android.util.Log
import androidx.annotation.RequiresApi
import nz.scuttlebutt.tremolavossbol.MainActivity

/**
 * This class listens for Bluetooth state changes and starts or stops the BLE operations accordingly.
 */
class BluetoothEventListener(val service: BleForegroundService) : BroadcastReceiver() {

    @RequiresApi(
        Build.VERSION_CODES.O
    )
    override fun onReceive(context: Context, intent: Intent) {
        try {
            if (intent.action == BluetoothAdapter.ACTION_STATE_CHANGED) {
                val state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR)
                if (state == BluetoothAdapter.STATE_ON) {
                    Log.d("BluetoothEventListener", "Bluetooth is turned ON")
                    service.startOperations()
                    //act.ble = BlePeers(act)
                    //act.ble!!.startBluetooth()
                } else if (state == BluetoothAdapter.STATE_OFF) {
                    //act.ble = null
                    Log.d("BluetoothEventListener", "Bluetooth is turned OFF")
                    service.showToastOnMainThread("Bluetooth MUST be enabled for using BlueTooth-Low-Energy sync")
                    service.stopOperations()
                }
            }
        } catch (e: Exception) {
            Log.e("BluetoothEventListener", "Error in onReceive: $e")
            service.stopOperations()
        }
    }
}
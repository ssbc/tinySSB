package nz.scuttlebutt.tremolavossbol.tssb.ble

import android.bluetooth.BluetoothAdapter
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import nz.scuttlebutt.tremolavossbol.MainActivity

class BluetoothEventListener(val act: MainActivity) : BroadcastReceiver(){

    override fun onReceive(context: Context, intent: Intent) {

        if (intent.action == BluetoothAdapter.ACTION_STATE_CHANGED) {
            val state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR)
            if (state == BluetoothAdapter.STATE_ON) {
                act.ble = BlePeers(act)
                act.ble!!.startBluetooth()
            } else if (state == BluetoothAdapter.STATE_OFF) {
                act.ble = null
            }
        }
    }
}
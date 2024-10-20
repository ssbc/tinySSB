package nz.scuttlebutt.tremolavossbol.tssb.ble

import android.Manifest
import android.annotation.SuppressLint
import android.app.Activity
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice.*
import android.bluetooth.BluetoothManager
import android.bluetooth.le.*
import android.bluetooth.le.ScanSettings.PHY_LE_ALL_SUPPORTED
import android.bluetooth.le.ScanSettings.SCAN_MODE_LOW_LATENCY
import android.content.Context
import android.content.pm.PackageManager
import android.location.LocationManager
import android.os.Build
import android.os.ParcelUuid
import android.util.Log
import android.widget.Toast
import androidx.annotation.RequiresApi
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import nz.scuttlebutt.tremolavossbol.MainActivity
import nz.scuttlebutt.tremolavossbol.utils.Constants


class BlePeersBroadcast(val act: Context) {

    private val bluetoothManager = act.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
    private val bluetoothAdapter = bluetoothManager.adapter
    private var isAdvertising = false
    private var advdata: AdvertiseData? = null
    private var currAdvSet: AdvertisingSet? = null

    private val isLocationPermissionGranted
        get() = ContextCompat.checkSelfPermission(act, Manifest.permission.ACCESS_FINE_LOCATION) ==
                PackageManager.PERMISSION_GRANTED

    private fun isLocationEnabled(): Boolean {
        val locationManager = act.getSystemService(Context.LOCATION_SERVICE) as LocationManager
        return locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER)
    }

    fun checkPermissions(askUser: Boolean = false): Boolean {
        val pm: PackageManager = act.packageManager
        Log.d("BLE", "start checking permissions")
        if (!pm.hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            Log.d("BLE", "device doesnt support BLE")
            Toast.makeText(act, "NEW this device does NOT have Bluetooth LE - user Wifi to sync",
                Toast.LENGTH_LONG).show()
            return false
        }
        if (!bluetoothAdapter.isEnabled) {
            Log.d("BlE", "Bluetooth must be enabled")
            Toast.makeText(act, "NEW Bluetooth MUST be enabled for using BlueTooth-Low-Energy sync, then restart",
                Toast.LENGTH_LONG).show()
            /*
            val enableBtIntent = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
            startActivityForResult(act, enableBtIntent, 444, null)
            */
            return false
        }
        if (isLocationEnabled()) {
            Log.d("BLE", "Location must be enabled")
            val locationManager = act.getSystemService(Context.LOCATION_SERVICE) as LocationManager
            Log.d("BLE", "locmanager: ${locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER)}")
            Log.d("BLE", "loc enabled: ${locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER)}")
            Toast.makeText(
                act, "Location MUST be enabled for using BlueTooth-Low-Energy sync, then restart",
                Toast.LENGTH_LONG
            ).show()
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !isLocationPermissionGranted) {
            Log.d("BLE", "requesting location permission")
            if (askUser) {
                ActivityCompat.requestPermissions(
                    act as Activity, arrayOf(Manifest.permission.ACCESS_FINE_LOCATION), 555)
                return true
            }
            return false
        }

        return true
    }

    @SuppressLint("MissingPermission")
    @RequiresApi(Build.VERSION_CODES.O)
    fun advertise(data: ByteArray) {

        if(!isAdvertising) {
            Log.d("BLE", "Setup advertiser")
            Log.d("BLE", "CodedPhy supported: ${bluetoothManager.adapter.isLeExtendedAdvertisingSupported}")
            val advertiser = bluetoothManager.adapter.bluetoothLeAdvertiser
            val advParameters = AdvertisingSetParameters.Builder().setLegacyMode(false).setConnectable(false).setInterval(AdvertisingSetParameters.INTERVAL_HIGH).setTxPowerLevel(AdvertisingSetParameters.TX_POWER_MEDIUM).setPrimaryPhy(PHY_LE_CODED).build()
            advertiser.startAdvertisingSet(advParameters, advdata, null, null, null, advCallback);
        }
        if(currAdvSet != null) {
            val newData = AdvertiseData.Builder().addServiceData(ParcelUuid(Constants.TINYSSB_BLE_REPL_SERVICE_2022), data).setIncludeDeviceName(true).build()
            Log.d("BLE", "Updated advertising data")
            currAdvSet!!.setAdvertisingData(newData)
        }


    }


    @RequiresApi(Build.VERSION_CODES.O)
    private val advCallback =  object : AdvertisingSetCallback() {
        override fun onAdvertisingSetStarted(
            advertisingSet: AdvertisingSet?,
            txPower: Int,
            status: Int
        ) {
            super.onAdvertisingSetStarted(advertisingSet, txPower, status)
            Log.d("BLE Advertiser", "Started advertising")
            isAdvertising = true
            currAdvSet = advertisingSet
        }

        override fun onAdvertisingSetStopped(advertisingSet: AdvertisingSet?) {
            super.onAdvertisingSetStopped(advertisingSet)
            Log.d("BLE Advertiser", "Stopped advertising")
            isAdvertising = false
        }

        override fun onScanResponseDataSet(advertisingSet: AdvertisingSet?, status: Int) {
            super.onScanResponseDataSet(advertisingSet, status)
            Log.d("BLE", "Scan Response")
        }

    }

    @SuppressLint("MissingPermission")
    @RequiresApi(Build.VERSION_CODES.O)
    fun scan() { // PHY_LE_ALL_SUPPORTED
        val settings = ScanSettings.Builder().setLegacy(false).setPhy(PHY_LE_CODED).setScanMode(
            SCAN_MODE_LOW_LATENCY).build()
        bluetoothManager.adapter.bluetoothLeScanner.startScan(null, settings, scanCallback)
    }

    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult?) {
            super.onScanResult(callbackType, result)
            Log.d("BLE SCAN", "Result: Name: ${result!!.device.address}, Data: ${result!!.scanRecord!!.serviceData}, Name: ${result.scanRecord!!.deviceName}")
        }
    }


}
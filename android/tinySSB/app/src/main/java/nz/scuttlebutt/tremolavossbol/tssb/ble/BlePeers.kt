package nz.scuttlebutt.tremolavossbol.tssb.ble


import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.*
import android.bluetooth.BluetoothGatt.GATT_SUCCESS
import android.bluetooth.BluetoothGatt.STATE_CONNECTED
import android.bluetooth.BluetoothProfile.STATE_DISCONNECTED
import android.bluetooth.le.BluetoothLeAdvertiser
import android.bluetooth.le.AdvertiseCallback
import android.bluetooth.le.AdvertiseData
import android.bluetooth.le.AdvertiseSettings
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.bluetooth.le.ScanSettings.MATCH_MODE_STICKY
import android.bluetooth.BluetoothGattServer
import android.content.Context
import android.content.pm.PackageManager
import android.location.LocationManager
import android.os.Build
import android.os.ParcelUuid
import android.util.Log
import android.widget.Toast
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import nz.scuttlebutt.tremolavossbol.MainActivity
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_BLE_REPL_SERVICE_2022
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_BLE_RX_CHARACTERISTIC
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_BLE_RX_NAME_DESCRIPTOR
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_BLE_TX_CHARACTERISTIC
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.b32encode
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toHex


class BlePeers(val act: MainActivity) {

    private val bluetoothManager = act.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
    private val bluetoothAdapter = bluetoothManager.adapter
    private val bleScanner = bluetoothAdapter.bluetoothLeScanner
    private var gattServer: BluetoothGattServer? = null
    private var advertiser: BluetoothLeAdvertiser? = null

    private val device_pubkey = mutableMapOf<BluetoothDevice, ByteArray>() // Maps bluetooth devices to their advertised tinySSB public key

    private val connectedDevices = mutableSetOf<BluetoothDevice>() // Bluetooth devices connected to the hosted GATT-server
    private var pending = mutableMapOf<BluetoothDevice,BluetoothGatt>() // pending connection to other GATT-Servers, waiting for a successful reply
    var peers = mutableMapOf<BluetoothDevice,BluetoothGatt>() // Bluetooth devices we are connected to
    private var writeErrorCounter = mutableMapOf<BluetoothDevice,Int>() // Counts the number of write errors for the given Bluetooth device (not included in the set -> no errors occurred).
    var isScanning = false

    private val scanSettings = ScanSettings.Builder()
        .setScanMode(ScanSettings.SCAN_MODE_LOW_POWER)
        .setMatchMode(MATCH_MODE_STICKY)
        .build()

    private val scanFilter = ScanFilter.Builder()
        .setServiceUuid(ParcelUuid(TINYSSB_BLE_REPL_SERVICE_2022))
        .build()

    private val isLocationPermissionGranted
        get() = ContextCompat.checkSelfPermission(act, Manifest.permission.ACCESS_FINE_LOCATION) ==
                PackageManager.PERMISSION_GRANTED

    private fun isLocationEnabled(): Boolean {
        val locationManager = act.getSystemService(Context.LOCATION_SERVICE) as LocationManager
        return locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER)
    }

    @SuppressLint("MissingPermission")
    fun startBluetooth() {

        if (!act.settings!!.isBleEnabled())
            return

        val pm: PackageManager = act.getPackageManager()
        if (!pm.hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            Toast.makeText(act, "this device does NOT have Bluetooth LE - user Wifi to sync",
                Toast.LENGTH_LONG).show()
            return
        }
        if (!bluetoothAdapter.isEnabled) {
            Toast.makeText(act, "Bluetooth MUST be enabled for using BlueTooth-Low-Energy sync",
                Toast.LENGTH_LONG).show()
            /*
            val enableBtIntent = Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
            startActivityForResult(act, enableBtIntent, 444, null)
            */
            return
        }
        if (!isLocationEnabled()) {
            Toast.makeText(
                act, "Location MUST be enabled for using BlueTooth-Low-Energy sync, then restart",
                Toast.LENGTH_LONG
            ).show()
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !isLocationPermissionGranted) {
            ActivityCompat.requestPermissions(act, arrayOf(Manifest.permission.ACCESS_FINE_LOCATION), 555)
            return
        }

        //start GATT server + client
        startBleScan()
        startServer()

        //inform frontend that ble is enabled
        act.wai.eval("b2f_ble_enabled()")
    }

    @SuppressLint("MissingPermission")
    fun stopBluetooth() {
        stopBleScan()
        for (p in peers) {
            p.value.close()
        }
        stopServer()
        act.wai.eval("b2f_ble_disabled()")
    }

    @SuppressLint("MissingPermission")
    fun write(buf: ByteArray) {
        for (p in peers) {
            // Log.d("BlePeers", "ble_rx sending (rx charact.) ${buf.size} bytes")
            val service = p.value.getService(TINYSSB_BLE_REPL_SERVICE_2022)
            if(service == null) {
                Log.d("BlePeers", "reply service not available")
                return
            }
            // Log.d("ble_rx", "Service: $service")
            //Log.d("ble_rx", "Available chs: ${service.characteristics.get(0).uuid}")
            //Log.d("ble_rx", "expected: $TINYSSB_BLE_RX_CHARACTERISTIC")
            //Log.d("ble_rx", "Check? ${service.characteristics.get(0).uuid.toString() == TINYSSB_BLE_RX_CHARACTERISTIC.toString()}")
            val ch = service.getCharacteristic(TINYSSB_BLE_RX_CHARACTERISTIC)
            // Log.d("ble_rx", "Characteristic: ${ch}")
            ch.value = buf
            // Log.d("BlePeers", "ble_rx (send) $buf")
            p.value.writeCharacteristic(ch)
        }
    }

    @SuppressLint("MissingPermission")
    private fun startBleScan() {
        if (isScanning)
            return
        Log.d("BlePeers", "ble - starting scan")
            // scanResults.clear()
            // scanResultAdapter.notifyDataSetChanged()
        if (bleScanner != null) {
            bleScanner.startScan(mutableListOf(scanFilter), scanSettings, scanCallback)
            isScanning = true
        }
    }

    @SuppressLint("MissingPermission")
    private fun stopBleScan() {
        Log.d("BlePeers", "ble - Stopped scan")
        if (bleScanner != null)
            bleScanner.stopScan(scanCallback)
        isScanning = false
    }

    @SuppressLint("MissingPermission")
    private val gattCallback = object : BluetoothGattCallback() {

        override fun onDescriptorRead(
            gatt: BluetoothGatt?,
            descriptor: BluetoothGattDescriptor?,
            status: Int
        ) {
            Log.d("BlePeers", "ble - onDescriptorRead")
            super.onDescriptorRead(gatt, descriptor, status)

            if (gatt == null)
                return

            if (descriptor?.uuid == TINYSSB_BLE_RX_NAME_DESCRIPTOR) {
                Log.d("BlePeers", "ble - received name: ${descriptor!!.value.toHex()}")
                device_pubkey[gatt.device] = descriptor!!.value
                refreshShortNameForKey(descriptor!!.value)
            }

        }

        override fun onCharacteristicChanged(gatt: BluetoothGatt?, ch: BluetoothGattCharacteristic?) {
            Log.d("BlePeers", "ble change..")
            super.onCharacteristicChanged(gatt, ch)
            if (ch != null) {
                // Log.d("ble", "${ch.uuid.toString()} changed: ${ch.value.toHex()}, ${ch.value.size}")
                act.ioLock.lock()
                val rc = act.tinyDemux.on_rx(ch.value)
                act.ioLock.unlock()
                if (!rc)
                    Log.d("BlePeers", "ble - no dmx entry for ${ch.value.toHex()}")
            }
        }

        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            super.onConnectionStateChange(gatt, status, newState)
            Log.d("BlePeers", "ble - Connection change!")
            if (status == GATT_SUCCESS && newState == STATE_CONNECTED) {
                Log.d("BlePeers", "ble - connected! ${gatt.device.address.toString()}")
                //Log.d("ble", "updated client list: ${peers.get(gatt.device)!!.device.address}")
                val rc = gatt.requestMtu(128)
                Log.d("BlePeers", "ble - MTU request rc=$rc")
            } else if (newState == STATE_DISCONNECTED) {
                Log.d("BlePeers", "ble - disconnected")
                notifyFrontend(gatt.device, "offline")
                peers.remove(gatt.device)
                pending.remove(gatt.device)
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt?, status: Int) {
            super.onServicesDiscovered(gatt, status)

            var foundService = false

            if (gatt != null) {
                for (s in gatt.services) {
                    Log.d("BlePeers", "ble - discovered service ${s.uuid.toString()}")
                    if (s.uuid.toString() == TINYSSB_BLE_REPL_SERVICE_2022.toString()) {
                        for (c in s.characteristics)
                            Log.d("BlePeers","ble - discovered ch ${c.uuid.toString()}")
                        val ch = s.getCharacteristic(TINYSSB_BLE_TX_CHARACTERISTIC)
                        if (ch == null)
                            Log.d("BlePeers", "ble discovery - ch still is null")
                        else {
                            Log.d("BlePeers", "ble discovery - ch ${ch.uuid.toString()} found, enable notif")
                            val chRx = s.getCharacteristic(TINYSSB_BLE_RX_CHARACTERISTIC)
                            if (chRx != null ) {
                                val read = gatt.readDescriptor(chRx.getDescriptor(TINYSSB_BLE_RX_NAME_DESCRIPTOR))
                                Log.d("BlePeers", "ble - read: $read")
                            }
                            gatt.setCharacteristicNotification(ch, true)
                            val lst =
                                ch.getDescriptors(); //find the descriptors on the characteristic
                            // val ndx = lst.indexOfFirst { it.uuid == device.address == result.device.address }
                            // if (indexQuery != -1) { // A scan result already exists with the same address
                            // for (d in lst)
                            //    Log.d("ble", "descriptor ${d.uuid.toString()}")
                            val descr = lst.get(0); // there should be only one descriptor
                            descr.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                            gatt.writeDescriptor(descr); //apply these changes to the ble chip to tell it we are ready for the data

                            pending.remove(gatt.device)
                            peers[gatt.device] = gatt
                            notifyFrontend(gatt.device, "online")
                            foundService = true
                        }
                    } else
                        Log.d(
                            "BlePeers",
                            "hm, ble service is ${s.uuid.toString()} vs ${TINYSSB_BLE_REPL_SERVICE_2022.toString()}"
                        )

                }
                if (!foundService) {
                    Log.d("BlePeers", "ble - couldn't find the TinySSB-Service, disconnecting...")
                    gatt.disconnect()
                    pending.remove(gatt.device)
                    gatt.close()
                }
            }

            /*
            characteristic = gatt.getService(UUID.fromString(SERVICE_UUID)).getCharacteristic(UUID.fromString(CHARACTERISTIC_UUID)); //Find you characteristic
            mGatt.setCharacteristicNotification(characteristic, true); //Tell you gatt client that you want to listen to that characteristic
            List<BluetoothGattDescriptor> descriptors = characteristic.getDescriptors(); //find the descriptors on the characteristic
            BluetoothGattDescriptor descriptor = descriptors.get(1); //get the right descriptor for setting notifications
            descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
            mGatt.writeDescriptor(descriptor); //apply these changes to the ble chip to tell it we are ready for the data

             */
        }

        override fun onMtuChanged(gatt: BluetoothGatt?, mtu: Int, status: Int) {
            super.onMtuChanged(gatt, mtu, status)
            Log.d("BlePeers", "ble mtu status=$status, mtu=$mtu")

            if(gatt != null) {
                if (status == GATT_SUCCESS) {
                    Log.d("BlePeers", "ble gatt ${gatt.discoverServices()}")

                } else {
                    Log.d("BlePeers", "ble error onMtuChanged, closing connection...")
                    gatt.disconnect()
                    pending.remove(gatt.device)
                    gatt.close()
                }
            }
        }

        override fun onCharacteristicWrite(
            gatt: BluetoothGatt?,
            characteristic: BluetoothGattCharacteristic?,
            status: Int
        ) {
            super.onCharacteristicWrite(gatt, characteristic, status)
            // Log.d("gatt", "Write success status: $status")

            if(gatt != null) {
                if(status != 0) {

                    // increase error counter
                    if(writeErrorCounter.contains(gatt.device))
                        writeErrorCounter[gatt.device] = writeErrorCounter[gatt.device]!! + 1
                    else
                        writeErrorCounter[gatt.device] = 1

                    // to many write errors -> disconnect from GATT-Server
                    if(writeErrorCounter[gatt.device]!! > 5) {
                        gatt.disconnect()
                        gatt.close()
                        peers.remove(gatt.device)
                        device_pubkey.remove(gatt.device)
                        notifyFrontend(gatt.device, "offline")
                        writeErrorCounter.remove(gatt.device)
                    }
                } else {
                    // reset error counter
                    writeErrorCounter.remove(gatt.device)
                }
            }
        }
    }

    @SuppressLint("MissingPermission")
    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            super.onScanResult(callbackType, result)
            // Log.d("BlePeers", "ble scan callback - Peers: $peers, Pending: $pending")
            if (!(result.device in peers) and !(result.device in pending)) {
                // Log.d("BlePeers", "ble scan callback - Found BLE tinySSB device! adding address: ${result.device.address}")
                /*if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {

                }
                 */
                val g = result.device.connectGatt(act, false, gattCallback, BluetoothDevice.TRANSPORT_LE)
                pending[result.device] = g
                //val g = result.device.connectGatt(act, true, gattCallback)
                //peers[result.device] = g


            }

        }

        override fun onScanFailed(errorCode: Int) {
            Log.d("BlePeers","ble onScanFailed: code $errorCode")
        }
    }

    @SuppressLint("MissingPermission")
    private fun startServer() {
        val gattService = BluetoothGattService(TINYSSB_BLE_REPL_SERVICE_2022, BluetoothGattService.SERVICE_TYPE_PRIMARY)
        val chRX = BluetoothGattCharacteristic(
            TINYSSB_BLE_RX_CHARACTERISTIC,
            BluetoothGattCharacteristic.PROPERTY_WRITE,
            BluetoothGattCharacteristic.PERMISSION_WRITE)

        // chTX characteristic not actively used (provided for backward compatibility (android to esp32 ble implementation))
        val chTX = BluetoothGattCharacteristic(
            TINYSSB_BLE_TX_CHARACTERISTIC,
            BluetoothGattCharacteristic.PROPERTY_NOTIFY,
            BluetoothGattCharacteristic.PERMISSION_READ
        )
        val descriptor = BluetoothGattDescriptor(TINYSSB_BLE_TX_CHARACTERISTIC, BluetoothGattDescriptor.PERMISSION_WRITE)
        chTX.addDescriptor(descriptor)

        val nameDescr = BluetoothGattDescriptor(TINYSSB_BLE_RX_NAME_DESCRIPTOR, BluetoothGattDescriptor.PERMISSION_READ)
        chRX.addDescriptor(nameDescr)

        gattService.addCharacteristic(chRX)
        gattService.addCharacteristic(chTX)
        gattServer = bluetoothManager.openGattServer(act, gattServerCallback).apply { addService(gattService)}
        Log.d("BlePeers", "ble GATT Server started")
        startAdvertising()

    }

    @SuppressLint("MissingPermission")
    private fun startAdvertising() {
        advertiser = bluetoothAdapter.bluetoothLeAdvertiser

        val advertiseSettings = AdvertiseSettings.Builder().setAdvertiseMode(AdvertiseSettings.ADVERTISE_MODE_BALANCED).setTimeout(0).setConnectable(true).build()
        val advertiseData = AdvertiseData.Builder().addServiceUuid(ParcelUuid(TINYSSB_BLE_REPL_SERVICE_2022)).setIncludeDeviceName(bluetoothAdapter.name.length <= 8).setIncludeTxPowerLevel(false).build()
        //advertiser.startAdvertising(advertiseSettings, advertiseData, advertiseCallback)
        advertiser?.let { it.startAdvertising(advertiseSettings,advertiseData,advertiseCallback)}
    }

    @SuppressLint("MissingPermission")
    private fun stopAdvertising() {
        advertiser?.let { it.stopAdvertising(advertiseCallback) }
    }

    @SuppressLint("MissingPermission")
    private fun stopServer() {
        stopAdvertising()
        for (d in connectedDevices)
            gattServer?.cancelConnection(d)
        gattServer?.close()
        Log.d("BlePeers", "ble GATT Server stopped")
    }

    private val advertiseCallback = object : AdvertiseCallback() {
        override fun onStartSuccess(settingsInEffect: AdvertiseSettings) {
            Log.i("GATT_Advertiser", "ble GATT Advertise Started.")
        }

        override fun onStartFailure(errorCode: Int) {
            Log.w("GATT_Advertiser", "ble GATT Advertise Failed: $errorCode")
        }
    }

    @SuppressLint("MissingPermission")
    private val gattServerCallback = object : BluetoothGattServerCallback() {


        override fun onConnectionStateChange(device: BluetoothDevice?, status: Int, newState: Int) {
            // Log.d("BlePeers", "ble GATT server Connection changed")
            super.onConnectionStateChange(device, status, newState)
                if( status == GATT_SUCCESS && newState == STATE_CONNECTED && device != null) {
                    Log.d("BlePeers", "ble GATT server - Device connected: $device")
                    connectedDevices.add(device)
                    notifyFrontend(device, "online")
                } else if (newState == STATE_DISCONNECTED) {
                    connectedDevices.remove(device)
                    if (device != null)
                        notifyFrontend(device, "offline")
                    Log.d("BlePeers", "ble GATT server - Device disconnected: $device")
                }
        }

        override fun onDescriptorReadRequest(
            device: BluetoothDevice?,
            requestId: Int,
            offset: Int,
            descriptor: BluetoothGattDescriptor?
        ) {
            // Log.d("BlePeers", "ble server - onDescriptorReadRequest")
            super.onDescriptorReadRequest(device, requestId, offset, descriptor)
            gattServer?.sendResponse(device, requestId, GATT_SUCCESS, offset, act.idStore.identity.verifyKey)
        }



        override fun onCharacteristicWriteRequest(
            device: BluetoothDevice?,
            requestId: Int,
            characteristic: BluetoothGattCharacteristic?,
            preparedWrite: Boolean,
            responseNeeded: Boolean,
            offset: Int,
            value: ByteArray?
        ) {
            super.onCharacteristicWriteRequest(
                device,
                requestId,
                characteristic,
                preparedWrite,
                responseNeeded,
                offset,
                value
            )
            // Log.d("BlePeers", "ble GATT server - Characteristic Write Request! ${characteristic?.uuid}, $responseNeeded")
            if (characteristic != null) {
                if(characteristic.uuid == TINYSSB_BLE_RX_CHARACTERISTIC) {
                    // Log.d("GATT_Server", "Received characteristic: ${value}, sender: ${device?.address}")
                    gattServer?.sendResponse(device, requestId, GATT_SUCCESS,0, null)
//                    if (value != null) {
                        try {
                            act.ioLock.lock()
                            val rc = act.tinyDemux.on_rx(value!!, device?.address)
                            act.ioLock.unlock()
                            if (!rc)
                                Log.d("BlePeers", "ble rx: not dmx entry for ${value}")
                        } catch (e: Exception) {
                            Log.e("BlePeers", "ble GATT server - error on tinyDemux: ${e.stackTrace}")
                        }
//                    }
                }
            }
        }

        // only for backward compatibility (android to esp32 ble implementation)
        override fun onDescriptorWriteRequest(
            device: BluetoothDevice?,
            requestId: Int,
            descriptor: BluetoothGattDescriptor?,
            preparedWrite: Boolean,
            responseNeeded: Boolean,
            offset: Int,
            value: ByteArray?
        ) {
            Log.d("BlePeers","ble GATT server - DescriptorWriteRequest")
            super.onDescriptorWriteRequest(
                device,
                requestId,
                descriptor,
                preparedWrite,
                responseNeeded,
                offset,
                value
            )
            gattServer?.sendResponse(device, requestId, GATT_SUCCESS,0,null)
        }
    }

    @SuppressLint("MissingPermission")
    private fun notifyFrontend(device: BluetoothDevice, status: String) {
        val name = deviceToShortName(device)

        // Log.d("BlePeers", "ble - Connected devices: $connectedDevices, Peers: $peers, Server: ${bluetoothManager.getConnectedDevices(BluetoothProfile.GATT)}")
        if (connectedDevices.any { it.address == device.address} && peers.keys.any { it.address == device.address }) {
            // Log.d("BlePeers", "ble - device online: ${device}")
            act.wai.eval("b2f_local_peer(\"ble\", \"${device.address}\", \"${name}\", \"${status}\")")
        } else if (status == "offline") {
            act.wai.eval("b2f_local_peer(\"ble\", \"${device.address}\", \"${name}\", \"${status}\")")
        }
    }

    @SuppressLint("MissingPermission")
    private fun deviceToShortName(device: BluetoothDevice): String? {

        val id = device_pubkey[device]
        val name = device.name

        if (id == null && name != null)
            return "?? $name ??"

        if (id != null) {
            val b32 = id.sliceArray(0 until 7).b32encode().substring(0 until 10)
            val b32_name = "${b32.substring(0 until 5)}-${b32.substring(5)}"
            if (act.tinyGoset.keys.any { it.contentEquals(id) }) {
                return b32_name
            } else {
                if (name == null)
                    return "?? $b32_name ??"
                else
                    return "?? $name ($b32_name) ??"
            }
        }

        return null
    }

    fun refreshShortNameForKey(key: ByteArray) {
        val key32 = key.b32encode().substring(0 until 10)
        val name32 = "${key32.substring(0 until 5)}-${key32.substring(5)}"

        for (p in peers.keys) {
            if(deviceToShortName(p) == name32) {
                notifyFrontend(p, "online")
                return
            }
        }
    }
}

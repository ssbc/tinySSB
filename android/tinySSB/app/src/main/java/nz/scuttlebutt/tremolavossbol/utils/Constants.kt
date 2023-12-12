package nz.scuttlebutt.tremolavossbol.utils

import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.decodeHex
import java.util.*

class Constants{
    companion object{
        val SSB_IPV4_TCPPORT = 8008 // default listening port
        val SSB_IPV4_UDPPORT = 8008 // default destination port for LAN announcements (broadcasts)
        val SSB_VOSSBOL_MC_ADDR = "239.5.5.8"
        val SSB_VOSSBOL_MC_PORT = 1558
        val SSB_NETWORKIDENTIFIER = "d4a1cb88a66f02f8db635ce26441cc5dac1b08420ceaac230839b755845a9ffb".decodeHex()
        val UDP_BROADCAST_INTERVAL = 3000L     // millisec
        val WIFI_DISCOVERY_INTERVAL = 5L       // check every X sec
        val EBT_FORCE_FRONTIER_INTERVAL = 30L  // send frontier every X sec
        val frontierWindow = 86400000
        val LOCAL_URL_PREFIX = "http://appassets.androidplatform.net/blobs/"

        // tinySSB:
        val TINYSSB_PKT_LEN = 120
        val FID_LEN = 32
        val HASH_LEN = 20
        val DMX_LEN = 7
        val DMX_PFX = "tinyssb-v0".encodeToByteArray()
        val GOSET_DMX_STR = "tinySSB-0.1 GOset 1"
        val PKTTYPE_plain48 = 0
        val PKTTYPE_chain20 = 1

        //  app name                                              schema
        val TINYSSB_APP_ALIAS        = Bipf.mkString("ALI") // fid str
        val TINYSSB_APP_BOX          = Bipf.mkString("BOX") // bytes
        val TINYSSB_APP_BOX2         = Bipf.mkString("BX2") // bytes
        val TINYSSB_APP_KANBAN       = Bipf.mkString("KAN") // ...
        val TINYSSB_APP_TEXTANDVOICE = Bipf.mkString("TAV") // str bytes int (xref)
        val TINYSSB_APP_TEXT         = Bipf.mkString("TXT") // str int xref
        val TINYSSB_APP_IAM          = Bipf.mkString("IAM") // str
        var TINYSSB_APP_GAME         = Bipf.mkString("GME") // str str str str
        var TINYSSB_APP_GAME_END     = Bipf.mkString("GEE") // str str

        val TINYSSB_BLE_REPL_SERVICE_2022 = UUID.fromString("6e400001-7646-4b5b-9a50-71becce51558")
        val TINYSSB_BLE_RX_CHARACTERISTIC = UUID.fromString("6e400002-7646-4b5b-9a50-71becce51558") // for writing to the remote device
        val TINYSSB_BLE_TX_CHARACTERISTIC = UUID.fromString("6e400003-7646-4b5b-9a50-71becce51558") // for receiving from the remote device

        val TINYSSB_SIMPLEPUB_URL = "ws://meet.dmi.unibas.ch:8080"

        val TINYSSB_DIR = "tinyssb"
    }
}
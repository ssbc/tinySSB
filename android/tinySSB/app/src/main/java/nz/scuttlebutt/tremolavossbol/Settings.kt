package nz.scuttlebutt.tremolavossbol

import android.content.Context
import android.util.Log
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_SIMPLEPUB_URL
import org.json.JSONObject

class Settings(val context: MainActivity) {
    private val sharedPreferences = context.getSharedPreferences("tinyssbSettings", Context.MODE_PRIVATE)

    // These settings are only used in the frontend and will not affect the backend
    // The settingID must match the one used in the frontend (tremola.settings).
    private var defaultSettings = mapOf<String, String>(
        "confirmed_delivery_enabled" to "true",
        "geo_location_enabled" to "true",
        "show_chat_preview" to "false",
        "show_background_map" to "true",
        "show_shortnames" to "true",
        "hide_forgotten_conv" to "true",
        "hide_forgotten_contacts" to "true",
        "hide_forgotten_kanbans" to "true",
        // backend settings
        "ble_enabled" to "true",
        "udp_multicast_enabled" to "true",
        "websocket_enabled" to "false",
        "websocket_url" to TINYSSB_SIMPLEPUB_URL
    )
    fun getSettings(): String {
        val currentSettings = mutableMapOf<String, Any>()
        for ( (settingID, default) in defaultSettings) {
            val value = sharedPreferences.getString(settingID, default)!!
            // convert string back to boolean
            if (value.lowercase() == "true") {
                currentSettings[settingID] = true
            } else if (value.lowercase() == "false"){
                currentSettings[settingID] = false
            } else {
                currentSettings[settingID] = value
            }
        }
        return JSONObject(currentSettings.toMap()).toString()
    }

    // Sets a value for the provided settingID and executes necessary backend actions to align with the updated setting.
    fun set(settingID: String, value: String): Boolean {
        Log.d("set", "ID=${settingID} val=${value}")
        if (!defaultSettings.keys.contains(settingID)) {
            return false // default of setting id must be defined
        }
        val success = sharedPreferences.edit().putString(settingID, value).commit()
        if (!success)
            return false

        when (settingID) {
            "websocket_enabled" -> handleWebsocketEnabled(value.toBoolean())
            "ble_enabled" -> handleBleEnabled(value.toBoolean())
            "udp_multicast_enabled" -> handleUdpMulticastEnabled(value.toBoolean())
            "websocket_url" -> handleWebsocketUrl(value)
            "geo_location_enabled" -> handleGeoLocation(value.toBoolean())
        }
        return true
    }

    fun resetToDefault(): Boolean {
        return  sharedPreferences.edit().clear().commit()
    }

    // ------------------------------------------------------------------------------------//
    //                          Getter (backend settings)                                  //
    // ------------------------------------------------------------------------------------//
    fun isWebsocketEnabled(): Boolean {
        return sharedPreferences.getString("websocket_enabled", defaultSettings["websocket_enabled"]).toBoolean()
    }

    fun isBleEnabled(): Boolean {
        return sharedPreferences.getString("ble_enabled", defaultSettings["ble_enabled"]).toBoolean()
    }

    fun isUdpMulticastEnabled(): Boolean {
        return  sharedPreferences.getString("udp_multicast_enabled", defaultSettings["udp_multicast_enabled"]).toBoolean()
    }

    fun isGeoLocationEnabled(): Boolean {
        return  sharedPreferences.getString("geo_location_enabled", defaultSettings["geo_location_enabled"]).toBoolean()
    }

    fun getWebsocketUrl(): String {
        return sharedPreferences.getString("websocket_url", defaultSettings["websocket_url"])!!
    }

    // ------------------------------------------------------------------------------------------//
    //                       Backend settings handler (called from set())                        //
    // ------------------------------------------------------------------------------------------//
    fun handleWebsocketEnabled(value: Boolean) {
        if (!value)
            context.websocket?.stop()
        context.websocket?.start()
    }

    fun handleBleEnabled(value: Boolean) {
        if(!value)
            context.ble?.stopBluetooth()
        context.ble?.startBluetooth()
    }

    fun handleUdpMulticastEnabled(value: Boolean) {
        if (!value)
            context.rmSockets()
        context.mkSockets()
    }

    fun handleWebsocketUrl(value: String) {
        context.websocket?.updateUrl(value)
    }

    fun handleGeoLocation(value: Boolean) {
        Log.d("set", "no side effects yet for setting geoLocation to ${value}")
        if (value) {
            //TODO Stop sending location
        } else {
            //TODO start sending location
        }
    }

}
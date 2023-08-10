package nz.scuttlebutt.tremolavossbol

import android.content.Context
import androidx.core.content.withStyledAttributes
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_DIR
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_SIMPLEPUB_URL
import java.io.File

class Settings(val context: MainActivity) {
    private val sharedPreferences = context.getSharedPreferences("tinyssbSettings", Context.MODE_PRIVATE)

    private val WEBSOCKET_ENABLED_BY_DEFAULT = true
    private val BLE_ENABLED_BY_DEFAULT = true
    private val UDP_MULTICAST_ENABLED_BY_DEFAULT = true

    private val PREVIEW_ENABLED_BY_DEFAULT = false
    private val BACKGROUD_MAP_ENABLED_BY_DEFAULT = false
    private val HIDE_FORGOTTEN_CONVERSATIONS_BY_DEFAULT = true
    private val HIDE_FORGOTTEN_CONTACTS_BY_DEFAULT = true
    private val HIDE_FORGOTTEN_KANBAN_BY_DEFAULT = true
    private val SHOW_SHORTNAME_BY_DEFAULT = true

    private val WEBSOCKET_URL_DEFAULT = "ws://meet.dmi.unibas.ch:8989"



    // put for different data types
    private fun put(key: String, value: Boolean) {
        sharedPreferences.edit().putBoolean(key, value).apply()
    }

    private fun put(key: String, value: Float) {
        sharedPreferences.edit().putFloat(key, value).apply()
    }

    private fun put(key: String, value:Int) {
        sharedPreferences.edit().putInt(key, value).apply()
    }

    private fun put(key: String, value: Long) {
        sharedPreferences.edit().putLong(key, value).apply()
    }

    private fun put(key: String, value: String) {
        sharedPreferences.edit().putString(key, value).apply()
    }

    private fun put(key: String, value: Set<String>) {
        sharedPreferences.edit().putStringSet(key, value).apply()
    }

    private fun delete() {
        sharedPreferences.edit().clear().apply()
    }

    fun getAll(): MutableMap<String, *>? {
        return sharedPreferences.all
    }

    fun resetToDefault() {
        delete()
    }

    // ------------------------------------------------------------------------------------//
    fun isWebsocketEnabled(): Boolean {
        return sharedPreferences.getBoolean("websocket", WEBSOCKET_ENABLED_BY_DEFAULT)
    }

    fun isBleEnabled(): Boolean {
        return sharedPreferences.getBoolean("ble", BLE_ENABLED_BY_DEFAULT)
    }

    fun isUdpMulticastEnabled(): Boolean {
        return  sharedPreferences.getBoolean("udp_multicast", UDP_MULTICAST_ENABLED_BY_DEFAULT)
    }

    fun isHideForgottenConversationsEnabled(): Boolean {
        return sharedPreferences.getBoolean("hide_forgotten_conv", HIDE_FORGOTTEN_CONVERSATIONS_BY_DEFAULT)
    }

    fun isHideForgottenContactsEnabled(): Boolean {
        return sharedPreferences.getBoolean("hide_forgotten_contacts", HIDE_FORGOTTEN_CONTACTS_BY_DEFAULT)
    }

    fun isHideForgottenKanbanEnabled(): Boolean {
        return sharedPreferences.getBoolean("hide_forgotten_boards", HIDE_FORGOTTEN_KANBAN_BY_DEFAULT)
    }

    fun isPreviewBeforeSendingEnabled(): Boolean {
        return sharedPreferences.getBoolean("enable_preview", PREVIEW_ENABLED_BY_DEFAULT)
    }

    fun isBackgroundMapEnabled(): Boolean {
        return sharedPreferences.getBoolean("background_map", BACKGROUD_MAP_ENABLED_BY_DEFAULT)
    }

    fun isShowShortnameEnabled(): Boolean {
        return sharedPreferences.getBoolean("show_shortnames", SHOW_SHORTNAME_BY_DEFAULT)
    }

    fun getWebsocketUrl(): String {
        return sharedPreferences.getString("websocket_url", WEBSOCKET_URL_DEFAULT)!!
    }

    // ------------------------------------------------------------------------------------------//

    fun setWebsocketEnabled(value: Boolean) {
        put("websocket", value)
        if (!value)
            context.websocket?.stop()
        context.websocket?.start()
    }

    fun setBleEnabled(value: Boolean) {
        put("ble", value)
        if(!value)
            context.ble?.stopBluetooth()
        context.ble?.startBluetooth()
    }

    fun setUdpMulticastEnabled(value: Boolean) {
        put("udp_multicast", value)
        if (!value)
            context.rmSockets()
        context.mkSockets()
    }

    fun setWebsocketUrl(value: String) {
        put("websocket_url", value)
        context.websocket?.updateUrl(value)
    }

}
// prod/settings.js

"use strict";

// These default settings are only used for browser-only testing
// Normally, these settings below WILL BE IGNORED and loaded via the provided backend.
const BrowserOnlySettings = {
    'dark_mode': false,
    'show_chat_preview': false,
    'show_background_map': true,
    'websocket_enabled': false,
    'show_shortnames': true,
    'hide_forgotten_conv': true,
    'hide_forgotten_contacts': true,
    'hide_forgotten_kanbans': true,
    'udp_multicast_enabled': true,
    'ble_enabled': true,
    'websocket_url': "ws://meet.dmi.unibas.ch:8989",
    'geo_location': true
}

var advancedSettings = [
    'confirmed_delivery_enabled',
    'geo_location_enabled',
    'show_shortnames',
    'hide_forgotten_conv',
    'hide_forgotten_contacts',
    'hide_forgotten_kanbans',
    // backend settings
    'ble_enabled',
    'udp_multicast_enabled',
    'websocket_enabled',
    'danger_restream', 'danger_reset', 'danger_delete',
    'danger_import', 'danger_export',
];


// button/toggle handler for boolean settings; settingID is determined by the id of the html object that emitted the event (e.id)
function toggle_changed(e) {
    console.log("toggle:", e.id);
    tremola.settings[e.id] = e.checked;
    var cmd = "settings:set " + e.id + " " + e.checked
    backend(cmd)
    console.log(`sent "${cmd}" to backend`)
    persist()
    applySetting(e.id, e.checked);
}

// getter
function getSetting(settingID) {
    return tremola.settings[settingID]
}

// frontend handler when settings have changed
function applySetting(nm, val) {
    if (nm == 'dark_mode')
        apply_background();
    else if (nm == 'simple_mode')
        apply_simple_mode();
    else if (nm == 'show_background_map') {
        if (val)
            apply_background();
         else
            document.body.style.backgroundImage = null;
    } else if (nm == 'hide_forgotten_conv') {
        load_chat_list();
    } else if (nm == 'hide_forgotten_contacts') {
        load_contact_list();
    } else if (nm == 'websocket_enabled') {
        if (val)
            document.getElementById("container:settings_ws_url").style.display = 'flex'
        else
            document.getElementById("container:settings_ws_url").style.display = 'none'
    }
}

function apply_background() {
    var fn;
    if ('dark_mode' in tremola.settings && tremola.settings['dark_mode']) {
        document.documentElement.setAttribute('data-theme', 'dark');
        fn = 'img/splash-as-background-inverted.jpg';
    } else {
        document.documentElement.setAttribute('data-theme', 'light');
        fn = 'img/splash-as-background.jpg';
    }
    document.body.style.backgroundImage = `url(${fn})`;
}

function apply_simple_mode() {
    var v = 'simple_mode' in tremola.settings && tremola.settings['simple_mode'];
    for (var m in advancedSettings) {
        // console.log("apply_simple " + advancedSettings[m])
        document.getElementById(advancedSettings[m]).parentNode.parentNode.parentNode.style.display = v ? 'none' : null;
    }
    document.getElementById('btn:syncStat').style.display = v ? 'none' : null;
}

// setter, this will also save the given settingID and its value in the backend
function setSetting(nm, val) {
    // console.log("setting", nm, val);
    if (nm == "websocket_url") {
      document.getElementById("settings_urlInput").value = val;
      return;
    }
    applySetting(nm, val);
    document.getElementById(nm).checked = val;
}

// calls the backend to wipe everything, including the ID
function settings_wipe() {
    closeOverlay();
    backend("wipe"); // will not return, because of app restart
}

// button handler for websocket url textfield
function btn_setWebsocketUrl() {
   var new_url = document.getElementById("settings_urlInput").value

   if(!(new_url.startsWith("ws://") || new_url.startsWith("wss://"))) {
      launch_snackbar("Invalid Websocket Url")
      document.getElementById("settings_urlInput").classList.add("invalid")
      return
   }

   document.getElementById("settings_urlInput").classList.remove("invalid")

   document.getElementById("settings_urlInput").classList.add("valid")
   setTimeout(function() {
           document.getElementById("settings_urlInput").classList.remove("valid");
       }, 700);
   document.getElementById("settings_urlInput").blur();
   backend("settings:set websocket_url " + new_url)
   tremola.settings["websocket_url"] = new_url
   persist()
   launch_snackbar("New Websocket Url saved")
}

function enter_setWebsocketUrl(ev) {
    console.log(ev.target)
    if (ev.key == "Enter") {
        btn_setWebsocketUrl()
    }
}

function settings_restream_posts() {
    // closeOverlay();
    setScenario('chats')
    launch_snackbar("DB restreaming launched");
    backend("restream");
}

function settings_reset_ui() {
    closeOverlay();
    resetTremola();
    setScenario('chats');
    menu_redraw();
    launch_snackbar("reloading DB");
    backend("reset");
}

function settings_clear_other_feeds() {
    backend("wipe:others")
    closeOverlay()
    settings_reset_ui()

}

// eof

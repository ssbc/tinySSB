// tremola_settings.js

"use strict";

function get_default_settings() {
    return {
        'enable_preview': false,
        'background_map': true,
        'websocket': true,
        'show_shortnames': true,
        'hide_forgotten_conv': true,
        'hide_forgotten_contacts': true,
        'udp_multicast': true,
        'ble': true,
        'websocket_url': "ws://meet.dmi.unibas.ch:8989"
    }
}

function toggle_changed(e) {
    // console.log("toggle ", e.id);
    tremola.settings[e.id] = e.checked;
    backend("settings:set " + e.id + " " + e.checked)
    persist()
    applySetting(e.id, e.checked);
}

function getSetting(nm) {
    return document.getElementById(nm).checked
}

function applySetting(nm, val) {
    if (nm == 'background_map') {
        if (val)
            document.body.style.backgroundImage = "url('img/splash-as-background.jpg')";
        else
            document.body.style.backgroundImage = null;
    } else if (nm == 'hide_forgotten_conv') {
        load_chat_list();
    } else if (nm == 'hide_forgotten_contacts') {
        load_contact_list();
    } else if (nm == 'websocket') {
        if (val)
            document.getElementById("container:settings_ws_url").style.display = 'flex'
        else
            document.getElementById("container:settings_ws_url").style.display = 'none'
    }
}

function setSetting(nm, val) {
    // console.log("setting", nm, val)
    if (nm == "websocket_url") {
      document.getElementById("settings_urlInput").value = val
      return
    }
    applySetting(nm, val);
    document.getElementById(nm).checked = val;
}

/* async */
function settings_wipe() {
    closeOverlay();
    backend("wipe"); // will not return
    /*
    window.localStorage.setItem("tremola", "null");
    backend("ready"); // will call initialize()
    await new Promise(resolve => setTimeout(resolve, 500));
    // resetTremola();
    menu_redraw();
    setScenario('chats');
    */
}

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

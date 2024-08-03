// prod/contacts.js

"use strict";

var new_contact_id = '';

// --- menu callbacks

function menu_new_contact() {
    document.getElementById('new_contact-overlay').style.display = 'initial';
    document.getElementById('overlay-bg').style.display = 'initial';
    // document.getElementById('chat_name').focus();
    overlayIsActive = true;
}

function menu_import_id() {
    closeOverlay();
    document.getElementById('import-id-overlay').style.display = 'initial'
    document.getElementById('overlay-bg').style.display = 'initial'
}

function btn_import_id() {
    var str = document.getElementById('import-id-input').value
    if(str == "")
        return
    var r = import_id(str)
    if(r) {
        launch_snackbar("Successfully imported, restarting...")
    } else {
        launch_snackbar("wrong format")
    }
}

function load_contact_list() {
    document.getElementById("lst:contacts").innerHTML = '';
    for (var id in tremola.contacts)
        if (!tremola.contacts[id].forgotten)
            load_contact_item([id, tremola.contacts[id]]);
    if (!tremola.settings.hide_forgotten_contacts)
        for (var id in tremola.contacts) {
            var c = tremola.contacts[id]
            if (c.forgotten)
                load_contact_item([id, c]);
        }
}

function load_contact_item(c) { // [ id, { "alias": "thealias", "initial": "T", "color": "#123456" } ] }
    var row, item = document.createElement('div'), bg;
    item.setAttribute('style', 'padding: 0px 5px 10px 5px;'); // old JS (SDK 23)
    if (!("initial" in c[1])) {
        c[1]["initial"] = c[1].alias.substring(0, 1).toUpperCase();
        persist();
    }
    if (!("color" in c[1])) {
        c[1]["color"] = colors[Math.floor(colors.length * Math.random())];
        persist();
    }
    // console.log("load_c_i", JSON.stringify(c[1]))
    bg = c[1].forgotten ? ' gray' : ' light';
    row = "<button class=contact_picture style='margin-right: 0.75em; background: " + c[1].color + ";'>" + c[1].initial + "</button>";
    row += "<button class='chat_item_button" + bg + "' style='overflow: hidden; width: calc(100% - 4em);' onclick='show_contact_details(\"" + c[0] + "\");'>";
    row += "<div style='white-space: nowrap;'><div style='text-overflow: ellipsis; overflow: hidden;'>" + escapeHTML(c[1].alias) + "</div>";
    row += "<div style='text-overflow: clip; overflow: ellipsis;'><font size=-2>" + c[0] + "</font></div></div></button>";
    // var row  = "<td><button class=contact_picture></button><td style='padding: 5px;'><button class='contact_item_button light w100'>";
    // row += escapeHTML(c[1].alias) + "<br><font size=-2>" + c[0] + "</font></button>";
    // console.log(row);
    item.innerHTML = row;
    document.getElementById('lst:contacts').appendChild(item);
}

function show_contact_details(id) {
    if (id == myId) {
        document.getElementById('old_contact_alias_hdr').innerHTML = "Alias: (own name, visible to others)"
    } else {
        document.getElementById('old_contact_alias_hdr').innerHTML = "Alias: (only you can see this alias)"
    }
    var c = tremola.contacts[id];
    new_contact_id = id;
    document.getElementById('old_contact_alias').value = c.alias ? c['alias'] : "";
    var details = '';
    details += '<br><div>IAM-Alias: &nbsp;' + (c.iam != "" ? c.iam : "&mdash;") + '</div>\n';
    details += '<br><div>Shortname: &nbsp;' + id2b32(id) + '</div>\n';
    details += '<br><div style="word-break: break-all;">SSB identity: &nbsp;<tt>' + id + '</tt></div>\n';
    details += '<br><div class=settings style="padding: 0px;"><div class=settingsText>Forget this contact</div><div style="float: right;"><label class="switch"><input id="hide_contact" type="checkbox" onchange="toggle_forget_contact(this);"><span class="slider round"></span></label></div></div>'
    document.getElementById('old_contact_details').innerHTML = details;
    document.getElementById('old_contact-overlay').style.display = 'initial';
    document.getElementById('overlay-bg').style.display = 'initial';
    document.getElementById('hide_contact').checked = c.forgotten;

    document.getElementById('old_contact_alias').focus();
    overlayIsActive = true;
}

function toggle_forget_contact(e) {
    var c = tremola.contacts[new_contact_id];
    c.forgotten = !c.forgotten;
    persist();
    closeOverlay();
    load_contact_list();
}

function save_content_alias() {
    var c = tremola.contacts[new_contact_id];
    var val = document.getElementById('old_contact_alias').value;
    var deleteAlias = false

    val.trim()

    if (val == '') {
        deleteAlias = true
        if (c.iam != "" && new_contact_id != myId) {
            val = c.iam
        } else {
            val = id2b32(new_contact_id);
        }
    }
    var old_alias = c.alias
    c.alias = val;
    c.initial = val.substring(0, 1).toUpperCase();
    c.color = colors[Math.floor(colors.length * Math.random())];

    // update names in connected devices menu
    for (var l in localPeers) {
        if (localPeers[l].alias == old_alias) {
            localPeers[l].alias = val
            refresh_connection_entry(l)
        }
    }

    // share new alias with others via IAM message
    if(new_contact_id == myId) {
        if(deleteAlias) {
            backend("iam " + btoa(""))
            c.iam = ""
        } else {
            backend("iam " + btoa(val))
            c.iam = val
        }
    }

    persist();
    menu_redraw();
    closeOverlay();
}

// --- eof

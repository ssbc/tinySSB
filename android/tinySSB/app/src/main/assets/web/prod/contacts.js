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
    document.getElementById("lst:contacts-trustLevelZero").innerHTML = '';
    document.getElementById("lst:contacts-trustLevelOne").innerHTML = '';
    document.getElementById("lst:contacts-trustLevelTwo").innerHTML = '';
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
    console.log("load_c_i", JSON.stringify(c[1]))
    bg = c[1].forgotten ? ' gray' : ' light';
    row = "<button class=contact_picture style='margin-right: 0.75em; background: " + c[1].color + ";'>" + c[1].initial + "</button>";
    row += "<button class='chat_item_button" + bg + "' style='overflow: hidden; width: calc(100% - 4em);' onclick='show_contact_details(\"" + c[0] + "\");'>";
    row += "<div style='white-space: nowrap;'><div style='text-overflow: ellipsis; overflow: hidden;'>" + escapeHTML(c[1].alias) + "</div>";
    row += "<div style='text-overflow: clip; overflow: ellipsis;'><font size=-2>" + c[0] + "</font></div></div></button>";
    // var row  = "<td><button class=contact_picture></button><td style='padding: 5px;'><button class='contact_item_button light w100'>";
    // row += escapeHTML(c[1].alias) + "<br><font size=-2>" + c[0] + "</font></button>";
    // console.log(row);
    item.innerHTML = row;
    if (c[1]["trusted"] == 0) {
        document.getElementById('lst:contacts-trustLevelZero').appendChild(item);
    } else if (c[1]["trusted"] == 1) {
        document.getElementById('lst:contacts-trustLevelOne').appendChild(item);
    } else if (c[1]["trusted"] == 2) {
        document.getElementById('lst:contacts-trustLevelTwo').appendChild(item);
    }
}
// reload trust levels of all contacts, parameter is a list of contact IDs and their trust levels
function reloadContactsTrustLevels(contactsTrustLevelsJSON) {
    const contactsTrustLevels = JSON.parse(contactsTrustLevelsJSON);
    for (const contactId in contactsTrustLevels) {
        const id = encodeScuttlebuttId(contactId);
        console.log(tremola.contacts);
        //check if contact trust level is different from the one in the contacts list
        if (tremola.contacts[id]["trusted"] != contactsTrustLevels[contactId]) {
            launch_snackbar("Updated trust level for " + tremola.contacts[id].alias + " to " + contactsTrustLevels[contactId]);
        }
        tremola.contacts[id]["trusted"] = contactsTrustLevels[contactId];
        console.log("contactsTrustLevel: " + contactId + " " + contactsTrustLevels[contactId]);
    }
    persist();
    load_contact_list();
}

function base64ToHex(base64String) {
    const binaryString = atob(base64String);  // Decode Base64 to binary string
    let hexString = '';

    for (let i = 0; i < binaryString.length; i++) {
        const hex = binaryString.charCodeAt(i).toString(16);
        hexString += ('00' + hex).slice(-2);  // Ensure each byte is 2 hexadecimal digits
    }

    return hexString;
}

function decodeScuttlebuttId(idString) {
    // Step 1: Remove the '@' prefix and '.ed25519' suffix
    const base64String = idString.slice(1, idString.indexOf('.ed25519'));

    try {
        // Step 2: Decode the Base64 string to a binary string
        const binaryString = atob(base64String);

        // Step 3: Convert the binary string to a readable hex string
        let hexString = '';
        for (let i = 0; i < binaryString.length; i++) {
            const hex = binaryString.charCodeAt(i).toString(16);
            hexString += ('00' + hex).slice(-2);  // Ensure 2 digits
        }

        return hexString;
    } catch (e) {
        console.error("Failed to decode the ID string:", e);
        return null;
    }
}

function encodeScuttlebuttId(hexString) {
    try {
        // Step 1: Convert the hex string to a binary string
        let binaryString = '';
        for (let i = 0; i < hexString.length; i += 2) {
            binaryString += String.fromCharCode(parseInt(hexString.substr(i, 2), 16));
        }

        // Step 2: Encode the binary string to a Base64 string
        const base64String = btoa(binaryString);

        // Step 3: Add the '@' prefix and '.ed25519' suffix
        return '@' + base64String + '.ed25519';
    } catch (e) {
        console.error("Failed to encode the ID string:", e);
        return null;
    }
}

function show_contact_details(id) {
    if (id == myId) {
        document.getElementById('old_contact_alias_hdr').innerHTML = "Alias: (own name, visible to others)"
    } else {
        document.getElementById('old_contact_alias_hdr').innerHTML = "Alias: (only you can see this alias)"
    }
    var c = tremola.contacts[id];
    console.log(id);
    backend("contacts:getTrust " + decodeScuttlebuttId(id));

    new_contact_id = id;
    document.getElementById('old_contact_alias').value = c.alias ? c['alias'] : "";
    var details = '';
    details += '<br><div>IAM-Alias: &nbsp;' + (c.iam != "" ? c.iam : "&mdash;") + '</div>\n';
    details += '<br><div>Shortname: &nbsp;' + id2b32(id) + '</div>\n';
    details += '<br><div style="word-break: break-all;">SSB identity: &nbsp;<tt>' + id + '</tt></div>\n';
    details += '<br><div class=settings style="padding: 0px;"><div class=settingsText>Forget this contact</div><div style="float: right;"><label class="switch"><input id="hide_contact" type="checkbox" onchange="toggle_forget_contact(this);"><span class="slider round"></span></label></div></div>'

    // Add slider with colored gradient
    details += '<br><div>Trust Level:</div>\n';
    details += '<div style="display: flex; align-items: center;">';
    details += '<input type="range" id="slider" min="0" max="2" step="1" value="' + c['trusted'] + '" style="flex: 1; margin-right: 10px; appearance: none; -webkit-appearance: none; background: linear-gradient(to right, red 0%, orange 50%, green 100%); height: 8px; border-radius: 5px;">';
    details += '<button id="setButton" style="flex: 0;">Set</button>\n';
    details += '</div>';

    // Add the numbers under the slider with precise positioning using percentage for better alignment
    details += '<div style="position: relative; width: 100%; padding: 0;">';
    details += '<span style="position: absolute; left: 0;">0</span>';
    details += '<span style="position: absolute; left: 40%; transform: translateX(-40%);">1</span>';
    details += '<span style="position: absolute; right: 20%;">2</span>';
    details += '</div>';

    // Add error message field to the details HTML
    details += '<br><div id="errorMessage" class="error" style="color: red;"></div>\n';

    document.getElementById('old_contact_details').innerHTML = details;
    document.getElementById('old_contact-overlay').style.display = 'initial';
    document.getElementById('overlay-bg').style.display = 'initial';
    document.getElementById('hide_contact').checked = c.forgotten;

    // Function to handle the set button click
    function setSliderValue() {
        const slider = document.getElementById("slider");
        const errorMessage = document.getElementById("errorMessage");

        const sliderValue = slider.value;

        // Example condition for error
        if (sliderValue == 2) {
            //check if current trust level is 2
            if (c['trusted'] == 2) {
                errorMessage.textContent = "Error: Contact's trust level is already 2.";
                return;
            }
            errorMessage.textContent = "Error: To set a contact's trust level to 2, you must scan their QR Code.";
        } else {
            errorMessage.textContent = ''; // Clear any previous errors
            handleSetValue(sliderValue); // Call the function with slider value
            closeOverlay();
        }
    }

    // Function to handle the slider value (this is your callback)
    function handleSetValue(value) {
        console.log("Set value to:", value);
        changeTrustLevel(decodeScuttlebuttId(id), value);
    }

    // Attach event listener to the button after it is added to the DOM
    document.getElementById("setButton").addEventListener("click", setSliderValue);

    document.getElementById('old_contact_alias').focus();
    overlayIsActive = true;
}


function changeTrustLevel(contactID, value) {
    console.log("Supposed to set contact " + contactID + "'s Trust Level to " + value);
    let ch = tremola.chats[recps2nm([myId])];
    let tips = JSON.stringify([])

    if (typeof ch.timeline.get_tips === 'function') {
        tips = JSON.stringify(ch.timeline.get_tips())
    }

    console.log("Tips: " + JSON.stringify(tips))
    backend("contacts:setTrust " + contactID + " " + value + " " + tips);
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

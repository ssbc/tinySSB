// tremola.js

"use strict";

var tremola;
var myId;
var myShortId;
var qr;
var shortToFidMap = {}
var localPeers = {}; // feedID ~ [isOnline, isConnected] - TF, TT, FT - FF means to remove this entry
var must_redraw = false;
var edit_target = '';
var colors = ["#d9ceb2", "#99b2b7", "#e6cba5", "#ede3b4", "#8b9e9b", "#bd7578", "#edc951",
    "#ffd573", "#c2a34f", "#fbb829", "#ffab03", "#7ab317", "#a0c55f", "#8ca315",
    "#5191c1", "#6493a7", "#bddb88"]
var loaded_settings = {} // the settings provided bz the backend, will overwrite tremola.settings after initialization

// --- menu callbacks

function menu_redraw() {
    closeOverlay();

    load_chat_list()

    document.getElementById("lst:contacts").innerHTML = '';
    load_contact_list();

    if (curr_scenario == "posts")
        load_chat(curr_chat);
}

function menu_edit(target, title, text) {
    closeOverlay()
    document.getElementById('edit-overlay').style.display = 'initial';
    document.getElementById('overlay-bg').style.display = 'initial';
    document.getElementById('edit_title').innerHTML = title;
    document.getElementById('edit_text').value = text;
    document.getElementById('edit_text').focus();
    overlayIsActive = true;
    edit_target = target;
}

function onEnter(ev) {
    if (ev.key == "Enter") {
        switch(ev.target.id) {
            case 'edit_text':
                edit_confirmed()
                break
            case 'settings_urlInput':
                btn_setWebsocketUrl()
                break
            case 'import-id-input':
                btn_import_id()
                break
        }
    }
}

function edit_confirmed() {
    closeOverlay()
    console.log("edit confirmed: " + edit_target)
    var val = document.getElementById('edit_text').value;
    if (edit_target == 'convNameTarget') {
        var ch = tremola.chats[curr_chat];
        ch.alias = val;
        persist();
        // load_chat_title(ch); // also have to update entry in chats
        load_chat_list() // regenerate list for when we come back from the posts
        load_chat(curr_chat)
        // menu_redraw();
    } else if (edit_target == 'new_contact_alias' || edit_target == 'trust_wifi_peer') {
        document.getElementById('contact_id').value = '';
        if (val == '')
            val = id2b32(new_contact_id);
        tremola.contacts[new_contact_id] = {
            "alias": val, "initial": val.substring(0, 1).toUpperCase(),
            "color": colors[Math.floor(colors.length * Math.random())],
            "iam": "", "forgotten": false
        };
        var recps = [myId, new_contact_id];
        var nm = recps2nm(recps);
        tremola.chats[nm] = {
            "alias": "Chat w/ " + val, "posts": {}, "members": recps,
            "touched": Date.now(), "lastRead": 0, "timeline": new Timeline()
        };
        persist();
        backend("add:contact " + new_contact_id + " " + btoa(val))
        menu_redraw();
    } else if (edit_target == 'new_invite_target') {
        backend("invite:redeem " + val)
    } else if (edit_target == 'new_board') {
        console.log("action for new_board")
        if (val == '') {
            console.log('empty')
            return
        }
        //create new board with name = val
        createBoard(val)
    } else if (edit_target == 'new_event') {
         console.log("action for new_event")
         if (val == '') {
             console.log('empty')
             return
         }
         //create new event with name = val
         console.log("VALUE", val)
         dpi_createEvent(val)
    } else if (edit_target == 'board_rename') {
        var board = tremola.board[curr_board]
        if (val == '') {
            menu_edit('board_rename', 'Enter a new name for this board', board.name)
            launch_snackbar("Enter a name")
            return
        }
        if (val == board.name) {
            menu_edit('board_rename', 'Enter a new name for this board', board.name)
            launch_snackbar('This board already have this name')
            return
        }
        renameBoard(curr_board, val)
    } else if (edit_target == 'board_new_column') {
        if (val == '') {
            menu_edit('board_new_column', 'Enter name of new List: ', '')
            launch_snackbar("Enter a name")
            return
        }
        createColumn(curr_board, val)

    } else if (edit_target == 'board_new_item') {
        if (val == '') {
            menu_edit('board_new_item', 'Enter name of new Card: ', '')
            launch_snackbar("Enter a name")
            return
        }
        createColumnItem(curr_board, curr_column, val)
    } else if (edit_target == 'board_rename_column') {
        if (val == '') {
            menu_rename_column(curr_column)
            launch_snackbar("Please enter a new Name")
            return
        }

        if (val == tremola.board[curr_board].columns[curr_column].name)
            return

        renameColumn(curr_board, curr_column, val)
    } else if (edit_target == 'board_rename_item') {

        if (val != tremola.board[curr_board].items[curr_rename_item].name && val != '') {
            renameItem(curr_board, curr_rename_item, val)
        }
        item_menu(curr_rename_item)
    }
}

function members_confirmed() {
    if (prev_scenario == 'chats') {
        new_conversation()
    } else if (prev_scenario == 'kanban') {
        menu_new_board_name()
    } else if (prev_scenario == 'scheduling') {
        dpi_menu_new_event_name()
    }
}

function menu_dump() {
    backend('dump:');
    closeOverlay();
}

function fill_members() {
    var choices = '';
    for (var m in tremola.contacts) {
        choices += '<div style="margin-bottom: 10px;"><label><input type="checkbox" id="' + m;
        choices += '" style="vertical-align: middle;"><div class="contact_item_button light" style="white-space: nowrap; width: calc(100% - 40px); padding: 5px; vertical-align: middle;">';
        choices += '<div style="text-overflow: ellipis; overflow: hidden;">' + escapeHTML(fid2display(m)) + '</div>';
        choices += '<div style="text-overflow: ellipis; overflow: hidden;"><font size=-2>' + m + '</font></div>';
        choices += '</div></label></div>\n';
    }
    document.getElementById('lst:members').innerHTML = choices
    /*
      <div id='lst:members' style="display: none;margin: 10pt;">
        <div style="margin-top: 10pt;"><label><input type="checkbox" id="toggleSwitches2" style="margin-right: 10pt;"><div class="contact_item_button light" style="display: inline-block;padding: 5pt;">Choice1<br>more text</div></label></div>
      </div>
    */
    document.getElementById(myId).checked = true;
    document.getElementById(myId).disabled = true;
}

// --- util

function unicodeStringToTypedArray(s) {
    var escstr = encodeURIComponent(s);
    var binstr = escstr.replace(/%([0-9A-F]{2})/g, function (match, p1) {
        return String.fromCharCode('0x' + p1);
    });
    return binstr;
}

function toHex(s) {
    return Array.from(s, function (c) {
        return ('0' + (c.charCodeAt(0) & 0xFF).toString(16)).slice(-2);
    }).join('')
}

var b32enc_map = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

function b32enc_do40bits(b40) {
    var long = 0, s = '';
    for (var i = 0; i < 5; i++) long = long * 256 + b40[i];
    for (var i = 0; i < 8; i++, long /= 32) s = b32enc_map[long & 0x1f] + s;
    return s;
}

function b32encode(bytes) {
    var b32 = '', cnt = bytes.length % 5, buf;
    if (cnt == 0) buf = new Uint8Array(bytes.length);
    else buf = new Uint8Array(bytes.length + 5 - cnt);
    for (var i = 0; i < bytes.length; i++) {
        buf[i] = bytes.charCodeAt(i);
    }
    while (buf.length > 0) {
        b32 += b32enc_do40bits(buf.slice(0, 5));
        buf = buf.slice(5, buf.length);
    }
    if (cnt != 0) {
        cnt = Math.floor(8 * (5 - cnt) / 5);
        b32 = b32.substring(0, b32.length - cnt) + '======'.substring(0, cnt)
    }
    return b32;
}

function id2b32(str) { // derive a shortname from the SSB id
    try {
        var b = atob(str.slice(1, -9)); // atob(str.substr(1, str.length-9));
        b = b32encode(b.slice(0, 7)).substr(0, 10);
        return b.substring(0, 5) + '-' + b.substring(5);
    } catch (err) {
    }
    return '??'
}

function escapeHTML(str) {
    return new Option(str).innerHTML;
}

function recps2nm(rcps) { // use concat of sorted FIDs as internal name for conversation
                          // return "ALL";
    // console.log(`recps2nm ${rcps}`)
    return rcps.sort().join('').replace(/.ed25519/g, '');
}

function recps2display(rcps) {
    if (rcps == null) return 'ALL';
    var lst = rcps.map(function (fid) {
        return fid2display(fid)
    });
    return '[' + lst.join(', ') + ']';
}

function fid2display(fid) {
    var a = '';
    if (fid in tremola.contacts)
        a = tremola.contacts[fid].alias;
    if (a == '')
        a = fid.substring(0, 9);
    return a;
}

function import_id(json_str) {
    var json
    try {
        json = JSON.parse(json_str)
    } catch (e) {
        return false // argument is not a valid json string
    }
    if (Object.keys(json).length != 2 || !('curve' in json) || !('secret' in json)) {
        return false // wrong format
    }

    backend("importSecret " + json['secret'])
    return true
}


// --- Interface to Kotlin side and local (browser) storage

function backend(cmdStr) { // send this to Kotlin (or simulate in case of browser-only testing)
    if (typeof Android != 'undefined') {
        Android.onFrontendRequest(cmdStr);
        return;
    }
    cmdStr = cmdStr.split(' ')
    if (cmdStr[0] == 'ready')
        b2f_initialize('@AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=.ed25519')
    else if (cmdStr[0] == 'exportSecret')
        b2f_showSecret('secret_of_id_which_is@AAAA==.ed25519')
    else if (cmdStr[0] == "wipe") {
        resetTremola()
        location.reload()
    } else if (cmdStr[0] == 'publ:post') {
        var draft = atob(cmdStr[2])
        cmdStr.splice(0, 2)
        console.log("CMD STRING", cmdStr)
        var e = {
            'header': {
                'tst': Date.now(),
                'ref': Math.floor(1000000 * Math.random()),
                'fid': myId
            },
            'confid': {},
            'public': ["TAV", atob(cmdStr[0]), null, Date.now()].concat(args)
        }
        b2f_new_event(e)
    } else if (cmdStr[0] == 'kanban') {
        var prev = cmdStr[2] //== "null" ? null : cmdStr[2]
        if (prev != "null") {
            prev = atob(cmdStr[2])
            prev = prev.split(",").map(atob)
        }
        var args = cmdStr[4]
        if (args != "null") {
            args = atob(cmdStr[4])
            args = args.split(",").map(atob)
        }
        var data = {
            'bid': cmdStr[1],
            'prev': prev,
            'op': cmdStr[3],
            'args': args
        }
        var e = {
            'header': {
                'tst': Date.now(),
                'ref': Math.floor(1000000 * Math.random()),
                'fid': myId
            },
            'confid': {},
            'public': ["KAN", cmdStr[1], prev, cmdStr[3]].concat(args)
        }
        // console.log('e=', JSON.stringify(e))
        b2f_new_event(e)
        console.log(e)
    } else {
        // console.log('backend', JSON.stringify(cmdStr))
    }
}

function resetTremola() { // wipes browser-side content
    tremola = {
        "chats": {},
        "contacts": {},
        "profile": {},
        "id": myId,
        "settings": {},
        "board": {},
        "kahoot": {}
    }

    var n = recps2nm([myId])
    tremola.chats[n] = {
        "alias": "Local notes (for my eyes only)", "posts": {}, "forgotten": false,
        "members": [myId], "touched": Date.now(), "lastRead": 0,
        "timeline": new Timeline()
    };

    tremola.chats["ALL"] = {
        "alias": "Public channel", "posts": {},
        "members": ["ALL"], "touched": Date.now(), "lastRead": 0,
        "timeline": new Timeline()
    };
    tremola.contacts[myId] = {"alias": "me", "initial": "M", "color": "#bd7578", "iam": "", "forgotten": false};
    persist();
}

function persist() {
    // console.log(tremola);
    window.localStorage.setItem("tremola", JSON.stringify(tremola));
}

function b2f_ble_enabled() {
    //ble_status = "enabled"
    //TODO update ui
}

function b2f_ble_disabled() {
    for(var p in localPeers) {
        if(localPeers[p].type == "ble") {
            delete localPeers[p]
            refresh_connection_entry(p)
        }
    }
    //ble_status = "disabled"
}

function b2f_update_progress(min_entries, old_min_entries, old_want_entries, curr_want_entries, max_entries) {
    refresh_connection_progressbar(min_entries, old_min_entries, old_want_entries, curr_want_entries, max_entries)
}

function b2f_local_peer(type, identifier, displayname, status) {
    // type: 'udp' or 'ble'
    // identifier: unique identifier of the peer
    // displayname
    // status: 'connected', 'disconnected'
    console.log(`local_peer: type=${type} id=${identifier} dn=${displayname} status=${status}`)
    if (displayname == "null") {
        displayname = identifier
    }

    localPeers[identifier] = {
        'type' : type,
        'name': displayname,
        'status': status,
        'alias': null,
        'remaining': null
    }

    if (tremola != null) // can be the case during the first initialisation
        for (var c in tremola["contacts"]) {
            if (id2b32(c) == displayname) {
                localPeers[identifier].alias = tremola.contacts[c].alias
            }
        }

    if (status == "offline") {
      delete localPeers[identifier]
      //refresh_connection_progressbar()
    }

    if (document.getElementById('connection-overlay').style.display != 'none')
        refresh_connection_entry(identifier)
}

/**
 * This function is called, when the backend received a new log entry and successfully completed the corresponding sidechain.
 * The backend assures, that the log entries are sent to the frontend in the exact same sequential order as in the append-only log.
 *
 * @param {Object} e     Object containing all information of the log_entry.
 * @param {Object} e.hdr Contains basic information about the log entry.
 * @param {number} e.hdr.tst Timestamp at which the message was created. (Number of milliseconds elapsed since midnight at the beginning of January 1970 00:00 UTC)
 * @param {string} e.hdr.ref The message ID of this log entry.
 * @param {string} e.hdr.fid The public key of the author encoded in base64.
 * @param {[]} e.public The payload of the message. The first entry is a String that represents the application to which the message belongs. All additional entries are application-specific parameters.
 *
 */
function b2f_new_in_order_event(e) {
    if (e.public)
        console.log("b2f inorder event:", JSON.stringify(e.public))
    if (e.confid)
        console.log("b2f inorder event:", JSON.stringify(e.confid))

    if (!(e.header.fid in tremola.contacts)) {
        var a = id2b32(e.header.fid);
        tremola.contacts[e.header.fid] = {
            "alias": a, "initial": a.substring(0, 1).toUpperCase(),
            "color": colors[Math.floor(colors.length * Math.random())],
            "iam": "", "forgotten": false
        }
        load_contact_list()
    }

    if (e.public) switch (e.public[0]) {
        case "KAN":
            console.log("New kanban event")
            kanban_new_event(e)
            break
        case "C4B":
            connect4_game_new_event(e);
            break
        case "C4E":
            connect4_game_end_event(e);
            break
        case "C4I":
            connect4_recv_invite(e);
            break
        case "C4D":
            connect4_invite_declined(e);
            break
        case "KAH":
            console.log("New kahoot event")
            Kahoot_new_event(e)
            break
        case "SCH":
            console.log("New scheduling event")
            dpi_scheduling_new_event(e)
            break
        default:
            return
    }
    persist();
    must_redraw = true;
}

/**
 * This function is invoked whenever the backend receives a new log entry, regardless of whether the associated sidechain is fully loaded or not.
 *
 * @param {Object} e     Object containing all information of the log_entry.
 * @param {Object} e.hdr Contains basic information about the log entry.
 * @param {number} e.hdr.tst Timestamp at which the message was created. (Number of milliseconds elapsed since midnight at the beginning of January 1970 00:00 UTC)
 * @param {string} e.hdr.ref The message ID of this log entry.
 * @param {string} e.hdr.fid The public key of the author encoded in base64.
 * @param {[]} e.public The payload of the logentry, without the content of the sidechain
 *
 */
function b2f_new_incomplete_event(e) {

    if (!(e.header.fid in tremola.contacts)) {
        var a = id2b32(e.header.fid);
        tremola.contacts[e.header.fid] = {
            "alias": a, "initial": a.substring(0, 1).toUpperCase(),
            "color": colors[Math.floor(colors.length * Math.random())],
            "iam": "", "forgotten": false
        }
        load_contact_list()
    }

    switch (e.public[0]) {
        default:
            return
    }
    persist();
    must_redraw = true;
}

/**
 * This function is called, when the backend received a new log entry and successfully completed the corresponding sidechain.
 * This callback does not ensure any specific order; the log entries are forwarded in the order they are received.
 *
 * @param {Object} e     Object containing all information of the log_entry.
 * @param {Object} e.hdr Contains basic information about the log entry.
 * @param {number} e.hdr.tst Timestamp at which the message was created. (Number of milliseconds elapsed since midnight at the beginning of January 1970 00:00 UTC)
 * @param {string} e.hdr.ref The message ID of this log entry.
 * @param {string} e.hdr.fid The public key of the author encoded in base64.
 * @param {[]} e.public The payload of the message. The first entry is a String that represents the application to which the message belongs. All additional entries are application-specific parameters.
 *
 */
function b2f_new_event(e) { // incoming SSB log event: we get map with three entries
                            // console.log('hdr', JSON.stringify(e.header))
    console.log("New Frontend Event: " + JSON.stringify(e.header))
    if (e.public)
        console.log('pub', JSON.stringify(e.public))
    if (e.confid)
        console.log('cfd', JSON.stringify(e.confid))

    //add
    if (!(e.header.fid in tremola.contacts)) {
        var a = id2b32(e.header.fid);
        tremola.contacts[e.header.fid] = {
            "alias": a, "initial": a.substring(0, 1).toUpperCase(),
            "color": colors[Math.floor(colors.length * Math.random())],
            "iam": "", "forgotten": false
        }
        load_contact_list()
    }

    if (e.public) {
        if (e.public[0] == 'TAV') { // text and voice
            // console.log("new post 0 ", tremola)
            var conv_name = "ALL";
            if (!(conv_name in tremola.chats)) { // create new conversation if needed
                tremola.chats[conv_name] = {
                    "alias": "Public channel X", "posts": {},
                    "members": ["ALL"], "touched": Date.now(), "lastRead": 0,
                    "timeline": new Timeline()
                };
                load_chat_list()
            }
            // console.log("new post 1")
            var ch = tremola.chats[conv_name];
            if (!(ch.timeline instanceof Timeline))
                ch.timeline = Timeline.fromJSON(ch.timeline)

            // console.log("new post 1 ", ch)
            if (!(e.header.ref in ch.posts)) { // new post
                var a = e.public;
                var p = {
                    "key": e.header.ref, "from": e.header.fid, "body": a[2],
                    "voice": a[3], "when": a[4] * 1000, "prev": a[1]
                };
                // console.log("new post 2 ", JSON.stringify(p))
                // console.log("time: ", a[3])
                ch["posts"][e.header.ref] = p;
                // console.log(`chat add tips ${a[1]}`)
                ch.timeline.add(e.header.ref, a[1])
                if (ch["touched"] < e.header.tst)
                    ch["touched"] = e.header.tst
                if (curr_scenario == "posts" && curr_chat == conv_name) {
                    load_chat(conv_name); // reload all messages (not very efficient ...)
                    ch.lastRead = Object.keys(ch.posts).length; // Date.now();
                }
                set_chats_badge(conv_name)
            } else {
                console.log(`post ${e.header.ref} known already?`)
            }
            // if (curr_scenario == "chats") // the updated conversation could bubble up
            load_chat_list();
        } else if (e.public[0] == "KAN") { // Kanban board event
        } else if (e.public[0] == "IAM") {
            var contact = tremola.contacts[e.header.fid]
            var old_iam = contact.iam
            var old_alias = contact.alias

            contact.iam = e.public[1]

            if ((contact.alias == id2b32(e.header.fid) || contact.alias == old_iam)) {
                contact.alias = e.public[1] == "" ? id2b32(e.header.fid) : e.public[1]
                contact.initial = contact.alias.substring(0, 1).toUpperCase()
                load_contact_list()
                load_kanban_list()
                dpi_load_event_list(tremola)

                // update names in connected devices menu
                for (var l in localPeers) {
                    if (localPeers[l].alias == old_alias) {
                        localPeers[l].alias = contact.alias
                        refresh_connection_entry(l)
                    }
                }
            }
        } else if (e.public[0] == "GAM") {
            // TODO autoretransmit answer if necessary
              if (window.GamesHandler && typeof window.GamesHandler.onGameBackendEvent === 'function') {
                  var response = window.GamesHandler.onGameBackendEvent(e.public[1]);
                  var responseList = response.split("!CERBERUS!")
                  if (responseList.length >= 1 && responseList[0].startsWith("games")) {
                      // TODO anpassen von GUI
                      backend(responseList[0]);
                  }
                  if (responseList.length == 2) {
                      update_game_gui(responseList[1])
                  } else {
                      update_game_gui(responseList[0])
                  }
              } else {
                  console.error("GamesHandler.onGameBackendEvent is not a function");
              }
              //Android.onGameBackendEvent(e.public);
        }
        persist();
        must_redraw = true;
    }
    if (e.confid) {
        var a = e.confid;
        var rcpts = null;
        if (a[0] == 'TAV')
            rcpts = a[5];
        else if (a[0] == 'DLV' || a[0] == 'ACK')
            if (e.header.fid != myId) // ignore our own confirmation msgs
                rcpts = [e.header.fid, myId]
        if (rcpts != null) {
            var conv_name = recps2nm(rcpts)
            // console.log("conv_name " + JSON.stringify(conv_name))
            if (!(conv_name in tremola.chats)) { // create new conversation if needed
                tremola.chats[conv_name] = {
                    "alias": recps2display(rcpts), "posts": {},
                    "members": rcpts, "touched": Date.now(), "lastRead": 0,
                    "timeline": new Timeline()
                };
                load_chat_list()
            }
            // console.log("new priv post 0")
            let ch = tremola.chats[conv_name];

            if (a[0] == 'DLV' || a[0] == 'ACK') { // remote delivery notification
                if (e.header.fid != myId && ch.members.length == 2 && a[1] in ch.posts) { // only for person-to-person
                    var p = ch.posts[a[1]];
                    if (p.status != 'ACC') // prevent downgrading to DLV
                        p.status = a[0]
                    console.log(`post status of ${e.header.ref} is now ${p.status}`)
                    if (curr_scenario == 'posts' && curr_chat == conv_name)
                        load_chat(conv_name)
               }
            } else if (a[0] == 'TAV') { // text and voice
                if (ch.timeline == null)
                    ch["timeline"] = new Timeline();
                else if (!(ch.timeline instanceof Timeline))
                    ch.timeline = Timeline.fromJSON(ch.timeline)
                // console.log(`chat add tips ${a[1]}`)
                ch.timeline.add(e.header.ref, a[1])

                // console.log("new priv post 1 ", tremola)
                // console.log("new priv post 1 ", ch)
                if (!(e.header.ref in ch.posts)) { // new post
                    var p = {
                        "key": e.header.ref, "from": e.header.fid, "body": a[2],
                        "voice": a[3], "when": a[4] * 1000, "prev": a[1], "status":""
                    };
                    // console.log("new priv post 2 ", p)
                    // console.log("time: ", a[3])
                    ch["posts"][e.header.ref] = p;
                    // persist()
                    // now that we have stored it, send a delivery confirmation
                    if (e.header.fid != myId && ch.members.length == 2 && tremola.settings.confirmed_delivery_enabled)
                        p.status = 'needs_ack';
                        setTimeout(function () {
                            // let tips = JSON.stringify(ch.timeline.get_tips())
                            backend("conf_dlv " + e.header.ref + " " + e.header.fid)
                        }, 200)
                    // console.log(`chat add ${a[1]}`)
                    if (ch["touched"] < e.header.tst)
                        ch["touched"] = e.header.tst
                    if (curr_scenario == "posts" && curr_chat == conv_name)
                        load_chat(conv_name); // reload all messages (not very efficient ...)
                    set_chats_badge(conv_name)
                } else {
                    console.log("known already?")
                }
                // if (curr_scenario == "chats") // the updated conversation could bubble up
                load_chat_list();
            }
            persist();
            must_redraw = true;
        }
    }
}

// backend callback method when calling backend("settings:get")
function b2f_get_settings(settings) {
    tremola.settings = settings
}

function b2f_new_contact(fid) {
    if (typeof tremola == 'undefined' || fid in tremola.contacts) // do not overwrite existing entry
        return;
    var id = id2b32(fid);
    tremola.contacts[fid] = {
        "alias": id, "initial": id.substring(0, 1).toUpperCase(),
        "color": colors[Math.floor(colors.length * Math.random())],
        "iam": "", "forgotten": false
    };
    persist()
    load_contact_list();
}

function b2f_new_voice(voice_b64) {
    new_voice_post(voice_b64)
}

function b2f_showSecret(json) {
    //setScenario(prev_scenario);
    generateQR(json)
}

function b2f_new_image_blob(ref) {
    console.log("new image: ", ref);
    curr_img_candidate = ref;
    ref = ref.replace(new RegExp('/'), "_");
    ref = "http://appassets.androidplatform.net/blobs/" + ref;
    ref = "<object type='image/jpeg' data='" + ref + "' style='width: 100%; height: 100%; object-fit: scale-down;'></object>"
    document.getElementById('image-preview').innerHTML = ref
    document.getElementById('image-caption').value = '';
    var s = document.getElementById('image-overlay').style;
    s.display = 'initial';
    s.height = '80%'; // 0.8 * docHeight;
    document.getElementById('overlay-bg').style.display = 'initial';
    overlayIsActive = true;
}

function b2f_initialize(id, settings) {
    myId = id
    myShortId = id2b32(myId)
    if (window.localStorage.tremola) {
        tremola = JSON.parse(window.localStorage.getItem('tremola'));

        if (tremola != null && id != tremola.id) // check for clash of IDs, erase old state if new
            tremola = null;
    } else
        tremola = null;
    if (tremola == null) {
        resetTremola();
        console.log("reset tremola")
        if (typeof Android == 'undefined')
            tremola.settings = BrowserOnlySettings // browser-only testing
    }
    if (typeof Android == 'undefined')
        console.log("loaded ", JSON.stringify(tremola))
    else
        tremola.settings = JSON.parse(settings)
    var nm, ref;
    for (nm in tremola.settings)
        setSetting(nm, tremola.settings[nm])
    load_chat_list()
    load_prod_list()
    load_game_list()
    load_contact_list()
    loadShortIds()
    // load_kanban_list()
    // dpi_load_event_list() // problem with access to tremola object
    if (tremola.kahoot == undefined) {
      tremola.kahoot = {};
      persist();
    }

    closeOverlay();
    setScenario('chats');
    // load_chat("ALL");
}


function addToMap(fid) {
    const fidShort = id2b32(fid) // fid.substring(0, 7);  // Extract the shortID from the longID
    shortToFidMap[fidShort] = fid;         // Map shortID to longID
}
function loadShortIds() {
    for (var id in tremola.contacts)
        addToMap(id);
}

// --- eof

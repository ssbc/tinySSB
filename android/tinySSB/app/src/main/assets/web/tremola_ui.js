// tremola_ui.js

"use strict";

var overlayIsActive = false;

var display_or_not = [
    'div:qr', 'div:back',
    'core', 'lst:chats', 'lst:posts', 'lst:contacts', 'lst:members', 'the:connex',
    'lst:kanban', 'div:footer', 'div:textarea', 'div:confirm-members', 'plus',
    'div:settings', 'div:board', 'the:game', 'lst:players', 'the:game-session'
];

var prev_scenario = 'chats';
var curr_scenario = 'chats';

var scenarioDisplay = {
    'chats': ['div:qr', 'core', 'lst:chats', 'div:footer'], // 'plus' TODO reactivate when encrypted chats are implemented
    'contacts': ['div:qr', 'core', 'lst:contacts', 'div:footer', 'plus'],
    'posts': ['div:back', 'core', 'lst:posts', 'div:textarea'],
    'connex': ['div:qr', 'core', 'the:connex', 'div:footer', 'plus'],
    'members': ['div:back', 'core', 'lst:members', 'div:confirm-members'],
    'settings': ['div:back', 'div:settings'],
    'kanban': ['div:qr', 'core', 'lst:kanban', 'div:footer', 'plus'],
    'board': ['div:back', 'core', 'div:board'],
    'game': ['div:qr', 'core', 'the:game', 'div:footer', 'plus'],
    'game-players': ['div:back', 'core', 'lst:players', 'div:confirm-player'],
    'game-session': ['div:back', 'core', 'the:game-session']
    }

var scenarioMenu = {
    'chats': [['Connected Devices', 'menu_connection'], // '['New conversation', 'menu_new_conversation'],' TODO reactivate when encrypted chats are implemented
        ['Settings', 'menu_settings'],
        ['About', 'menu_about']],
    'contacts': [['New contact', 'menu_new_contact'],
        ['Connected Devices', 'menu_connection'],
        ['Settings', 'menu_settings'],
        ['About', 'menu_about']],
    'connex': [['New SSB pub', 'menu_new_pub'],
        ['Redeem invite code', 'menu_invite'],
        ['Connected Devices', 'menu_connection'],
        // ['<del>Force sync</del>', 'menu_sync'],
        ['Settings', 'menu_settings'],
        ['About', 'menu_about']],
    /*
      ['Redraw', 'menu_redraw'],
      ['Sync', 'menu_sync'],
      ['Redraw', 'menu_redraw'],
      ['Restream', 'menu_stream_all_posts'],
      ['Import ID', 'menu_import_id'],
      ['Process msgs', 'menu_process_msgs'],
      ['Add pub', 'menu_add_pub'],
      ['Dump', 'menu_dump'],
      ['Reset', 'menu_reset']]
    */
    'posts': [/* ['Take picture', 'menu_take_picture'],
                ['Pick image', 'menu_pick_image'], */
        ['Rename this chat', 'menu_edit_convname'],
        ['(un)Forget', 'menu_forget_conv'],
        ['Settings', 'menu_settings'],
        ['About', 'menu_about']],
    'members': [['Settings', 'menu_settings'],
        ['About', 'menu_about']],

    'settings': [],

    'kanban': [['New Kanban board', 'menu_new_board'],
        ['Invitations', 'menu_board_invitations'],
        ['Connected Devices', 'menu_connection'],
        ['Settings', 'menu_settings'],
        ['About', 'menu_about']],

    'board': [['Add list', 'menu_new_column'],
        ['Rename Kanban Board', 'menu_rename_board'],
        ['Invite Users', 'menu_invite'],
        ['History', 'menu_history'],
        ['Reload', 'reload_curr_board'],
        ['Leave', 'leave_curr_board'],
        ['(un)Forget', 'board_toggle_forget'],
        ['Debug', 'ui_debug']],

    'game': [['New Game', 'menu_game_players'],
            ['Connected Devices', 'menu_connection'],
            ['Settings', 'menu_settings'],
            ['About', 'menu_about']]
}

function onBackPressed() {
    if (overlayIsActive) {
        closeOverlay();
        return;
    }
    if (['chats', 'contacts', 'connex', 'board', 'game'].indexOf(curr_scenario) >= 0) {
        if (curr_scenario == 'chats')
            backend("onBackPressed");
        else if (curr_scenario == 'board')
            setScenario('kanban')
        else
            setScenario('chats')
    } else {
        if (curr_scenario == 'settings') {
            document.getElementById('div:settings').style.display = 'none';
            document.getElementById('core').style.display = null;
            document.getElementById('div:footer').style.display = null;
        }
        setScenario(prev_scenario);
        load_games_list();
    }
}

function setScenario(s) {
    // console.log('setScenario ' + s)
    closeOverlay();
    var lst = scenarioDisplay[s];
    if (lst) {
        // if (s != 'posts' && curr_scenario != "members" && curr_scenario != 'posts') {
        if (['chats', 'contacts', 'connex', 'kanban', 'game'].indexOf(curr_scenario) >= 0) {
            var cl = document.getElementById('btn:' + curr_scenario).classList;
            cl.toggle('active', false);
            cl.toggle('passive', true);
        }
        // console.log(' l: ' + lst)
        display_or_not.forEach(function (d) {
            // console.log(' l+' + d);
            if (lst.indexOf(d) < 0) {
                document.getElementById(d).style.display = 'none';
            } else {
                document.getElementById(d).style.display = null;
                // console.log(' l=' + d);
            }
        })
        // console.log('s: ' + s)
        if (s != "board") {
            document.getElementById('tremolaTitle').style.position = null;
        }

        if (s == "posts" || s == "settings" || s == "board") {
            document.getElementById('tremolaTitle').style.display = 'none';
            document.getElementById('conversationTitle').style.display = null;
            // document.getElementById('plus').style.display = 'none';
        } else {
            document.getElementById('tremolaTitle').style.display = null;
            // if (s == "connex") { /* document.getElementById('plus').style.display = 'none'; */}
            // else { /* document.getElementById('plus').style.display = null; */}
            document.getElementById('conversationTitle').style.display = 'none';
        }
        if (lst.indexOf('div:qr') >= 0) {
            prev_scenario = s;
        }
        curr_scenario = s;
        if (['chats', 'contacts', 'connex', 'kanban', 'game'].indexOf(curr_scenario) >= 0) {
            var cl = document.getElementById('btn:' + curr_scenario).classList;
            cl.toggle('active', true);
            cl.toggle('passive', false);
        }
        if (s == 'board')
            document.getElementById('core').style.height = 'calc(100% - 60px)';
        else
            document.getElementById('core').style.height = 'calc(100% - 118px)';

        if (s == 'kanban') {
            var personalBoardAlreadyExists = false
            for (var b in tremola.board) {
                var board = tremola.board[b]
                if (board.flags.indexOf(FLAG.PERSONAL) >= 0 && board.members.length == 1 && board.members[0] == myId) {
                    personalBoardAlreadyExists = true
                    break
                }
            }
            if(!personalBoardAlreadyExists && display_create_personal_board) {
                menu_create_personal_board()
            }
        }

    }
}

function btnBridge(e) {
    var e = e.id, m = '';
    if (['btn:chats', 'btn:posts', 'btn:contacts', 'btn:connex', 'btn:kanban', 'btn:game'].indexOf(e) >= 0) {
        setScenario(e.substring(4));
    }
    if (e == 'btn:menu') {
        if (scenarioMenu[curr_scenario].length == 0)
            return;
        document.getElementById("menu").style.display = 'initial';
        document.getElementById("overlay-trans").style.display = 'initial';
        scenarioMenu[curr_scenario].forEach(function (e) {
            m += "<button class=menu_item_button ";
            m += "onclick='" + e[1] + "();'>" + e[0] + "</button><br>";
        })
        m = m.substring(0, m.length - 4);
        // console.log(curr_scenario + ' menu! ' + m);
        document.getElementById("menu").innerHTML = m;
        return;
    }
    if (e == 'btn:attach') {
        if (scenarioMenu[curr_scenario].length == 0)
            return;
        backend('get:voice'); // + btoa(document.getElementById('draft').value));
        return;
    }

    // if (typeof Android != "undefined") { Android.onFrontendRequest(e); }
}

function menu_settings() {
    closeOverlay();
    setScenario('settings')
    document.getElementById("settings_urlInput").classList.remove("invalid")
    document.getElementById("settings_urlInput").value = tremola.settings["websocket_url"]
    if (tremola.settings["websocket"])
      document.getElementById("container:settings_ws_url").style.display = 'flex'
    /*
    prev_scenario = curr_scenario;
    curr_scenario = 'settings';
    document.getElementById('core').style.display = 'none';
    document.getElementById('div:footer').style.display = 'none';
    document.getElementById('div:settings').style.display = null;

    document.getElementById("tremolaTitle").style.display = 'none';
    */
    var c = document.getElementById("conversationTitle");
    c.style.display = null;
    c.innerHTML = "<div style='text-align: center;'><font size=+1><strong>Settings</strong></font></div>";
}

function closeOverlay() {
    document.getElementById('menu').style.display = 'none';
    document.getElementById('qr-overlay').style.display = 'none';
    document.getElementById('preview-overlay').style.display = 'none';
    document.getElementById('image-overlay').style.display = 'none';
    document.getElementById('new_chat-overlay').style.display = 'none';
    document.getElementById('new_contact-overlay').style.display = 'none';
    document.getElementById('confirm_contact-overlay').style.display = 'none';
    document.getElementById('overlay-bg').style.display = 'none';
    document.getElementById('overlay-trans').style.display = 'none';
    document.getElementById('overlay-bg-core').style.display = 'none';
    document.getElementById('overlay-trans-core').style.display = 'none';
    document.getElementById('about-overlay').style.display = 'none';
    document.getElementById('edit-overlay').style.display = 'none';
    document.getElementById('new_contact-overlay').style.display = 'none';
    document.getElementById('old_contact-overlay').style.display = 'none';
    document.getElementById('attach-menu').style.display = 'none';
    document.getElementById('div:modal_img').style.display = 'none';
    document.getElementById('connection-overlay').style.display = 'none';

    // kanban overlays
    document.getElementById('div:menu_history').style.display = 'none';
    document.getElementById('div:item_menu').style.display = 'none';
    document.getElementById("kanban-invitations-overlay").style.display = 'none';
    document.getElementById('kanban-create-personal-board-overlay').style.display = 'none';
    curr_item = null
    close_board_context_menu()
    document.getElementById('btn:item_menu_description_save').style.display = 'none'
    document.getElementById('btn:item_menu_description_cancel').style.display = 'none'
    document.getElementById('div:debug').style.display = 'none'
    document.getElementById("div:invite_menu").style.display = 'none'

    overlayIsActive = false;

    if (curr_img_candidate != null) {
        backend('del:blob ' + curr_img_candidate);
        curr_img_candidate = null;
    }
}

function showPreview() {
    var draft = escapeHTML(document.getElementById('draft').value);
    if (draft.length == 0) return;
    if (!getSetting("enable_preview")) {
        new_text_post(draft);
        return;
    }
    var draft2 = draft.replace(/\n/g, "<br>\n");
    var to = recps2display(tremola.chats[curr_chat].members)
    document.getElementById('preview').innerHTML = "To: " + to + "<hr>" + draft2 + "&nbsp;<hr>";
    var s = document.getElementById('preview-overlay').style;
    s.display = 'initial';
    s.height = '80%'; // 0.8 * docHeight;
    document.getElementById('overlay-bg').style.display = 'initial';
    overlayIsActive = true;
}

function menu_about() {
    closeOverlay()
    document.getElementById('about-overlay').style.display = 'initial';
    document.getElementById('overlay-bg').style.display = 'initial';
    overlayIsActive = true;
}

function plus_button() {
    closeOverlay();
    if (curr_scenario == 'chats') {
        menu_new_conversation();
    } else if (curr_scenario == 'contacts') {
        menu_new_contact();
    } else if (curr_scenario == 'connex') {
        menu_new_pub();
    } else if (curr_scenario == 'kanban') {
        menu_new_board();
    } else if (curr_scenario == 'game' ) {
        menu_game_players();
    }
}

function launch_snackbar(txt) {
    var sb = document.getElementById("snackbar");
    sb.innerHTML = txt;
    sb.className = "show";
    setTimeout(function () {
        sb.className = sb.className.replace("show", "");
    }, 3000);
}

// --- QR display and scan

function showQR() {
    generateQR('did:ssb:ed25519:' + myId.substring(1).split('.')[0])
}

function generateQR(s) {
    document.getElementById('qr-overlay').style.display = 'initial';
    document.getElementById('overlay-bg').style.display = 'initial';
    document.getElementById('qr-text').innerHTML = s;
    if (!qr) {
        var w, e, arg;
        w = window.getComputedStyle(document.getElementById('qr-overlay')).width;
        w = parseInt(w, 10);
        e = document.getElementById('qr-code');
        arg = {
            height: w,
            width: w,
            text: s,
            correctLevel: QRCode.CorrectLevel.M // L, M, Q, H
        };
        qr = new QRCode(e, arg);
    } else {
        qr.clear();
        qr.makeCode(s);
    }
    overlayIsActive = true;
}

function qr_scan_start() {
    // test if Android is defined ...
    backend("qrscan.init");
    closeOverlay();
}

function qr_scan_success(s) {
    closeOverlay();
    var t = "did:ssb:ed25519:";
    if (s.substring(0, t.length) == t) {
        s = '@' + s.substring(t.length) + '.ed25519';
    }
    var b = '';
    try {
        b = atob(s.substr(1, s.length - 9));
        // FIXME we should also test whether it is a valid ed25519 public key ...
    } catch (err) {
    }
    if (b.length != 32) {
        launch_snackbar("unknown format or invalid identity");
        return;
    }
    new_contact_id = s;
    // console.log("tremola:", tremola)
    if (new_contact_id in tremola.contacts) {
        launch_snackbar("This contact already exists");
        return;
    }
    // FIXME: do sanity tests
    menu_edit('new_contact_alias', "Assign alias to new contact:<br>(only you can see this alias)", "");
}

function qr_scan_failure() {
    launch_snackbar("QR scan failed")
}

function qr_scan_confirmed() {
    var a = document.getElementById('alias_text').value;
    var s = document.getElementById('alias_id').innerHTML;
    // c = {alias: a, id: s};
    var i = (a + "?").substring(0, 1).toUpperCase()
    var c = {"alias": a, "initial": i, "color": colors[Math.floor(colors.length * Math.random())], "iam": "", "forgotten": false};
    tremola.contacts[s] = c;
    persist();
    backend("add:contact " + s + " " + btoa(a))
    load_contact_item([s, c]);
    closeOverlay();
}

function modal_img(img) {
    var modalImg = document.getElementById("modal_img");
    modalImg.src = img.data;
    var modal = document.getElementById('div:modal_img');
    modal.style.display = "block";
    overlayIsActive = true;
    let pz = new PinchZoom(modalImg,
        {
            onDoubleTap: function () {
                closeOverlay();
            }, maxZoom: 8
        }
    );
}

function menu_connection() {
    closeOverlay();
    //refresh_connection_progressbar()

    document.getElementById('connection-overlay-content').innerHTML = '';

    for (var peer in localPeers) {
        refresh_connection_entry(peer);
    }

    document.getElementById('overlay-bg').style.display = 'initial';
    document.getElementById('connection-overlay').style.display = 'initial';
    overlayIsActive = true;
}

function refresh_connection_entry(id) {
    var content = document.getElementById('connection-overlay-content')

    // only update existing entry
    if (document.getElementById('connection_' + id)) {
        if(id in localPeers) {
            var name = localPeers[id].alias != null ? localPeers[id].alias : localPeers[id].name
            if (name.length > 28)
                name = name.slice(0,27)
            document.getElementById('connection_name_' + id).innerHTML = name
            document.getElementById('connection_type_' + id).innerHTML = "via " + localPeers[id].type
            document.getElementById('connection_remaining_' + id).innerHTML = localPeers[id].remaining
        } else {
            document.getElementById('connection_' + id).outerHTML = ""
        }
        return
    }

    if(!(id in localPeers))
        return

    // create new entry

    var peer = localPeers[id]
    var name = localPeers[id].alias != null ? peer.alias : peer.name
    if (name.length > 28)
        name = name.slice(0,27)
    var remaining = peer.remaining != null ? peer.remaining : ""//"Remaining: "+ peer.remaining + " messages" : "Remaining messages unknown"
    var type = (peer.type != null) && (peer.type != "") ? peer.type : ""

    var entryHTML = "<div id='connection_" + id + "' class = 'connection_entry_container'>"
    entryHTML += "<div class='connection_entry_name_container'>"
    entryHTML += "<div id='connection_name_" + id + "' style='grid-area: name; margin-left: 5px; margin-top: 4px;font-size: 16px; font-weight: bold;white-space: nowrap;'>" + name + "</div>"
    entryHTML += "<div id='connection_type_" + id + "' style='grid-area: type; margin-left: 5px; font-size: 13px'>via " + type + "</div>"
    entryHTML += "</div>"
    entryHTML += "<div id='connection_remaining_" + id + "' style='grid-area: remaining;align-self: center; text-align: end; padding-right: 5px; '>" + remaining + "</div>"
    entryHTML += "</div>"

    document.getElementById('connection-overlay-content').innerHTML += entryHTML

}

function refresh_connection_progressbar(min_entries, old_min_entries, old_want_entries, curr_want_entries, max_entries) {

    console.log("min:", min_entries)
    console.log("old_min:", old_min_entries)
    console.log("old_curr:", old_want_entries)
    console.log("curr:", curr_want_entries)
    console.log("max:", max_entries)

    if(curr_want_entries == 0)
      return

  // update want progress

  if(curr_want_entries >= max_entries || old_want_entries == max_entries) {
    document.getElementById('connection-overlay-progressbar-want').value = 100
    document.getElementById('connection-overlay-progressbar-label-want').textContent = "Requesting — Synchronized"
  } else {
    var newPosReq = (curr_want_entries - old_want_entries) / (max_entries - old_want_entries) * 100

    console.log("newPosMax:", newPosReq)

    document.getElementById('connection-overlay-progressbar-want').value = newPosReq
    document.getElementById('connection-overlay-progressbar-label-want').textContent = Math.trunc(newPosReq) + "% - " + (max_entries - curr_want_entries) + " entries left"

  }

  // update gift progress
  if (curr_want_entries <= min_entries || old_min_entries == curr_want_entries) {
    document.getElementById('connection-overlay-progressbar-gift').value = 100
    document.getElementById('connection-overlay-progressbar-label-gift').textContent = "Offering — Synchronized"
  } else {
    var newPosOff = (min_entries - old_min_entries) / (curr_want_entries - old_min_entries) * 100

    document.getElementById('connection-overlay-progressbar-gift').value = newPosOff
    document.getElementById('connection-overlay-progressbar-label-gift').textContent = Math.trunc(newPosOff) + "% - " + (curr_want_entries - min_entries) + " entries left"
  }
}

// ---
// tremola_ui.js

"use strict";

var overlayIsActive = false;

var display_or_not = [
    'div:qr', 'div:back', 'core', 'plus',
    'lst:chats', 'lst:prod', 'lst:games', 'lst:contacts', 'lst:members',
    'div:posts', 'lst:kanban', 'div:board',
    'lst:duels', 'div:battleships', // battleship
    'lst:connect4-game', 'lst:connect4-players', 'the:connect4-game-session', // connect4
    'lst:kahoot',
    'lst:scheduling', 'div:event',
    'div:footer', 'div:textarea', 'div:confirm-members', 'div:settings'
];

var prev_scenario = 'chats';
var curr_scenario = 'chats';
var game_scenario = false;

var scenarioDisplay = {
    'chats': ['div:qr', 'core', 'lst:chats', 'div:footer', 'plus'],
    'contacts': ['div:qr', 'core', 'lst:contacts', 'div:footer', 'plus'],
    'posts': ['div:back', 'core', 'div:posts', 'div:textarea'],
    'games': ['div:back', 'core', 'lst:games', 'div:footer'],
    'members': ['div:back', 'core', 'lst:members', 'div:confirm-members'],
    'productivity': ['div:back', 'core', 'lst:prod', 'div:footer'],
    'settings': ['div:back', 'div:settings', 'core'],
    'kanban': ['div:back', 'core', 'lst:kanban', 'div:footer', 'plus'], // KANBAN
    'board': ['div:back', 'core', 'div:board'], // KANBAN
    'kahoot': ['div:back', 'core', 'lst:kahoot'],
    'duels': ['div:back', 'core', 'lst:duels', 'plus'], // BATTLESHIP
    'battleships': ['div:back', 'core', 'div:battleships'], // BATTLESHIP
    'connect4-game': ['div:back', 'core', 'lst:connect4-game', 'div:footer', 'plus'],
    'connect4-game-players': ['div:back', 'core', 'lst:connect4-players', 'div:connect4-confirm-player'],
    'connect4-game-session': ['div:back', 'core', 'the:connect4-game-session'],
    'scheduling': ['div:back', 'core', 'lst:scheduling', 'div:footer', 'plus'],
    'event': ['div:back', 'core', 'div:event']
}

var scenarioMenu = {
    'chats': [
        // ['New Channel', 'menu_new_conversation'],
        ['Settings', 'menu_settings'],
        ['About', 'menu_about']],
    'productivity': [
        ['Settings', 'menu_settings'],
        ['About', 'menu_about']],
    'games': [
        ['Settings', 'menu_settings'],
        ['About', 'menu_about']],
    'contacts': [
        // ['New contact', 'menu_new_contact'],
        ['Settings', 'menu_settings'],
        ['About', 'menu_about']],
    'posts': [/* ['Take picture', 'menu_take_picture'],
                ['Pick image', 'menu_pick_image'], */
        ['Rename this chat', 'menu_edit_convname'],
        ['(un)Forget', 'menu_forget_conv'],
        ['Settings', 'menu_settings'],
        ['About', 'menu_about']],
    'members': [
        ['Settings', 'menu_settings'],
        ['About', 'menu_about']],

    'settings': [],

    'kanban': [
        // ['New Kanban board', 'menu_new_board'], // no redundant functionality
        ['Invitations', 'menu_board_invitations'],
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

    'duels': [], // BATTLESHIP
    'battleships': [['Quit Game', 'quit_bsh']], // BATTLESHIP

    'connect4-game': [// ['New Game'], //, 'connect4_menu_game_players'],
            ['Settings', 'menu_settings'],
            ['About', 'menu_about']],

    'scheduling': [
        // ['New Event', 'dpi_menu_new_event'],
        ['Invitations', 'dpi_menu_event_invitations'],
        ['Settings', 'menu_settings'],
        ['About', 'menu_about']],

    'event': [
        ['Add appointment', 'dpi_menu_new_appointment'],
        ['Invite Users', 'dpi_menu_invite'],
        ['Reload', 'dpi_reload_curr_event'],
        ['Debug', 'dpi_ui_debug']]
}

const QR_SCAN_TARGET = {
    ADD_CONTACT: 0,
    IMPORT_ID: 1
}

var curr_qr_scan_target = QR_SCAN_TARGET.ADD_CONTACT

function onBackPressed() {
    if (overlayIsActive) {
        closeOverlay();
        return;
    }
    if (curr_scenario == 'settings') {
         document.getElementById('div:settings').style.display = 'none';
         document.getElementById('core').style.display = null;
         document.getElementById('div:footer').style.display = null;
         setScenario(prev_scenario);
         return;
    }
    if (curr_scenario == 'chats')
        backend("onBackPressed");
    else if (['productivity', 'games', 'contacts'].indexOf(curr_scenario) >= 0)
        setScenario(prev_scenario)
    else if (['connect4-game'].indexOf(curr_scenario) >= 0)
        setScenario('games')
    else if (['kanban','scheduling'].indexOf(curr_scenario) >= 0)
        setScenario('productivity')
    else if (curr_scenario == 'posts')
        setScenario('chats')
    else if (curr_scenario == 'board')
        setScenario('kanban')
    else if (curr_scenario == 'event')
        setScenario('scheduling')
    else if (curr_scenario == 'battleships') { // BATTLESHIP // TODO prev_scenario für duels unc posts und nicht in chat
        reset_battleship_mode()
        show_duels()
    } else if (curr_scenario == 'connect4-game-session') {
        connect4_load_games_list();
        setScenario('connect4-game')
    }
}

function setScenario(s) {
    // console.log('setScenario ' + s)
    closeOverlay();
    var lst = scenarioDisplay[s];
    if (lst) {
        // if (s != 'posts' && curr_scenario != "members" && curr_scenario != 'posts') {
        if (['chats', 'productivity', 'games', 'contacts'].indexOf(curr_scenario) >= 0) {
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
        if (s != "board" && s != '') { // show "tinySSB" by default
            document.getElementById('tremolaTitle').style.position = null;
        }

        if (s == "posts" || s == "settings" || s == "board" || s == "event" ||
            s == 'battleships') {
            document.getElementById('tremolaTitle').style.display = 'none';
            document.getElementById('conversationTitle').style.display = null;
            document.getElementById('plus').style.display = 'none';
        } else if (s == "duels") {
            document.getElementById('tremolaTitle').style.display = 'none';
        } else {
            document.getElementById('tremolaTitle').style.display = null;
            document.getElementById('conversationTitle').style.display = 'none';
        }
        if (lst.indexOf('div:qr') >= 0) {
            prev_scenario = s;
        }
        if (lst.indexOf('div:back') >= 0) { // remember where we came from)
            prev_scenario = curr_scenario;
        }
        curr_scenario = s;

        if (['chats', 'productivity', 'games', 'contacts'].indexOf(curr_scenario) >= 0) {
            var cl = document.getElementById('btn:' + curr_scenario).classList;
            cl.toggle('active', true);
            cl.toggle('passive', false);
        }
        if (s == 'chats') {
            document.getElementById('tremolaTitle').style.display = null;
            document.getElementById('conversationTitle').style.display = 'none';
            /*
            c.style.display = null;
            c.innerHTML = "<font size=+1><strong>List of Chat Channels</strong></font><br>Pick or create a channel";
            */
        }

        if (s == 'contacts') {
            document.getElementById("tremolaTitle").style.display = 'none';
            var c = document.getElementById("conversationTitle");
            c.style.display = null;
            c.innerHTML = "<font size=+2><strong>Contacts</strong></font>";
        }

        if (s == 'board') // a specific Kanban board: use all space (beyond the footer)
            document.getElementById('core').style.height = 'calc(100% - 60px)';
        else
            document.getElementById('core').style.height = 'calc(100% - 118px)';

        if (s == 'kanban') {
            document.getElementById("tremolaTitle").style.display = 'none';
            var c = document.getElementById("conversationTitle");
            c.style.display = null;
            c.innerHTML = "<font size=+1><strong>List of Kanban Boards</strong></font><br>Pick or create a board";

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

        if (s == 'kahoot') {
            console.log('Kahoot scenario activated');
            // Any additional initialization logic for Kahoot can go here
            //document.getElementById('lst:kahoot').style.display = 'block';
            document.getElementById('quiz-master-title').style.display = 'block';
            document.getElementById('kahoot-buttons').style.display = 'block';
            //document.getElementById('user-scores').style.display = 'block';
            //document.getElementById('enter-quiz-button-list-container').style.display = 'block';
            //document.getElementById('fill-quiz-button-list-container').style.display = 'block';
        }

        if (s == 'posts') {
          setTimeout(function () { // let image rendering (fetching size) take place before we scroll
              let c = document.getElementById('core');
              c.scrollTop = c.scrollHeight;
              let p = document.getElementById('div:posts');
              p.scrollTop = p.scrollHeight;
              }, 100);
        }

        if (s == 'connect4-game') {
            document.getElementById("tremolaTitle").style.display = 'none';
            var c = document.getElementById("conversationTitle");
            c.style.display = null;
            c.innerHTML = "<font size=+1><strong>Connect4 Sessions</strong></font><br>Pick or create a session";
            connect4_load_games_list();
        } else if (s == 'connect4-game-session') {
           document.getElementById("tremolaTitle").style.display = 'none';
           var c = document.getElementById("conversationTitle");
           c.style.display = null;
           c.innerHTML = "<font size=+2><strong>Connect4</strong></font>";
        }

        if (s == 'scheduling') {
            document.getElementById("tremolaTitle").style.display = 'none';
            var c = document.getElementById("conversationTitle");
            c.style.display = null;
            c.innerHTML = "<font size=+1><strong>List of Events</strong></font><br>Pick or create an event";
        /*
             var personalEventAlreadyExists = false
             if(tremola && tremola.event && tremola.event.length > 0){
             launch_snackbar("COUNT EVENTS" + tremola.event.length)
             console.log(tremola.event)

             } else {
              dpi_menu_create_personal_event();
        */
        }

    }
}

function btnBridge(e) {
    var e = e.id, m = '';
    if (['btn:chats', 'btn:contacts', 'btn:games',
         'btn:posts', 'btn:productivity'].indexOf(e) >= 0) {
         // console.log('btn', e)
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
    document.getElementById('geo-menu').style.display = 'none';
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
    document.getElementById('import-id-overlay').style.display = 'none';

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

    // scheduling overlays
    document.getElementById('scheduling-create-personal-event-overlay').style.display = 'none';
    document.getElementById('add-appointment-overlay').style.display='none';
    document.getElementById('scheduling-invitations-overlay').style.display='none';

    overlayIsActive = false;

    if (curr_img_candidate != null) {
        backend('del:blob ' + curr_img_candidate);
        curr_img_candidate = null;
    }
}

function showPreview() {
    var draft = escapeHTML(document.getElementById('draft').value);
    if (draft.length == 0) return;
    if (!getSetting("show_chat_preview")) {
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
    } else if (curr_scenario == 'kanban') {
        menu_new_board();
    } else if (curr_scenario == 'duels') {
        inviteForDuel('BSH');
    } else if (curr_scenario == 'connect4-game' ) {
        connect4_menu_game_players();
    } else if (curr_scenario == 'scheduling') {
        dpi_menu_new_event();
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

function chat_open_attachments_menu() {
    closeOverlay()
    document.getElementById('overlay-bg').style.display = 'initial'
    document.getElementById('attach-menu').style.display = 'initial'
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

function qr_scan_start(target) {
    // test if Android is defined ...
    curr_qr_scan_target = target
    backend("qrscan.init");
    closeOverlay();
}

function qr_scan_success(s) {
    closeOverlay();
    switch (curr_qr_scan_target) {
        case QR_SCAN_TARGET.ADD_CONTACT:
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
            break
        case QR_SCAN_TARGET.IMPORT_ID:
            r = import_id(s)
            if (r) {
                launch_snackbar("Successfully imported, restarting...")
            } else {
                launch_snackbar("wrong format")
            }
            break
    }
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

// --- sync status

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

function refresh_goset_progressbar(curr, max) {
    console.log("refresh_goset_progressbar", curr, max)
    var delta = max - curr

    document.getElementById('connection-overlay-progressbar-goset').value = (curr / max) * 100
    document.getElementById('connection-overlay-progressbar-label-goset').textContent = "GoSet - " + delta + " key" + (delta > 1 ? "s" : "") + " left"
    if (delta > 0) {
        console.log("display progress")
        // main html:
        document.getElementById('gosetBar').style.display = "initial"
        // document.getElementById('progressBars').style.display = "none"
        // overlay:
        document.getElementById('goset-progress-container').style.display = "initial"
        document.getElementById('progress-container').style.display = "none"
    } else {
        // main html:
        document.getElementById('gosetBar').style.display = "none"
        // document.getElementById('progressBars').style.display = "initial"
        // overlay:
        document.getElementById('goset-progress-container').style.display = "none"
        document.getElementById('progress-container').style.display = "initial"
    }

}

var max_chnks = 0
function refresh_chunk_progressbar(remaining) {
    if (remaining != 0) {
        max_chnks = Math.max(max_chnks, remaining)
    } else {
        max_chnks = 0 // reset
    }
    console.log(`refresh_chunk_progressbar - remaining:${remaining} max_chnks:${max_chnks}`)

    if (remaining > 0) {
        var percentage = Math.round( (1 - ((remaining - 0) / (max_chnks - 0))) * 100 )
        // main html:
        document.getElementById('progBarChunks').style.width = `${percentage}%`;
        // overlay:
        document.getElementById('connection-overlay-progressbar-chnk').value = percentage
        document.getElementById('connection-overlay-progressbar-label-chnk').textContent = remaining + " Chunks left"
    } else {
        document.getElementById('progBarChunks').style.width = '100%';
        document.getElementById('connection-overlay-progressbar-chnk').value = 100;
        document.getElementById('connection-overlay-progressbar-label-chnk').textContent = "Chunks — Synchronized"
    }
}

function refresh_connection_progressbar(min_entries, old_min_entries, old_want_entries, curr_want_entries, max_entries) {
    console.log(`refresh_connection_progressbar - min:${min_entries} old_min:${old_min_entries} ` +
                `old_curr:${old_want_entries} curr:${curr_want_entries} max:${max_entries}`)
    if(curr_want_entries == 0)
      return

    // update want progress
    if (curr_want_entries >= max_entries || old_want_entries == max_entries) {
        document.getElementById('progBarMissing').style.width = '100%';
        document.getElementById('connection-overlay-progressbar-want').value = 100
        document.getElementById('connection-overlay-progressbar-label-want').textContent = "Missing — Synchronized"
    } else {
        var newPosReq = Math.round((curr_want_entries - old_want_entries) / (max_entries - old_want_entries) * 100)
        console.log("newPosMax:", newPosReq)
        document.getElementById('progBarMissing').style.width = `${newPosReq}%`;
        document.getElementById('connection-overlay-progressbar-want').value = newPosReq
        document.getElementById('connection-overlay-progressbar-label-want').textContent = "Missing - " + (max_entries - curr_want_entries) + " entries left"
    }

    // update gift progress
    if (curr_want_entries <= min_entries || old_min_entries == curr_want_entries) {
        document.getElementById('progBarAhead').style.width = '100%';
        document.getElementById('connection-overlay-progressbar-gift').value = 100
        document.getElementById('connection-overlay-progressbar-label-gift').textContent = "Ahead — Synchronized"
    } else {
        var newPosOff = Math.round((min_entries - old_min_entries) / (curr_want_entries - old_min_entries) * 100)
        document.getElementById('progBarAhead').style.width = `${newPosOff}%`;
        document.getElementById('connection-overlay-progressbar-gift').value = newPosOff
        document.getElementById('connection-overlay-progressbar-label-gift').textContent = "Ahead - " + (curr_want_entries - min_entries) + " entries left"
    }
}

function show_geo_location(locPlus) {
//    var win = window.open("https://maps.app.goo.gl/Z7WWLG8UTAnfyJpb7", '_blank');

    var win = window.open("https://plus.codes/" + locPlus, '_blank');
    win.focus();
}

function copyToClipboard(text) {
    navigator.clipboard.writeText(text).then(() => {
        console.log('Text copied to clipboard successfully.');
    }).catch(err => {
        console.error('Failed to copy text to clipboard:', err);
    });
}

function showGeoMenu(plusCode) {
    closeOverlay();
    var latLongString = Android.getCoordinatesForPlusCode(plusCode);
    var latLong = JSON.parse(latLongString);
    var LatitudeLongitude = latLong.latitude + " " + latLong.longitude;
    var m = '';
    m += "<button class=menu_item_button ";
    m += "onclick='copyToClipboard(\"" + plusCode + "\");'>" + plusCode + "</button><br> ";
    m += "<button class=menu_item_button ";
    m += "onclick='copyToClipboard(\"" + LatitudeLongitude + "\");'>Lat: " + latLong.latitude + " Long: " + latLong.longitude + "</button><br>";
    m += "<button class=menu_item_button ";
    m += "onclick='show_geo_location(\"" + plusCode + "\");'>Show Location</button>";
    document.getElementById("geo-menu").innerHTML = m;
    document.getElementById("geo-menu").style.display = 'initial';
    document.getElementById("overlay-trans").style.display = 'initial';
}

function showGeoVoiceMenu(plusCode, chat, key) {
    closeOverlay();
    var latLongString = Android.getCoordinatesForPlusCode(plusCode);
    var latLong = JSON.parse(latLongString);
    var LatitudeLongitude = latLong.latitude + " " + latLong.longitude;
    var m = '';
    m += "<button class=menu_item_button ";
    m += "onclick='copyToClipboard(\"" + plusCode + "\");'>" + plusCode + "</button><br>";
    m += "<button class=menu_item_button ";
    m += "onclick='copyToClipboard(\"" + LatitudeLongitude + "\");'>Lat: " + latLong.latitude + " Long: " + latLong.longitude + "</button><br>";
    m += "<button class=menu_item_button ";
    m += "onclick='show_geo_location(\"" + plusCode + "\");'>Show Location</button><br>";
    m += "<button class=menu_item_button ";
    m += "onclick='play_voice(\"" + chat + "\", \"" + key + "\");'>Play Voice Message</button>";
    document.getElementById("geo-menu").innerHTML = m;
    document.getElementById("geo-menu").style.display = 'initial';
    document.getElementById("overlay-trans").style.display = 'initial';
}

// ---
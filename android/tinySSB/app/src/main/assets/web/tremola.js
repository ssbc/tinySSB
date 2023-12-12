// tremola.js

"use strict";

var tremola;
var curr_chat;
var qr;
var myId;
var localPeers = {}; // feedID ~ [isOnline, isConnected] - TF, TT, FT - FF means to remove this entry
var must_redraw = false;
var edit_target = '';
var new_contact_id = '';
var colors = ["#d9ceb2", "#99b2b7", "#e6cba5", "#ede3b4", "#8b9e9b", "#bd7578", "#edc951",
    "#ffd573", "#c2a34f", "#fbb829", "#ffab03", "#7ab317", "#a0c55f", "#8ca315",
    "#5191c1", "#6493a7", "#bddb88"]
var curr_img_candidate = null;
var pubs = []
var wants = {}

var restream = false // whether the backend is currently restreaming all posts

// --- menu callbacks

/*
function menu_sync() {
  if (localPeers.length == 0)
    launch_snackbar("no local peer to sync with");
  else {
    for (var i in localPeers) {
      backend("sync " + i);
      launch_snackbar("sync launched");
      break
    }
  }
  closeOverlay();
}
*/

function menu_new_conversation() {
    fill_members();
    prev_scenario = 'chats';
    setScenario("members");
    document.getElementById("div:textarea").style.display = 'none';
    document.getElementById("div:confirm-members").style.display = 'flex';
    document.getElementById("tremolaTitle").style.display = 'none';
    var c = document.getElementById("conversationTitle");
    c.style.display = null;
    c.innerHTML = "<font size=+1><strong>Create New Conversation</strong></font><br>Select up to 7 members";
    document.getElementById('plus').style.display = 'none';
    closeOverlay();
}

function menu_new_contact() {
    document.getElementById('new_contact-overlay').style.display = 'initial';
    document.getElementById('overlay-bg').style.display = 'initial';
    // document.getElementById('chat_name').focus();
    overlayIsActive = true;
}

function menu_new_pub() {
    menu_edit('new_pub_target', "Enter address of trustworthy pub<br><br>Format:<br><tt>net:IP_ADDR:PORT~shs:ID_OF_PUB</tt>", "");
}

function menu_invite() {
    menu_edit('new_invite_target', "Enter invite code<br><br>Format:<br><tt>IP_ADDR:PORT:@ID_OF_PUB.ed25519~INVITE_CODE</tt>", "");
}

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

function menu_edit_convname() {
    menu_edit('convNameTarget', "Edit conversation name:<br>(only you can see this name)", tremola.chats[curr_chat].alias);
}

// function menu_edit_new_contact_alias() {
//   menu_edit('new_contact_alias', "Assign alias to new contact:", "");
// }

function edit_confirmed() {
    closeOverlay()
    console.log("edit confirmed: " + edit_target)
    var val = document.getElementById('edit_text').value;
    if (edit_target == 'convNameTarget') {
        var ch = tremola.chats[curr_chat];
        ch.alias = val;
        persist();
        load_chat_title(ch); // also have to update entry in chats
        menu_redraw();
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
        // TODO reactivate when encrypted chats are implemented
        /*
        tremola.chats[nm] = {
            "alias": "Chat w/ " + val, "posts": {}, "members": recps,
            "touched": Date.now(), "lastRead": 0, "timeline": new Timeline()
        };
        */
        persist();
        backend("add:contact " + new_contact_id + " " + btoa(val))
        menu_redraw();
    } else if (edit_target == 'new_pub_target') {
        console.log("action for new_pub_target")
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
    }
}

function menu_forget_conv() {
    // toggles the forgotten flag of a conversation
    if (curr_chat == recps2nm([myId])) {
        launch_snackbar("cannot be applied to own notes");
        return;
    }
    tremola.chats[curr_chat].forgotten = !tremola.chats[curr_chat].forgotten;
    persist();
    load_chat_list() // refresh list of conversations
    closeOverlay();
    if (curr_scenario == 'posts' /* should always be true */ && tremola.chats[curr_chat].forgotten)
        setScenario('chats');
    else
        load_chat(curr_chat) // refresh currently displayed list of posts
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

function menu_process_msgs() {
    backend('process.msg');
    closeOverlay();
}

function menu_add_pub() {
    // ...
    closeOverlay();
}

function menu_dump() {
    backend('dump:');
    closeOverlay();
}

function menu_take_picture() {
    disabled6676863(); // breakpoint using a non-existing fct,in case
    closeOverlay();
    var draft = unicodeStringToTypedArray(document.getElementById('draft').value); // escapeHTML(
    if (draft.length == 0)
        draft = null;
    else
        draft = atob(draft);
    console.log("getVoice" + document.getElementById('draft').value);
    backend('get:voice ' + atob(draft));
}

function menu_pick_image() {
    closeOverlay();
    backend('get:media');
}

// ---

function new_text_post(s) {
    if (s.length == 0) {
        return;
    }
    var draft = unicodeStringToTypedArray(document.getElementById('draft').value); // escapeHTML(
    var recps;
    if (curr_chat == "ALL") {
        recps = "ALL";
        backend("publ:post [] " + btoa(draft) + " null"); //  + recps)
    } else {
        recps = tremola.chats[curr_chat].members.join(' ');
        backend("priv:post [] " + btoa(draft) + " null " + recps);
    }
    document.getElementById('draft').value = '';
    closeOverlay();
    setTimeout(function () { // let image rendering (fetching size) take place before we scroll
        var c = document.getElementById('core');
        c.scrollTop = c.scrollHeight;
    }, 100);
}

function new_voice_post(voice_b64) {
    var draft = unicodeStringToTypedArray(document.getElementById('draft').value); // escapeHTML(
    if (draft.length == 0)
        draft = "null"
    else
        draft = btoa(draft)
    if (curr_chat == "ALL") {
        // recps = "ALL";
        backend("publ:post [] " + draft + " " + voice_b64); //  + recps)
    } else {
        recps = tremola.chats[curr_chat].members.join(' ');
        backend("priv:post [] " + draft + " " + voice_b64 + " " + recps);
    }
    document.getElementById('draft').value = '';
}

function play_voice(nm, ref) {
    var p = tremola.chats[nm].posts[ref];
    var d = new Date(p["when"]);
    d = d.toDateString() + ' ' + d.toTimeString().substring(0, 5);
    backend("play:voice " + p["voice"] + " " + btoa(fid2display(p["from"])) + " " + btoa(d));
}

function new_image_post() {
    if (curr_img_candidate == null) {
        return;
    }
    var draft = "![](" + curr_img_candidate + ")\n";
    var caption = document.getElementById('image-caption').value;
    if (caption && caption.length > 0)
        draft += caption;
    var recps = tremola.chats[curr_chat].members.join(' ')
    backend("priv:post " + btoa(draft) + " " + recps);
    curr_img_candidate = null;
    closeOverlay();
    setTimeout(function () { // let image rendering (fetching size) take place before we scroll
        var c = document.getElementById('core');
        c.scrollTop = c.scrollHeight;
    }, 100);
}

function load_post_item(p) { // { 'key', 'from', 'when', 'body', 'to' (if group or public)>
    var pl = document.getElementById('lst:posts');
    var is_other = p["from"] != myId;
    var box = "<div class=light style='padding: 3pt; border-radius: 4px; box-shadow: 0 0 5px rgba(0,0,0,0.7); word-break: break-word;'"
    if (p.voice != null)
        box += " onclick='play_voice(\"" + curr_chat + "\", \"" + p.key + "\");'";
    box += ">"
    // console.log("box=", box);
    if (is_other)
        box += "<font size=-1><i>" + fid2display(p["from"]) + "</i></font><br>";
    var txt = ""
    if (p["body"] != null) {
        txt = escapeHTML(p["body"]).replace(/\n/g, "<br>\n");
        var re = /!\[.*?\]\((.*?)\)/g;
        txt = txt.replace(re, " &nbsp;<object type='image/jpeg' style='width: 95%; display: block; margin-left: auto; margin-right: auto; cursor: zoom-in;' data='http://appassets.androidplatform.net/blobs/$1' ondblclick='modal_img(this)'></object>&nbsp; ");
        // txt = txt + " &nbsp;<object type='image/jpeg' width=95% data='http://appassets.androidplatform.net/blobs/25d444486ffb848ed0d4f1d15d9a165934a02403b66310bf5a56757fec170cd2.jpg'></object>&nbsp; (!)";
        // console.log(txt);
    }
    if (p.voice != null)
        box += "<span style='color: red;'>&#x1f50a;</span>&nbsp;&nbsp;"
    box += txt
    var d = new Date(p["when"]);
    d = d.toDateString() + ' ' + d.toTimeString().substring(0, 5);
    box += "<div align=right style='font-size: x-small;'><i>";
    box += d + "</i></div></div>";
    var row;
    if (is_other) {
        var c = tremola.contacts[p.from]
        row = "<td style='vertical-align: top;'><button class=contact_picture style='margin-right: 0.5em; margin-left: 0.25em; background: " + c.color + "; width: 2em; height: 2em;'>" + c.initial + "</button>"
        // row  = "<td style='vertical-align: top; color: var(--red); font-weight: 900;'>&gt;"
        row += "<td colspan=2 style='padding-bottom: 10px;'>" + box + "<td colspan=2>";
    } else {
        row = "<td colspan=2><td colspan=2 style='padding-bottom: 10px;'>" + box;
        row += "<td style='vertical-align: top; color: var(--red); font-weight: 900;'>&lt;"
    }
    pl.insertRow(pl.rows.length).innerHTML = row;
}

function load_chat(nm) {
    var ch, pl, e;
    ch = tremola.chats[nm]
    if (ch.timeline == null)
        ch["timeline"] = new Timeline();
    pl = document.getElementById("lst:posts");
    while (pl.rows.length) {
        pl.deleteRow(0);
    }
    pl.insertRow(0).innerHTML = "<tr><td>&nbsp;<td>&nbsp;<td>&nbsp;<td>&nbsp;<td>&nbsp;</tr>";
    curr_chat = nm;
    var lop = []; // list of posts
    for (var p in ch.posts) lop.push(p)
    lop.sort(function (a, b) {
        return ch.posts[a].when - ch.posts[b].when
    })
    lop.forEach(function (p) {
        load_post_item(ch.posts[p])
    })
    load_chat_title(ch);
    setScenario("posts");
    document.getElementById("tremolaTitle").style.display = 'none';
    // update unread badge:
    ch["lastRead"] = Date.now();
    persist();
    document.getElementById(nm + '-badge').style.display = 'none' // is this necessary?
    setTimeout(function () { // let image rendering (fetching size) take place before we scroll
        var c = document.getElementById('core');
        c.scrollTop = c.scrollHeight;
    }, 100);
    /*
    // scroll to bottom:
    var c = document.getElementById('core');
    c.scrollTop = c.scrollHeight;
    document.getElementById('lst:posts').scrollIntoView(false)
    // console.log("did scroll down, but did it do it?")
    */
}

function load_chat_title(ch) {
    var c = document.getElementById("conversationTitle"), bg, box;
    c.style.display = null;
    c.setAttribute('classList', ch.forgotten ? 'gray' : '') // old JS (SDK 23)
    box = "<div style='white-space: nowrap;'><div style='text-overflow: ellipsis; overflow: hidden;'><font size=+1><strong>" + escapeHTML(ch.alias) + "</strong></font></div>";
    box += "<div style='color: black; text-overflow: ellipsis; overflow: hidden;'>" + escapeHTML(recps2display(ch.members)) + "</div></div>";
    c.innerHTML = box;
}

function load_chat_list() {
    var meOnly = recps2nm([myId])
    // console.log('meOnly', meOnly)
    document.getElementById('lst:chats').innerHTML = '';
    // load_chat_item(meOnly) TODO reactivate when encrypted chats are implemented
    var lop = [];
    for (var p in tremola.chats) {
        if (p != meOnly && !tremola.chats[p]['forgotten'])
            lop.push(p)
    }
    lop.sort(function (a, b) {
        return tremola.chats[b]["touched"] - tremola.chats[a]["touched"]
    })
    lop.forEach(function (p) {
        load_chat_item(p)
    })
    // forgotten chats: unsorted
    if (!tremola.settings.hide_forgotten_conv)
        for (var p in tremola.chats)
            if (p != meOnly && tremola.chats[p]['forgotten'])
                load_chat_item(p)
}

function load_chat_item(nm) { // appends a button for conversation with name nm to the conv list
    var cl, mem, item, bg, row, badge, badgeId, cnt;
    cl = document.getElementById('lst:chats');
    // console.log(nm)
    if (nm == "ALL")
        mem = "ALL";
    else
        mem = recps2display(tremola.chats[nm].members);
    item = document.createElement('div');
    // item.style = "padding: 0px 5px 10px 5px; margin: 3px 3px 6px 3px;";
    item.setAttribute('class', 'chat_item_div'); // old JS (SDK 23)
    if (tremola.chats[nm].forgotten) bg = ' gray'; else bg = ' light';
    row = "<button class='chat_item_button w100" + bg + "' onclick='load_chat(\"" + nm + "\");' style='overflow: hidden; position: relative;'>";
    row += "<div style='white-space: nowrap;'><div style='text-overflow: ellipsis; overflow: hidden;'>" + tremola.chats[nm].alias + "</div>";
    row += "<div style='text-overflow: clip; overflow: ellipsis;'><font size=-2>" + escapeHTML(mem) + "</font></div></div>";
    badgeId = nm + "-badge"
    badge = "<div id='" + badgeId + "' style='display: none; position: absolute; right: 0.5em; bottom: 0.9em; text-align: center; border-radius: 1em; height: 2em; width: 2em; background: var(--red); color: white; font-size: small; line-height:2em;'>&gt;9</div>";
    row += badge + "</button>";
    row += ""
    item.innerHTML = row;
    cl.appendChild(item);
    set_chats_badge(nm)
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

function show_contact_details(id) {
    if (id == myId) {
        document.getElementById('old_contact_alias_hdr').innerHTML = "Alias: (own name, visible to others)"
    } else {
        document.getElementById('old_contact_alias_hdr').innerHTML = "Alias: (only you can see this alias)"
    }
    var c = tremola.contacts[id];
    new_contact_id = id;
    document.getElementById('old_contact_alias').value = c['alias'];
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

function new_conversation() {
    // { "alias":"local notes (for my eyes only)", "posts":{}, "members":[myId], "touched": millis }
    var recps = []
    for (var m in tremola.contacts) {
        if (document.getElementById(m).checked)
            recps.push(m);
    }
    if (recps.indexOf(myId) < 0)
        recps.push(myId);
    if (recps.length > 7) {
        launch_snackbar("Too many recipients");
        return;
    }
    var cid = recps2nm(recps)
    if (cid in tremola.chats) {
        if (tremola.chats[cid].forgotten) {
            tremola.chats[cid].forgotten = false;
            load_chat_list(); // refresh
        } else
            launch_snackbar("Conversation already exists");
        return;
    }
    var nm = recps2nm(recps);
    if (!(nm in tremola.chats)) {
        tremola.chats[nm] = {
            "alias": "Unnamed conversation", "posts": {},
            "members": recps, "touched": Date.now(), "timeline": new Timeline()
        };
        persist();
    } else
        tremola.chats[nm]["touched"] = Date.now()
    load_chat_list();
    setScenario("chats")
    curr_chat = nm
    menu_edit_convname()
}

function load_peer_list() {
    var i, lst = '', row;
    for (i in localPeers) {
        var x = localPeers[i], color, row, nm, tmp;
        if (x[1]) color = ' background: var(--lightGreen);'; else color = '';
        tmp = i.split('~');
        nm = '@' + tmp[1].split(':')[1] + '.ed25519'
        if (nm in tremola.contacts)
            nm = ' / ' + tremola.contacts[nm].alias
        else
            nm = ''
        row = "<button class='flat buttontext' style='border-radius: 25px; width: 50px; height: 50px; margin-right: 0.75em;" + color + "'><img src=img/signal.svg style='width: 50px; height: 50px; margin-left: -3px; margin-top: -3px; padding: 0px;'></button>";
        row += "<button class='chat_item_button light' style='overflow: hidden; width: calc(100% - 4em);' onclick='show_peer_details(\"" + i + "\");'>";
        row += "<div style='white-space: nowrap;'><div style='text-overflow: ellipsis; overflow: hidden;'>" + tmp[0].substring(4) + nm + "</div>";
        row += "<div style='text-overflow: clip; overflow: ellipsis;'><font size=-2>" + tmp[1].substring(4) + "</font></div></div></button>";
        lst += '<div style="padding: 0px 5px 10px 5px;">' + row + '</div>';
        // console.log(row)
    }
    document.getElementById('the:connex').innerHTML = lst;
}

function show_peer_details(id) {
    new_contact_id = "@" + id.split('~')[1].substring(4) + ".ed25519";
    // if (new_contact_id in tremola.constacts)
    //  return;
    menu_edit("trust_wifi_peer", "Trust and Autoconnect<br>&nbsp;<br><strong>" + new_contact_id + "</strong><br>&nbsp;<br>Should this WiFi peer be trusted (and autoconnected to)? Also enter an alias for the peer - only you will see this alias", "?")
}

function getUnreadCnt(nm) {
    var c = tremola.chats[nm], cnt = 0;
    for (var p in c.posts) {
        if (c.posts[p].when > c.lastRead)
            cnt++;
    }
    return cnt;
}

function set_chats_badge(nm) {
    var e = document.getElementById(nm + '-badge'), cnt;
    cnt = getUnreadCnt(nm)
    if (cnt == 0) {
        e.style.display = 'none';
        return
    }
    e.style.display = null;
    if (cnt > 9) cnt = ">9"; else cnt = "" + cnt;
    e.innerHTML = cnt
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
        "settings": get_default_settings(),
        "board": {}
    }
    var n = recps2nm([myId])

    //TODO reactivate when encrypted chats are implemented
    /*
    tremola.chats[n] = {
        "alias": "local notes (for my eyes only)", "posts": {}, "forgotten": false,
        "members": [myId], "touched": Date.now(), "lastRead": 0,
        "timeline": new Timeline()
    };
    */

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

/*
function b2f_local_peer(p, status) { // wireless peer: online, offline, connected, disconnected
    console.log("local peer", p, status);
    if (!(p in localPeers))
        localPeers[p] = [false, false]
    if (status == 'online') localPeers[p][0] = true
    if (status == 'offline') localPeers[p][0] = false
    if (status == 'connected') localPeers[p][1] = true
    if (status == 'disconnected') localPeers[p][1] = false
    if (!localPeers[p][0] && !localPeers[p][1])
        delete localPeers[p]
    load_peer_list()
}
*/


// type: 'udp' or 'ble'
// identifier: unique identifier of the peer
// displayname
// status: 'connected', 'disconnected'

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

/*
var want = {} // all received want vectors, id: [[want vector], timestamp], want vectors older than 90 seconds are discarded
var max_want = [] // current max vector
var old_curr = [] // own want vector at the time when the maximum want vector was last updated

function b2f_want_update(identifier, wantVector) {

    console.log("b2f received want:", wantVector, "from: ", identifier)

    // remove old want vectors
    var deleted = false;
    for (var id in want) {
        var ts = want[id][1]
        if(Date.now() - ts > 90000) {
            console.log("removed want of", id)
            delete want[id]
            deleted = true
        }

    }

    // if the want vector didn't change, no further updates are required
    if(identifier in want) {
        if( equalArrays(want[identifier][0], wantVector)) {
            console.log("update only")
            want[identifier][1] = Date.now()
            if(!deleted)  //if a want vector was previously removed, the max_want needs to be recalculated otherwise it is just an update without an effect
                return
        }
    }

    want[identifier] = [wantVector, Date.now()]

    // calculate new max want vector
    var all_vectors = Object.values(want).map(val => val[0])
    var new_max_want = all_vectors.reduce((accumulator, curr) => accumulator.len >= curr.len ? accumulator : curr) //return want vector with most entries

    for (var vec of all_vectors) {
        for(var i in vec) {
            if (vec[i] > new_max_want[i])
                new_max_want[i] = vec[i]
        }
    }

    // update
    if (!equalArrays(max_want,new_max_want)) {
        old_curr = want['me'][0]
        max_want = new_max_want
        console.log("new max")
    }

    refresh_connection_progressbar()

    console.log("max:", max_want)
}
*/

function b2f_local_peer_remaining_updates(identifier, remaining) {
    //TODO
}

function b2f_update_progress(min_entries, old_min_entries, old_want_entries, curr_want_entries, max_entries) {
    refresh_connection_progressbar(min_entries, old_min_entries, old_want_entries, curr_want_entries, max_entries)
}

function b2f_local_peer(type, identifier, displayname, status) {
    console.log("incoming displayname:", displayname)
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


    console.log("local_peer:", type, identifier, displayname, status)

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

    console.log("b2f inorder event:", JSON.stringify(e.public))

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
        case "KAN":
            console.log("New kanban event")
            kanban_new_event(e)
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
    console.log('pub', JSON.stringify(e.public))
    // console.log('cfd', JSON.stringify(e.confid))
    console.log("New Frontend Event: " + JSON.stringify(e.header))

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
            console.log("new post 0 ", tremola)
            var conv_name = "ALL";
            if (!(conv_name in tremola.chats)) { // create new conversation if needed
                console.log("xx")
                tremola.chats[conv_name] = {
                    "alias": "Public channel X", "posts": {},
                    "members": ["ALL"], "touched": Date.now(), "lastRead": 0,
                    "timeline": new Timeline()
                };
                load_chat_list()
            }
            console.log("new post 1")
            var ch = tremola.chats[conv_name];
            if (ch.timeline == null)
                ch["timeline"] = new Timeline();
            console.log("new post 1 ", ch)
            if (!(e.header.ref in ch.posts)) { // new post
                var a = e.public;
                // var d = new Date(e.header.tst);
                // d = d.toDateString() + ' ' + d.toTimeString().substring(0,5);
                // var txt = null;
                // if (a[1] != null)
                //   txt = a[1];
                var p = {
                    "key": e.header.ref, "from": e.header.fid, "body": a[1],
                    "voice": a[2], "when": a[3] * 1000
                };
                console.log("new post 2 ", p)
                console.log("time: ", a[3])
                ch["posts"][e.header.ref] = p;
                if (ch["touched"] < e.header.tst)
                    ch["touched"] = e.header.tst
                if (curr_scenario == "posts" && curr_chat == conv_name) {
                    load_chat(conv_name); // reload all messages (not very efficient ...)
                    ch["lastRead"] = Date.now();
                }
                set_chats_badge(conv_name)
            } else {
                console.log("known already?")
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
                load_board_list()

                // update names in connected devices menu
                for (var l in localPeers) {
                    if (localPeers[l].alias == old_alias) {

                        localPeers[l].alias = contact.alias
                        refresh_connection_entry(l)
                    }
                }
            }

        }
        persist();
        must_redraw = true;
    }
}

function b2f_new_contact(fid) {
    if ((fid in tremola.contacts)) // do not overwrite existing entry
        return
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

function b2f_initialize(id) {
    myId = id
    if (window.localStorage.tremola) {
        tremola = JSON.parse(window.localStorage.getItem('tremola'));

        if (tremola != null && id != tremola.id) // check for clash of IDs, erase old state if new
            tremola = null;
    } else
        tremola = null;
    if (tremola == null) {
        resetTremola();
        console.log("reset tremola")
    }
    if (typeof Android == 'undefined')
        console.log("loaded ", JSON.stringify(tremola))
    if (!('settings' in tremola))
        tremola.settings = {}
    var nm, ref;
    for (nm in tremola.settings)
        setSetting(nm, tremola.settings[nm])
    load_chat_list()
    load_contact_list()
    load_board_list()

    closeOverlay();
    setScenario('chats');
    // load_chat("ALL");
}

// --- eof

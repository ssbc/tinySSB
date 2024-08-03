// prod/chat.js

"use strict";

var curr_chat;
var curr_img_candidate = null;
// var restream = false // whether the backend is currently restreaming all posts

// --- menu callbacks

function menu_new_conversation() {
    fill_members();
    prev_scenario = 'chats';
    setScenario("members");
    document.getElementById("div:textarea").style.display = 'none';
    document.getElementById("div:confirm-members").style.display = 'flex';
    document.getElementById("tremolaTitle").style.display = 'none';
    var c = document.getElementById("conversationTitle");
    c.style.display = null;
    c.innerHTML = "<font size=+1><strong>Create Private Channel</strong></font><br>Select up to 7 members";
    document.getElementById('plus').style.display = 'none';
    closeOverlay();
}

function menu_edit_convname() {
    menu_edit('convNameTarget', "Edit conversation name:<br>(only you can see this name)", tremola.chats[curr_chat].alias);
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

/* ??
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
*/

// --- workflow entry points

function new_text_post(s) {
    if (s.length == 0) {
        return;
    }
    var draft = unicodeStringToTypedArray(document.getElementById('draft').value); // escapeHTML(
    var ch = tremola.chats[curr_chat]
    if (!(ch.timeline instanceof Timeline)) {
        ch.timeline = Timeline.fromJSON(ch.timeline)
    }
    let tips = JSON.stringify(ch.timeline.get_tips())
    // console.log(`tips: ${tips}`)
    if (curr_chat == "ALL") {
        var cmd = `publ:post ${tips} ` + btoa(draft) + " null"; // + recps
        // console.log(cmd)
        backend(cmd);
    } else {
        var recps = tremola.chats[curr_chat].members.join(' ');
        var cmd = `priv:post ${tips} ` + btoa(draft) + " null " + recps;
        backend(cmd);
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
    var ch = tremola.chats[curr_chat]
    if (!(ch.timeline instanceof Timeline)) {
        ch.timeline = Timeline.fromJSON(ch.timeline)
    }
    let tips = JSON.stringify(ch.timeline.get_tips())
    // console.log(`tips: ${tips}`)

    if (curr_chat == "ALL") {
        var cmd = `publ:post ${tips} ` + draft + " " + voice_b64;
        // console.log(cmd)
        backend(cmd);
    } else {
        var recps = tremola.chats[curr_chat].members.join(' ');
        var cmd = `priv:post ${tips} ` + draft + " " + voice_b64 + " " + recps;
        backend(cmd);
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
        // Sketch app
        if (txt.startsWith("data:image/png;base64")) { // check if the string is a data url
                let compressedBase64 = txt.split(',')[1];
                // We Convert the compressed data from a base64 string to a Uint8Array
                let compressedData = atob(compressedBase64)
                  .split('')
                  .map(function (char) {
                    return char.charCodeAt(0);
                  });
                let uint8Array = new Uint8Array(compressedData);

                // We to decompress the Uint8Array
                let decompressedData = pako.inflate(uint8Array);
                // We Convert the decompressed data back to a base64 string
                let decompressedBase64 = btoa(String.fromCharCode.apply(null, decompressedData));
                // We Create a new data URL with the decompressed data
                let decompressedDataURL = 'data:image/png;base64,' + decompressedBase64;
                //display the data url as an image element
                box += "<img src='" + decompressedDataURL + "' alt='Drawing' style='width: 50vw;'>";
                txt = "";
        } else if (txt.startsWith("data:image/svg+bipf;base64")) {
            let b64 = txt.split(',')[1];
            var binStr = atob(b64)
            var buf = new ArrayBuffer(binStr.length);
            var ui8 = new Uint8Array(buf);
            for (var i = 0; i < binStr.length; i++)
               ui8[i] = binStr.charCodeAt(i);
            var src;
            try {
               let img = bipf_decode(buf, 0);
               // console.log('got svg', JSON.stringify(img));
               // img[0] -- version of this svg encoding, currently 1, ignored
               if (Number.isInteger(img[0]))
                 img = img.slice(1)
               var svg = `<svg version="1.1" width="${img[0][1]}" height="${img[0][2]}"`
               svg += ' style="background-color:white" xmlns="http://www.w3.org/2000/svg">'
               svg += '<g stroke-linecap="round" fill="none">'
               let coltab = ['#ffffff', '#000000', '#ee0000', '#00aa00', '#4040ff', '#ffee00', '#ff00ff'];
               // let coltab = ['#ffffff', '#000000', '#ff0000', '#00ff00', '#0000ff', '#ffff00', '#ff00ff'];
               let widtab = [2, 5, 10, 30];
               var colNdx = 0;
               var widNdx = 0;
               img.slice(1).forEach((e) => {
                 if (e[0] == 'c') {
                   colNdx = e[1];
                 } else if (e[0] == 'w') {
                   widNdx = e[1];
                 } else if (e[0] == 'p') {
                   svg += `<path d="M ${e[1]} ${e[2]} l`
                   e.slice(3).forEach((i) => {
                     svg += ` ${i}`
                   })
                   svg += `" stroke="${coltab[colNdx+1]}" stroke-width="${widtab[widNdx]}"/>`
                 }
               })
               svg += '</g></svg>';
               // console.log('svg:', svg)
               src = `data:image/svg+xml;base64,${btoa(svg)}`;
            } catch (error) {
               console.error(error);
               src = "data:null";
            }
            box += `<img src="${src}"`;
            box += 'alt="Drawing" width="100%">';
            txt = "";
        }

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
    for (var n of ch.timeline.linear) {
        load_post_item(ch.posts[n.name])
    }
    /*
    var lop = []; // list of posts
    for (var p in ch.posts) lop.push(p)
    lop.sort(function (a, b) {
        return ch.posts[a].when - ch.posts[b].when
    })
    lop.forEach(function (p) {
        load_post_item(ch.posts[p])
    })
    */
    load_chat_title(ch);
    setScenario("posts");
    document.getElementById("tremolaTitle").style.display = 'none';
    // update unread badge:
    ch["lastRead"] = Date.now();
    persist();
    document.getElementById(nm + '-badge').style.display = 'none' // is this necessary?
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
    var mem = "[ALL]"
    if (ch.members.length > 1 || ch.members[0] != "ALL")
       mem = "🔒 " + recps2display(ch.members)
    box += "<div style='color: black; text-overflow: ellipsis; overflow: hidden;'>" + escapeHTML(mem) + "</div></div>";
    c.innerHTML = box;
}

function load_chat_list() {
    document.getElementById('lst:chats').innerHTML = '';
    load_chat_item("ALL")
    var meOnly = recps2nm([myId])
    if (!(meOnly in tremola.chats)) {
        tremola.chats[meOnly] = {
            "alias": "--- local notes (for my eyes only)", "posts": {}, "forgotten": false,
            "members": [myId], "touched": Date.now(), "lastRead": 0,
            "timeline": new Timeline()
        };
    }
    load_chat_item(meOnly)
    var lop = [];
    for (var p in tremola.chats) {
        if (p != "ALL" && p != meOnly && !tremola.chats[p]['forgotten'])
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
        mem = "🔒 " + recps2display(tremola.chats[nm].members);
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
            "alias": "Private", "posts": {},
            "members": recps, "touched": Date.now(),
            "timeline": new Timeline()
        };
        persist();
    } else
        tremola.chats[nm]["touched"] = Date.now()
    load_chat_list();
    setScenario("chats")
    curr_chat = nm
    menu_edit_convname()
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

// --- eof

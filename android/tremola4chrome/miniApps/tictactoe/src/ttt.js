/* ttt.js */

"use strict";


/*

  TTT command:
    NEW peerID    create new game
    ACC refG       accept referenced game
    END refG       close referenced game (can also be used to decline)
    MOV refG int
    GUP refG       give up
    VIO refG refM       report a violation

    state diagram:

           0=inviting ---rcvA.
                              \  2=open --> 3=closed
                              /
   rcvN -> 1=invited --sendA-'

   ttt data structure:
     closed = [ref_to_old_games]
     active = {ref : {
       peer: FID,
       state: 0-3
       close_reason:
       board:  []9x -1=other, 0=empty, 1=me
       mov_cnt: even=my turn, odd=other's turn
     } }
*/

// Ensure miniApps is attached to the topmost window (global scope)
const globalWindow = window.top || window; // Ensures cross-context access

if (!globalWindow.miniApps) {
    globalWindow.miniApps = {}; 
}

globalWindow.miniApps["tictactoe"] = {
    handleRequest: function(command, args) {
        console.log("Tic-Tac-Toe handling request:", command);
        switch (command) {
          case "onBackPressed":
              console.log("TTTScenario: ", TTTScenario);
              if (TTTScenario == 'tictactoe-board') {
                  setTTTScenario('tictactoe-list');
              } else if (TTTScenario == 'tictactoe-list') {
                  quitApp();
              }
              break;
          case "plus_button":
              console.log("Tic-Tac-Toe plus_button");
              ttt_new_game();
              break;
          case "members_confirmed":
              console.log("Tic-Tac-Toe members_confirmed");
              ttt_new_game_confirmed();
              break;
          case "edit_confirmed":
              console.log("Tic-Tac-Toe edit_confirmed");
              kanban_edit_confirmed();
              break;
          case "b2f_initialize":
              console.log("Tic-Tac-Toe b2f_initialize");
              ttt_load_list();
              break;
          case "b2f_new_event":
              console.log("Tic-Tac-Toe b2f_new_event");
              ttt_load_list();
              break;
          case "incoming_notification": //incoming_notification
              console.log("Tic-Tac-Toe incoming_notification:", JSON.stringify(args, null, 2));
              decodeRequest(args.args);
              break;
        }
        return "Response from Tic-Tac-Toe";
    }
};

// ✅ Debugging Log: Ensure it's available globally
console.log("✅ Global MiniApps Object AFTER assignment:", globalWindow.miniApps);


console.log("Tic-Tac-Toe loaded");

var ttt_iwon = '(I won)';
var ttt_ilost = '(I lost)';
var ttt_ogaveup = '(peer gave up)';
var ttt_igaveup = '(I gave up)';

function decodeRequest(request) {
    var args = request; // request is a stringified JSON array
    console.log("Type: ", args[0]?.type); // Add optional chaining to avoid errors
    ttt_on_rx(args, 0);
}

function ttt_load_list() {
  closeOverlay();
  let lst = document.getElementById('div:tictactoe-list');
  lst.innerHTML = '';
  if (typeof tremola.tictactoe == "undefined")
    tremola.tictactoe = { 'active': {}, 'closed': {} }
  for (var nm in tremola.tictactoe.active) {
    if (nm == "undefined")
      continue
    let g = tremola.tictactoe.active[nm];
    let item = document.createElement('div');
    // item.setAttribute('class', 'chat_item_div'); // old JS (SDK 23)
    // item.setAttribute('onclick', `ttt_load_board(${nm})`)
    var row = "<button class='ttt_list_button' onclick='ttt_load_board(\"" + nm + "\");' style='overflow: hidden; width: 70%; background-color: #ebf4fa;'>";
    row += "<div style='white-space: nowrap;'><div style='text-overflow: ellipsis; overflow: hidden;'>"
    row += "TTT with " + fid2display(g.peer) + "<br>";
    row += g.state;
    if (g.state == 'closed')
      row += " " + g.close_reason;
    else if (g.state == 'invited')
      row += " (click here to accept)"
    row += "</div></button></div>"
    var btxt;
    if (g.state == 'invited')     btxt = 'decline';
    else if (g.state == 'closed') btxt = 'delete';
    else                          btxt = 'end';
    row += `<button class='ttt_list_button' style='width: 20%; text-align: center;' onclick='ttt_list_callback("${nm}","${btxt}")'>${btxt}</button>`;
    // console.log("list building " + row)
    item.innerHTML = row;
    lst.appendChild(item);
  }
}

function ttt_load_board(nm) {
  console.log("load board " + nm)
  let g = tremola.tictactoe.active[nm];
  if (g.state == 'inviting')
    return;
  if (g.state == 'invited') {
    ttt_list_callback(nm,'accept');
    return;
  }
  let t = document.getElementById('ttt_title');
  if (g.state == 'open') {
      let m = (g.cnt % 2) ? "my turn ..." : "... not my turn" ;
      t.innerHTML = `<font size=+2><strong>${m}</strong></font>`;
  } else { // must be in close state
    var m;
    if (g.close_reason == ttt_iwon)
      m = 'I win ! 😀';
    else if (g.close_reason == ttt_ilost)
      m = 'I lose ! 🙁';
    else
      m = g.close_reason.slice(1,g.close_reason.length-1);
    t.innerHTML = `<font size=+2 color=red><strong>${m}</strong></font>`
  }
  let f = document.getElementById('ttt_footer')
  f.style.display = g.state == 'closed' ? 'none' : null;

  let board = g.board
  let tab = document.getElementById('ttt_table');
  tab.innerHTML = '';
  for (var i = 0; i < 9; i++) {
    let item = document.createElement('div');
    item.setAttribute('class', 'ttt_cell');
    item.setAttribute('id', 'ttt' + i);
    item.setAttribute('onclick', 'ttt_cellclick(id)');
    if (board[i] == -1)
      item.innerHTML = "<img src='" + miniAppDirectory + "assets/cross.svg' width='100%'>";
    else if (board[i] == 1)
      item.innerHTML = "<img src='" + miniAppDirectory + "assets/dot.svg' width='100%'>";
    tab.appendChild(item);
  }
  tremola.tictactoe.current = nm;
  setTTTScenario('tictactoe-board');
}

function ttt_new_game() {
    launchContactsMenu('Tic Tac Toe', 'Pick a friend to play with');
    readLogEntries(10);
}

function ttt_new_game_confirmed() {
    console.log("ttt_new_game_confirmed");
    for (var m in tremola.contacts) {
        if (m != myId && document.getElementById(m).checked) {
            console.log("ttt invite " + m)
            //generate random number to use as game reference
            let randomNum = Math.floor(Math.random() * 1000000);
            // json structure with type, from, and to
            let json = { type: 'N', from: myId, to: m, nm: myId + "" + m + "" + randomNum };
            console.log("ttt invite " + JSON.stringify(json))
            writeLogEntry(JSON.stringify(json));
            // console.log("ttt invited: " + m)
            break;
        }
    }
    if (curr_scenario == 'members')
        setTTTScenario('tictactoe-list')
}

function ttt_list_callback(nm,action) {
   // console.log("ttt_list_callback " + nm + " " + action)
   let g = tremola.tictactoe.active[nm]
   if (action == 'accept') {
       let json = { type: 'A', nm: nm };
       writeLogEntry(JSON.stringify(json));
   }
   else if (action == 'end' || action == 'decline') {
       let json = { type: 'E', nm: nm, from: myId };
       writeLogEntry(JSON.stringify(json));
   }
   else if (action == 'delete') {
     delete tremola.tictactoe.active[nm];
     tremola.tictactoe.closed[nm] = g.peer; // remember peer
     persist();
   }
   ttt_load_list();
}

function ttt_cellclick(id) {
  // console.log("clicked " + id + ' ' + id[3]);
  let nm = tremola.tictactoe.current
  let g = tremola.tictactoe.active[nm]
  console.log("clicked " + id + ' ' + id[3] + ' ' + g.state + ' ' + g.cnt);
  if (g.state != 'open' || (g.cnt % 2) != 1)
    return;
  let i = parseInt(id[3], 10);
  if (g.board[i] == 0) {
     let json = { type: 'M', nm: nm, i: i, from: myId };
     writeLogEntry(JSON.stringify(json));

  }
}

function ttt_on_rx(args, index=0) {
  console.log("ttt_on_rx args " + JSON.stringify(args));
  if (typeof tremola.tictactoe == "undefined")
    tremola.tictactoe = { 'active': {}, 'closed': {} }
  let ta = tremola.tictactoe.active;
  if (args[index].type == 'N') {
    if (args[index].from != myId && args[index].to != myId)
        return;
    ta[args[index].nm] = {
      'peer': args[index].to == myId ? args[index].from : args[index].to,
      'state': args[index].to == myId ? 'invited' : 'inviting',
            //'peer': args[1] == myId ? from : args[1],
            //'state': args[1] == myId ? 'invited' : 'inviting',
      'close_reason': '',
      'board': [0,0,0,0,0,0,0,0,0], // -1=other, 0=empty, 1=me
      'cnt': 0,
    }
    persist();
    if (TTTScenario == 'tictactoe-list')
      ttt_load_list();
    return;
  }
  let g = ta[args[index].nm];
  if (args[index].type == 'A') { // accepts
      if (g.state == 'inviting' || g.state == 'invited') { //
        if (g.state == 'invited')
            g.cnt = 1;
        g.state = 'open';
        persist();
        if (TTTScenario == 'tictactoe-list')
          ttt_load_list();
      } // else discard
      return;
  }
  if (args[index].type == 'E') { // end
      g.state = 'closed';
      g.close_reason = 'by ' + (args[index].from == myId ? 'myself' : 'peer');
  } if (args[index].type == 'M') { // move
      // TODO: check that we are open, raise violation error otherwise
      // TODO: check that cell is empty, raise violation error otherwise
      // TODO: check that the turn is made by the right party, raise violation error otherwise
      console.log("move " + args[index].i + " " + args[index].from);
      let ndx = parseInt(args[index].i,10);
      g.board[ndx] = args[index].from == myId ? -1 : 1;
      g.cnt++;
      if (ttt_winning(g.board)) {
        g.state = 'closed'
        g.close_reason = args[index].from == myId ? ttt_iwon : ttt_ilost;
      }
  } else if (args[index].type == 'G') { // give up
      g.state = 'closed';
      g.close_reason = args[index].from == myId ? ttt_igaveup : ttt_ogaveup;
  } else if (args[index].type == 'V') { // violation
      g.state = 'closed';
      g.close_reason = '(protocol violation)';
  }

  persist();
  if (TTTScenario == 'tictactoe-list')
    ttt_load_list();
  if (TTTScenario == 'tictactoe-board' && args[index].type != 'A' && tremola.tictactoe.current == args[index].nm)
    ttt_load_board(args[index].nm);
}

function ttt_give_up() {
    let nm = tremola.tictactoe.current;
    let json = { type: 'G', nm: nm, from: myId };
    writeLogEntry(JSON.stringify(json));
    setTTTScenario('tictactoe-list');
}

function ttt_walk_away() {
    let nm = tremola.tictactoe.current;
    let json = { type: 'E', nm: nm, from: myId };
    writeLogEntry(JSON.stringify(json));
    setTTTScenario('tictactoe-list');
}

function ttt_winning(board) {
    var c, w;
    for (var i=0; i < 3; i++ ) {
        // check rows
        c = board[3*i];
        if (c != 0) {
            w = true;
            for (var j=1; j < 3; j++) {
                if (c != board[3*i + j]) {
                    w = false;
                    break;
                }
            }
            if (w) return true;
        }
        // check columns
        c = board[i];
        if (c != 0) {
            w = true;
            for (var j=1; j < 3; j++) {
                if (c != board[3*j + i]) {
                    w = false;
                    break;
                }
            }
            if (w) return true;
        }
    }
    // check diagonals
    c = board[3 + 1]; // center
    if (c != 0) {
        if (board[0] == c && board[8] == c)
            return true;
        if (board[2] == c && board[6] == c)
            return true;
    }
    return false;
}

// eof
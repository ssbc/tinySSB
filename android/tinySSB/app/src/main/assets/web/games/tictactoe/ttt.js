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

var ttt_iwon = '(I won)';
var ttt_ilost = '(I lost)';
var ttt_ogaveup = '(peer gave up)';
var ttt_igaveup = '(I gave up)';

function ttt_load_list() {
  let lst = document.getElementById('div:tictactoe_list');
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
  // console.log("load board " + nm)
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
      m = 'I loose ! 🙁';
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
        item.innerHTML = "<img src='games/tictactoe/cross.svg' width='100%'>";
    else if (board[i] == 1)
        item.innerHTML = "<img src='games/tictactoe/dot.svg' width='100%'>";
    tab.appendChild(item);
  }
  tremola.tictactoe.current = nm;
  setScenario('tictactoe-board')
}

function ttt_new_game() {
    closeOverlay()
    fill_members(true);
    prev_scenario = 'tictactoe-list';
    setScenario("members");

    document.getElementById("div:textarea").style.display = 'none';
    document.getElementById("div:confirm-members").style.display = 'flex';
    document.getElementById("tremolaTitle").style.display = 'none';
    var c = document.getElementById("conversationTitle");
    c.style.display = null;
    c.innerHTML = "<font size=+1><strong>Launch TicTacToe</strong></font><br>Select peer to invite";
    document.getElementById('plus').style.display = 'none';
}

function ttt_new_game_confirmed() {
    for (var m in tremola.contacts) {
        if (m != myId && document.getElementById(m).checked) {
            backend("tictactoe N " + m)
            // console.log("ttt invited: " + m)
            break;
        }
    }
    if (curr_scenario == 'members')
        setScenario('tictactoe-list')
}

function ttt_list_callback(nm,action) {
   // console.log("ttt_list_callback " + nm + " " + action)
   let g = tremola.tictactoe.active[nm]
   if (action == 'accept')
     backend('tictactoe A ' + nm);
   else if (action == 'end' || action == 'decline')
     backend('tictactoe E ' + nm);
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
  if (g.state != 'open' || (g.cnt % 2) != 1)
    return;
  let i = parseInt(id[3], 10);
  if (g.board[i] == 0)
      backend(`tictactoe M ${nm} ${i}`)
}

function ttt_on_rx(ref, from, args) {
  // console.log("ttt_on_rx args " + JSON.stringify(args) + ` from=${from} ref=${ref}`);
  if (typeof tremola.tictactoe == "undefined")
    tremola.tictactoe = { 'active': {}, 'closed': {} }
  let ta = tremola.tictactoe.active;
  if (args[0] == 'N') {
    if (from != myId && args[1] != myId)
        return;
    ta[ref] = {
      'peer': args[1] == myId ? from : args[1],
      'state': args[1] == myId ? 'invited' : 'inviting',
      'close_reason': '',
      'board': [0,0,0,0,0,0,0,0,0], // -1=other, 0=empty, 1=me
      'cnt': 0,
    }
    persist();
    if (curr_scenario == 'tictactoe-list')
      ttt_load_list();
    return;
  }
  let g = ta[args[1]];
  if (args[0] == 'A') { // accepts
      if (g.state == 'inviting' || g.state == 'invited') { //
        if (g.state == 'invited')
            g.cnt = 1;
        g.state = 'open';
        persist();
        if (curr_scenario == 'tictactoe-list')
          ttt_load_list();
      } // else discard
      return;
  }
  if (args[0] == 'E') { // end
      g.state = 'closed';
      g.close_reason = 'by ' + (from == myId ? 'myself' : 'peer');
  } if (args[0] == 'M') { // move
      // TODO: check that we are open, raise violation error otherwise
      // TODO: check that cell is empty, raise violation error otherwise
      // TODO: check that the turn is made by the right party, raise violation error otherwise
      let ndx = parseInt(args[2],10);
      g.board[ndx] = from == myId ? -1 : 1;
      g.cnt++;
      if (ttt_winning(g.board)) {
        g.state = 'closed'
        g.close_reason = from == myId ? ttt_iwon : ttt_ilost;
      }
  } else if (args[0] == 'G') { // give up
      g.state = 'closed';
      g.close_reason = from == myId ? ttt_igaveup : ttt_ogaveup;
  } else if (args[0] == 'V') { // violation
      g.state = 'closed';
      g.close_reason = '(protocol violation)';
  }

  persist();
  if (curr_scenario == 'tictactoe-list')
    ttt_load_list();
  if (curr_scenario == 'tictactoe-board' && args[0] != 'A' && tremola.tictactoe.current == args[1])
    ttt_load_board(args[1]);
}

function ttt_give_up() {
  let nm = tremola.tictactoe.current;
  backend("tictactoe G " + nm);
  setScenario('tictactoe-list');
}

function ttt_walk_away() {
  let nm = tremola.tictactoe.current;
  backend("tictactoe E " + nm);
  setScenario('tictactoe-list');
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

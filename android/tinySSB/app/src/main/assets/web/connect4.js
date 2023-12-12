const CONNECT4_GAME_COLUMNS = 7;
const CONNECT4_GAME_ROWS = 6;

function connect4_menu_game_players() {
    connect4_fill_players();
    prev_scenario = 'connect4-game';
    setScenario("connect4-game-players");
    document.getElementById("div:textarea").style.display = 'none';
    document.getElementById("div:connect4-confirm-player").style.display = 'flex';
    document.getElementById("tremolaTitle").style.display = 'none';
    var c = document.getElementById("conversationTitle");
    c.style.display = null;
    c.innerHTML = "<font size=+1><strong>Create New Game</strong></font><br>Select contact to play";
    document.getElementById('plus').style.display = 'none';
    closeOverlay();
}

function connect4_fill_players() {
    var choices = '';
    for (var m in tremola.contacts) {
        choices += '<div style="margin-bottom: 10px;"><label><input type="radio" name="players" id="' + m;
        choices += '" style="vertical-align: middle;"><div class="contact_item_button light" style="white-space: nowrap; width: calc(100% - 40px); padding: 5px; vertical-align: middle;">';
        choices += '<div style="text-overflow: ellipis; overflow: hidden;">' + escapeHTML(fid2display(m)) + '</div>';
        choices += '<div style="text-overflow: ellipis; overflow: hidden;"><font size=-2>' + m + '</font></div>';
        choices += '</div></label></div>\n';
    }
    document.getElementById('lst:connect4-players').innerHTML = choices
    document.getElementById(myId).disabled = true;
}

/**
 * Starts new game, creates it and stores it into the store.
 * Gives over to connect4_open_game_session().
 */
function connect4_new_game_start() {
    var opponent = {}
    for (var m in tremola.contacts) {
        if (document.getElementById(m).checked) {
            opponent = m;
        }
    }

    var players = [myId, opponent];
    var gameId = recps2nm(players);

    if (tremola.game_connect4 == null) {
        tremola.game_connect4 = {};
    }

    if (!(gameId in tremola.game_connect4)) {
        tremola.game_connect4[gameId] = {
            alias: fid2display(opponent) + " vs " + fid2display(myId),
            board: Array.from(new Array(7), () => Array.from(new Array(6), () => ({}))),
            currentPlayer: opponent,
            members: players,
            gameOver: false
        };
    }

    document.getElementById("div:connect4-confirm-player").style.display = 'none';
    connect4_open_game_session(gameId);

    persist();
    connect4_send_board(gameId);
}

/**
 * Clear all ongoing games within lst:connect4-games and builds the game button
 * for each ongoing game.
 */
function connect4_load_games_list() {
    document.getElementById("lst:connect4-games").innerHTML = '';
    for (let gameId in tremola.game_connect4) {
        connect4_build_game_item([gameId, tremola.game_connect4[gameId]]);
    }
}

/**
 * Create game button to resume ongoing game.
 *
 */
function connect4_build_game_item(game) { // [ id, { "alias": "player1 vs player2", "moves": {}, members: [] } ] }
    var row, item = document.createElement('div'), bg;
    item.setAttribute('style', 'padding: 0px 5px 10px 5px;'); // old JS (SDK 23)

    row = "<button class='chat_item_button light' style='overflow: hidden; width: calc(100% - 4em);' onclick='connect4_open_game_session(\"" + game[0] + "\");'>";
    row += "<div style='white-space: nowrap;'><div style='text-overflow: ellipsis; overflow: hidden;'>" + escapeHTML(game[1].alias) + "</div>";
    row += "<div style='text-overflow: clip; overflow: ellipsis;'><font size=-2>" + game[0] + "</font></div></div></button>";

    item.innerHTML = row;
    document.getElementById('lst:connect4-games').appendChild(item);
}

/**
 * Is called after a new turn is received via the backend.
 * Based on the playerToMove it assigns all board tiles to their
 * respective owner and gives over to connect4_populate_game().
 */
function connect4_game_new_event(e) {
    const gameId = e.public[1];
    const playerToMove = e.public[2];
    const members = e.public[3].split(',');
    const boardString = e.public[4];

    if (gameId in tremola.game_connect4) {
        delete tremola.game_connect4[gameId];
    }

    let idlePlayer = members.find(member => member != playerToMove);
    const board = Array.from(new Array(7), () => Array.from(new Array(6), () => ({})));

    let i = 0;
    for (let x = 0; x < 7; x++) {
    	for (let y = 0; y < 6; y++) {
      	if (boardString[i] == '1') {
        	board[x][y].owner = playerToMove;
        } else if (boardString[i] == '2') {
        	board[x][y].owner = idlePlayer;
        }

        i++;
      }
    }

    const opponent = playerToMove == myId ? idlePlayer : playerToMove;

    tremola.game_connect4[gameId] = {
        board: board,
        members: members,
        currentPlayer: playerToMove,
        alias: fid2display(opponent) + " vs " + fid2display(myId),
        gameOver: false
    };
    persist();

    connect4_populate_game(gameId);
    connect4_load_games_list();
}

/**
 * This is received when the game is over, either if someone
 * won or if a player gave up. It updates the up and marks the
 * game as over in the store.
 *
 */
function connect4_game_end_event(e) {
    const gameId = e.public[1];
    const loser = e.public[2];

    tremola.game_connect4[gameId].gameOver = true;
    persist();

    if (loser != myId) {
        document.getElementById("connect4-game-turn-indicator").innerHTML = "You WON!";
    } else {
        document.getElementById("connect4-game-turn-indicator").innerHTML = "You LOST!";
    }

    document.getElementById("connect4-game-end-button").innerHTML = "End!";
    connect4_load_games_list();
}

/**
 * Sets game-session scenario, title and button.
 * Gives over to connect4_populate_game() afterwards.
 */
function connect4_open_game_session(gameId) {
    setScenario('connect4-game-session');

    document.getElementById("connect4-game-session-title").innerHTML = tremola.game_connect4[gameId].alias;
    document.getElementById("connect4-game-end-button").onclick = () => connect4_end_game(gameId);
    connect4_set_turn_indicator(gameId);

    connect4_populate_game(gameId);
}

/**
 * Creates the board and all the clickable tile elements.
 * Color of tiles is given based on owner of the tile.
 * Can also be used when game is not shown currently.
 */
function connect4_populate_game(gameId) {
    document.getElementById('connect4-game-board').innerHTML = '';

    const { board, members } = tremola.game_connect4[gameId];
    const opponent = members.find(member => member != myId);

    for (let y = 0; y < CONNECT4_GAME_ROWS; y++) {
        for (let x = 0; x < CONNECT4_GAME_COLUMNS; x++) {
            let tile = document.createElement('div');
            tile.className = 'connect4-game_tile';
            tile.onclick = () => connect4_add_stone(gameId, x);

            const { owner } = board[x][y];
            if (owner == myId) {
                tile.style.backgroundColor = "yellow";
            } else if (owner != myId && owner != null) {
                tile.style.backgroundColor = "red";
            }

            document.getElementById('connect4-game-board').appendChild(tile);
            board[x][y].tile = tile;

        }
    }

    connect4_set_turn_indicator(gameId);
}

/**
 * Tries to add a playing stone to the game field.
 * After the stone is placed, checks if game is over and if so,
 * informs the backend. If not gives over to connect4_end_turn().
 */
function connect4_add_stone(gameId, column) {
    const { board, currentPlayer, members, gameOver } = tremola.game_connect4[gameId];

    if (currentPlayer != myId || gameOver) {
        return;
    }

    const freeSlots = board[column].filter(t => t.owner == null).length;
    if (freeSlots > 0) {
        const boardElement = board[column][freeSlots - 1];
        boardElement.owner = myId;
        boardElement.tile.style.backgroundColor = "yellow";

        const gameover = connect4_check_gameover(gameId);
        if (gameover) {
            const loser = members.find(member => member != myId);
            tremola.game_connect4[gameId].gameOver = true;
            persist();
            backend(`connect_four_end ${gameId} ${loser}`);
            return;
        }

        connect4_end_turn(gameId);
    }
}

/**
 * Checks if game is over by checking if stones align so
 * that 4 stones are adjacent to each other.
 */
function connect4_check_gameover(gameId) {
    const { board: b } = tremola.game_connect4[gameId];

    // Check down
    for (let y = 0; y < 3; y++)
        for (let x = 0; x < 7; x++)
            if (connect4_check_line(b[x][y], b[x][y+1], b[x][y+2], b[x][y+3]))
                return true;

    // Check right
    for (let y = 0; y < 6; y++)
        for (let x = 0; x < 4; x++)
            if (connect4_check_line(b[x][y], b[x+1][y], b[x+2][y], b[x+3][y]))
                return true;

    // Check down-right
    for (let y = 0; y < 3; y++)
        for (let x = 0; x < 4; x++)
            if (connect4_check_line(b[x][y], b[x+1][y+1], b[x+2][y+2], b[x+3][y+3]))
                return true;

    // Check down-left
    for (let y = 3; y < 6; y++)
        for (let x = 0; x < 4; x++)
            if (connect4_check_line(b[x][y], b[x+1][y-1], b[x+2][y-2], b[x+3][y-3]))
                return true;

    return false;
}

/**
 * Helper function for connect4_check_gameover() to check if 4 stones
 * are adjacent.
 */
function connect4_check_line(a, b, c, d) {
    // Check first cell non-zero and all cells match
    return a.owner != null &&
            a.owner == b.owner &&
            a.owner == c.owner &&
            a.owner == d.owner;
}

/**
 * Sets the new currentPlayer to the store and updates
 * the UI accordingly with the turn indicator.
 * Sends board information after turn is over via backend.
 */
function connect4_end_turn(gameId) {
    const { currentPlayer } = tremola.game_connect4[gameId];
    const opponent = tremola.game_connect4[gameId].members.find(member => member != myId);

    if (currentPlayer == myId) {
        tremola.game_connect4[gameId].currentPlayer = opponent;
    } else {
        tremola.game_connect4[gameId].currentPlayer = myId;
    }
    persist();
    connect4_set_turn_indicator(gameId);
    connect4_send_board(gameId);
}

/**
 * Converts board state to a string encoding and sends
 * game information via backend.
 */
function connect4_send_board(gameId) {
    const { board, currentPlayer: playerToMove } = tremola.game_connect4[gameId];
    let boardString = ""

    board.forEach((column, ci) => {
      column.forEach((_, ri) => {
        if (board[ci][ri].owner != null) {
          if (board[ci][ri].owner == playerToMove) {
            boardString += "1"
          } else {
            boardString += "2"
          }
        } else {
          boardString += "0"
        }
      })
    })

    const { members } = tremola.game_connect4[gameId];

    backend(`connect_four ${gameId} ${playerToMove} ${members.join(',')} ${boardString}`);
}

/**
 * Sets UI turn indicator accoring to currentPlayer.
 */
function connect4_set_turn_indicator(gameId) {
    if (tremola.game_connect4[gameId].currentPlayer == myId) {
        document.getElementById("connect4-game-turn-indicator").innerHTML = "Your turn!";
    } else {
        document.getElementById("connect4-game-turn-indicator").innerHTML = "Wait for your opponent.";
    }
}

/**
 * Ends game, either by Giving up or if game is over.
 */
function connect4_end_game(gameId) {
    document.getElementById("connect4-game-end-button").innerHTML = "Give up";
    backend(`connect_four_end ${gameId} ${myId}`);
    delete tremola.game_connect4[gameId];
    setScenario('game');
    persist();
    connect4_load_games_list();
}
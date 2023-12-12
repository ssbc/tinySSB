const GAME_COLUMNS = 7;
const GAME_ROWS = 6;

function menu_game_players() {
    fill_players();
    prev_scenario = 'game';
    setScenario("game-players");
    document.getElementById("div:textarea").style.display = 'none';
    document.getElementById("div:confirm-player").style.display = 'flex';
    document.getElementById("tremolaTitle").style.display = 'none';
    var c = document.getElementById("conversationTitle");
    c.style.display = null;
    c.innerHTML = "<font size=+1><strong>Create New Game</strong></font><br>Select contact to play";
    document.getElementById('plus').style.display = 'none';
    closeOverlay();
}

function fill_players() {
    var choices = '';
    for (var m in tremola.contacts) {
        choices += '<div style="margin-bottom: 10px;"><label><input type="radio" name="players" id="' + m;
        choices += '" style="vertical-align: middle;"><div class="contact_item_button light" style="white-space: nowrap; width: calc(100% - 40px); padding: 5px; vertical-align: middle;">';
        choices += '<div style="text-overflow: ellipis; overflow: hidden;">' + escapeHTML(fid2display(m)) + '</div>';
        choices += '<div style="text-overflow: ellipis; overflow: hidden;"><font size=-2>' + m + '</font></div>';
        choices += '</div></label></div>\n';
    }
    document.getElementById('lst:players').innerHTML = choices
    document.getElementById(myId).disabled = true;
}

/**
 * Starts new game, creates it and stores it into the store.
 * Gives over to open_game_session().
 */
function new_game_start() {
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

    document.getElementById("div:confirm-player").style.display = 'none';
    open_game_session(gameId);

    persist();
    send_board(gameId);
}

/**
 * Clear all ongoing games within lst:games and builds the game button
 * for each ongoing game.
 */
function load_games_list() {
    document.getElementById("lst:games").innerHTML = '';
    for (let gameId in tremola.game_connect4) {
        build_game_item([gameId, tremola.game_connect4[gameId]]);
    }
}

/**
 * Create game button to resume ongoing game.
 *
 */
function build_game_item(game) { // [ id, { "alias": "player1 vs player2", "moves": {}, members: [] } ] }
    var row, item = document.createElement('div'), bg;
    item.setAttribute('style', 'padding: 0px 5px 10px 5px;'); // old JS (SDK 23)

    row = "<button class='chat_item_button light' style='overflow: hidden; width: calc(100% - 4em);' onclick='open_game_session(\"" + game[0] + "\");'>";
    row += "<div style='white-space: nowrap;'><div style='text-overflow: ellipsis; overflow: hidden;'>" + escapeHTML(game[1].alias) + "</div>";
    row += "<div style='text-overflow: clip; overflow: ellipsis;'><font size=-2>" + game[0] + "</font></div></div></button>";

    item.innerHTML = row;
    document.getElementById('lst:games').appendChild(item);
}

/**
 * Is called after a new turn is received via the backend.
 * Based on the playerToMove it assigns all board tiles to their
 * respective owner and gives over to populate_game().
 */
function game_new_event(e) {
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

    populate_game(gameId);
    load_games_list();
}

/**
 * This is received when the game is over, either if someone
 * won or if a player gave up. It updates the up and marks the
 * game as over in the store.
 *
 */
function game_end_event(e) {
    const gameId = e.public[1];
    const loser = e.public[2];

    tremola.game_connect4[gameId].gameOver = true;
    persist();

    if (loser != myId) {
        document.getElementById("game-turn-indicator").innerHTML = "You WON!";
    } else {
        document.getElementById("game-turn-indicator").innerHTML = "You LOST!";
    }

    document.getElementById("game-end-button").innerHTML = "End!";
    load_games_list();
}

/**
 * Sets game-session scenario, title and button.
 * Gives over to populate_game() afterwards.
 */
function open_game_session(gameId) {
    setScenario('game-session');

    document.getElementById("game-session-title").innerHTML = tremola.game_connect4[gameId].alias;
    document.getElementById("game-end-button").onclick = () => end_game(gameId);
    set_turn_indicator(gameId);

    populate_game(gameId);
}

/**
 * Creates the board and all the clickable tile elements.
 * Color of tiles is given based on owner of the tile.
 * Can also be used when game is not shown currently.
 */
function populate_game(gameId) {
    document.getElementById('game-board').innerHTML = '';

    const { board, members } = tremola.game_connect4[gameId];
    const opponent = members.find(member => member != myId);

    for (let y = 0; y < GAME_ROWS; y++) {
        for (let x = 0; x < GAME_COLUMNS; x++) {
            let tile = document.createElement('div');
            tile.className = 'game_tile';
            tile.onclick = () => add_stone(gameId, x);

            const { owner } = board[x][y];
            if (owner == myId) {
                tile.style.backgroundColor = "yellow";
            } else if (owner != myId && owner != null) {
                tile.style.backgroundColor = "red";
            }

            document.getElementById('game-board').appendChild(tile);
            board[x][y].tile = tile;

        }
    }

    set_turn_indicator(gameId);
}

/**
 * Tries to add a playing stone to the game field.
 * After the stone is placed, checks if game is over and if so,
 * informs the backend. If not gives over to end_turn().
 */
function add_stone(gameId, column) {
    const { board, currentPlayer, members, gameOver } = tremola.game_connect4[gameId];

    if (currentPlayer != myId || gameOver) {
        return;
    }

    const freeSlots = board[column].filter(t => t.owner == null).length;
    if (freeSlots > 0) {
        const boardElement = board[column][freeSlots - 1];
        boardElement.owner = myId;
        boardElement.tile.style.backgroundColor = "yellow";

        const gameover = check_gameover(gameId);
        if (gameover) {
            const loser = members.find(member => member != myId);
            tremola.game_connect4[gameId].gameOver = true;
            persist();
            backend(`connect_four_end ${gameId} ${loser}`);
            return;
        }

        end_turn(gameId);
    }
}

/**
 * Checks if game is over by checking if stones align so
 * that 4 stones are adjacent to each other.
 */
function check_gameover(gameId) {
    const { board: b } = tremola.game_connect4[gameId];

    // Check down
    for (let y = 0; y < 3; y++)
        for (let x = 0; x < 7; x++)
            if (check_line(b[x][y], b[x][y+1], b[x][y+2], b[x][y+3]))
                return true;

    // Check right
    for (let y = 0; y < 6; y++)
        for (let x = 0; x < 4; x++)
            if (check_line(b[x][y], b[x+1][y], b[x+2][y], b[x+3][y]))
                return true;

    // Check down-right
    for (let y = 0; y < 3; y++)
        for (let x = 0; x < 4; x++)
            if (check_line(b[x][y], b[x+1][y+1], b[x+2][y+2], b[x+3][y+3]))
                return true;

    // Check down-left
    for (let y = 3; y < 6; y++)
        for (let x = 0; x < 4; x++)
            if (check_line(b[x][y], b[x+1][y-1], b[x+2][y-2], b[x+3][y-3]))
                return true;

    return false;
}

/**
 * Helper function for check_gameover() to check if 4 stones
 * are adjacent.
 */
function check_line(a, b, c, d) {
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
function end_turn(gameId) {
    const { currentPlayer } = tremola.game_connect4[gameId];
    const opponent = tremola.game_connect4[gameId].members.find(member => member != myId);

    if (currentPlayer == myId) {
        tremola.game_connect4[gameId].currentPlayer = opponent;
    } else {
        tremola.game_connect4[gameId].currentPlayer = myId;
    }
    persist();
    set_turn_indicator(gameId);
    send_board(gameId);
}

/**
 * Converts board state to a string encoding and sends
 * game information via backend.
 */
function send_board(gameId) {
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
function set_turn_indicator(gameId) {
    if (tremola.game_connect4[gameId].currentPlayer == myId) {
        document.getElementById("game-turn-indicator").innerHTML = "Your turn!";
    } else {
        document.getElementById("game-turn-indicator").innerHTML = "Wait for your opponent.";
    }
}

/**
 * Ends game, either by Giving up or if game is over.
 */
function end_game(gameId) {
    document.getElementById("game-end-button").innerHTML = "Give up";
    backend(`connect_four_end ${gameId} ${myId}`);
    delete tremola.game_connect4[gameId];
    setScenario('game');
    persist();
    load_games_list();
}
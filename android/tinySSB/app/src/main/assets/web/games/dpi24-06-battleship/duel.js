"use strict";

var battleships_turn = false
var battleships_horizontal = true
var battleships_ship_positions = ""
var battleship_ship_lengths = [2, 3, 3, 4 ,5]
var battleship_status = "STOPPED"
var battleship_timestamp = "0"

var owner = "-"
var peer = "-"
var game = "-"

/*
*   This method opens the selection menu for possible games needed in order to send an invite.
*/

function duel_openDuels() {
    // return;
    console.log('duel_openDuels()')
    setScenario('duels')
    /*
    // closeOverlay();
    var duelInviteContainer = document.getElementById('div:duelInviteContainer');
    if (duelInviteContainer) {
        duelInviteContainer.style.display = 'block';
    } else {
        console.error('Duel-Invite-Container not found!');
    }
    */
}

/**
*   This method gets called if you click on an icon in the invitemenu.
*/
function inviteForDuel(gameType) {
    closeDuelOverlay();
    var stringToEncode = "BSH INV " + myId;
    console.log('Invited battleship ' + JSON.stringify(stringToEncode))
    backend("games "+ stringToEncode);
}

/**
*   This function closes the invitemenu. It gets called by the Cancel-Button.
*/
function closeDuelOverlay() {
    var duelInviteContainer = document.getElementById('div:duelInviteContainer');
    if (duelInviteContainer) {
        duelInviteContainer.style.display = 'none';
    }
}

// ---------- GAME-SCREEN -----------

function update_game_gui(response) {
    console.log("BSH updating GUI ... " + JSON.stringify(response))
    if (curr_scenario == 'battleships') {
        if (window.GamesHandler && typeof window.GamesHandler.getInstanceDescriptorFromFids === 'function' && typeof window.GamesHandler.getInstanceDescriptorFromFid === 'function') {
            var instanceDescriptor = ""
            var responseList = response.split(" ")
            console.log("BSH updating GUI after split")
            if (responseList.length > 1) {
                console.log("BSH updating GUI > 1")
                if (owner == responseList[0] && peer == "-" && battleship_status == "INVITED") {
                    console.log("BSH updating GUI setting peer")
                    peer = responseList[1]
                }
            }
            console.log("BSH updating GUI before instanceDescriptor")
            if (peer == "-") {
                instanceDescriptor = window.GamesHandler.getInstanceDescriptorFromFid(game, owner)
            } else {
                instanceDescriptor = window.GamesHandler.getInstanceDescriptorFromFids(game, owner, peer, battleship_timestamp)
            }

            console.log("BSH update_gui ", JSON.stringify(instanceDescriptor))
            var instanceList = instanceDescriptor.split(" ")
            battleship_status = instanceList[4]
            battleship_timestamp = instanceList[3]
            if (instanceList.length < 6) { return }
            var playerTurn = instanceList[5]
            if (playerTurn == "1") {
                battleships(true, instanceList[6])
            } else {
                battleships(false, instanceList[6])
            }
        }
    } else if (curr_scenario == 'duels') {
        show_duels()
    }
}


// The main function that launches the game GUI
function battleships(turn, ships_fired_recv) {
    var args = ships_fired_recv.split("^");
    battleships_turn = turn;
    console.log("Called BSH GUI:", JSON.stringify(ships_fired_recv));

    setScenario("battleships")

    /*
    var c = document.getElementById("conversationTitle");
    c.style.display = null;
    c.innerHTML = "<div style='text-align: center; color: Blue;'><font size=+2><strong>My Battleships</strong></font></div>";
    */
    battleships_load_config(battleship_status, args[0], args[1], args[2]);
}

/**
*   This function is called if a user wants to quit the game.
*/
function quit_bsh() {
    backend("games BSH DUELQUIT " + owner + " " + peer);
    reset_battleship_mode();
    show_duels()
}


// Gets called when the user clicks on a square of the bottom field
function battleship_bottom_field_click(i) {
    console.log("BSH registered Shot: ", JSON.stringify(i))
    var square = document.getElementById("battleships:bottom" + i)
    // Do nothing if it is not our turn
    if (!battleships_turn) {
        return
    } else {
        backend("games BSH SHOT " + owner + " " + peer + " " + (((i % 10) + 9) % 10) + "" + Math.floor((i - 1) / 10))
        battleships_set_turn(false)
        battleships_show_turn()
    }
    square.childNodes[0].className = "hole field_clicked"
    setTimeout(function () {
            square.childNodes[0].className = "hole"
        }, 100);
}

function battleship_top_field_click(i) {
   // ignore
}

// Generates the HTML of the playing field
function battleships_setup() {
    console.log("BSH_setup()", JSON.stringify(battleship_status));
    battleships_hide_controls()
    var board = document.getElementById("battleships:board")
    var topgrid = document.getElementById("battleships:topgrid")
    topgrid.innerHTML = ""
    topgrid.insertAdjacentHTML("beforeend", "<span class='battleshipsTopRow zero'></span>")
    for (var i = 1; i < 11; i++) {
        topgrid.insertAdjacentHTML("beforeend", "<span class='battleshipsTopRow'>" + i + "</span>")
    }
    var counter = 1;
    for (var i = 12; i <= 121; i++) {
        if ((i % 11) - 1 == 0) {
            topgrid.insertAdjacentHTML("beforeend", "<span class='battleshipsLeftRow'>" + String.fromCharCode(64 + (i - 1) / 11).toUpperCase() + "</span>")
        } else {
            topgrid.insertAdjacentHTML("beforeend", "<li id='battleships:top" + counter + "'class='field' onclick='battleship_top_field_click(" + counter + ")'><span class='hole'></span></li>")
            counter++
        }
    }
    var bottomgrid = document.getElementById("battleships:bottomgrid")
    bottomgrid.innerHTML = ""
    bottomgrid.insertAdjacentHTML("beforeend", "<span class='battleshipsTopRow zero'></span>")
    for (var i = 1; i < 11; i++) {
        bottomgrid.insertAdjacentHTML("beforeend", "<span class='battleshipsTopRow'>" + i + "</span>")
    }
    var counter = 1;
    for (var i = 12; i <= 121; i++) {
        if ((i % 11) - 1 == 0) {
            bottomgrid.insertAdjacentHTML("beforeend", "<span class='battleshipsLeftRow'>" + String.fromCharCode(64 + (i - 1) / 11).toUpperCase() + "</span>")
        } else {
            bottomgrid.insertAdjacentHTML("beforeend", "<li id='battleships:bottom" + counter + "'class='field' onclick='battleship_bottom_field_click(" + counter + ")'><span class='hole'></span></li>")
            counter++
        }
    }
}

function battleships_set_turn(is_turn) {
    battleships_turn = is_turn
}

// Hides everything below the two grids
function battleships_hide_controls() {
    document.getElementById("battleships:turn").style.display = "none"
}

// Shows the turn indicator below the grid
function battleships_show_turn() {
    console.log("BSH Showing Turn...");
    battleships_hide_controls();
    var peerId = myId;
    var turn = document.getElementById("battleships:turn");
    turn.style.display = null;

    console.log("BSH Determining what to display as turn ...");

    // Set default styles
    turn.style.width = '70vw';
    turn.style.textAlign = 'center';
    turn.style.fontSize = '1.3em';
    turn.style.padding = '15px';
    turn.style.marginTop = '10px';
    turn.style.border = '1px solid #000';
    turn.style.borderRadius = '15px';
    turn.style.backgroundColor = '#50A4D3';
    turn.style.color = '#fff';

    if (peerId == owner || peerId == peer) {
        if (battleship_status == "WON") {
            turn.innerHTML = "You Won!";
            turn.style.backgroundColor = 'green';
            turn.style.color = 'white';
        } else if (battleship_status == "LOST") {
            turn.innerHTML = "You Lost!";
            turn.style.backgroundColor = 'red';
            turn.style.color = 'white';
        } else if (battleship_status == "STOPPED") {
            turn.innerHTML = "The game is stopped!";
        } else if (battleship_status == "INVITED") {
            console.log("BSH invite-button init ...");
            turn.innerHTML = "Waiting for other!";
        } else if (battleship_status == "WAITING") {
            if (peerId == "-") {
                turn.innerHTML = "Waiting ...";
            }
        } else if (battleship_status == "RUNNING") {
            if (battleships_turn) {
                turn.innerHTML = "Your Turn";
                turn.style.backgroundColor = '#d4edda'; // Light green
                turn.style.color = 'black';
            } else {
                turn.innerHTML = "Enemy Turn";
                turn.style.backgroundColor = '#f8d7da'; // Light red
                turn.style.color = 'black';
            }
        }
    } else {
        if (battleship_status == "WON") {
            if (battleships_turn) {
                turn.innerHTML = "Owner has Won!";
            } else {
                turn.innerHTML = "Peer has Won!";
            }
            turn.style.backgroundColor = 'green';
            turn.style.color = 'white';
        } else if (battleship_status == "LOST") {
            if (battleships_turn) {
                turn.innerHTML = "Owner has Lost!";
            } else {
                turn.innerHTML = "Peer has Lost!";
            }
            turn.style.backgroundColor = 'red';
            turn.style.color = 'white';
        } else {
            if (battleships_turn) {
                turn.innerHTML = "Owner's Turn!";
            } else {
                turn.innerHTML = "Peer's Turn!";
            }
        }
    }
}


// Displays the config given from the backend. The format of the config is
// well described in the backend.
function battleships_load_config(state, ships, deliv, recv) {
    document.getElementById("conversationTitle").innerHTML = "<div style='text-align: center; color: Blue;'><font size=+2><strong>My Battleships</strong></font></div>";
    battleships_ship_positions = ""
    console.log("BSH_load_config", JSON.stringify(state + " " + ships + " " + recv + " " + deliv));
    if (state === "STOPPED") {
        document.getElementById("conversationTitle").innerHTML = "<div style='text-align: center; color: Red;'><font size=+2><strong>Stopped Game!</strong></font></div>";
        battleships_setup()
        battleships_show_turn()
    } else if (state === "INVITED") {
        battleships_setup()
        battleships_show_turn()
        //return
    } else if (state === "WON") {
        document.getElementById("conversationTitle").innerHTML = "<div style='text-align: center; color: Blue;'><font size=+2><strong>You Won!</strong></font></div>";
        battleships_setup()
        battleships_show_turn()
    } else if (state === "LOST") {
        document.getElementById("conversationTitle").innerHTML = "<div style='text-align: center; color: Blue;'><font size=+2><strong>You Lost!</strong></font></div>";
        battleships_setup()
        battleships_show_turn()
    } else if (state === "RUNNING") {
        battleships_setup()
        battleships_show_turn()
    }

    console.log("BSH chunking ships now ...", JSON.stringify(ships))
    var shipPositions = splitIntoChunks(ships, 3)
    console.log("BSH chunky ships: ", JSON.stringify(shipPositions))
    for (var i = 0; i < shipPositions.length; i++) {
        console.log("BSH Processing Ship: ", shipPositions[i])
        var ship = shipPositions[i]
        var x = parseInt(ship.charAt(0))
        var y = parseInt(ship.charAt(1))
        if (x != -1) {
            battleships_ship_positions = battleships_ship_positions + "" + ship
        }
        var direction = ship.slice(2, 100)
        for (var j = 0; j < battleship_ship_lengths[i]; j++) {
            var position = y * 10 + x + 1
            if (direction === "U") {
                position = position - 10 * j
            } else if (direction === "D") {
                position = position + 10 * j
            } else if (direction === "L") {
                position = position - j
            } else if (direction === "R") {
                position = position + j
            }
            var field = document.getElementById("battleships:top" + position)
            field.className = "ship"
            field.innerHTML = ""
            field.onclick = null
        }
    }
    console.log("BSH parsing shotsFired ", JSON.stringify(deliv))
    var shots_fired = splitIntoChunks(deliv, 3)
    for (var i = 0; i < shots_fired.length; i++) {
        var shot = shots_fired[i]
        if (shot === "") {
            continue
        }
        var x = parseInt(shot.charAt(0))
        var y = parseInt(shot.charAt(1))
        var outcome = shot.slice(2, 100)
        var position = y * 10 + x + 1
        var field = document.getElementById("battleships:bottom" + position)
        if (outcome === "M") {
            field.className = "miss"
        } else if (outcome === "H") {
            field.className = "hit"
        } else if (outcome === "S") {
            field.className = "sunken"
        }
        field.innerHTML = ""
        field.onclick = null
    }
    console.log("BSH Parsing Shots Received ", JSON.stringify(recv))
    var shots_received = splitIntoChunks(recv, 3)
    for (var i = 0; i < shots_received.length; i++) {
        var shot = shots_received[i]
        if (shot === "") {
            continue
        }
        var x = parseInt(shot.charAt(0))
        var y = parseInt(shot.charAt(1))
        var outcome = shot.slice(2, 100)
        var position = y * 10 + x + 1
        var field = document.getElementById("battleships:top" + position)
        if (outcome === "M") {
            field.className = "miss"
        } else if (outcome === "H") {
            field.className = "hit"
        } else if (outcome === "S") {
            field.className = "sunken"
        }
        field.innerHTML = ""
        field.onclick = null
    }
    if (state === "WAITING") {
        battleships_show_turn()
        return
    }
    battleships_show_turn()
}

/*
*   This method parses the ships into readable format.
*/
function splitIntoChunks(str, chunkSize) {
    const chunks = [];
    for (let i = 0; i < str.length; i += chunkSize) {
        chunks.push(str.substring(i, i + chunkSize));
    }
    return chunks;
}

function reset_battleship_mode() {
    battleships_turn = null
    battleships_ship_positions = ""
    battleship_status = "STOPPED"
    battleship_timestamp = "0"
    game = "-"

    owner = "-"
    peer = "-"
}


function show_duels() {
    setScenario('duels');
    let c = document.getElementById("conversationTitle");
    c.style.display = null;
    c.innerHTML = "<div style='text-align: center;'><font size=+2><strong>Battleship Duels</strong></font></div>";
    var container = document.getElementById("duels-container");
    container.innerHTML = "";

    console.log('show_duels ' + JSON.stringify("gamelist before"));
    var gameListString = "";
//    var gameListString = "BSH ownerid1 participantid1 12 STOPPED"
    if (window.GamesHandler && typeof window.GamesHandler.createInstanceList === 'function') {
        gameListString = window.GamesHandler.createInstanceList();
        console.log('show_duels - gamelist received: ' + gameListString);
    } else {
        console.error("GamesHandler.createInstanceList is not a function");
    }

    if (gameListString === "") {
        console.log('show_duels ' + JSON.stringify("No active duels found."));
        var noDuelDiv = document.createElement("div");
        noDuelDiv.className = "no-duel-box";
        noDuelDiv.innerHTML = "No active duels available...";
        container.appendChild(noDuelDiv);
    } else {
        var gameList = gameListString.split('$');
        gameList.forEach(function(game) {
            var gameParts = game.split(" ");
            var gameName = gameParts[0];
            var ownerName = gameParts[1];
            //var ownerAlias = tremola.contacts[owner].alias;
            var participantName = gameParts[2];
            //var participantAlias = tremola.contacts[participant].alias;
            var startTimeRaw = parseInt(gameParts[3]);;
            // Format start time
            var date = new Date(startTimeRaw);
            var options = {
                weekday: 'long',
                year: 'numeric',
                month: 'long',
                day: 'numeric',
                hour: 'numeric',
                minute: 'numeric',
                second: 'numeric',
                hour12: true
            };
            options = {
                // weekday: 'none', //'long',
                // year: 'numeric',
                // month: 'long',
                // day: 'numeric',
                hour: 'numeric',
                minute: 'numeric',
                second: 'numeric',
                hour12: false
            };
            var startTime = new Intl.DateTimeFormat('en-US', options).format(date);
            var state = gameParts[4];
            console.log('My Id: ' + JSON.stringify(myId) + ` part:${participantName}` + ` own:${ownerName}`);
            var pname = '-';
            if (participantName != '-')
                pname = `${tremola.contacts[participantName].alias} (${id2b32(participantName)})`;
            var oname = myId;
            if (ownerName != myId)
                oname = `${tremola.contacts[ownerName].alias} (${id2b32(ownerName)})`;
            if (ownerName == myId) {
                ownerName = `Me (${tremola.contacts[myId].alias})`
                participantName = pname;
            } else if (participantName == myId) {
                participantName = `Me (${tremola.contacts[myId].alias})`;
                ownerName = oname;
            } else {
                participantName = pname;
                ownerName = oname;
            }
            var turn = gameList[5];
            var ships_rec_delivered = gameList[6];

            console.log('Game-Container for: ' + JSON.stringify(name));

            var gameDiv = document.createElement("button");
            gameDiv.className = "duel-button";
            gameDiv.onclick = () => onDuelButtonClicked(game);

            // Change background color based on state
            if (state === 'STOPPED') {
                gameDiv.classList.add('duel-button-stopped');
            } else if (state === 'INVITED') {
                gameDiv.classList.add('duel-button-invited');
            } else if (state === 'RUNNING') {
                gameDiv.classList.add('duel-button-running');
            } else if (state === 'WAITING') {
                gameDiv.classList.add('duel-button-waiting');
            } else if (state === 'WON') {
                gameDiv.classList.add('duel-button-won');
            } else if (state === 'LOST') {
                gameDiv.classList.add('duel-button-lost');
            }
            // Create Icon for duel
            const img = document.createElement("img");
            if (gameName === "BSH") {
                img.src = "./games/dpi24-06-battleship/battleship.svg";
            } else {
                // other game icons
                img.src = "./img/cancel.svg";
            }
            img.alt = `Duel Image`;
            img.className = "duel-image";
            gameDiv.appendChild(img);

            // Create text for duel button
            const span = document.createElement("div");
            span.className = "duel-text";
            span.innerHTML = `Owner: ${ownerName}<br>Participant: ${participantName}<br>Start Time: ${startTime}<br>State: ${state}`;

            gameDiv.appendChild(span);
            container.appendChild(gameDiv);
        });
    }
}

/**
*   Triggered when you click on an instance in duels overview.
*/
function onDuelButtonClicked(duelString) {
  console.log("Button clicked for: " + JSON.stringify(duelString));
  console.log("myId: ", JSON.stringify(myId));
  var duelList = duelString.split(" ");
  game = duelList[0]
  console.log("owner: ", JSON.stringify(duelList[1]));
  battleship_timestamp = duelList[3]
  battleship_status = duelList[4]
  switch (battleship_status) {
    case "STOPPED":
        return;
    case "INVITED":
        if (duelList[1] != myId) { // check if I am not the owner
            // I am not owner
            backend("games BSH INVACC " + duelList[1] + " " + myId); // nicht peerId
            // TODO possibly add cooldown
        } else {
            // TODO open game to see ships
            owner = duelList[1];
            peer = "-"
            battleships(null, duelList[6]);
        }
        return;
    case "WON": // 6 = shotsDeliverOutcome, 7 = shotsReceivedOutcome, 8 = ships
        owner = duelList[1];
        peer = duelList[2];
        battleships(null, duelList[6]);
        return;
    case "LOST":
        owner = duelList[1];
        peer = duelList[2];
        battleships(null, duelList[6]);
        return;
    case "WAITING":
        owner = duelList[1];
        peer = duelList[2];
        battleships(false, duelList[6]);
        return;
    case "RUNNING":
        owner = duelList[1];
        peer = duelList[2];
        if (duelList[5] == "0") {
            battleships(false, duelList[6]);
        } else {
            battleships(true, duelList[6]);
        }
        return;
    case "SPEC":
        owner = duelList[1];
        peer = duelList[2];
        battleships(null, duelList[6]);
        return;
    default:
        return;
  }
}

// eof

/**
* HTML objects
*/



function menu_start_game_of_blackjack() {
    closeOverlay();
    document.getElementById('blackjack-start-game').style.display = 'initial';
}

/**
*   PHASE 1: DEALER STARTS A NEW GAME
*/
function start_game_as_dealer_button_pressed() {

    // Hide initial display and open confirmation dialog
    const initialDisplay = document.getElementById('initial-display');
    const confirmationDialog = document.getElementById('confirmation-dialog');
    if (initialDisplay) {
        initialDisplay.style.display = 'none';
    }
    if (confirmationDialog) {
        confirmationDialog.style.display = 'initial';
    }

}

function dealer_confirmed_pressed() {

    // Setting environment variables - cards don't get drawn yet, only when player joins
    myRole = "dealer"
    gameID = generateGameID(); // generating a new game ID
    myCurrentGameID = gameID

    // Dealer Screen opens - confirmation that he successfully started a game as dealer
    // Confirmation dialog closes
    const dealerWaitingLobby = document.getElementById('dealer-waiting-lobby');
    const confirmationDialog = document.getElementById('confirmation-dialog');
    const gameIDDisplay = document.getElementById('game-id-display');
    if (dealerWaitingLobby) {
       dealerWaitingLobby.style.display = 'initial';
       if(gameIDDisplay) {
        gameIDDisplay.innerHTML = gameID;
       }
    }
    if (confirmationDialog) {
       confirmationDialog.style.display = 'none';
    }

    // Emmit the opening of the game
    sendMessage(gameID + " " + "open " + "_ " + "_ " + "_ " + "_ " + "_ ")
}

function dealer_canceled_pressed() {
    // Hide confirmation dialog and show initial display again
    const initialDisplay = document.getElementById('initial-display');
    const confirmationDialog = document.getElementById('confirmation-dialog');
    if (confirmationDialog) {
        confirmationDialog.style.display = 'none';
    }
    if (initialDisplay) {
        initialDisplay.style.display = 'flex';
    }
}

function start_game_as_player_button_pressed() {

    // Hide initial display and start showing the player lobby
    const initialDisplay = document.getElementById('initial-display');
    const playerWaitingLobby = document.getElementById('player-waiting-lobby');
    if (initialDisplay) {
        initialDisplay.style.display = 'none';
    }
    if (playerWaitingLobby) {
        playerWaitingLobby.style.display = 'initial';
    }

    // Set env variables
    myRole = "player"
}

function leave_player_lobby_pressed() {
    // Hide player lobby and show initial display
    const initialDisplay = document.getElementById('initial-display');
    const playerWaitingLobby = document.getElementById('player-waiting-lobby');
    if (initialDisplay) {
        initialDisplay.style.display = 'flex';
    }
    if (playerWaitingLobby) {
        playerWaitingLobby.style.display = 'none';
    }
}

function gameScreen() {
    // Hide bottom decoration and show player option bar
        const bottomDecoration = document.getElementById('bottom-decoration');
        const playerOptionBar = document.getElementById('player-option-bar');
        if(bottomDecoration) {
            bottomDecoration.style.display = 'none';
        }
        if(playerOptionBar) {
            playerOptionBar.style.display = 'initial';
        }

        // Create initial CRDT for the game
        const initialCRDT = createInitialCRDT();
        console.log(initialCRDT); // Debugging purposes

        // Display Game State inside the app
        displayCurrentGameState(initialCRDT); // Pass initialCRDT as argument
}

function displayCurrentGameState(currentCRDT) {
    const gameStateDisplay = document.getElementById('game-state');
    if (!gameStateDisplay) {
        console.error('Element with id "game-state" not found.');
        return;
    }




}

function displayMessage(message) {
    console.log("MESSAGE RECEIVED: " + message)
}

function displayGameListItem(gameID, gameStatus) {
    // Identify game list element
    var gamesList = document.getElementById("games-list");
    if (gamesList) {
        // Create the game list item container
        const gameListItem = document.createElement("div");
        gameListItem.innerHTML = "GameID: " + gameID + " GameStatus: " + gameStatus;
        gameListItem.style.height = "40px";
        gameListItem.style.border = "1px solid white";
        gameListItem.style.width = "100%";
        gamesList.appendChild(gameListItem);

        // Add JOIN button if game status is "open"
        if (gameStatus === "open") {
            const joinButton = document.createElement("button");
            joinButton.innerHTML = "JOIN";
            joinButton.onclick = function() {
                joinGameButtonClicked(gameID);
            };
            gameListItem.appendChild(joinButton);
        }
    }
}

function joinGameButtonClicked(gameID) {
    // Close Player Waiting Lobby and Open Player game screen
    const playerGameScreen = document.getElementById('player-game-screen');
    const playerWaitingLobby = document.getElementById('player-waiting-lobby');

    if (playerGameScreen) {
        playerGameScreen.style.display = 'initial';
        const gameIdDisplay = document.createElement("div");
        gameIdDisplay.innerHTML = "You joined game with GameID: " + gameID;
        const waitMessage = document.createElement("div");
        waitMessage.innerHTML = "Please wait until the dealer draws your cards"
        playerGameScreen.appendChild(gameIdDisplay);
        playerGameScreen.appendChild(waitMessage);
    }

    if (playerWaitingLobby) {
        playerWaitingLobby.style.display = 'none';
    }

    // Set env variable
    myCurrentGameID = gameID

    // Player emmits message "BJ gameID gameStatus[ongoing]"
    sendMessage(gameID + " " + "ongoing " + "_ " + "_ " + "_ " + "_ " + "_")

}

function initializeGame() {
    console.log("1. Starting function initializeGame()")
    // Dealer gets confirmation that a player joined
    // Waiting lobby stops getting displayed
    const dealerWaitingLobby = document.getElementById('dealer-waiting-lobby');
    if(dealerWaitingLobby) {
        console.log("2. Dealer Waiting Lobby has been found")
        dealerWaitingLobby.style.display = "none"
    }

    const currentGameScreen = document.getElementById('current-game-screen');
    if(currentGameScreen) {
        console.log("3. CurrentGameScreen has been found")
        currentGameScreen.style.display = "initial"
    }

    console.log("4. Started dealing cards")
    // The game logic starts - dealer and player both receive two cards
    dealCard(dealerCards)
    dealCard(playerCards)
    dealCard(dealerCards)
    dealCard(playerCards)


    // Game screen starts getting displayed - The dealer can see the cards now
    const dealerCardContainer = document.getElementById('dealer-card-container');
        if (!dealerCardContainer) {
            console.error('Element with id "dealer-card-container" not found.');
            return;
        }
        dealerCardContainer.innerHTML = "";

    console.log("5. Started displaying Dealer Cards")
        dealerCards.forEach(card => {
            console.log(card);
            const cardImg = document.createElement('img');
            cardImg.src = `img/bj_cards/${card}.svg`;
            cardImg.alt = 'card';
            cardImg.width = 100;
            cardImg.height = 150;
            cardImg.classList.add('card-img');
            dealerCardContainer.appendChild(cardImg);
        })

    console.log("6. Started displaying Player Cards")
        const playerCardContainer = document.getElementById('player-card-container');
            if (!playerCardContainer) {
                console.error('Element with id "player-card-container" not found.');
                return;
            }
            playerCardContainer.innerHTML = "";

            playerCards.forEach(card => {
                console.log(card);
                const cardImg = document.createElement('img');
                cardImg.src = `img/bj_cards/${card}.svg`;
                cardImg.alt = 'card';
                cardImg.width = 100;
                cardImg.height = 150;
                cardImg.classList.add('card-img');
                playerCardContainer.appendChild(cardImg);
            })

    // Dealer gets prompted to type in a message and hit send button
    const dealerOptionBar = document.getElementById("dealer-option-bar")
    if(dealerOptionBar) {
        console.log("7.Displaying Dealer Option Bar")
        dealerOptionBar.style.display = "initial"
    }


    console.log("END: End of function reached")
}

function arrayToString(arr) {
    // Using Array.join() to concatenate array elements with ","
    return arr.join(', ');
}


function dealer_sends_current_status_to_player() {
    // Access messageInput field
    var messageInput = document.getElementById('dealer-message-input');

    var message = messageInput.value.trim();

    // Clear the input field
    messageInput.value = '';

    // Sends the first dealer card “BJ gameID gameStatus[ongoing] turn[player] dealerCards[”XX”]
    // playerCards[”XX,XX”] playerAction[”_”] dealerMessage[dealerMessage]
    dealerCardsString = arrayToString(dealerCards).trim();
    playerCardsString = arrayToString(playerCards).trim();
    console.log("SENDING: "+ gameID + " " + "ongoing " + "player " + dealerCards + " " + playerCards + " " + "_" + message )
    sendMessage(gameID + " " + "ongoing " + "player " + dealerCards + " " + playerCards + " " + "_ " + message )

}

function initializePlayerGame(dealerCards, playerCards, dealerMessage) {
    console.log("We made it inside initializePlayerGame")


    const playerGameScreen = document.getElementById('player-game-screen');
    if(!playerGameScreen) {
        console.log("playerGameScreen not found")
    }
    const dealerCardsDisplay = document.createElement("div")
    dealerCardsDisplay.innerHTML = "Dealer Cards: " + dealerCards

    const playerCardsDisplay = document.createElement("div")
    playerCardsDisplay.innerHTML = "Player Cards: " + playerCards

    const dealerMessageDisplay = document.createElement("div")
    dealerMessageDisplay.innerHTML = "Dealer Message: " + dealerMessage
    playerGameScreen.appendChild(dealerCardsDisplay)
    playerGameScreen.appendChild(playerCardsDisplay)
    playerGameScreen.appendChild(dealerMessageDisplay)

    console.log("End of initializePlayerGame")
}

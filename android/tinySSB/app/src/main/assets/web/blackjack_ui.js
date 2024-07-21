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
    const waitingScreen = document.getElementById('waiting-screen');
    const waitingText = document.getElementById('waiting-text');
    waitingText.innerHTML = "";
    const confirmationDialog = document.getElementById('confirmation-dialog');

    const waitingTextInfo = document.createElement("p");
    waitingTextInfo.innerHTML = "Nice! You just started a game with game id: " + gameID
    waitingTextInfo.style.fontWeight = "bold"
    const waitingTextPhrase = document.createElement("p")
    waitingTextPhrase.innerHTML = "Let's wait until somebody joins your game"

    if (waitingScreen && waitingText) {
       waitingScreen.style.display = 'initial';
       waitingText.appendChild(waitingTextInfo);
       waitingText.appendChild(waitingTextPhrase);
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

function displayGameListItem(gameID, gameStatus) {
    // Identify game list element
    var gamesList = document.getElementById("games-list");
    if (gamesList) {
        // Create the game list item container
        const gameListItem = document.createElement("div");
        gamesList.classList.add("games-list")
        gameListItem.classList.add("game-list-item")
        gameListItem.innerHTML = "#" + gameID + " " + gameStatus

        // Add JOIN button if game status is "open"
        if (gameStatus === "open") {

            const joinButton = document.createElement("button");
            joinButton.innerHTML = "JOIN";
            joinButton.onclick = function() {
                joinGameButtonClicked(gameID);
            };
            gamesList.appendChild(gameListItem);
            gameListItem.appendChild(joinButton);
        }

        gamesList.appendChild(gameListItem)
    }
}

function joinGameButtonClicked(gameID) {
    // Close Player Waiting Lobby and Open Player game screen
    const waitingScreen = document.getElementById('waiting-screen');
    const waitingText = document.getElementById('waiting-text');
    const playerWaitingLobby = document.getElementById('player-waiting-lobby');

    if (waitingText && waitingScreen) {
        waitingScreen.style.display = "flex";
        waitingText.innerHTML = "";
        waitingText.style.display = 'initial';
        const gameIdDisplay = document.createElement("p");
        gameIdDisplay.innerHTML = "You joined game with GameID: " + gameID;
        const waitMessage = document.createElement("p");
        waitMessage.innerHTML = "Please wait until the dealer draws your cards"
        waitingText.appendChild(gameIdDisplay);
        waitingText.appendChild(waitMessage);
    }

    if (playerWaitingLobby) {
        playerWaitingLobby.style.display = 'none';
    }

    // Set env variable
    myCurrentGameID = gameID

    // Player emits message "BJ gameID gameStatus[ongoing]"
    sendMessage(gameID + " " + "ongoing " + "_ " + "_ " + "_ " + "_ " + "_")

}

function initializeGame() {
    console.log("1. Starting function initializeGame()")
    // Dealer gets confirmation that a player joined
    // Waiting lobby stops getting displayed
    const waitingScreen = document.getElementById('waiting-screen');
    if(waitingScreen) {
        console.log("2. Dealer Waiting Lobby has been found")
        waitingScreen.style.display = "none"
    }

    const dealerGameScreen = document.getElementById('dealer-game-screen');
    if(dealerGameScreen) {
        console.log("3. CurrentGameScreen has been found")
        dealerGameScreen.style.display = "initial"
    }

    console.log("4. Started dealing cards")
    // The game logic starts - dealer and player both receive two cards
    if (!(gameStatusENV === "playerHitNoBust")) {
        dealCard(dealerCards)
        dealCard(playerCards)
        dealCard(dealerCards)
        dealCard(playerCards)
    }

    // Game screen starts getting displayed - The dealer can see the cards now
    const dealerCardContainer = document.getElementById('dealer-game-screen-dealer-card-container');
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
            cardImg.classList.add('card-img');
            dealerCardContainer.appendChild(cardImg);
        })

    console.log("6. Started displaying Player Cards")
        const playerCardContainer = document.getElementById('dealer-game-screen-player-card-container');
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
                cardImg.classList.add('card-img');
                playerCardContainer.appendChild(cardImg);
            })

    // Dealer gets prompted to type in a message and hit send button
    const dealerOptionBar = document.getElementById("dealer-option-bar")
    if(dealerOptionBar) {
        console.log("7.Displaying Dealer Option Bar")
        dealerOptionBar.style.display = "flex"
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
    console.log("SENDING: "+ gameID + " " + "initialCards " + "player " + dealerCards + " " + playerCards + " " + "_ " + message )

    if (gameStatusENV === "playerHitNoBust") {
        sendMessage(gameID + " " + "ongoingCards " + "player " + dealerCards + " " + playerCards + " " + "_ " + message )
    } else {
        sendMessage(gameID + " " + "initialCards " + "player " + dealerCards + " " + playerCards + " " + "_ " + message )
    }


    const dealerGameScreen = document.getElementById('dealer-game-screen');
    if (dealerGameScreen) {
        dealerGameScreen.style.display = "none"
    }
    const waitingScreen = document.getElementById('waiting-screen');
    const waitingText = document.getElementById('waiting-text');
    if(waitingScreen && waitingText) {
        waitingScreen.style.display = "flex"
        waitingText.innerHTML = "Hopefully the player likes your message. He is making a decision now"
    }


}

function initializePlayerGame(dealerCards, playerCards, dealerMessage) {
    console.log("We made it inside initializePlayerGame")

    // If second turn first hide waiting screen
    const waitingScreen = document.getElementById("waiting-screen");
        if (waitingScreen) {
            waitingScreen.style.display = "none"
        }

    // Displaying the game state received from the dealer
    const playerGameScreen = document.getElementById('player-game-screen');
    playerGameScreen.style.display = "initial"

    if(!playerGameScreen) {
        console.log("playerGameScreen not found")
    }

    /*
    *   DISPLAYING THE CARDS OF THE DEALER
    */
    const dealerCardsDisplay = document.getElementById("player-game-screen-dealer-card-container")

    dealerCards = dealerCards.split(",")
    // Appending first dealer card
    const cardImg = document.createElement('img');
    cardImg.src = `img/bj_cards/${dealerCards[1]}.svg`;
    console.log(cardImg.src)
    cardImg.alt = 'card';
    cardImg.classList.add('card-img');
    dealerCardsDisplay.appendChild(cardImg);

    // Appending the backside card because the player is only allowed to see one card
    const cardImgBackside = document.createElement('img');
    cardImgBackside.src = `img/bj_cards/1B.svg`;
    cardImgBackside.alt = 'card';
    cardImgBackside.classList.add('card-img');
    dealerCardsDisplay.appendChild(cardImgBackside);

    /*
    *   DISPLAYING THE CARDS OF THE PLAYER
    */
    const playerCardsDisplay = document.getElementById("player-game-screen-player-card-container")

    // Convert string representation to an actual array
    let playerCardsArray = playerCards.split(",")

    if(Array.isArray(playerCardsArray)) {
        playerCardsArray.forEach( card => {
            console.log(card);
            const cardImg = document.createElement('img');
            cardImg.src = `img/bj_cards/${card}.svg`;
            cardImg.alt = 'card';
            cardImg.classList.add('card-img');
            playerCardsDisplay.appendChild(cardImg);
        })
    } else {
        console.log("Player Cards is not an array")
    }

    /*
    * DISPLAYING THE MESSAGE THE DEALER SENT TO THE PLAYER
    */
    const dealerMessageDisplay = document.getElementById("dealer-message-display")

    const avatarImg = document.createElement('img');
    avatarImg.src = 'img/dealer-avatar.png'; // Adjust path as necessary
    avatarImg.alt = 'Dealer Avatar';
    avatarImg.classList.add("avatar-img");
    dealerMessageDisplay.appendChild(avatarImg);

    const speechBubble = document.createElement("div");
    speechBubble.classList.add("speech-bubble")
    speechBubble.innerHTML = dealerMessage;
    dealerMessageDisplay.appendChild(speechBubble)

    dealerMessageDisplay.style.display = "flex"

    // Showing the option buttons of the player at the bottom
    const optionBar = document.getElementById("player-option-bar");
    if (optionBar) {
        optionBar.style.display = "flex"
    }


    console.log("End of initializePlayerGame")
}

function sendDecisionToDealer(decision) {
    console.log("Sending decision to dealer: ", decision)
    sendMessage(myCurrentGameID + " " + "playerDecision " + "dealer " + dealerCards + " " + playerCards + " " + decision + " " + "_")

    // Hiding the options bar and displaying the waiting screen
    const playerGameScreen = document.getElementById("player-game-screen");
    if (playerGameScreen) {
        playerGameScreen.style.display = "none"
    }

    const waitingScreen = document.getElementById("waiting-screen");
    const waitingText = document.getElementById("waiting-text");
    if (waitingScreen && waitingText) {
        waitingScreen.style.display = "flex"
        waitingText.innerHTML = "Good Decision for sure! Lets wait for the dealer to calculate the result. "
    }

    // Clear cards display on dealer side
    const dealerCardsDisplay = document.getElementById("player-game-screen-dealer-card-container")
    if (dealerCardsDisplay) {
        dealerCardsDisplay.innerHTML = ""
    }
    const playerCardsDisplay = document.getElementById("player-game-screen-player-card-container")
    if (playerCardsDisplay) {
        playerCardsDisplay.innerHTML = ""
    }

    const dealerMessageDisplay = document.getElementById("dealer-message-display")
    if (dealerMessageDisplay) {
        dealerMessageDisplay.innerHTML = ""
    }

    
}

function displayWinScreen() {

    const waitingScreen = document.getElementById("waiting-screen");
        if (waitingScreen) {
            waitingScreen.style.display = "none"
        }

    const playerWinScreen = document.getElementById("player-win-screen");
    if(playerWinScreen) {
        playerWinScreen.style.display = "flex"
    }
}

function displayLooseScreen() {
    const waitingScreen = document.getElementById("waiting-screen");
            if (waitingScreen) {
                waitingScreen.style.display = "none"
            }

        const playerLooseScreen = document.getElementById("dealer-win-screen");
        if(playerLooseScreen) {
            playerLooseScreen.style.display = "flex"
        }
}

function backToMainMenu() {
    const playerWinScreen = document.getElementById("player-win-screen");
    if(playerWinScreen) {
        playerWinScreen.style.display = "none"
    }
    const playerLooseScreen = document.getElementById("dealer-win-screen");
    if(playerLooseScreen) {
        playerLooseScreen.style.display = "none"
    }
    const initialDisplay = document.getElementById('initial-display');
    if (initialDisplay) {
        initialDisplay.style.display = 'flex';
    }

    var gamesList = document.getElementById("games-list");
    if (gamesList) {
        // Clear Lobby Screen
        gamesList.innerHTML = "";
    }
    resetGame();

}

function debugPlayerGameScreen() {
    // Hide initial display and start showing the player lobby
    const initialDisplay = document.getElementById('initial-display');
    if (initialDisplay) {
        initialDisplay.style.display = 'none';
    }

    initializePlayerGame("4C,4C", "4D,4D", "Hello")

}

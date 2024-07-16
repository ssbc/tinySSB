function menu_start_game_of_blackjack() {
    closeOverlay();
    document.getElementById('blackjack-start-game').style.display = 'initial';
}

function start_game_as_dealer_button_pressed() {
    // Hide initial display and show button pressed display
    const initialDisplay = document.getElementById('initial-display');
    const buttonPressedDisplay = document.getElementById('button-pressed-display');
    if (initialDisplay) {
        initialDisplay.style.display = 'none';
    }
    if (buttonPressedDisplay) {
        buttonPressedDisplay.style.display = 'initial';
    }

    // Create initial CRDT for the game
    const initialCRDT = createInitialCRDT();
    console.log(initialCRDT); // Debugging purposes

    // Display Game State inside the app
    displayCurrentGameState(initialCRDT); // Pass initialCRDT as argument

    // Sending a test message
    sendMessage("Hello Test")
}

function start_game_as_player_button_pressed() {
    // Hide initial display and show button pressed display
    const initialDisplay = document.getElementById('initial-display');
    const buttonPressedDisplay = document.getElementById('button-pressed-display');
    if (initialDisplay) {
        initialDisplay.style.display = 'none';
    }
    if (buttonPressedDisplay) {
        buttonPressedDisplay.style.display = 'initial';
    }

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

    const dealerCardContainer = document.getElementById('dealer-card-container');
    if (!dealerCardContainer) {
        console.error('Element with id "dealer-card-container" not found.');
        return;
    }
    dealerCardContainer.innerHTML = "";

    currentCRDT.dealerHand.forEach(card => {
        console.log(card);
        const cardImg = document.createElement('img');
        cardImg.src = `img/bj_cards/${card}.svg`;
        cardImg.alt = 'card';
        cardImg.width = 100;
        cardImg.height = 150;
        cardImg.classList.add('card-img');
        dealerCardContainer.appendChild(cardImg);
    })

    const playerCardContainer = document.getElementById('player-card-container');
        if (!playerCardContainer) {
            console.error('Element with id "player-card-container" not found.');
            return;
        }
        playerCardContainer.innerHTML = "";

        currentCRDT.playerHand.forEach(card => {
            console.log(card);
            const cardImg = document.createElement('img');
            cardImg.src = `img/bj_cards/${card}.svg`;
            cardImg.alt = 'card';
            cardImg.width = 100;
            cardImg.height = 150;
            cardImg.classList.add('card-img');
            playerCardContainer.appendChild(cardImg);
        })


}

function displayMessage(message) {
    console.log("MESSAGE RECEIVED: " + message)
}

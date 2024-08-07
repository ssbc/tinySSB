/**
*  ENVIRONMENT VARIABLES
*/
const cards = [
        '2C', '2D', '2H', '2S',
        '3C', '3D', '3H', '3S',
        '4C', '4D', '4H', '4S',
        '5C', '5D', '5H', '5S',
        '6C', '6D', '6H', '6S',
        '7C', '7D', '7H', '7S',
        '8C', '8D', '8H', '8S',
        '9C', '9D', '9H', '9S',
        'TC', 'TD', 'TH', 'TS',
        'JC', 'JD', 'JH', 'JS',
        'QC', 'QD', 'QH', 'QS',
        'KC', 'KD', 'KH', 'KS',
        'AC', 'AD', 'AH', 'AS'
    ];
let myRole = "Undefined"
let myCurrentGameID = "1234"
let dealerCards = []
let playerCards = []
let deck = shuffle(cards)
let gameStatusENV = "";
let playerMoney = 1000
let playerWager = 0
let gamesInMyArea = []



// Function to shuffle the cards
function shuffle(cards) {
    let shuffledCards = [...cards];
    for (let i = shuffledCards.length - 1; i > 0; i--) {
        const j = Math.floor(Math.random() * (i + 1));
        [shuffledCards[i], shuffledCards[j]] = [shuffledCards[j], shuffledCards[i]];
    }
    return shuffledCards;
}

// Function to deal a card from the deck to a specified hand
function dealCard(hand) {
    hand.push(deck.pop());
}

// Generate a random Id between 1111 and 9999
function generateGameID() {
    const min = 1111;
    const max = 9999;
    // Generate a random number between min (inclusive) and max (inclusive)
    const randomId = Math.floor(Math.random() * (max - min + 1)) + min;
    return randomId;
}

function sendMessage(message) {
    backend("blackjack:send " + message) // Sending message for BlackJack application
}

function handleMessage(message) {
    // Phase 1: message should be "BJ gameId gameStatus"
    var gameId = message[1]
    var gameStatus = message[2]
    var turn = message[3]
    var incomingDealerCards = message[4]
    var incomingPlayerCards = message[5]
    var playerAction = message[6]
    var dealerMessage = message[7]

    console.log("myRole: " + myRole);
    console.log("myCurrentGameID: " + myCurrentGameID)
    console.log("gameStatus: " + gameStatus)
    console.log("turn: " + turn)
    console.log("incomingDealerCards: " + incomingDealerCards)
    console.log("incomingPlayerCards: " + incomingPlayerCards)
    console.log("dealerMessage: " + dealerMessage)

    if(myRole === "player" && dealerMessage === "_") {
        displayGameListItem(gameId, gameStatus);
    } else if(myRole === "dealer" && myCurrentGameID.toString() === gameId && gameStatus === "ongoing") {
        initializeGame()
    } else if(myRole === "player" && myCurrentGameID.toString() === gameId && turn === "player" && gameStatus === "initialCards") {
        console.log("0. Player if-else branch reached")
        initializePlayerGame(incomingDealerCards, incomingPlayerCards, dealerMessage)
    } else if(myRole === "dealer" && myCurrentGameID.toString() === gameId && turn === "dealer" && gameStatus === "playerDecision") {
        // Dealer receives a decision from the player
        console.log("Handling player decision")
        handlePlayerDecision(playerAction)
    } else if(myRole === "player" && myCurrentGameID.toString() === gameId && turn === "player" && gameStatus === "ongoingCards") {
        console.log("Player gets to have another decision");
        initializePlayerGame(incomingDealerCards, incomingPlayerCards, dealerMessage)
    } else if(myRole === "player" && myCurrentGameID.toString() === gameId && turn === "player" && gameStatus === "PlayerWins") {
        console.log("Player won the game");
        gameStatusENV = "PlayerWins"
        playerCards = incomingPlayerCards.split(",")
        dealerCards = incomingDealerCards.split(",")
        displayFinalCards()
    } else if(myRole === "player" && myCurrentGameID.toString() === gameId && turn === "player" && gameStatus === "DealerWins") {
        console.log("Dealer won the game");
        gameStatusENV = "DealerWins"
        playerCards = incomingPlayerCards.split(",")
        dealerCards = incomingDealerCards.split(",")
        displayFinalCards()
    } else if (myRole === "player" && myCurrentGameID.toString() === gameId && turn === "player" && gameStatus === "Tie") {
        console.log("Game tied");
        gameStatusENV = "Tie"
        playerCards = incomingPlayerCards.split(",")
        dealerCards = incomingDealerCards.split(",")
        displayFinalCards()
    } else if(myRole === "dealer" && gameStatus === "searching") {
        sendMessage(myCurrentGameID + " " + "open " + "_ " + "_ " + "_ " + "_ " + "_ ")
    } else {
        console.log("No matching condition found.")
    }

}

function handlePlayerDecision(decision) {
    switch (decision) {
        case 'Hit':
            // Add a card to the player's hand
            dealCard(playerCards);
            console.log('Player hits. New hand:', playerCards);
            const playerScore = calculateHandScore(playerCards)
            console.log("Player score: ", playerScore )

            // Checking if player busted
            if (playerScore > 21) {
                console.log("Player score over 21, Player busted")
                gameStatusENV = "DealerWins"
                sendMessage(gameID + " " + "DealerWins " + "player " + dealerCards + " " + playerCards + " " + "_ " + "XXX" )
                displayFinalCards();
                return "dealerWin"
            } else {
                gameStatusENV = "playerHitNoBust"
                initializeGame();
            }


            break;

        case 'Stand':
            // Player chooses to stand, so nothing needs to be done for the player
            console.log('Player stands. Hand remains:', playerCards);
            // In a complete game, the dealer's turn would follow
            let dealerStandScore = dealerTurn(dealerCards);
            console.log("Dealer Hand: ", dealerCards)
            console.log("Dealer Score: ", dealerStandScore)
            const winner = compareScores(calculateHandScore(playerCards), dealerStandScore)
            sendMessage(gameID + " " + winner  + " " + "player " + dealerCards + " " + playerCards + " " + "_ " + "XXX" )
            if (winner === "DealerWins") {
                gameStatusENV = "DealerWins"
                displayFinalCards();
            } else if(winner === "PlayerWins") {
                gameStatusENV = "PlayerWins"
                displayFinalCards();
            } else {
                gameStatusENV = "Tie"
                displayFinalCards();
            }

            break;

        case 'DoubleDown':
            // Add a card to the player's hand and double the bet (betting logic not shown here)
            dealCard(playerCards);
            console.log('Player doubles down. New hand:', playerCards);
            let doubleDownPlayerScore = calculateHandScore(playerCards);
            console.log("Player score after double down:", doubleDownPlayerScore);
            if (doubleDownPlayerScore > 21) {
                console.log("Player Busts")
                gameStatusENV = "DealerWins"
                sendMessage(gameID + " " + "DealerWins"  + " " + "player " + dealerCards + " " + playerCards + " " + "_ " + "XXX" )
                displayFinalCards()
            }
            // Player's turn ends after this, so no further actions for the player
            let dealerScoreAfterDoubleDown = dealerTurn(dealerCards);
            console.log("Dealer Hand: ", dealerCards)
            console.log("Dealer Score: ", dealerScoreAfterDoubleDown)
            const result = compareScores(calculateHandScore(playerCards), dealerScoreAfterDoubleDown)
            gameStatusENV = result
            sendMessage(gameID + " " + result  + " " + "player " + dealerCards + " " + playerCards + " " + "_ " + "XXX" )
            displayFinalCards()

            break;

        case 'Split':
            // Assuming the player can only split pairs
            // Split the player's hand into two hands
            /*

            if (playerHand.length >= 2 && playerHand[0] === playerHand[1]) {
                const newHand1 = [playerHand.shift()]; // First hand
                const newHand2 = [playerHand.shift()]; // Second hand
                dealCard(newHand1); // Deal a card to the first new hand
                dealCard(newHand2); // Deal a card to the second new hand
                console.log('Player splits. New hands:', newHand1, newHand2);
            } else {
                console.log('Cannot split. Player hand does not contain a pair.');
            }
            */
            console.log("Split not implemented.")
            break;

        default:
            console.error('Unknown decision:', decision);
            break;
    }
}

/*
* This function calculates the score of a given hand in Blackjack, considering the value of the
* cards and handling the special case of aces.
*/
function calculateHandScore(hand) {
    let score = 0;
    let numberOfAces = 0;

    // Define card values
    const cardValues = {
        '2': 2, '3': 3, '4': 4, '5': 5, '6': 6,
        '7': 7, '8': 8, '9': 9, 'T': 10, 'J': 10,
        'Q': 10, 'K': 10, 'A': 11
    };

    // Calculate score and count aces
    hand.forEach(card => {
        const cardValue = cardValues[card[0]]; // card[0] is the rank of the card
        score += cardValue;
        if (card[0] === 'A') {
            numberOfAces++;
        }
    });

    // Adjust for aces if score is over 21
    while (score > 21 && numberOfAces > 0) {
        score -= 10; // Convert an ace from 11 to 1
        numberOfAces--;
    }

    return score;
}

/**
* This function handles the dealer's actions according to Blackjack rules, which typically
* involve hitting until the dealer's score is at least 17.
*/
function dealerTurn(dealerHand) {
    let dealerScore = calculateHandScore(dealerHand);

    // Dealer's turn logic
    while (dealerScore < 17) {
        dealCard(dealerHand); // Draw a card from the deck to the dealer's hand
        dealerScore = calculateHandScore(dealerHand);
    }

    return dealerScore;
}

// Helper function to compare scores
function compareScores(playerScore, dealerScore) {
    if (playerScore > 21) {
        return 'DealerWins';
    } else if (dealerScore > 21) {
        return 'PlayerWins';
    } else if (playerScore > dealerScore) {
        return 'PlayerWins';
    } else if (dealerScore > playerScore) {
        return 'DealerWins';
    } else {
        return 'Tie'; // Tie
    }
}

function resetGame() {
    dealerCards = []
    playerCards = []
    deck = shuffle(cards)
}

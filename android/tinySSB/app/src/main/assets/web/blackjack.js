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
var myRole = "Undefined"
var myCurrentGameID = "1234"
var dealerCards = []
var playerCards = []
var deck = shuffle(cards)


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
    var dealerCards = message[4]
    var playerCards = message[5]
    var playerAction = message[6]
    var dealerMessage = message[7]

    console.log("myRole: " + myRole);
    console.log("myCurrentGameID: " + myCurrentGameID)
    console.log("gameStatus: " + gameStatus)
    console.log("turn: " + turn)
    console.log("dealerCards: " + dealerCards)
    console.log("playerCards: " + playerCards)
    console.log("dealerMessage: " + dealerMessage)

    if(myRole === "player" && dealerMessage === "_") {
        displayGameListItem(gameId, gameStatus);
    } else if(myRole === "dealer" && myCurrentGameID.toString() === gameId && gameStatus === "ongoing") {
        initializeGame()
    } else if(myRole === "player" && myCurrentGameID.toString() === gameId && turn === "player") {
        console.log("0. Player if-else branch reached")
        initializePlayerGame(dealerCards, playerCards, dealerMessage)
    }

}



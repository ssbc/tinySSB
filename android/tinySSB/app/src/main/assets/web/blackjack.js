// Function to create initial CRDT for a blackjack game
function createInitialCRDT() {
    // Define an array of all cards in a standard deck (in the format rank_suit)
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

    // Shuffle the deck of cards
    const shuffledDeck = shuffle(cards);

    // Initialize the CRDT object
    const initialCRDT = {
        playerHand: [], // Placeholder for player's hand
        dealerHand: [], // Placeholder for dealer's hand
        deck: shuffledDeck, // Shuffled deck of cards
        gameStatus: 'started'
    };

    // Deal initial cards to player and dealer
    dealCard(initialCRDT.playerHand, initialCRDT);
    dealCard(initialCRDT.dealerHand, initialCRDT);
    dealCard(initialCRDT.playerHand, initialCRDT);
    dealCard(initialCRDT.dealerHand, initialCRDT);

    return initialCRDT;
}

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
function dealCard(hand, initialCRDT) {
    hand.push(initialCRDT.deck.pop());
}

function sendMessage(message) {
    backend("blackjack:send " + message) // Sending message for BlackJack application
}



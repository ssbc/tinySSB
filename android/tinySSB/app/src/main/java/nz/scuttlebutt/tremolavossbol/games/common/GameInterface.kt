package nz.scuttlebutt.tremolavossbol.games.common

import nz.scuttlebutt.tremolavossbol.games.battleships.GameStates

/**
 * This interface defines the necessities for any game.
 */
interface GameInterface {
    var state : GameStates
    var isRunning : Boolean

    /**
     * Every Game needs this function returning its identifier.
     * Make sure the string is unique among all other games.
     */
    override fun toString(): String
}
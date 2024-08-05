package nz.scuttlebutt.tremolavossbol.games.common

import android.util.Log
import nz.scuttlebutt.tremolavossbol.games.battleships.BattleshipGame
import nz.scuttlebutt.tremolavossbol.games.battleships.GameStates

/**
 * This class represents a Instance of a Battleship game. The peerfid is the ID of the opponent.
 * The state marks the progress in the game. STOPPED is the default state.
 */
class GameInstance(gameType: String, fid: String, time: Long, initialState: GameStates) {
    var game : GameInterface? = null
    var participantFid : String = "-"
    var ownerFid : String = "-"
    var startTime : Long = 0
    var state : GameStates = initialState

    init {
        ownerFid = fid
        startTime = time
        game = createGameInstance(gameType)
        if (game == null) {
            Log.d("GameInstance", "Unknown Game.")
        }
    }

    /**
     * This method creates the instance of the desired game.
     */
    private fun createGameInstance(gameType: String): GameInterface? {
        return when (gameType) {
            "BSH" -> BattleshipGame()
            else -> null
        }
    }

    override fun toString(): String {
        return game.toString()
    }

    fun isActive(): Boolean {
        return state.isActive()
    }

    /**
     * This sets the participant's id
     */
    fun setParticipant(part: String) {
        participantFid = part
    }
}


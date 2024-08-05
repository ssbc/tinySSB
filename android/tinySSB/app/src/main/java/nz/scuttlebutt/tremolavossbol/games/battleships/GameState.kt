package nz.scuttlebutt.tremolavossbol.games.battleships
import android.util.Log

/**
 * This class represents the current state of the game.
 *
 * @param shipSizes An array of all the boat sizes
 */
class GameState(
    var turn: Boolean,
    shipSizes: Array<Int>
) {
    private val sizes: Array<Int> = shipSizes
    private val ships =
        Array(
            shipSizes.size
        ) { i ->
            Ship(
                shipSizes[i]
            )
        }
    val shotsFired =
        mutableListOf<Position2D>()
    val shotReceived =
        mutableListOf<Pair<Position2D, ShotOutcome>>()
    val shotsFiredWithOutcome =
        mutableListOf<Pair<Position2D, ShotOutcome>>()
    val moveValidator =
        MoveValidator()
    var enemyHash: String? = null


    /**
     * Sets the boat on your own battlefield
     *
     * @param index: The index of the boat getting set
     * @param x: X coordinate of the anchorpoint
     * @param y: Y coordinate of the anchorpoint
     * @param direction: The direction the boat is facing starting from the anchor point
     */
    fun placeShip(
        index: Int,
        x: Int,
        y: Int,
        direction: Direction
    ): Boolean {
        ships[index].setPosition(
            x,
            y,
            direction
        )
        if (moveValidator.isValidPlacement(
                ships,
                index
            )
        ) return true
        ships[index].setPosition(
            -1,
            -1,
            Direction.UP
        )
        return false
    }

    /**
     * Checks if the enemy has won
     *
     * @return True if the enemy has won
     */
    fun enemyHasWon(): Boolean {
        ships.forEach { ship -> if (!ship.isSunken()) return false }
        return true
    }

    /**
     * Shot sent at enemy, checks whether it is a valid shot
     *
     * @return True if the shot is valid and can be sent to the enemy
     */
    fun shoot(
        x: Int,
        y: Int
    ): Boolean {
        if (!turn) {
            return false
        }
        val shot =
            Position2D(
                x,
                y
            )
        if (!moveValidator.isValidShot(
                shot,
                shotsFired
            )
        ) return false
        shotsFired.add(
            shot
        )
        turn = false
        return true
    }

    /**
     * Takes an incoming shot and returns weather the boat was hit, sunken or
     * the shot missed entirely
     */
    fun receiveShot(x: Int, y: Int): ShotOutcome {
        for (ship in ships) {
            when (ship.isHit(x, y)) {
                ShotOutcome.MISS -> {
                    continue
                }
                ShotOutcome.HIT -> {
                    shotReceived.add(Pair(Position2D(x, y), ShotOutcome.HIT))
                    return ShotOutcome.HIT
                }
                ShotOutcome.SUNKEN -> {
                    shotReceived.removeIf {
                        ship.getPositions().contains(it.first)
                    }
                    ship.getPositions().forEach {
                        shotReceived.add(Pair(it, ShotOutcome.SUNKEN))
                    }
                    return ShotOutcome.SUNKEN
                }
            }
        }
        shotReceived.add(Pair(Position2D(x, y), ShotOutcome.MISS))
        return ShotOutcome.MISS
    }

    /**
     * Get the outcome of a shot from the communication
     */
    fun shotOutcome(x: Int, y: Int, shotOutcome: ShotOutcome) {
        val shotPosition = Position2D(x, y)
        shotsFiredWithOutcome.removeIf {
            it.first == shotPosition
        }
        shotsFiredWithOutcome.add(Pair(Position2D(x, y), shotOutcome))
        if (shotOutcome == ShotOutcome.SUNKEN) {
            changeHitsToSunken(x, y)
        }
    }

    /**
     * Helper-method: splits ship-string (09U28L...) in parts of 3. Each ship in one element.
     */
    fun splitIntoChunks(input: String, chunkSize: Int): List<String> {
        val chunks = mutableListOf<String>()
        var index = 0
        while (index < input.length) {
            val endIndex = Math.min(index + chunkSize, input.length)
            chunks.add(input.substring(index, endIndex))
            index += chunkSize
        }
        return chunks
    }

    /**
     * This method returns a list of all cells in the game, which are being occupied by the same ship
     * given as parameters (f.e. 09U) and its length. It is used together with changeHitsToSunken.
     */
    fun getShipPositions(ship: String, length: Int): List<Position2D> {
        val positions = mutableListOf<Position2D>()
        val x = ship[0].digitToInt()
        val y = ship[1].digitToInt()
        val direction = ship[2]

        for (i in 0 until length) {
            when (direction) {
                'U' -> positions.add(Position2D(x, y - i))
                'D' -> positions.add(Position2D(x, y + i))
                'L' -> positions.add(Position2D(x - i, y))
                'R' -> positions.add(Position2D(x + i, y))
            }
        }
        return positions
    }

    /**
     * This function is being used to update previous hits on the same boat to sunken.
     * This is needed to visualize the game properly.
     */

    fun changeHitsToSunken(x: Int, y: Int) {
        try {
            Log.d("changeHitsToSunken", "Starting changeHitsToSunken with coordinates ($x, $y)")

            // Teilt den enemyHash in 3er Gruppen auf
            val enemyShips = splitIntoChunks(enemyHash ?: "", 3)
            Log.d("changeHitsToSunken", "Enemy ships split into chunks: $enemyShips")

            var iteration = 0
            while (iteration < enemyShips.size) {
                val ship = enemyShips[iteration]
                Log.d("changeHitsToSunken", "Processing ship: $ship at iteration $iteration")

                // Ermittelt die Positionen des aktuellen Schiffs
                val shipPositions = getShipPositions(ship, sizes[iteration])
                Log.d("changeHitsToSunken", "Ship positions: $shipPositions ${sizes[iteration]}")

                // Prüft, ob die übergebene Position Teil dieses Schiffs ist
                if (shipPositions.contains(Position2D(x, y))) {
                    Log.d("changeHitsToSunken", "Position ($x, $y) is part of ship $ship")

                    // Aktualisiert die Positionen des Schiffs auf SUNKEN
                    shipPositions.forEach { pos ->
                        shotsFiredWithOutcome.removeIf { it.first == pos }
                        shotsFiredWithOutcome.add(Pair(pos, ShotOutcome.SUNKEN))
                        Log.d("changeHitsToSunken", "Updated position $pos to SUNKEN")
                    }
                }
                iteration += 1
            }
        } catch (e: Exception) {
            Log.d("changeHitsToSunken", "${e}")
        }

    }

    /**
     * Serializes the game state information for communication to the frontend
     */
    fun serialize(): String {
        val state = StringBuilder()
        ships.forEach {
            state.append(it.getPositions()[0].getXPosition())
                .append(it.getPositions()[0].getYPosition())
                .append(it.getDirection())
        }
        state.append("^")
        shotsFiredWithOutcome.forEach {
            state.append(it.first.getXPosition())
                .append(it.first.getYPosition())
                .append(it.second)
        }
        state.append("^")
        shotReceived.forEach {
            state.append(it.first.getXPosition())
                .append(it.first.getYPosition())
                .append(it.second)
        }
        return state.toString()
    }

    fun getShipAtPosition(x: Int, y: Int): Array<Position2D> {
        val position = Position2D(x, y)
        ships.forEach { ship ->
            if (ship.getPositions().contains(position))
                return ship.getPositions()
        }
        return emptyArray()
    }

    /**
     * Returns a serialized string of all ship positions
     */
    fun getShipPosition(): String {
        val state = StringBuilder()
        ships.forEach {
            state.append(it.getPositions()[0].getXPosition())
                .append(it.getPositions()[0].getYPosition())
                .append(it.getDirection())
        }
        return state.toString()
    }

    /**
     * This method returns a string for all moves the owner of the game has done.
     */
    fun shotsFiredWithOutcomeToString(): String {
        val result = StringBuilder()
        shotsFiredWithOutcome.forEach { (position, outcome) ->
            result.append("${position.getXPosition()}${position.getYPosition()}${outcome}")
        }
        if (result.isEmpty()) {
            return "-"
        }
        return result.toString()
    }

    fun isMyTurn(): Boolean {
        return turn
    }

    /**
     * This method returns a string for all moves the peer of the game has done.
     */
    fun shotsReceivedWithOutcomeToString(): String {
        val result = StringBuilder()
        shotReceived.forEach { (position, outcome) ->
            result.append("${position.getXPosition()}${position.getYPosition()}${outcome}")
        }
        if (result.isEmpty()) {
            return "-"
        }
        return result.toString()
    }

    /**
     * This method is used for the spectator mode. It replaces the previous ship's of the owner,
     * with the correct ships, which the owner has sent via DUELACC.
     */
    fun overwriteSpectatorOwnerShip(s: String) {
        val boats = splitIntoChunks(s, 3)
        var iterations = 0
        boats.forEach {
            ships[iterations] = Ship(sizes[iterations], it.substring(0,1).toInt(), it.substring(1,2).toInt(),
                Direction.fromString(it.substring(2,3))!!
            )
        }
    }
}
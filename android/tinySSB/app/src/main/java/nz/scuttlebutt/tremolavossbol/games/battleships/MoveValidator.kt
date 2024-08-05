package nz.scuttlebutt.tremolavossbol.games.battleships

import android.util.Log

class  MoveValidator {

    /**
     * Checks if an outgoing shot is out of bounds of the board or a duplicate
     */
    fun isValidShot(
        position2D: Position2D,
        shotsFired: MutableList<Position2D>
    ): Boolean {
        if (!inRange(
                position2D.getXPosition()
            ) || !inRange(
                position2D.getYPosition()
            )
        ) {
            return false
        }
        shotsFired.forEach { position -> if (position == position2D) return false }
        return true
    }

    /**
     * Checks if an incoming shot is out of bounds of the board
     */
    fun isValidShot(
        position2D: Position2D
    ): Boolean {
        if (!inRange(
                position2D.getXPosition()
            ) || !inRange(
                position2D.getYPosition()
            )
        ) {
            return false
        }
        return true
    }

    /**
     * Checks whether the ships' placement is not out of bounds and does not overlap with existing ships
     *
     * @param ships: The array of already placed ships including the newly placed
     * @param index: The index of the newly placed ship
     */
    fun isValidPlacement(
        ships: Array<Ship>,
        index: Int
    ): Boolean {
        val placedShip =
            ships[index]
        if (placedShip.isOutOfBounds()) {
            Log.d(
                "Placed Ship",
                "${placedShip.getPositions()[0].getXPosition()}, ${placedShip.getPositions()[0].getYPosition()}, ${placedShip.getDirection()} is out of bounds"
            )
            return false
        }
        var hasOverlap =
            false
        ships.forEachIndexed { i, ship ->
            if (i != index) {
                hasOverlap = hasOverlap || placedShip.intersects(ship)
            }
        }
        return !hasOverlap
    }


    fun Ship.isOutOfBounds(): Boolean {
        for (position in this.getPositions()) {
            if (position.getXPosition() <= 0 || position.getXPosition() >= 9 ||
                position.getYPosition() <= 0 || position.getYPosition() >= 9) {
                return true
            }
        }
        return false
    }

    fun Ship.intersects(other: Ship): Boolean {
        val thisPositions = this.getPositions().toSet()
        val otherPositions = other.getPositions().toSet()
        return thisPositions.intersect(otherPositions).isNotEmpty()
    }

    /**
     * Helper function to determine the out-of-bounds coordinates
     */
    private fun inRange(
        position: Int
    ): Boolean {
        if (position < 0 || position > 9) {
            return false
        }
        return true
    }
}
package nz.scuttlebutt.tremolavossbol.games.battleships

import android.util.Log

/**
 * This class represents a ship of the battleship game.
 * It comes with a range of useful functions and keeps track of how many hits it got.
 *
 * @param length The length of the ship
 * @param x The x coordinate of the anchor point of the ship (an outer point)
 * @param y The y coordinate of the anchor point
 * @param direction The direction the ship extends in starting from the anchor point
 */
class Ship(
    private val length: Int,
    x: Int = -1,
    y: Int = -1,
    private var direction: Direction = Direction.UP
) {
    // Holds the anchor position of the ship
    private val position =
        Position2D(
            x,
            y
        )

    // Boolean array, true where the ship got hit
    private val hits =
        Array(
            length
        ) { false }

    init {
        if (length <= 0) {
            throw java.lang.IllegalArgumentException(
                "Ship length must be greater than zero"
            )
        }
    }

    /**
     * For a given shot, checks if this ship got hit
     *
     * @param x X coordinate of the fired shot
     * @param y Y coordinate of the fired shot
     * @return Whether it is hit, sunken or missed
     */
    fun isHit(
        x: Int,
        y: Int
    ): ShotOutcome {
        if ((x != position.getXPosition()) && y != position.getYPosition()) {
            Log.d(
                "Is Hit",
                "${position.getXPosition()}, ${position.getYPosition()}, ${direction}, Total Miss"
            )
            return ShotOutcome.MISS
        }
        val hitPosition =
            Position2D(
                x,
                y
            )
        for (i in 0 until length) {
            when (direction) {
                Direction.DOWN -> hitPosition.setYPosition(
                    y - i
                )
                Direction.UP -> hitPosition.setYPosition(
                    y + i
                )
                Direction.RIGHT -> hitPosition.setXPosition(
                    x - i
                )
                Direction.LEFT -> hitPosition.setXPosition(
                    x + i
                )
            }
            if (hitPosition == position) {
                hits[i] =
                    true
                if (isSunken()) {
                    return ShotOutcome.SUNKEN
                }
                return ShotOutcome.HIT
            }
        }
        return ShotOutcome.MISS
    }

    /**
     * Checks if this ship is sunken. It's only sunken if every position of the ship got hit.
     */
    fun isSunken(): Boolean {
        var sunken = true
        for (i in 0 until length) {
            sunken = sunken && hits[i]
        }
        return sunken
    }

    /**
     * Sets the anchor position and the direction of the ship
     *
     * @param x The x coordinate of the anchor point
     * @param y The y coordinate of the anchor point
     * @param direction The direction the ship will be facing
     */
    fun setPosition(
        x: Int,
        y: Int,
        direction: Direction
    ) {
        position.setPosition(
            x,
            y
        )
        this.direction =
            direction
    }

    /**
     * Checks each ship position against each other ship position ot see if they have overlap.
     * This is really slow but since the ship size is at maximum 5 it should be fine
     */
    fun intersects(other: Ship): Boolean {
        val thisPositions = getPositions()
        val otherPositions = other.getPositions()

        return thisPositions.any { it in otherPositions }
    }

    /**
     * Checks if the ships is partly or fully out of bounds of the 10x10 board
     *
     * @return True if it is out of bounds
     */
    fun isOutOfBounds(): Boolean {
        val x = position.getXPosition()
        val y = position.getYPosition()
        val gridSize = 10

        if (x < 0 || x >= gridSize || y < 0 || y >= gridSize) {
            return true
        }

        return when (direction) {
            Direction.UP -> y - (length - 1) < 0
            Direction.DOWN -> y + (length - 1) >= gridSize
            Direction.RIGHT -> x + (length - 1) >= gridSize
            Direction.LEFT -> x - (length - 1) < 0
        }
    }

    /**
     * Returns an array of all positions this ships inhabits
     */
    fun getPositions(): Array<Position2D> {
        return Array(length) { i ->
            getPosition(
                i
            )
        }
    }


    /**
     * Gets the i-th position of the ship
     */
    private fun getPosition(i: Int): Position2D {
        return when (direction) {
            Direction.UP -> Position2D(position.getXPosition(), position.getYPosition() - i)
            Direction.DOWN -> Position2D(position.getXPosition(), position.getYPosition() + i)
            Direction.RIGHT -> Position2D(position.getXPosition() + i, position.getYPosition())
            Direction.LEFT -> Position2D(position.getXPosition() - i, position.getYPosition())
        }
    }

    /**
     * Simple getter for the ship direction
     */
    fun getDirection(): Direction {
        return direction
    }
}
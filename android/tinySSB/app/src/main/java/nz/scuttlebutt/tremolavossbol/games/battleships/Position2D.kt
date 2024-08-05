package nz.scuttlebutt.tremolavossbol.games.battleships

/**
 * This class is a simple 2D vector with limited functionality. It can mainly
 * hold two integers and check for equality and has some getters and setters.
 */
class Position2D(
    private var x: Int,
    private var y: Int
) {

    fun setPosition(
        x: Int,
        y: Int
    ) {
        this.x =
            x
        this.y =
            y
    }

    fun getXPosition(): Int {
        return this.x
    }

    fun getYPosition(): Int {
        return this.y
    }

    fun setXPosition(
        x: Int
    ) {
        this.x =
            x
    }

    fun setYPosition(
        y: Int
    ) {
        this.y =
            y
    }

    override fun equals(
        other: Any?
    ): Boolean {
        if (other is Position2D) {
            return (other.getYPosition() == this.y) && (other.getXPosition() == this.x)
        }
        return false
    }

    override fun hashCode(): Int {
        var result =
            x
        result =
            31 * result + y
        return result
    }
}
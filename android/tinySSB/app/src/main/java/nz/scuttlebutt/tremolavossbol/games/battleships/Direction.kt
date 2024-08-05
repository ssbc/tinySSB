package nz.scuttlebutt.tremolavossbol.games.battleships

/**
 * Simple enum to define directions on the playing field
 */
enum class Direction(val string: String) {
    UP("UP"),
    DOWN("DOWN"),
    RIGHT("RIGHT"),
    LEFT("LEFT");

    override fun toString(): String {
        when (this.string) {
            "UP" -> {
                return "U"
            }
            "DOWN" -> {
                return "D"
            }
            "LEFT" -> {
                return "L"
            }
            "RIGHT" -> {
                return "R"
            }
        }
        return ""
    }

    companion object {
        fun fromString(s: String): Direction?{
            when (s) {
                "U" -> {
                    return UP
                }
                "D" -> {
                    return DOWN
                }
                "L" -> {
                    return LEFT
                }
                "R" -> {
                    return RIGHT
                }
            }
            return null
        }
    }
}
package nz.scuttlebutt.tremolavossbol.games.battleships

/**
 * Enum to describe the outcome of a shot in the game
 */
enum class ShotOutcome(val string: String) {
    MISS("M"),
    HIT("H"),
    SUNKEN("S");

    override fun toString(): String {
        return this.string
    }

    companion object {
        fun getFromString(s: String): ShotOutcome? {
            if (s == "M") {
                return MISS
            } else if (s == "S") {
                return SUNKEN
            } else if (s == "H") {
                return HIT
            }
            return null
        }
    }
}
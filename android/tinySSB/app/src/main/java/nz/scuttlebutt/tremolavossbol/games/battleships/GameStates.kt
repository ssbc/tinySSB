package nz.scuttlebutt.tremolavossbol.games.battleships

enum class GameStates(val string: String) {
    STOPPED ("STOPPED"),
    INVITED ("INVITED"),
    WAITING ("WAITING"),
    WON ("WON"),
    LOST ("LOST"),
    SPEC ("SPECTATE"),
    RUNNING ("RUNNING");

    override fun toString(): String {
        return this.string
    }

    /**
     * This method returns, whether a given GameInstance is still ongoing.
     */
    fun isActive(): Boolean {
        return (this == RUNNING || this == INVITED || this == WAITING || this == SPEC)
    }
}
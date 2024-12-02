package nz.scuttlebutt.tremolavossbol.tssb.ble

enum class ForegroundNotificationType(val value: String) {
    EVALUATION("EVALUATION"),
    TINY_EVENT("TINY_EVENT"),
    INCOMPLETE_EVENT("INCOMPLETE_EVENT"),
    EDIT_FRONTEND_FRONTIER("EDIT_FRONTEND_FRONTIER")
}
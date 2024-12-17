package nz.scuttlebutt.tremolavossbol.tssb.ble

enum class ApplicationNotificationType(val type: String) {
    PUBLISH_PUBLIC_CONTENT("PUBLISH_PUBLIC_CONTENT"), // ByteArray
    REPO_GET_REPLICA("REPO_GET_REPLICA"), // ByteArray -> Replica?
    ADD_NUMBER_OF_PENDING_CHUNKS("ADD_NUMBER_OF_PENDING_CHUNKS"), // Int
    BEACON("BEACON"),
    IDENTITY_TO_REF("IDENTITY_TO_REF"), // -> ByteArray
    DELETE_FEED("DELETE_FEED"), // ByteArray
    VERIFY_KEY("VERIFY_KEY"), // -> ByteArray
    TO_EXPORT_STRING("TO_EXPORT_STRING"), // -> String?
    SET_NEW_IDENTITY("SET_NEW_IDENTITY"), // ByteArray -> Boolean
    RESET("RESET"),
    STOP_BLUETOOTH("STOP_BLUETOOTH"),
    IS_BLE_ENABLED("IS_BLE_ENABLED"), // -> Boolean
    RESET_TO_DEFAULT("RESET_TO_DEFAULT"),
    GET_SETTINGS("GET_SETTINGS"), // -> String?
    LIST_FEEDS("LIST_FEEDS"), // -> List<ByteArray>
    IS_GEO_ENABLED("IS_GEO_ENABLED"), // -> Boolean
    SET_SETTINGS("SET_SETTINGS"), // String -> ByteArray
    ADD_KEY("ADD_KEY") // ByteArray
}
package nz.scuttlebutt.tremolavossbol.utils

import kotlin.math.max
import kotlin.math.min
import kotlin.math.pow

/**
 * Utility class to encode and decode location coordinates to/from a Google Plus Code.
 *
 * Created by translating [this](https://github.com/google/open-location-code/blob/main/java/src/main/java/com/google/openlocationcode/OpenLocationCode.java)
 * Java code (freely provided by Google) to Kotlin
 */
object PlusCodesUtils {
    private const val CODE_ALPHABET: String = "23456789CFGHJMPQRVWX"
    private const val SEPARATOR: Char = '+'
    private const val MAX_DIGIT_COUNT: Int = 15
    private const val PAIR_CODE_LENGTH: Int = 10
    private const val GRID_CODE_LENGTH: Int = MAX_DIGIT_COUNT - PAIR_CODE_LENGTH
    private const val ENCODING_BASE = CODE_ALPHABET.length
    private const val LATITUDE_MAX: Int = 90
    private const val LONGITUDE_MAX: Int = 180
    private const val GRID_COLUMNS: Int = 4
    private const val GRID_ROWS: Int = 5
    private const val LAT_INTEGER_MULTIPLIER: Long = 8000 * 3125
    private const val LNG_INTEGER_MULTIPLIER: Long = 8000 * 1024
    private const val LAT_MSP_VALUE: Long = LAT_INTEGER_MULTIPLIER * ENCODING_BASE * ENCODING_BASE
    private const val LNG_MSP_VALUE: Long = LNG_INTEGER_MULTIPLIER * ENCODING_BASE * ENCODING_BASE

    fun encode(latitude: Double, longitude: Double): String {
        var mutableLatitude = clipLatitude(latitude)

        // Latitude 90 needs to be adjusted to be just less, so the returned code can also be decoded.
        if (mutableLatitude == LATITUDE_MAX.toDouble()) {
            mutableLatitude = 89.9998875
        }

        val revBuilder = StringBuilder()
        var latVal = getLatVal(mutableLatitude)
        var lngVal = getLngVal(normalizeLongitude(longitude))

        for (i in 0..4) {
            revBuilder.append(CODE_ALPHABET[(lngVal % ENCODING_BASE).toInt()])
            revBuilder.append(CODE_ALPHABET[(latVal % ENCODING_BASE).toInt()])
            latVal /= ENCODING_BASE
            lngVal /= ENCODING_BASE

            if (i == 0) {
                revBuilder.append(SEPARATOR)
            }
        }
        return revBuilder.reverse().toString()
    }

    private fun clipLatitude(latitude: Double): Double {
        return min(max(latitude, -LATITUDE_MAX.toDouble()), LATITUDE_MAX.toDouble())
    }

    private fun normalizeLongitude(longitude: Double): Double {
        if (longitude >= -LONGITUDE_MAX && longitude < LONGITUDE_MAX)
            return longitude
        val circleDeg = 2 * LONGITUDE_MAX
        return (longitude % circleDeg + circleDeg + LONGITUDE_MAX) % circleDeg - LONGITUDE_MAX

    }

    private fun getLatVal(latitude: Double) =
        (Math.round((latitude + LATITUDE_MAX) * LAT_INTEGER_MULTIPLIER * 1e6) / 1e6 /
                GRID_ROWS.toDouble().pow(GRID_CODE_LENGTH)).toLong()

    private fun getLngVal(longitude: Double) =
        (Math.round((longitude + LONGITUDE_MAX) * LNG_INTEGER_MULTIPLIER * 1e6) / 1e6 /
                GRID_COLUMNS.toDouble().pow(GRID_CODE_LENGTH)).toLong()

    fun decode(code: String): Pair<Double, Double> {
        val clean = code.filterNot { it == SEPARATOR }

        var latVal = -LATITUDE_MAX * LAT_INTEGER_MULTIPLIER
        var lngVal = -LONGITUDE_MAX * LNG_INTEGER_MULTIPLIER

        var latPlaceVal = LAT_MSP_VALUE
        var lngPlaceVal = LNG_MSP_VALUE
        for (i in clean.indices step 2) {
            latPlaceVal /= ENCODING_BASE
            lngPlaceVal /= ENCODING_BASE
            latVal += CODE_ALPHABET.indexOf(clean[i]) * latPlaceVal
            lngVal += CODE_ALPHABET.indexOf(clean[i + 1]) * lngPlaceVal
        }
        return Pair(latVal.toDouble() / LAT_INTEGER_MULTIPLIER,
            lngVal.toDouble() / LNG_INTEGER_MULTIPLIER)
    }
}

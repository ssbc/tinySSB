package nz.scuttlebutt.tremolavossbol.utils

import android.util.Base64
import kotlin.math.abs

class HelperFunctions {

    companion object {

        @JvmStatic
        fun ByteArray.toInt32(): Int { // big endian
            var v = 0
            for (i in 0..3) {
                v = (v shl 8) or (this[i].toInt() and 0xFF)
            }
            return v
        }

        @JvmStatic
        fun Int.toByteArray(): ByteArray { // big endian
            val a = ByteArray(4)
            var v = this
            for (i in 3 downTo 0) {
                a[i] = (v and 0xFF).toByte()
                v = v shr 8
            }
            return a
        }

        @JvmStatic
        fun String.decodeHex(): ByteArray {
            check(length % 2 == 0) { "Must have an even length" }

            return chunked(2)
                .map { it.toInt(16).toByte() }
                .toByteArray()
        }

        @JvmStatic
        fun String.deRef(): ByteArray { // only works for IDs, but not msg keys or blob hashes
            // Log.d("deRef", "<" + this + ">")
            val s = this.slice(1..this.lastIndex).removeSuffix(".ed25519")
            return Base64.decode(s, Base64.NO_WRAP)
        }

        @JvmStatic
        fun ByteArray.toHex(): String = joinToString("") { b ->
            "%02x".format(b)
        }

        @JvmStatic
        fun ByteArray.toBase64(): String {
            return Base64.encodeToString(this, Base64.NO_WRAP)
        }

        @JvmStatic
        fun ByteArray.utf8(): String {
            return this.toString(Charsets.UTF_8)
        }

        @JvmStatic
        fun byteArrayCmp(a: ByteArray, b: ByteArray): Int {
            for (i in 0..a.lastIndex) {
                val d = (a[i].toInt() and 0xff) - (b[i].toInt() and 0xff)
                if (d != 0)
                    return d
            }
            return 0
        }

        @JvmStatic
        fun Int.toEncodedUByte(): Byte { // argh, no unsigned bytes in Kotlin
            if (this < 0 || this > 255) throw Exception("Int val is supposed to be between 0..255)")
            if (this < 128) return this.toByte()
            return (this - 256).toByte()
        }

        @JvmStatic
        fun Byte.fromEncodedUByte(): Int {
            if (this >= 0) return this.toInt()
            return 256 - abs(this.toInt())
        }

        private const val b32encMap = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"

        private fun b32encDo40bits(b40: ByteArray): String {
            var long = 0L
            var s = ""
            for (i in 0 until 5) long = long * 256 + b40[i].fromEncodedUByte()
            for (i in 0 until 8) {
                s = b32encMap[(long and 0x1f).toInt()] + s
                long /= 32
            }
            return s
        }
        @JvmStatic
        fun ByteArray.b32encode(): String {
            var b32 = ""
            var cnt = this.size % 5
            var buf = if (cnt == 0) ByteArray(this.size) else ByteArray(this.size + 5 - cnt)
            for (i in this.indices) {
                buf[i] = this[i]
            }
            while (buf.isNotEmpty()) {
                b32 += b32encDo40bits(buf.sliceArray(0 until 5))
                buf = buf.sliceArray(5 until buf.size)
            }
            if (cnt != 0) {
                cnt = (8 * (5 - cnt) / 5).toInt()
                b32 = b32.substring(0, b32.length - cnt) + "======".substring(0, cnt)
            }
            return b32
        }
    }
}

class Json_PP() { // pretty printing for JSON output
    var output = ""

    fun makePretty(str: String): String {
        output = ""
        // val todo =
        pp(0, str)
        // println("<${todo}>")

        return output
    }

    private fun skipWhite(s: String): String {
        var offs = 0
        while (s[offs] == ' ' || s[offs] == '\t' || s[offs] == '\n')
            offs++
        return s.slice(offs .. s.lastIndex)
    }

    private fun pp(lev: Int, str: String): String { // JSON pretty printing
        var s = skipWhite(str)
        if (s[0] == '{') {
            if (s[1] == '}') {
                output += "{}"
                return s.slice(2 .. s.lastIndex)
            }
            output += "{\n"
            s = skipWhite(s.slice(1 .. s.lastIndex))
            while (true) {
                for (i in 0 .. lev)
                    output += "  "
                s = skipWhite(pp(lev+1, s))
                if (s[0] != ':')
                    return "?:?"
                output += ": "
                s = skipWhite(pp(lev+1, s.slice(1 .. s.lastIndex)))
                if (s[0] == '}')
                    break
                if (s[0] == ',') {
                    output += ",\n"
                    s = s.slice(1 .. s.lastIndex)
                } else {
                    output += "?!?"
                    return s.slice(1 .. s.lastIndex)
                }
            }
            output += "\n"
            for (i in 1 .. lev)
                output += "  "
            output += "}"
            return s.slice(1 .. s.lastIndex)
        }
        if (s[0] == '[') {
            if (s[1] == ']') {
                output += "[]"
                return s.slice(2 .. s.lastIndex)
            }
            output += "[\n"
            s = skipWhite(s.slice(1 .. s.lastIndex))
            while (true) {
                for (i in 0 .. lev)
                    output += "  "
                s = skipWhite(pp(lev+1, s))
                if (s[0] == ']')
                    break
                if (s[0] == ',') {
                    output += ",\n"
                    s = s.slice(1 .. s.lastIndex)
                } else {
                    output += "?!?"
                    return s.slice(1 .. s.lastIndex)
                }
            }
            output += "\n"
            for (i in 1 .. lev)
                output += "  "
            output += "]"
            return s.slice(1 .. s.lastIndex)
        }
        if (s[0] == '"') {
            output += "\""
            var offs = 1
            while (true) {
                if (s[offs] == '\\') {
                    output += s.slice(offs .. offs+1)
                    offs += 2
                    continue
                }
                if (s[offs] == '"') {
                    output += "\""
                    return s.slice(offs+1 .. s.lastIndex)
                }
                output += s.slice(offs .. offs)
                offs++
            }
        }
        if (s.slice(0..3) == "null") {
            output += "null"
            return s.slice(4 .. s.lastIndex)
        }
        if (s.slice(0..3) == "true") {
            output += "true"
            return s.slice(4 .. s.lastIndex)
        }
        if (s.slice(0..4) == "false") {
            output += "false"
            return s.slice(5 .. s.lastIndex)
        }
        if (s[0] >= '0' && s[0] <= '9') {
            var offs = 0
            while ((s[offs] >= '0' && s[offs] <= '9') || s[offs] == '.')
                offs++
            output += s.slice(0 .. offs-1)
            return s.slice(offs .. s.lastIndex)
        }
        return "!!??"
    }
}
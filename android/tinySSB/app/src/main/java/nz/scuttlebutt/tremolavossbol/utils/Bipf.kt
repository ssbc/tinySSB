package nz.scuttlebutt.tremolavossbol.utils

import android.util.Log
import nz.scuttlebutt.tremolavossbol.utils.Bipf.Companion.BIPF_BOOLNONE
import kotlin.experimental.and
import kotlin.experimental.or

import nz.scuttlebutt.tremolavossbol.utils.Bipf.Companion.BIPF_BYTES
import nz.scuttlebutt.tremolavossbol.utils.Bipf.Companion.BIPF_INT
import nz.scuttlebutt.tremolavossbol.utils.Bipf.Companion.BIPF_LIST
import nz.scuttlebutt.tremolavossbol.utils.Bipf.Companion.BIPF_STRING
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toBase64
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toHex
import org.json.JSONArray
import org.json.JSONObject

class Bipf_e(t: Int) {
    var typ: Int = t
    var cnt: Int = 0 // number of elements in LIST and DICT
    var v: Any? = null
    fun getString(): String   { if (typ == BIPF_STRING) return (v as ByteArray).decodeToString()
                                else throw Exception("BIPF type is ${typ} instead of STRING")}
    fun getBytes(): ByteArray { if (typ == BIPF_BYTES) return v as ByteArray
                                else throw Exception("BIPF type is ${typ} instead of BYTES")}
    fun getInt(): Int         { if (typ == BIPF_INT) return v as Int
                                else throw Exception("BIPF type is ${typ} instead of INT")}
    fun getList(): ArrayList<Bipf_e> { if (typ == BIPF_LIST) return v as ArrayList<Bipf_e>
                                else throw Exception("BIPF type is ${typ} instead of LIST")}
    fun getBoolean(): Boolean { if (typ == BIPF_BOOLNONE && v as Int >= 0) return (v as Int) > 0
                                else throw Exception("BIPF type is ${typ} instead of BOOL")}
    fun isNone(): Boolean     { return typ == BIPF_BOOLNONE && (v as Int) == -1 }
}

class Bipf {
    companion object {
        val BIPF_STRING = 0
        val BIPF_BYTES = 1
        val BIPF_INT = 2
        val BIPF_DOUBLE = 3
        val BIPF_LIST = 4
        val BIPF_DICT = 5
        val BIPF_BOOLNONE = 6
        val BIPF_RESERVED = 7
        val BIPF_EMPTY = 255

        val TAG_SIZE = 3
        val TAG_MASK = 7

        @JvmStatic
        fun mkBool(b: Boolean): Bipf_e {
            val e = Bipf_e(BIPF_BOOLNONE)
            e.v = if (b) 1 else 0
            return e
        }

        @JvmStatic
        fun mkBytes(buf: ByteArray): Bipf_e {
            // Log.d("bipf", "mkBytes ${buf.size}")
            val e = Bipf_e(BIPF_BYTES)
            e.v = buf.clone()
            return e
        }

        @JvmStatic
        fun mkInt(i: Int): Bipf_e {
            val e = Bipf_e(BIPF_INT)
            e.v = i
            return e
        }

        @JvmStatic
        fun mkNone(): Bipf_e {
            // Log.d("bipf", "None")
            val e = Bipf_e(BIPF_BOOLNONE)
            e.v = -1
            return e
        }

        @JvmStatic
        fun mkList(): Bipf_e {
            val e = Bipf_e(BIPF_LIST)
            e.v = ArrayList<Bipf_e>()
            return e
        }

        @JvmStatic
        fun mkString(s: String): Bipf_e {
            val e = Bipf_e(BIPF_STRING)
            e.v = s.encodeToByteArray()
            return e
        }

        @JvmStatic
        fun list_append(lst: Bipf_e, e: Bipf_e) {
            val alst = lst.v as ArrayList<Bipf_e>
            alst.add(e)
            lst.cnt++
        }

        @JvmStatic
        fun varint_decode(buf: ByteArray, old: Int, maxPos: Int): Pair<Int,Int> {
            var v = 0
            var shift = 1
            var pos = old
            while (pos < maxPos) {
                val b = buf[pos];
                v = v or ((b and 0x7f) * shift)
                if ((b and 0x80.toByte()) == 0.toByte() )
                  break;
                shift *= 128;
                pos += 1;
            }
            // Log.d("bipf", "varint decode x${buf.sliceArray(old..pos-1).toHex()} -> ${v}")
            return v to (pos - old + 1)
        }

        @JvmStatic
        fun varint_encode(v: Int): ByteArray {
            var buf = ByteArray(0)
            var v2 = v
            while (true) {
                var i: Byte = (v2 and 0x7f).toByte()
                v2 = v2 shr 7
                if (v2 != 0)
                    i = i or 128.toByte()
                buf += byteArrayOf(i)
                if (v2 == 0) {
                    // Log.d("bipf", "varint encode ${v} -> x${buf.toHex()}")
                    return buf
                }
            }
        }

        @JvmStatic
        fun _decode_body(tag: Int, buf: ByteArray, pos: Int, maxPos: Int): Pair<Bipf_e?,Int> {
            // if (tag == BIPF_BOOLNONE) { //  i.e., length is 0
            //     return Bipf_e(BIPF_BOOLNONE) to 0
            // }
            val t = tag and TAG_MASK
            val sz = tag shr TAG_SIZE
            val lim = pos + sz // one beyond last valid pos
            // Log.d("bipf inner", "t=${t}, sz=${sz}, pos=${pos}, maxPos=${maxPos} lim=${lim}")
            if (lim > maxPos)
                return null to -1
            // Serial.println(" inner dec: t=" + String(t) + " sz=" + String(sz) + " pos=" + String(pos));
            when (t) {
                BIPF_BOOLNONE -> {
                    val e = Bipf_e(BIPF_BOOLNONE)
                    if (sz == 0) e.v = -1
                    else e.v = (buf[pos] != 0.toByte()) as Boolean
                    return e to sz
                }
                BIPF_BYTES -> {
                    val e = Bipf_e(BIPF_BYTES)
                    // e.cnt = sz
                    e.v = buf.sliceArray(pos..pos+sz-1)
                    return e to sz
                }
                BIPF_INT -> {
                    var p = pos
                    var v = 0
                    var i = 0
                    while (i < 8*sz) { // little endian
                        val b = buf[p].toInt()
                        v = v or ((if (b >= 0) b else (256 + b)) shl i)
                        p++
                        i += 8
                    }
                    val m = 1 shl (i-1)
                    val e = Bipf_e(BIPF_INT)
                    if ((v and m) == 0)
                        e.v = v
                    else
                        e.v = v - (m shl 1)
                    // Log.d("bipf", "int ${e.v}, m=${m}")
                    return e to (p - pos)
                }
                BIPF_LIST -> {
                    // Log.d("bipf", "list")
                    var p = pos
                    val lst = mkList()
                    while (p < lim) {
                        val (e, szE) = decode(buf, p, lim)
                        if (e == null)
                            return null to -1
                        list_append(lst, e)
                        p += szE
                    }
                    return lst to (p - pos)
                }
                BIPF_STRING -> {
                    val e = Bipf_e(BIPF_STRING)
                    // e.cnt = sz
                    e.v = buf.sliceArray(pos..pos+sz-1)
                    return e to sz
                }
                else -> {
                    Log.d("bipf", " not implemented or wrong tag ${tag} ${pos}");
                    return null to -1
                }
            }
        }

        @JvmStatic
        fun decode(buf: ByteArray, pos: Int, maxPos: Int): Pair<Bipf_e?, Int> {
            // read the next value from buffer at start.
            // returns a tuple with the value and the consumed bytes
            val (tag, sz1) = varint_decode(buf, pos, maxPos)
            // Serial.println("bipf tag " + String(tag,DEC) + "/" + String(tag>>3) + " " + String(sz1) + " " + String(sz2));
            val (e, sz2) = _decode_body(tag, buf, pos + sz1, maxPos)
            // Log.d("bipf", "decode ${buf.sliceArray(pos..pos+sz1-1).toHex()} (${sz1}), sz2=${sz2}")
            return e to (sz1 + sz2)
        }

        @JvmStatic
        fun decode(buf: ByteArray): Bipf_e? {
            val (b,i) = decode(buf, 0, buf.size)
            return b
        }

        @JvmStatic
        fun encode(e: Bipf_e): ByteArray? {
            val body = _encodeBody(e)
            if (body == null) return null
            val tagInt = (body.size shl TAG_SIZE) or e.typ
            val tagbuf = varint_encode(tagInt)
            // Log.d("bipf", "  ${tagbuf.toHex()} ${body.toHex()}")
            return tagbuf + body
        }

        @JvmStatic
        fun _encodeBody(e: Bipf_e): ByteArray? {
            // Log.d("wai", "encoding starts ${e.typ} ${BIPF_BOOLNONE}")
            when (e.typ) {
                BIPF_BOOLNONE -> {
                    // Log.d("bipf", "start BOOLNONE ${e.v as Int}")
                    val v = e.v as Int
                    val b = if (v < 0) ByteArray(0) else byteArrayOf(v.toByte())
                    // Log.d("bipf", "BOOLNONE ${b.toHex()}")
                    return b
                }
                BIPF_BYTES -> {
                    return e.v as ByteArray
                }
                BIPF_INT -> {
                    var v = e.v as Int
                    if (v < 0) v = -v - 1
                    var buf = byteArrayOf((v and 0xff).toByte()) // caution: bytes are signed in Kotlin
                    v = v shr 8
                    while (v > 0) {
                        buf += byteArrayOf((v and 0xff).toByte())
                        v = v shr 8
                    }
                    return buf
                }
                BIPF_LIST -> {
                    // Log.d("bipf", "LIST")
                    val al = e.v!! as ArrayList<Bipf_e>
                    var buf: ByteArray = ByteArray(0)
                    for (u in al) {
                        val enc = encode(u)
                        if (enc == null)
                            return null
                        buf = buf + enc
                    }
                    return buf
                }
                BIPF_STRING -> {
                    return e.v as ByteArray
                }
                else -> {
                    Log.d("bipf", "typ ${e.typ} not implemented")
                }
            }
            return null
        }

        @JvmStatic
        fun bipf_dumps(e: Bipf_e): ByteArray? {
            return encode(e)
        }

        @JvmStatic
        fun bipf_loads(buf: ByteArray): Bipf_e? {
            val (e,len) = decode(buf, 0, buf.size)
            return e
        }

        @JvmStatic
        fun bipf_list2JSON(e: Bipf_e): JSONArray? {
            // Log.d("bipf", "list2J typ=${e.typ}")
            if (e.typ != BIPF_LIST) return null
            val lst = e.getList()
            // Log.d("bipf", "list2J ${lst.size}")
            val obj = JSONArray()
            for (i in 0..lst.lastIndex) {
                val le = lst[i]
                when (lst[i].typ) {
                    BIPF_BYTES    -> { obj.put(i, le.getBytes().toBase64()) }
                    BIPF_STRING   -> { obj.put(i, le.getString()) }
                    BIPF_INT      -> { obj.put(i, le.getInt()) }
                    BIPF_LIST     -> { obj.put(i, bipf_list2JSON(le)) }
                    BIPF_BOOLNONE -> {
                        if ((lst[i].v as Int) < 0)
                            obj.put(i, null)
                        else
                            obj.put(i, le.getBoolean())
                    }
                    else -> {
                        Log.d("bipf", "list2JSON -- not implemented for typ=${lst[i].typ}")
                    }
                }
            }
            return obj
        }

    }
}

// eof

package nz.scuttlebutt.tremolavossbol.crypto

import com.goterl.lazysodium.LazySodiumAndroid
import com.goterl.lazysodium.SodiumAndroid
import com.goterl.lazysodium.interfaces.Hash
import com.goterl.lazysodium.interfaces.SecretBox
import com.goterl.lazysodium.utils.Key
import java.nio.charset.StandardCharsets

class SodiumAPI {
    companion object {
        @JvmStatic
        val lazySodiumInst = LazySodiumAndroid(SodiumAndroid(), StandardCharsets.UTF_8)

        @JvmStatic
        fun ByteArray.increment(): ByteArray {
            for (i in size - 1 downTo 0) {
                if (this[i] == 0xFF.toByte()) {
                    this[i] = 0x00.toByte()
                } else {
                    ++this[i]
                    break
                }
            }
            return this
        }

        @JvmStatic
        fun ByteArray.toKey(): Key {
            return Key.fromBytes(this)
        }

        @JvmStatic
        fun ByteArray.sha256(): ByteArray {
            var h = ByteArray(Hash.SHA256_BYTES)
            lazySodiumInst.cryptoHashSha256(h, this, this.size.toLong())
            return h
        }

        @JvmStatic
        fun secretBox(message: ByteArray, nonce: ByteArray, key: ByteArray): ByteArray {
            val encrypted = ByteArray(message.size + SecretBox.MACBYTES)
            lazySodiumInst.cryptoSecretBoxEasy(encrypted, message, message.size.toLong(), nonce, key)
            return encrypted
        }

        @JvmStatic
        fun secretUnbox(encrypted: ByteArray, nonce: ByteArray, key: ByteArray): ByteArray? {
            val decrypted = ByteArray(encrypted.size - SecretBox.MACBYTES)
            val valid = lazySodiumInst.cryptoSecretBoxOpenEasy(
                decrypted,
                encrypted,
                encrypted.size.toLong(),
                nonce,
                key
            )
            return if (valid) decrypted else null
        }

        @JvmStatic
        fun signDetached(data: ByteArray, key: ByteArray): ByteArray {
            val sig = ByteArray(64) // ed25519 signature
            lazySodiumInst.cryptoSignDetached (sig, data, data.size.toLong(), key)
            return sig
        }

        @JvmStatic
        fun verifySignDetached(signature: ByteArray, message: ByteArray, key: ByteArray): Boolean {
            return lazySodiumInst.cryptoSignVerifyDetached(signature, message, message.size, key)
        }
    }
}
package nz.scuttlebutt.tremolavossbol.tssb

import android.content.Intent
import android.util.Log
import androidx.localbroadcastmanager.content.LocalBroadcastManager
import kotlinx.coroutines.*
import nz.scuttlebutt.tremolavossbol.MainActivity
import nz.scuttlebutt.tremolavossbol.Settings
import nz.scuttlebutt.tremolavossbol.tssb.ble.BleForegroundService
import nz.scuttlebutt.tremolavossbol.tssb.ble.ForegroundNotificationType
import okhttp3.*
import okio.ByteString
import okio.ByteString.Companion.toByteString
import java.util.concurrent.locks.Lock
import java.util.concurrent.locks.ReentrantLock

const val WS_CLOSE_NORMAL = 1000

class WebsocketIO(val service: BleForegroundService, private var url: String) {
    private var ws: WebSocket? = null
    private var reconnectJob: Job? = null
    private val client = OkHttpClient()


    fun start() {

        if (!BleForegroundService.getTinySettings()!!.isWebsocketEnabled())
            return

        Log.d("Websocket","starting Websocket")

        val connectReq = Request.Builder().url(url).build()

        ws = client.newWebSocket(connectReq, wslistener)
    }

    fun stop() {
        ws?.close(WS_CLOSE_NORMAL, "connection closed by client")
        reconnectJob?.cancel()
    }

    private val wslistener = object : WebSocketListener() {
        override fun onMessage(webSocket: WebSocket, bytes: ByteString) {
            super.onMessage(webSocket, bytes)
            service.ioLock.lock()
            val rc = BleForegroundService.getTinyDemux()!!.on_rx(bytes.toByteArray(), url)
            service.ioLock.unlock()
            Log.d("Websocket", "received binary data, len: ${bytes.size}")
            if(!rc)
                Log.d("Websocket", "on rx failed")
        }

        override fun onMessage(webSocket: WebSocket, text: String) {
            super.onMessage(webSocket, text)
            Log.d("Websocket", "recieved String but expected binary data, received: $text")
        }

        override fun onOpen(webSocket: WebSocket, response: Response) {
            super.onOpen(webSocket, response)
            //activity.wai.eval("b2f_local_peer(\"ws\", \"$url\", \"${url}\", \"online\")")
            sendEvalToActivity("b2f_local_peer(\"ws\", \"$url\", \"${url}\", \"online\")")
        }

        override fun onClosed(webSocket: WebSocket, code: Int, reason: String) {
            super.onClosed(webSocket, code, reason)
            //activity.wai.eval("b2f_local_peer(\"ws\", \"$url\", \"${url}\", \"offline\")")
            sendEvalToActivity("b2f_local_peer(\"ws\", \"$url\", \"${url}\", \"offline\")")
        }

        override fun onFailure(webSocket: WebSocket, t: Throwable, response: Response?) {
            super.onFailure(webSocket, t, response)
            Log.e("Websocket", "error: ${t.localizedMessage}")
            //activity.wai.eval("b2f_local_peer(\"ws\", \"$url\", \"${url}\", \"offline\")")
            sendEvalToActivity("b2f_local_peer(\"ws\", \"$url\", \"${url}\", \"offline\")")
            reconnect()
        }

    }

    fun getUrl(): String {
        return url
    }

    fun send(buf: ByteArray) {
        if (ws == null)
            return

        ws!!.send(buf.toByteString())

    }

    //try to reconnect after failure
    private fun reconnect() {
        reconnectJob?.cancel()

        reconnectJob = CoroutineScope(Dispatchers.IO).launch {
            delay(5000)
            start()
        }
    }

    fun updateUrl(newUrl: String) {
        ws?.close(WS_CLOSE_NORMAL, "connection closed by client")
        reconnectJob?.cancel()
        sendEvalToActivity("b2f_local_peer(\"ws\", \"$url\", \"${url}\", \"offline\")")
        //activity.wai.eval("b2f_local_peer(\"ws\", \"$url\", \"${url}\", \"offline\")")
        url = newUrl

        val connectReq = Request.Builder().url(newUrl).build()
        ws = client.newWebSocket(connectReq, wslistener)
        Log.d("ws", "changed to new address: $newUrl")

    }

    private fun sendEvalToActivity(message: String) {
        val intent = Intent(ForegroundNotificationType.EVALUATION.value)
        intent.putExtra("message", message)
        LocalBroadcastManager.getInstance(service).sendBroadcast(intent)
    }
}
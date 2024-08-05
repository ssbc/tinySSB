package nz.scuttlebutt.tremolavossbol

import android.Manifest
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.util.Base64
import android.util.Log
import android.webkit.JavascriptInterface
import android.webkit.WebView
import android.widget.Toast
import androidx.core.content.ContextCompat.checkSelfPermission
import androidx.annotation.RequiresPermission
import com.google.zxing.integration.android.IntentIntegrator
import com.google.android.gms.location.LocationServices
import com.google.android.gms.tasks.CancellationTokenSource
import com.google.android.gms.tasks.Tasks

import org.json.JSONArray
import org.json.JSONObject
import java.sql.Time
import java.util.concurrent.TimeUnit

import nz.scuttlebutt.tremolavossbol.utils.Bipf
import nz.scuttlebutt.tremolavossbol.utils.Bipf.Companion.BIPF_BYTES
import nz.scuttlebutt.tremolavossbol.utils.Bipf.Companion.BIPF_LIST

import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_APP_C4_START
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_APP_C4_DECLINE
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_APP_C4_END
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_APP_C4_INVITE

import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_APP_IAM
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_APP_TEXTANDVOICE
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_APP_KANBAN
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_APP_SCHEDULING
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.deRef
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toBase64
import nz.scuttlebutt.tremolavossbol.utils.HelperFunctions.Companion.toHex
import nz.scuttlebutt.tremolavossbol.utils.PlusCodesUtils
import nz.scuttlebutt.tremolavossbol.games.battleships.BattleshipGame
import nz.scuttlebutt.tremolavossbol.games.common.GamesHandler
import nz.scuttlebutt.tremolavossbol.games.battleships.GameStates
import nz.scuttlebutt.tremolavossbol.utils.Constants.Companion.TINYSSB_APP_GAMETEXT



// pt 3 in https://betterprogramming.pub/5-android-webview-secrets-you-probably-didnt-know-b23f8a8b5a0c

class WebAppInterface(val act: MainActivity, val webView: WebView, val gameHandler: GamesHandler) {

    val frontend_frontier = act.getSharedPreferences("frontend_frontier", Context.MODE_PRIVATE)
    var gamesHandler = gameHandler

    /**
     * Retrieves the current geolocation of the Android device and returns it as a PlusCode.
     */
    @JavascriptInterface
    @RequiresPermission(
        anyOf = [Manifest.permission.ACCESS_COARSE_LOCATION, Manifest.permission.ACCESS_FINE_LOCATION],
    )
    fun getCurrentLocationAsPlusCode(): String {
        val locationClient = LocationServices.getFusedLocationProviderClient(act)

        try {
            val currentLocationTask = locationClient.getCurrentLocation(102, CancellationTokenSource().token)
            val currentLocation = Tasks.await(currentLocationTask, 2, TimeUnit.SECONDS)
            return PlusCodesUtils.encode(currentLocation.latitude, currentLocation.longitude)
        } catch (e: Exception) {
            val lastLocationTast = locationClient.lastLocation
            try {
                val lastLocation = Tasks.await(lastLocationTast, 2, TimeUnit.SECONDS)
                return PlusCodesUtils.encode(lastLocation.latitude, lastLocation.longitude)
            } catch (e: Exception) {
                Toast.makeText(act, "Failed to get location. Location is not sent with this message.", Toast.LENGTH_LONG).show()
                return ""
            }
        }

    }

    @JavascriptInterface
    fun getCoordinatesForPlusCode(code: String): String {
        val (latitude, longitude) = PlusCodesUtils.decode(code)
        return JSONObject()
            .put("latitude", latitude)
            .put("longitude", longitude)
            .toString()
    }

    @JavascriptInterface
    fun isGeoLocationEnabled(): String {
        return if (act.settings!!.isGeoLocationEnabled()) {
            "true"
        } else {
            "false"
        }
    }

    @JavascriptInterface
    fun onFrontendRequest(s: String) {
        //handle the data captured from webview}
        Log.d("FrontendRequest", s)
        val args = s.split(" ")
        when (args[0]) {
            "onBackPressed" -> {
                (act as MainActivity)._onBackPressed()
            }
            "ready" -> {
                eval("b2f_initialize('${act.idStore.identity.toRef()}', '${act.settings!!.getSettings()}')")
                act.frontend_ready = true
                act.tinyRepo.addNumberOfPendingChunks(0) // initialize chunk progress bar
                act.tinyNode.beacon()
            }
            "reset" -> { // UI reset
                // erase DB content
                eval("b2f_initialize(\"${act.idStore.identity.toRef()}\")")
                onFrontendRequest("restream")
            }
            "restream" -> {
                eval("restream = true")
                for (fid in act.tinyRepo.listFeeds()) {
                    Log.d("wai", "restreaming ${fid.toHex()}")
                    var i = 1
                    while (true) {
                        val r = act.tinyRepo.fid2replica(fid)
                        if(r == null)
                            break
                        val payload = r.read_content(i)
                        val mid = r.get_mid(i)
                        if (payload == null || mid == null) break
                        Log.d("restream", "${i}, ${payload.size} Bytes")
                        sendTinyEventToFrontend(fid, i, mid, payload)
                        i++
                    }
                }
                eval("restream = false")
            }
            "wipe:others" -> {
                for (fid in act.tinyRepo.listFeeds()) {
                    if (fid.contentEquals(act.idStore.identity.verifyKey))
                        continue
                    act.tinyRepo.delete_feed(fid)
                }
            }
            "qrscan.init" -> {
                val intentIntegrator = IntentIntegrator(act)
                intentIntegrator.setBeepEnabled(false)
                intentIntegrator.setCameraId(0)
                intentIntegrator.setPrompt("SCAN")
                intentIntegrator.setBarcodeImageEnabled(false)
                intentIntegrator.initiateScan()
                return
            }
            "exportSecret" -> {
                val json = act.idStore.identity.toExportString()!!
                eval("b2f_showSecret('${json}');")
                val clipboard = act.getSystemService(ClipboardManager::class.java)
                val clip = ClipData.newPlainText("simple text", json)
                clipboard.setPrimaryClip(clip)
                Toast.makeText(act, "secret key was also\ncopied to clipboard",
                    Toast.LENGTH_LONG).show()
            }
            "importSecret" -> {
                act.idStore.setNewIdentity(Base64.decode(args[1], Base64.NO_WRAP))
                act.tinyRepo.reset()

                // restart App
                if (act.websocket != null)
                    act.websocket!!.stop()
                if (act.ble != null)
                    act.ble!!.stopBluetooth()
                val ctx = act.applicationContext
                ctx.startActivity(Intent.makeRestartActivityTask(act.applicationContext.packageManager.getLaunchIntentForPackage(ctx.packageName)!!.component))
                Runtime.getRuntime().exit(0)
            }
            "wipe" -> {
                act.settings!!.resetToDefault()
                act.idStore.setNewIdentity(null) // creates new identity
                act.tinyRepo.reset()

                if (act.websocket != null)
                    act.websocket!!.stop()
                if (act.ble != null)
                    act.ble!!.stopBluetooth()
                val ctx = act.applicationContext
                ctx.startActivity(Intent.makeRestartActivityTask(act.applicationContext.packageManager.getLaunchIntentForPackage(ctx.packageName)!!.component))
                Runtime.getRuntime().exit(0)

                // eval("b2f_initialize(\"${tremolaState.idStore.identity.toRef()}\")")
                // FIXME: should kill all active connections, or better then the app
                //act.finishAffinity()
            }
            "add:contact" -> {

                val id = args[1].substring(1,args[1].length-8)
                Log.d("ADD", id)
                act.tinyGoset._add_key(Base64.decode(id, Base64.NO_WRAP))
            }
            /* no alias publishing in tinyTremola
            "add:contact" -> { // ID and alias
                tremolaState.addContact(args[1],
                    Base64.decode(args[2], Base64.NO_WRAP).decodeToString())
                val rawStr = tremolaState.msgTypes.mkFollow(args[1])
                val evnt = tremolaState.msgTypes.jsonToLogEntry(rawStr,
                    rawStr.encodeToByteArray())
                evnt?.let {
                    rx_event(it) // persist it, propagate horizontally and also up
                    tremolaState.peers.newContact(args[1]) // inform online peers via EBT
                }
                    return
            }
            */
            "publ:post" -> { // publ:post tips txt voice
                val a = JSONArray(args[1])
                val tips = ArrayList<String>(0)
                for (i in 0..a.length()-1) {
                    val s = a[i].toString() // (a[i] as JSONObject).toString()
                    tips.add(s)
                }
                var t: String? = null
                if (args[2] != "null")
                    t = Base64.decode(args[2], Base64.NO_WRAP).decodeToString()
                var v: ByteArray? = null
                if (args.size > 3 && args[3] != "null")
                    v = Base64.decode(args[3], Base64.NO_WRAP)
                public_post_with_voice(tips, t, v)
                return
            }
            "priv:post" -> { // priv:post tips atob(text) atob(voice) rcp1 rcp2 ...
                val a = JSONArray(args[1])
                val tips = ArrayList<String>(0)
                for (i in 0..a.length()-1) {
                    val s = a[i].toString() // (a[i] as JSONObject).toString()
                    tips.add(s)
                }
                var t: String? = null
                if (args[2] != "null")
                    t = Base64.decode(args[2], Base64.NO_WRAP).decodeToString()
                var v: ByteArray? = null
                if (args.size > 3 && args[3] != "null")
                    v = Base64.decode(args[3], Base64.NO_WRAP)
                private_post_with_voice(tips, t, v, args.slice(4..args.lastIndex))
                return
            }
            "get:media" -> {
                if (checkSelfPermission(act, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
                    Toast.makeText(act, "No permission to access media files",
                        Toast.LENGTH_SHORT).show()
                    return
                }
                val intent = Intent(Intent.ACTION_OPEN_DOCUMENT); // , MediaStore.Images.Media.EXTERNAL_CONTENT_URI)
                intent.type = "image/*"
                act.startActivityForResult(intent, 1001)
            }

            "get:voice" -> { // get:voice
                val intent = Intent(act, RecordActivity::class.java)
                act.startActivityForResult(intent, 808)
                return
            }
            "play:voice" -> { // play:voice b64enc(codec2) from date)
                Log.d("wai", s)
                val voice = Base64.decode(args[1], Base64.NO_WRAP)
                val intent = Intent(act, PlayActivity::class.java)
                intent.putExtra("c2Data", voice)
                if (args.size > 2)
                    intent.putExtra("from", Base64.decode(args[2], Base64.NO_WRAP).decodeToString())
                if (args.size > 3)
                    intent.putExtra("date", Base64.decode(args[3], Base64.NO_WRAP).decodeToString())
                act.startActivity(intent)
                return
            }
            "kanban" -> { // kanban bid atob(prev) atob(operation) atob(arg1) atob(arg2) atob(...)
                /*var bid: String = args[1]
                var prevs: List<String>? = null
                if(args[2] != "null") // prevs == "null" for the first board event (create bord event)
                    prevs = Base64.decode(args[2], Base64.NO_WRAP).decodeToString().split(" ")
                var operation: String = Base64.decode(args[3], Base64.NO_WRAP).decodeToString()
                var argList: List<String>? = null
                if(args[4] != "null")
                    argList = Base64.decode(args[4], Base64.NO_WRAP).decodeToString().split(" ")

                 */
                //var data = JSONObject(Base64.decode(args[1], Base64.NO_WRAP).decodeToString())
                val bid: String? = if (args[1] != "null") args[1] else null
                val prev: List<String>? = if (args[2] != "null") Base64.decode(args[2], Base64.NO_WRAP).decodeToString().split(",").map{ Base64.decode(it, Base64.NO_WRAP).decodeToString()} else null
                val op: String = args[3]
                val argsList: List<String>? = if(args[4] != "null") Base64.decode(args[4], Base64.NO_WRAP).decodeToString().split(",").map{ Base64.decode(it, Base64.NO_WRAP).decodeToString()} else null

                kanban(bid, prev , op, argsList)
            }
            "kahoot" -> {
                val SendID: String? = if (args[1] != "null") args[1] else null
                val op: String = args[2]
                val ignore: String = args[3]
                val args:String? = args[4]
                Kahoot(SendID, op, args, ignore)
            }
            "scheduling" -> {
                val bid: String? = if (args[1] != "null") args[1] else null
                val prev: List<String>? = if (args[2] != "null") Base64.decode(args[2], Base64.NO_WRAP).decodeToString().split(",").map{ Base64.decode(it, Base64.NO_WRAP).decodeToString()} else null
                val op: String = args[3]
                val argsList: List<String>? = if(args[4] != "null") Base64.decode(args[4], Base64.NO_WRAP).decodeToString().split(",").map{ Base64.decode(it, Base64.NO_WRAP).decodeToString()} else null

                scheduling(bid, prev , op, argsList)
            }
            "iam" -> {
                val new_alias = Base64.decode(args[1], Base64.NO_WRAP).decodeToString()
                val lst = Bipf.mkList()
                Bipf.list_append(lst, TINYSSB_APP_IAM)
                Bipf.list_append(lst, Bipf.mkString(new_alias))

                val body = Bipf.encode(lst)
                if (body != null) {
                    act.tinyNode.publish_public_content(body)
                }
            }
            "games" -> { // Handle battleship communication
                Log.d("GAM - WebApp", args.toString())
                //gamesHandler.processGameRequest(s.substring(6))
                // TODO here you can add restrictions, if a command is not allowed
                if (args[1] == "BSH") {
                    when (args[2]) {
                        "INV" -> {
                            if (gamesHandler.getInviteCount("BSH") != 0) {
                                Log.d("BSH-Handler INV", "inviteCounter is not 0")
                                return
                            }
                            gameHandler.incInviteCount("BSH")
                            gamesHandler.addOwnGame(args[1], args[3], GameStates.INVITED)
                            val inst = gamesHandler.getInstanceFromFid(args[1], args[3])
                            (inst!!.game as BattleshipGame).setupGame(true)
                            val req = "${args[1]} INV ${args[3]} ${inst.startTime}"
                            public_post_game_request(
                                Base64.encodeToString(
                                    req.toByteArray(),
                                    Base64.NO_WRAP
                                )
                            )
                            return
                        }

                        "INVACC" -> {
                            val inst = gamesHandler.getInstanceFromFid("BSH", args[3])
                            var peerHash: String = ""
                            if (inst != null) {
                                (inst.game as BattleshipGame).setupGame(false)
                                peerHash = (inst.game as BattleshipGame).getShipPosition()
                            }

                            val invacc =
                                "BSH INVACC ${args[3]} ${gamesHandler.myId.toRef()} $peerHash" // Appending Peer's Shiphash
                            Log.d("GAM APP (INVACC)", invacc)
                            public_post_game_request(
                                Base64.encodeToString(
                                    invacc.toByteArray(),
                                    Base64.NO_WRAP
                                )
                            )
                            return
                        }

                        "SHOT" -> {
                            val inst = gamesHandler.getInstanceFromFids("BSH", args[3], args[4])
                            var isPeer: String = ""
                            if (gamesHandler.isIdEqualToMine(args[3])) { // I am owner
                                isPeer = "0"
                            } else if (gamesHandler.isIdEqualToMine(args[4])) {
                                isPeer = "1"
                            } else {
                                return
                            }
                            if (inst != null) {
                                if (!(inst.game as BattleshipGame).gameState!!.isMyTurn()) {
                                    return
                                }
                                (inst.game as BattleshipGame).gameState!!.turn = false
                            } else {
                                return
                            }
                            val shot =
                                "${args[1]} ${args[2]} ${args[3]} ${args[4]} $isPeer ${args[5]}"
                            Log.d("GAM APP (SHOT)", shot)
                            public_post_game_request(
                                Base64.encodeToString(
                                    shot.toByteArray(),
                                    Base64.NO_WRAP
                                )
                            )
                        }

                        "DUELQUIT" -> {
                            val inst = gamesHandler.getInstanceFromFids("BSH", args[3], args[4])
                            if (inst == null || !inst.state.isActive()) {
                                return
                            }
                            inst.state = GameStates.STOPPED
                            (inst.game as BattleshipGame).gameState!!.turn = false
                            var isPeer: String = ""
                            if (gamesHandler.isIdEqualToMine(args[3])) { // I am owner
                                isPeer = "0"
                            } else if (gamesHandler.isIdEqualToMine(args[4])) {
                                isPeer = "1"
                            }
                            val quit = "${args[1]} ${args[2]} ${args[3]} ${args[4]} $isPeer"
                            Log.d("GAM APP (SHOT)", quit)
                            public_post_game_request(
                                Base64.encodeToString(
                                    quit.toByteArray(),
                                    Base64.NO_WRAP
                                )
                            )
                        }

                        else -> {
                            public_post_game_request(
                                Base64.encodeToString(
                                    s.substring(6).toByteArray(), Base64.NO_WRAP
                                )
                            )
                        }
                    }
                }
            }
            "settings:set" -> {
                act.settings!!.set(args[1], args[2])
            }
            "settings:get" -> {
                val settings = act.settings!!.getSettings()
                act.wai.eval("b2f_get_settings('${settings}')")
            }

            "connect_four" -> {
                connect_four(args[1], args[2], args[3], args[4]);
            }
            "connect_four_end" -> {
                connect_four_end(args[1], args[2], args[3]);
            }
            "connect_four_invite" -> {
                connect_four_invite(args[1], args[2]);
            }
            "connect_four_decline_invite" -> {
                connect_four_decline_invite(args[1],args[2]);
            }

            else -> {
                Log.d("onFrontendRequest", "unknown")
            }
        }
    }

    fun eval(js: String) { // send JS string to webkit frontend for execution
        webView.post(Runnable {
            webView.evaluateJavascript(js, null)
        })
    }

    private fun importIdentity(secret: String): Boolean {
        Log.d("D/importIdentity", secret)
        if (act.idStore.setNewIdentity(Base64.decode(secret, Base64.DEFAULT))) {
            // FIXME: remove all decrypted content in the database, try to decode new one
            Toast.makeText(act, "Imported of ID worked. You must restart the app.",
                Toast.LENGTH_SHORT).show()
            return true
        }
        Toast.makeText(act, "Import of new ID failed.", Toast.LENGTH_LONG).show()
        return false
    }

    fun public_post_with_voice(tips: ArrayList<String>, text: String?, voice: ByteArray?) {
        if (text != null)
            Log.d("wai", "post_voice t- ${text}/${text.length}")
        if (voice != null)
            Log.d("wai", "post_voice v- ${voice}/${voice.size}")
        val lst = Bipf.mkList()
        Bipf.list_append(lst, TINYSSB_APP_TEXTANDVOICE)
        val tip_lst = Bipf.mkList()
        for (t in tips) {
            Bipf.list_append(tip_lst, Bipf.mkString(t))
        }
        Bipf.list_append(lst, tip_lst)
        Bipf.list_append(lst, if (text == null) Bipf.mkNone() else Bipf.mkString(text))
        Bipf.list_append(lst, if (voice == null) Bipf.mkNone() else Bipf.mkBytes(voice))
        val tst = Bipf.mkInt((System.currentTimeMillis() / 1000).toInt())
        Log.d("wai", "send time is ${tst.getInt()}")
        Bipf.list_append(lst, tst)
        val body = Bipf.encode(lst)
        if (body != null)
            act.tinyNode.publish_public_content(body)
    }

    fun private_post_with_voice(tips: ArrayList<String>, text: String?, voice: ByteArray?, rcps: List<String>) {
        if (text != null)
            Log.d("wai", "private post_voice t- ${text}/${text.length}")
        if (voice != null)
            Log.d("wai", "private post_voice v- ${voice}/${voice.size}")
        val lst = Bipf.mkList()
        Bipf.list_append(lst, TINYSSB_APP_TEXTANDVOICE)
        val tip_lst = Bipf.mkList()
        for (t in tips) {
            Bipf.list_append(tip_lst, Bipf.mkString(t))
        }
        Bipf.list_append(lst, tip_lst)
        Bipf.list_append(lst, if (text == null) Bipf.mkNone() else Bipf.mkString(text))
        Bipf.list_append(lst, if (voice == null) Bipf.mkNone() else Bipf.mkBytes(voice))
        val tst = Bipf.mkInt((System.currentTimeMillis() / 1000).toInt())
        Log.d("wai", "private send time is ${tst.getInt()}")
        Bipf.list_append(lst, tst)

        val recps = Bipf.mkList()
        val keys: MutableList<ByteArray> = mutableListOf()
        val me = act.idStore.identity.toRef()
        for (r in rcps) {
            if (r != me) {
                Bipf.list_append(recps, Bipf.mkString(r))
                keys.add(r.deRef())
            }
        }
        Bipf.list_append(recps, Bipf.mkString(me))
        keys.add(me.deRef())
        Bipf.list_append(lst, recps)

        val body_clear = Bipf.encode(lst)
        val encrypted = body_clear!!.let { act.idStore.identity.encryptPrivateMessage(it, keys) }
        // return encrypted.let { Bipf.mkString(it) }
        val body_encr = Bipf.encode(Bipf.mkBytes(encrypted))
        if (body_encr != null)
            act.tinyNode.publish_public_content(body_encr)
    }

    /* Kahoot-Function: Encapsulates the data from Kahoot and sends it as public content. */
    fun Kahoot(SendID:String?, operation:String, args:String?, ignore:String?){
        val lst = Bipf.mkList()
        Bipf.list_append(lst, Bipf.mkString("KAH"))
        if(SendID != null) {
            Bipf.list_append(lst, Bipf.mkString(SendID))
        } else {
            Bipf.list_append(lst, Bipf.mkString("null"))  // TODO: Change to Bipf.mkNone(), but would be incompatible with the old format
        }
        Bipf.list_append(lst, Bipf.mkString(operation))
        if(ignore != null) {
            Bipf.list_append(lst, Bipf.mkString(ignore))
        } else {
            Bipf.list_append(lst, Bipf.mkString("null"))  // TODO: Change to Bipf.mkNone(), but would be incompatible with the old format
        }
        if (args != null) {
            Bipf.list_append(lst, Bipf.mkString(args))
        } else { // arg is not a b64 string
            Log.d("KAHOOT-INFO", "null-value")
            Bipf.list_append(lst, Bipf.mkString("null"))
        }
        val body = Bipf.encode(lst)
        if (body != null) {
            Log.d("Kahoot", "published bytes: " + Bipf.decode(body))
            act.tinyNode.publish_public_content(body)
        }
    }

    // BATTLESHIP
    fun public_post_game_request(text: String?) {
        val lst = Bipf.mkList()
        Bipf.list_append(lst, TINYSSB_APP_GAMETEXT)
        Bipf.list_append(lst, if (text == null) Bipf.mkNone() else Bipf.mkString(text))
        val tst = Bipf.mkInt((System.currentTimeMillis() / 1000).toInt())
        Log.d("wai", "send time is ${tst.getInt()}")
        Bipf.list_append(lst, tst)
        val body = Bipf.encode(lst)
        if (body != null)
            act.tinyNode.publish_public_content(body)
    }

    fun scheduling(bid: String?, prev: List<String>?, operation: String, args: List<String>?) {
        val lst = Bipf.mkList()
        Bipf.list_append(lst, TINYSSB_APP_SCHEDULING)
        if (bid != null)
            Bipf.list_append(lst, Bipf.mkBytes(Base64.decode(bid, Base64.NO_WRAP)))
        else
            Bipf.list_append(lst, Bipf.mkNone())

        if(prev != null) {
            val prevList = Bipf.mkList()
            for(p in prev) {
                Bipf.list_append(prevList, Bipf.mkBytes(Base64.decode(p, Base64.NO_WRAP)))
            }
            Bipf.list_append(lst, prevList)
        } else {
            Bipf.list_append(lst, Bipf.mkString("null"))
        }

        Bipf.list_append(lst, Bipf.mkString(operation))

        if(args != null) {
            for(arg in args) {
                if (Regex("^([A-Za-z0-9+/]{4})*([A-Za-z0-9+/]{3}=|[A-Za-z0-9+/]{2}==)?\$").matches(arg)) {
                    Bipf.list_append(lst, Bipf.mkBytes(Base64.decode(arg, Base64.NO_WRAP)))
                } else { // arg is not a b64 string
                    Bipf.list_append(lst, Bipf.mkString(arg))
                }
            }
        }

        val body = Bipf.encode(lst)

        if (body != null) {
            Log.d("scheduling", "published bytes: " + Bipf.decode(body))
            act.tinyNode.publish_public_content(body)
        }
    }

    fun kanban(bid: String?, prev: List<String>?, operation: String, args: List<String>?) {
        val lst = Bipf.mkList()
        Bipf.list_append(lst, TINYSSB_APP_KANBAN)
        if (bid != null)
            Bipf.list_append(lst, Bipf.mkBytes(Base64.decode(bid, Base64.NO_WRAP)))
        else
            Bipf.list_append(lst, Bipf.mkNone())

        if(prev != null) {
            val prevList = Bipf.mkList()
            for(p in prev) {
                Bipf.list_append(prevList, Bipf.mkBytes(Base64.decode(p, Base64.NO_WRAP)))
            }
            Bipf.list_append(lst, prevList)
        } else {
            Bipf.list_append(lst, Bipf.mkString("null"))  // TODO: Change to Bipf.mkNone(), but would be incompatible with the old format
        }

        Bipf.list_append(lst, Bipf.mkString(operation))

        if(args != null) {
            for(arg in args) {
                if (Regex("^([A-Za-z0-9+/]{4})*([A-Za-z0-9+/]{3}=|[A-Za-z0-9+/]{2}==)?\$").matches(arg)) {
                    Bipf.list_append(lst, Bipf.mkBytes(Base64.decode(arg, Base64.NO_WRAP)))
                } else { // arg is not a b64 string
                    Bipf.list_append(lst, Bipf.mkString(arg))
                }
            }
        }

        val body = Bipf.encode(lst)

        if (body != null) {
            Log.d("kanban", "published bytes: " + Bipf.decode(body))
            act.tinyNode.publish_public_content(body)
        }
        //val body = Bipf.encode(lst)
        //Log.d("KANBAN BIPF ENCODE", Bipf.bipf_list2JSON(Bipf.decode(body!!)!!).toString())
        //if (body != null)
            //act.tinyNode.publish_public_content(body)

    }

    fun connect_four(gameId: String, currentPlayer: String, members: String, stonePos: String) {
        val lst = Bipf.mkList()
        Bipf.list_append(lst, TINYSSB_APP_C4_START)
        Bipf.list_append(lst, Bipf.mkString(gameId))
        Bipf.list_append(lst, Bipf.mkString(currentPlayer))
        Bipf.list_append(lst, Bipf.mkString(members))
        Bipf.list_append(lst, Bipf.mkString(stonePos))

        val body = Bipf.encode(lst)

        if (body != null) {
            Log.d("connect_four", "published bytes: " + Bipf.decode(body))
            act.tinyNode.publish_public_content(body)
        }
    }

    fun connect_four_invite(inviter: String, invitee: String) {
        val lst = Bipf.mkList()
        Bipf.list_append(lst, TINYSSB_APP_C4_INVITE)
        Bipf.list_append(lst, Bipf.mkString(inviter))
        Bipf.list_append(lst, Bipf.mkString(invitee))

        val body = Bipf.encode(lst)

        if (body != null) {
            Log.d("connect_four_invite", "published bytes: " + Bipf.decode(body))
            act.tinyNode.publish_public_content(body)
        }
    }

    fun connect_four_decline_invite(inviter: String, invitee: String) {
        val lst = Bipf.mkList()
        Bipf.list_append(lst, TINYSSB_APP_C4_DECLINE)
        Bipf.list_append(lst, Bipf.mkString(inviter))
        Bipf.list_append(lst, Bipf.mkString(invitee))

        val body = Bipf.encode(lst)

        if (body != null) {
            Log.d("connect_four_decline_invite", "published bytes: " + Bipf.decode(body))
            act.tinyNode.publish_public_content(body)
        }
    }

    fun connect_four_end(gameId: String, loser: String, stonePos: String) {
        val lst = Bipf.mkList()
        Bipf.list_append(lst, TINYSSB_APP_C4_END)
        Bipf.list_append(lst, Bipf.mkString(gameId))
        Bipf.list_append(lst, Bipf.mkString(loser))
        Bipf.list_append(lst, Bipf. mkString(stonePos))

        val body = Bipf.encode(lst)
        if (body != null) {
            Log.d("connect_four", "published bytes: " + Bipf.decode(body))
            act.tinyNode.publish_public_content(body)
        }
    }

    fun return_voice(voice: ByteArray) {
        var cmd = "b2f_new_voice('" + voice.toBase64() + "');"
        Log.d("CMD", cmd)
        eval(cmd)
    }

    fun sendIncompleteEntryToFrontend(fid: ByteArray, seq: Int, mid:ByteArray, body: ByteArray) {
        val e = toFrontendObject(fid, seq, mid, body)
        if (e != null)
            eval("b2f_new_incomplete_event($e)")

    }

    fun sendTinyEventToFrontend(fid: ByteArray, seq: Int, mid:ByteArray, body: ByteArray) {
        Log.d("wai","sendTinyEvent ${body.toHex()}")
        var e = toFrontendObject(fid, seq, mid, body)
        if (e != null)
            eval("b2f_new_event($e)")

        // in-order api
        val replica = act.tinyRepo.fid2replica(fid)

        if (frontend_frontier.getInt(fid.toHex(), 1) == seq && replica != null) {
            for (i in seq .. replica.state.max_seq ) {
                val content = replica.read_content(i)
                val message_id= replica.get_mid(seq)
                if(content == null || message_id == null || !replica.isSidechainComplete(i))
                    break
                e = toFrontendObject(fid, i, message_id, content)
                if (e != null)
                    eval("b2f_new_in_order_event($e)")
                frontend_frontier.edit().putInt(fid.toHex(), i + 1).apply()
            }
        }
    }

    fun toFrontendObject(fid: ByteArray, seq: Int, mid: ByteArray, payload: ByteArray): String? {
        val bodyList = Bipf.decode(payload)
        if (bodyList == null)
            return null
        if (bodyList.typ == BIPF_LIST) { // clear text log entry
            val param = Bipf.bipf_list2JSON(bodyList)
            var hdr = JSONObject()
            hdr.put("fid", "@" + fid.toBase64() + ".ed25519")
            hdr.put("ref", mid.toBase64())
            hdr.put("seq", seq)
            return "{header:${hdr.toString()}, public:${param.toString()}}"
        }
        if (bodyList.typ == BIPF_BYTES) { // encrypted
            Log.d("rcvd encrypted log entry", "@" + fid.toBase64() + ".ed25519, seq=" + seq)
            val clear = act.idStore.identity.decryptPrivateMessage(bodyList.getBytes())
            if (clear != null) {
                val bodyList2 = Bipf.decode(clear)
                if (bodyList2 != null) {
                    val param = Bipf.bipf_list2JSON(bodyList2)
                    var hdr = JSONObject()
                    hdr.put("fid", "@" + fid.toBase64() + ".ed25519")
                    hdr.put("ref", mid.toBase64())
                    hdr.put("seq", seq)
                    return "{header:${hdr.toString()}, confid:${param.toString()}}"
                }
            }
        }
        Log.d("toFrontendObject", "could not decode or decrypt")
        return null
    }


}

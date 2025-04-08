package nz.scuttlebutt.tremolavossbol.miniapps

import android.util.Log
import android.webkit.WebView
import nz.scuttlebutt.tremolavossbol.MainActivity
import org.json.JSONObject
import java.io.BufferedReader
import java.io.File
import java.io.InputStreamReader
import kotlin.system.measureNanoTime

/**
 * Abstract class representing a Mini App Plugin.
 *
 * This class provides methods to inject CSS, JavaScript, HTML content, and various
 * display configurations into a WebView. Each Mini App should extend this class and
 * provide specific implementations for the `initialize` and `handleRequest` methods.
 */
open class MiniAppPlugin(val act: MainActivity, val webView: WebView) {

    /**
    * Initializes the Mini App Plugin.
    *
    * This function is called by the PluginLoader when loading all the Plugins and has to be
    * overridden for every MiniApp specific Plugin. It is used to inject all the needed content.
    */
    fun initialize(jsonString: String) {
        Log.d("MiniAppPlugin", "Initializing Mini App Plugin")

        try {
            //Convert the JSON string to a JSONObject
            val jsonObj = JSONObject(jsonString)

            val miniAppID = jsonObj.getString("id")

            // Construct directories in the internal storage ("data" folder)
            val miniAppsDir = act.miniAppDirectory
            // Get the CSS file name from JSON and remove any leading '/'
            val cssFileName = jsonObj.getString("cssFile").removePrefix("/")

            // Build the full path to the CSS file in the internal storage
            val cssFile = File(miniAppsDir, miniAppID + "/$cssFileName")
            val cssPath = cssFile.absolutePath

            var htmlPath = jsonObj.getString("htmlFile")
            //add miniAppID to the htmlPath
            htmlPath = "$miniAppID/$htmlPath"
            val html = File(miniAppsDir, htmlPath).absolutePath
            val css = File(miniAppsDir, cssPath).absolutePath

            val scriptsPaths = jsonObj.getJSONArray("scripts")
            val scriptFiles = mutableListOf<String>()
            for (i in 0 until scriptsPaths.length()) {
                val scriptPath = File(miniAppsDir, miniAppID + "/" + scriptsPaths.getString(i)).path
                // add scriptPath to the scriptFiles list
                scriptFiles.add(scriptPath)
            }

            // Intialize String array to store the script files in scriptFiles
            val scriptFilesArray = scriptFiles.toTypedArray()
            Log.d("Script Files", scriptFilesArray.contentToString())

            val scenarioJson = jsonObj.getJSONObject("scenario")

            // Create a displayOrNotList by using each scenario key (e.g., "div:" + key)
            val displayOrNotList = mutableListOf<String>()
            // Create a map for scenario display arrays
            val scenarioDisplayMap = mutableMapOf<String, List<String>>()
            // Create a map for scenario menus (list of key-value pairs)
            val scenarioMenu = mutableMapOf<String, List<Pair<String, String>>>()

            for (key in scenarioJson.keys()) {
                val scenarioEntry = scenarioJson.getJSONObject(key)

                // Build displayOrNotList using the scenario key.
                displayOrNotList.add("div:$key")

                Log.d("Scenario Key", scenarioEntry.toString())

                // Extract the display array from the scenario entry.
                val displayArray = scenarioEntry.getJSONArray("display")
                val displayList = mutableListOf<String>()
                for (i in 0 until displayArray.length()) {
                    displayList.add(displayArray.getString(i))
                }
                scenarioDisplayMap[key] = displayList

                // Extract the menu object and convert it into a list of key-value pairs.
                val menuJson = scenarioEntry.getJSONObject("menu")
                val menuList = mutableListOf<Pair<String, String>>()
                for (menuKey in menuJson.keys()) {
                    menuList.add(Pair(menuKey, menuJson.getString(menuKey)))
                }
                scenarioMenu[key] = menuList
            }

            Log.d("DisplayOrNotList", displayOrNotList.toString())
            Log.d("Scenario Display Map", scenarioDisplayMap.toString())
            Log.d("Scenario Menu", scenarioMenu.toString())

            val manifestPath = File(miniAppsDir, "$miniAppID/manifest.json").absolutePath

            injectAll(
                miniAppID,
                cssPath,
                scriptFilesArray,
                html,
                displayOrNotList,
                scenarioDisplayMap,
                scenarioMenu,
                manifestPath
            )
        } catch (e: Exception) {
            Log.e("MiniAppPlugin", "Error initializing Mini App Plugin", e)
        }
    }

    /**
     * Handles requests with the given arguments.
     *
     * @param args List of arguments to handle the request.
     */
        fun handleRequest(args: List<String>) {
            if (args.isEmpty()) {
                Log.e("MiniAppPlugin", "handleRequest called with empty args")
                return
            }

            Log.d("MiniAppPluginRequest", "Handling request: ${args[0]} with args: ${args.drop(1)}")

            val firstArg = args[0]
            Log.d("MiniAppPluginRequest", "First arg: $firstArg")

            if (!firstArg.contains(":")) {
                val miniAppID = firstArg
                val jsonArgs = args.drop(1)

                val argsJson = JSONObject().apply {
                    put("args", jsonArgs)
                }.toString()

                val jsCode = """
                (function() {
                    if (window.miniApps && window.miniApps['$miniAppID'] && typeof window.miniApps['$miniAppID'].handleRequest === 'function') {
                        let response = window.miniApps['$miniAppID'].handleRequest($argsJson);
                        console.log("Response from $miniAppID:", response);
                    } else {
                        console.error("handleRequest function not found for MiniApp: $miniAppID");
                    }
                })();
            """.trimIndent()

                eval(jsCode)
                return
            }

            val parts = firstArg.split(":", limit = 2)
            if (parts.size < 2) {
                Log.e("MiniAppPlugin", "Invalid MiniApp request format: $firstArg")
                return
            }

            val miniAppID = parts[0]
            val command = parts[1] // The command (e.g., "onBackPressed")
            val jsonArgs = JSONObject().apply {
                put("args", args.drop(1)) // Remaining arguments
            }.toString()

            val jsCode = """
        (function() {
            if (window.miniApps && window.miniApps['$miniAppID'] && typeof window.miniApps['$miniAppID'].handleRequest === 'function') {
                let response = window.miniApps['$miniAppID'].handleRequest("$command", $jsonArgs);
                console.log("Response from $miniAppID:", response);
            } else {
                console.error("handleRequest function not found for MiniApp: $miniAppID");
            }
        })();
        """.trimIndent()

            eval(jsCode)
        }


    /**
     * Injects all specified resources into the WebView. To be used in the overridden initialize()
     * function for every mini App Plugin
     *
     * @param cssPath CSS code to be injected.
     * @param scriptPath Array of JavaScript file paths to be injected.
     * @param htmlPath Array of HTML content to be injected into the core div.
     * @param displayOrNot List of display configurations to be injected.
     * @param scenarioDisplay Map of scenario displays to be injected.
     * @param scenarioMenu Map of scenario menus to be injected.
     */
    open fun injectAll(miniAppID: String,
                      cssPath: String? = null,
                       scriptPath: Array<String>? = null,
                       htmlPath: String? = null,
                       displayOrNot: List<String>? = null,
                       scenarioDisplay: Map<String, List<String>>? = null,
                       scenarioMenu: Map<String, List<Pair<String, String>>>? = null,
                       manifestPath: String? = null) {
        cssPath?.let { injectCSS(it) }
        scriptPath?.let { injectScript(miniAppID, *it) }
        htmlPath?.let { injectContent(it) }
        displayOrNot?.let { injectDisplayOrNot(it) }
        scenarioDisplay?.let { injectScenarioDisplay(it) }
        scenarioMenu?.let { injectScenarioMenu(it) }
        manifestPath?.let { addMiniAppToList(it) }
    }

    /**
    * Sends a JavaScript string to the WebView frontend for execution.
    *
    * This function posts a Runnable to the WebView, which then evaluates the given
    * JavaScript string within the context of the web page loaded in the WebView.
    *
    * @param js The JavaScript code to be executed in the WebView.
    */
    open fun eval(js: String) { // send JS string to webkit frontend for execution
        webView.post(Runnable {
            webView.evaluateJavascript(js, null)
        })
    }

    /**
     * Injects the given CSS code into the header of the HTML file.
     *
     * @param css CSS code to be injected.
     */
    private fun injectCSS(cssPath: String) {
        Log.d("InjectCSS", "Attempting to inject CSS from: $cssPath")

        val script = """
        (function() {
            var link = document.createElement('link');
            link.rel = 'stylesheet';
            link.type = 'text/css';
            link.href = '$cssPath';
            document.head.appendChild(link);
        })();
    """.trimIndent()

        webView.evaluateJavascript(script, null)
    }

    /**
     * Injects the given JavaScript scripts into the header of the HTML file.
     *
     * @param scriptPaths Variable number of JavaScript file paths to be injected.
     */
    private fun injectScript(miniAppID: String, vararg scriptPaths: String) {
        scriptPaths.forEach { scriptPath ->
            Log.d("MiniAppPlugin", "Injecting script: $scriptPath")

            val scriptInjection = """
        (function() {
            window.miniApps = window.miniApps || {}; // Ensure namespace exists
            
            let scriptTag = document.createElement('script');
            scriptTag.type = 'text/javascript';
            scriptTag.src = '$scriptPath';
            
            scriptTag.onload = function() {
                console.log("Loaded script: $scriptPath");
                
                if (typeof window.miniApp === 'object' && typeof window.miniApp.handleRequest === 'function') {
                    console.log("Registering MiniApp: $miniAppID");
                    window.miniApps['$miniAppID'] = window.miniApp; // Assign it dynamically
                    window.miniApp = null; // Prevent global conflicts
                } else {
                    console.error("MiniApp $miniAppID does not define handleRequest correctly.");
                }
            };

            scriptTag.onerror = function() {
                console.error("Failed to load script: $scriptPath");
            };

            document.head.appendChild(scriptTag);
        })();
        """.trimIndent()

            webView.evaluateJavascript(scriptInjection, null)
        }
    }

    /**
     * Injects the given HTML content into the div with id 'core'.
     *
     * @param htmlPath Variable number of HTML content strings to be injected.
     */
    private fun injectContent(htmlPath: String) {

        try {
            Log.d("MiniAppPlugin", "Asset Path: $htmlPath")
            val htmlFile = File(htmlPath)
            if (htmlFile.exists()) {
                val htmlContent = htmlFile.readText(Charsets.UTF_8)
                val content = """
                (function() {
                    var coreDiv = document.getElementById('core');
                    if (coreDiv) {
                        coreDiv.innerHTML += `$htmlContent`;
                        console.log('HTML content added to core div');
                    } else {
                        console.log('Core div not found');
                    }
                })();
            """
                webView.evaluateJavascript(content, null)
            } else {
                Log.e("MiniAppPlugin", "File does not exist: ${htmlFile.absolutePath}")
            }
        } catch (e: Exception) {
            Log.e("InjectContent", "Error loading HTML from assets: $htmlPath", e)
        }
    }

    /**
     * Injects display configurations into the WebView.
     *
     * @param displayOrNot List of display configurations to be injected.
     */
    private fun injectDisplayOrNot(displayOrNot: List<String>) {

        val jsCodeBuilder = StringBuilder()

        displayOrNot.forEach {
            jsCodeBuilder.append("display_or_not.push('$it');")
        }

        // Evaluate the generated JavaScript code in the WebView
        webView.evaluateJavascript(jsCodeBuilder.toString(), null)
    }

    /**
     * Injects scenario display configurations into the WebView.
     *
     * @param scenarioDisplay Map of scenario displays to be injected.
     */
    private fun injectScenarioDisplay(scenarioDisplay: Map<String, List<String>>) {

        val jsCodeBuilder = StringBuilder()

        scenarioDisplay.forEach { (key, values) ->
            val valuesJs = values.joinToString(prefix = "[", postfix = "]") { "'$it'" }
            jsCodeBuilder.append("scenarioDisplay['$key'] = $valuesJs;")
        }

        // Evaluate the generated JavaScript code in the WebView
        webView.evaluateJavascript(jsCodeBuilder.toString(), null)

    }

    /**
     * Injects scenario menu configurations into the WebView.
     *
     * @param scenarioMenu Map of scenario menus to be injected.
     */
    private fun injectScenarioMenu(scenarioMenu: Map<String, List<Pair<String, String>>>) {

        val jsCodeBuilder = StringBuilder()

        scenarioMenu.forEach { (key, values) ->
            val valuesJs = values.joinToString(prefix = "[", postfix = "]") { (name, func) ->
                "['$name', '$func']"
            }
            jsCodeBuilder.append("scenarioMenu['$key'] = $valuesJs;")
        }

        // Evaluate the generated JavaScript code in the WebView
        webView.evaluateJavascript(jsCodeBuilder.toString(), null)

    }

    /**
     * Adds a Mini App to the list by loading its manifest data.
     *
     * @param manifestPath Path to the manifest file.
     */
    private fun addMiniAppToList(manifestPath: String) {
        try {
            val manifestFile = File(manifestPath)
            val content = manifestFile.readText(Charsets.UTF_8)

            val quotedContent = JSONObject.quote(content)

            val miniAppID = JSONObject(content).getString("id")

            // Get the icon file path
            val iconPath = JSONObject(content).getString("icon")
            val fullIconPath = (act.miniAppDirectory?.absolutePath) + "/" + miniAppID + "/" + iconPath

            // Generate absolute path for the icon (inside assets)
            val iconAbsolutePath = "file:///android_asset/$fullIconPath"

            // Update the icon path in the manifest content
            val updatedContent = JSONObject(content).put("icon", iconAbsolutePath).toString()

            // Quote the updated content for JavaScript processing
            val quotedUpdatedContent = JSONObject.quote(updatedContent)

            Log.d("Manifest content", updatedContent)
            Log.d("Manifest content (quoted)", quotedUpdatedContent)

            webView.evaluateJavascript("handleManifestContent($quotedUpdatedContent)", null)
        } catch (e: Exception) {
            Log.e("InjectContent", "Error reading manifest from assets: $manifestPath", e)
        }
    }

    fun parseNestedJson(obj: JSONObject): Any {
        val result = mutableMapOf<String, Any>()
        for (key in obj.keys()) {
            val value = obj.get(key)
            result[key] = if (value is JSONObject) parseNestedJson(value) else value
        }
        return result
    }

}
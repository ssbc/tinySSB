// tinyssblib/la_wifi_web

// local access: web via wifi

#include <Arduino.h>

#include <WiFi.h>
//#include <FS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebSrv.h>
 
const char* ws_ssid = "tinySSB";
const char* password =  "hermies";
 
AsyncWebServer server(80);
 
// const char index_html[] PROGMEM = "<html><body><form action=\"/network_config\" method=\"POST\"><fieldset><legend>Wifi</legend><label>SSID</label><input name=\"ssid\" type=\"text\" /><br /><label>Password</label><input id=\"pwd\" name=\"pwd\" type=\"password\" /><br /><input type=\"submit\" value=\"Submit\" /></fieldset></form><form action=\"/lora_config\"  method=\"POST\" oninput=\"sf=parseInt(SF.value);bw=parseFloat(BW.value);cr=parseInt(CR.value);DR.value=((sf*(4/cr)*bw*1000)/(2**sf)).toFixed(0);sfn = [];sfn[6]=-5;sfn[7]=-7.5;sfn[8]=-10;sfn[9]=-12.5;sfn[10]=-15;sfn[11]=-17.5;sfn[12]=-20;sen.value=((10*Math.log10(bw*1000))+sfn[sf]+6-174).toFixed(2);Ts.value=(2**sf)/bw;Tpacket.value=(((111*(cr/4))+4.25+parseInt(preamble.value))*Ts.value).toFixed(2)\"><fieldset><legend>Lora</legend><label>LoRa chipset</label><select id=\"chipset\" name=\"chipset\"><option selected=\"\" value=\"SX1276\">Semtech SX1276</option><option value=\"SX1262\">Semtech SX1262</option></select><br /><label>Carrier Frequency</label><input id=\"freq\" max=\"1020\" min=\"137\" name=\"freq\" step=\".061035\" type=\"number\" value=\"908\" /><br /><label>Spreading Factor</label><select id=\"SF\" name=\"SF\"><option value=\"7\">7</option><option selected=\"\" value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option></select><br /><label>Coding Rate</label><select id=\"CR\" name=\"CR\"><option selected=\"\" value=\"5\">4/5</option><option value=\"6\">4/6</option><option value=\"7\">4/7</option><option value=\"8\">4/8</option></select><br /><label>Bandwidth</label><select id=\"BW\" name=\"BW\"><option value=\"62.5\">62.5 kHz</option><option value=\"125\">125 kHz</option><option selected=\"\" value=\"250\">250 kHz</option><option value=\"500\">500 kHz</option></select><br /><label>Transmission Power (dBm)</label><input id=\"power\" max=\"20\" min=\"-4\" name=\"power\" step=\"1\" type=\"number\" value=\"17\" /><br /><label>Preamble (symbols)</label><input id=\"preamble\" max=\"65535\" min=\"6\" name=\"preamble\" step=\"1\" type=\"number\" value=\"6\" /><br /><input type=\"submit\" value=\"Submit\" /><hr /><label>Data Rate: </label><output name=\"DR\"></output> bps<br /><label>Sensitivity: </label><output name=\"sen\"></output> dBs<br /><label>Symbol Time: </label><output name=\"Ts\"></output> ms<br /><label>100 byte packet time: </label><output name=\"Tpacket\"></output> ms</fieldset></form><form action=\"/gps_config\"  method=\"POST\"><fieldset><legend>GPS</legend><label>Timezone offset</label><input id=\"offset\" max=\"12\" min=\"-12\" name=\"offset\" step=\"1\" type=\"number\" value=\"0\" /><br /><input type=\"submit\" value=\"Submit\" /></fieldset></form></body></html>";

const char index_html[] PROGMEM =
"<html><body>"

  "<p><a href=/>Home</a><p>"

  "<form action=\"/network_config\" method=\"POST\"><fieldset>"
     "<legend>Wifi</legend>"
     "<label>SSID</label>&nbsp;"
       "<input name=\"ssid\" type=\"text\" /><br>"
     "<label>Password</label>&nbsp;"
       "<input id=\"pwd\" name=\"pwd\" type=\"password\" /><br>"
       "<input type=\"submit\" value=\"Submit\" />"
  "</fieldset></form>"

  "<form action=\"/lora_config\"  method=\"POST\" oninput=\"sf=parseInt(SF.value);bw=parseFloat(BW.value);cr=parseInt(CR.value);DR.value=((sf*(4/cr)*bw*1000)/(2**sf)).toFixed(0);sfn = [];sfn[6]=-5;sfn[7]=-7.5;sfn[8]=-10;sfn[9]=-12.5;sfn[10]=-15;sfn[11]=-17.5;sfn[12]=-20;sen.value=((10*Math.log10(bw*1000))+sfn[sf]+6-174).toFixed(2);Ts.value=(2**sf)/bw;Tpacket.value=(((111*(cr/4))+4.25+parseInt(preamble.value))*Ts.value).toFixed(2)\"><fieldset>"
    "<legend>Lora</legend>"
    "<label>LoRa chipset</label>&nbsp;"
      "<select id=\"chipset\" name=\"chipset\">"
        "<option selected=\"\" value=\"SX1276\">Semtech SX1276</option>"
        "<option value=\"SX1262\">Semtech SX1262</option></select><br>"
    "<label>Carrier Frequency</label>&nbsp;"
      "<input id=\"freq\" max=\"1020\" min=\"137\" name=\"freq\" step=\".061035\" type=\"number\" value=\"908\" /><br>"
    "<label>Spreading Factor</label>&nbsp;"
      "<select id=\"SF\" name=\"SF\">"
        "<option value=\"7\">7</option>"
        "<option selected=\"\" value=\"8\">8</option>"
        "<option value=\"9\">9</option>"
        "<option value=\"10\">10</option>"
        "<option value=\"11\">11</option>"
        "<option value=\"12\">12</option></select><br>"
    "<label>Coding Rate</label>&nbsp;"
      "<select id=\"CR\" name=\"CR\">"
        "<option selected=\"\" value=\"5\">4/5</option>"
        "<option value=\"6\">4/6</option>"
        "<option value=\"7\">4/7</option>"
        "<option value=\"8\">4/8</option></select><br>"
    "<label>Bandwidth</label>&nbsp;"
      "<select id=\"BW\" name=\"BW\">"
        "<option value=\"62.5\">62.5 kHz</option>"
        "<option value=\"125\">125 kHz</option>"
        "<option selected=\"\" value=\"250\">250 kHz</option>"
        "<option value=\"500\">500 kHz</option></select><br>"
    "<label>Transmission Power (dBm)</label>&nbsp;"
      "<input id=\"power\" max=\"20\" min=\"-4\" name=\"power\" step=\"1\" type=\"number\" value=\"17\" /><br>"
    "<label>Preamble (symbols)</label>&nbsp;"
      "<input id=\"preamble\" max=\"65535\" min=\"6\" name=\"preamble\" step=\"1\" type=\"number\" value=\"6\" /><br>"
      "<input type=\"submit\" value=\"Submit\" /><hr />"
    "<label>Data Rate: </label>"
      "<output name=\"DR\"></output> bps<br>"
    "<label>Sensitivity: </label>"
      "<output name=\"sen\"></output> dBs<br>"
    "<label>Symbol Time: </label>"
      "<output name=\"Ts\"></output> ms<br>"
    "<label>100 byte packet time: </label>"
      "<output name=\"Tpacket\"></output> ms"
  "</fieldset></form>"

  "<form action=\"/gps_config\"  method=\"POST\"><fieldset>"
    "<legend>GPS</legend>"
    "<label>Timezone offset</label>&nbsp;"
      "<input id=\"offset\" max=\"12\" min=\"-12\" name=\"offset\" step=\"1\" type=\"number\" value=\"0\" /> (in hours)<br>"
      "<input type=\"submit\" value=\"Submit\" />"
  "</fieldset></form>"

"</body></html>";

const char _template[] PROGMEM = "<html><head><meta charset=\"UTF-8\"></head>"
  "<body>"
    "<p>Temperature: %INTERNAL_TEMP% ºC</p>"
    "<p>Hall Sensor: %HALL_EFFECT%</p>"
    "<p>Random Number: %RANDOM_NUM%</p>"
    "<p>&nbsp;<br>Configure this device: <a href=/config>/config</a></p>"
  "</body></html>";


/* html template processor */
String processor(const String& var)
{
  if(var == "INTERNAL_TEMP")
    return String(temperatureRead());

  if (var == "HALL_EFFECT")
    return String(hallRead());

  if (var == "RANDOM_NUM")
  return "<" + String(random(0,100)) + ">";

  return String();    
}


void setup_la_wifi_web(){
  //  Serial.begin(115200);
 
/*  
  // Connect to existing wifi instead of using a SoftAP

  WiFi.begin(ws_ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Could not initialize WiFi");
    return;
}
      
  Serial.print("Starting webserver at http://");
  Serial.print(WiFi.localIP());
  Serial.println("/config");
*/


  /* Start Soft Access Point */
 
  // Manually specify IPAddress
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1),
                    IPAddress(192, 168, 4, 1),
                    IPAddress(255, 255, 255, 0));
  WiFi.softAP(ws_ssid, password);

  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  /* Async Server routing */ 

  //example of template processing
  server.on("/", HTTP_GET,  [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", _template, processor);
  });

  server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });

  //TODO: process network_config form
  server.on("/network_config", HTTP_POST, [](AsyncWebServerRequest *request){
        String ssid;
        String pwd;
        if (request->hasParam("ssid", true)) {
            ssid = request->getParam("ssid", true)->value();
        } else {
            ssid = "none";
        }
        if (request->hasParam("pwd", true)) {
            pwd = request->getParam("pwd", true)->value();
        } else {
            pwd = "none";
        }
        request->send(200, "text/plain", "SSID: " + ssid + " Password: " + pwd);
    });

  //TODO: process lora_config form
  server.on("/lora_config", HTTP_POST, [](AsyncWebServerRequest * request){
    String chipset;
    String freq;
    String SF;
    String CR;
    String BW;
    String power;
    String preamble;
    if (request->hasParam("chipset", true)){
      chipset = request->getParam("chipset", true)->value();
      } else {
          chipset = "none";
      }
    if (request->hasParam("freq", true)){
      freq = request->getParam("freq", true)->value();
      } else {
          freq = "none";
      }
    if (request->hasParam("SF", true)){
      SF = request->getParam("SF", true)->value();
      } else {
          SF = "none";
      }
    if (request->hasParam("CR", true)){
      CR = request->getParam("CR", true)->value();
      } else {
          CR = "none";
      }
    if (request->hasParam("BW", true)){
      BW = request->getParam("BW", true)->value();
      } else {
          BW = "none";
      }
    if (request->hasParam("power", true)){
      power = request->getParam("power", true)->value();
      } else {
          power = "none";
      }
if (request->hasParam("preamble", true)){
      preamble = request->getParam("preamble", true)->value();
      } else {
          preamble = "none";
      } 
    request->send(200, "text/plain", "Chipset: " + chipset + " Frequency: " + freq + " Spreading Factor: " + SF + " Coding Rate: 4/" + CR + " Bandwidth: " + BW + " Transmit Power: " + power + " Preamble: " + preamble);
  });
  
  //TODO: process gps_config form
  server.on("/gps_config", HTTP_POST, [](AsyncWebServerRequest * request){
    String offset;
    if (request->hasParam("offset", true)){
      offset = request->getParam("offset", true)->value();
    } else {
      offset = "none";
    }
    request->send(200, "text/plain", "GPS offset: " + String(offset));
  });

  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "The content you are looking for was not found.");
  });
 
  server.begin();
}
 
// eof

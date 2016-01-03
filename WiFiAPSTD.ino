#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <ArduinoJson.h>

bool mdns_started = false;

ESP8266WebServer server(80);
MDNSResponder mdns;

String ssid;
String pass;
String host;

void readSettings() {
  //TODO close file
  ssid = SPIFFS.open("ssid", "r").readString();
  pass = SPIFFS.open("pass", "r").readString();
  host = SPIFFS.open("host", "r").readString();

  Serial.println("Einstellungen:");
  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(host);
}

void writeToFile(String path, String s) {
  File f = SPIFFS.open(path, "w");
  f.print(s);
  f.close();
}

void writeSettings() {
  writeToFile("ssid", ssid);
  writeToFile("pass", pass);
  writeToFile("host", host);

  WiFi.begin (ssid.c_str(), pass.c_str());
}

String inputField(String name, String value) {
  String s = "<input name=\"";
  s += name;
  s += "\" value=\"";
  s += value;
  s += "\" \\>";
  return s;
}

String ipToString(IPAddress addr) {
  char ip[24];
  sprintf(ip, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
  return String(ip);
}

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
 * connected to this access point to see it.
 */
void handleSettings() {
  if (server.hasArg("ssid") && server.hasArg("pass") && server.hasArg("host"))  {
    ssid = server.arg("ssid");
    pass = server.arg("pass");
    host = server.arg("host");

    writeSettings();
    server.send(200, "text/html", "OK");
  }
  else {
    String content;
    
    DynamicJsonBuffer jsonBuffer(JSON_OBJECT_SIZE(5));
    JsonObject& root = jsonBuffer.createObject();
    root["ssid"] = ssid;
    root["pass"] = pass;
    root["host"] = host;
    root.printTo(content);

    server.send(200, "application/json", content);
  }
}

void handleInfo() {
  String str = "{";
  str += "\"ip_ap\":\"" + ipToString(WiFi.softAPIP()) + "\",";
  str += "\"ip_local\":\"" + ipToString(WiFi.localIP()) + "\",";
  str += "\"status\":\"" + String(WiFi.status()) + "\",";
  str += "\"channel\":\"" + String(WiFi.channel()) + "\",";
  str += "\"signal\":\"" + String(WiFi.RSSI()) + "\"";
  str += "}";
  server.send(200, "application/json", str);
}

void handleRoot() {
  File f = SPIFFS.open("/index.html", "r");
  server.send(200, "text/html", f.readString());
}

void handleNotFound() {
  File f = SPIFFS.open(server.uri(), "r");
  if (!f) {
    server.send(404, "text/html", "Not found.");
    return;
  }

  String dataType = "text/html";

  if(server.uri().endsWith(".png")) dataType = "image/png";
  else if(server.uri().endsWith(".js")) dataType = "application/javascript";
  
  server.send(200, dataType, f.readString());
}

void setup() {
	delay(1000);
	Serial.begin(115200);
  
  WiFi.mode(WIFI_AP_STA);

  WiFi.begin (ssid.c_str(), pass.c_str());
  Serial.println ( "" );

  SPIFFS.begin();
  readSettings();

	Serial.println();
	Serial.print("Configuring access point...");
	/* You can remove the password parameter if you want the AP to be open. */
	WiFi.softAP("teest", "penis");

	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(myIP);
	server.on("/", handleRoot);
 server.on("/settings", handleSettings);
 server.on("/info", handleInfo);
 server.onNotFound(handleNotFound);
	server.begin();
	Serial.println("HTTP server started");
}

void loop() {
  if ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
    mdns_started = false;
  }
  else {
    if (!mdns_started) {
      mdns_started = mdns.begin ( "blubber", WiFi.localIP() );
      if (mdns_started) Serial.println("mDNS started");
    }
    else {
      mdns.update();
    }
  }
  
	server.handleClient();
 
}

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <time.h>

/* Wi-Fi アクセスポイントの設定 */
#define MY_SSID "2U-NTP-Clock-setup"
#define MY_PSK  "esp32setup"

/* Web サーバのポート番号を定義 */
#define WEBSERVER_PORT 80

/* デバッグ通信の速度を定義 */
#define BAUDRATE 115200

/* Web ページのテンプレート */
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Welcome to Setup Page!</title>
</head>
<body>
  <h2>Wi-Fi Connection Setting</h2>
  <form action="/save" method="POST">
    SSID: <input type="text" name="ssid"><br>
    PSK: <input type="password" name="psk"><br>
    NTP Server: <input type="next" name="ntp" value="ntp.nict.jp"><br>
    <input type="submit" value="save">
  </form>
</body>
</html>
)rawliteral";

/* 初期設定時の自身の IP アドレス */
IPAddress local(192, 168, 32, 254);
IPAddress gateway(192, 168, 32, 254);
IPAddress subnet(255, 255, 255, 0);

/* サーバ関係 */
WebServer server(WEBSERVER_PORT);
Preferences prefs;

void setup() {
  Serial.begin(BAUDRATE);

  prefs.begin("config", true);

  String ssid = prefs.getString("ssid", "");
  String psk  = prefs.getString("psk" , "");
  String ntp  = prefs.getString("ntp" , "");
  prefs.end();

  if(ssid == "" || psk == "" || ntp == "") {
    Serial.print("No any config found. Start configuration Web-UI.\n");
    wifi_setup();
    server_setup();
  }else {
    Serial.print("Try to connect with saved config.\n");

    WiFi.begin(ssid.c_str(), psk.c_str());

    uint32_t t = millis();

    while(WiFi.status() != WL_CONNECTED && millis() - t < 15000) {
      delay(500); Serial.print(".");
    }

    if(WiFi.status() == WL_CONNECTED) {
      Serial.print("\nSuccessed to connect to your network!\n");
      Serial.print("My IP : "); Serial.println(WiFi.localIP());
      Serial.print("NTP Server Address : "); Serial.println(ntp);
      get_time_from_server(ntp.c_str());
    }else {
      Serial.print("Failed to connect to your network.\nPlease setup again...\n");
      prefs.begin("config", false);
      prefs.clear();
      prefs.end();

      delay(1000);

      ESP.restart();
    }
  }
}

void loop() {
  if(WiFi.getMode() == WIFI_AP) server.handleClient();
}

void wifi_setup(void) {
  WiFi.mode(WIFI_MODE_AP);

  if(WiFi.softAP(MY_SSID, MY_PSK)) {
    Serial.print("Success to start AP.\n");
  }else {
    Serial.print("Failed to start AP...\n");

    return;
  }

  if(WiFi.softAPConfig(local, gateway, subnet)) {
    Serial.print("Success to configure.\n");
  }else {
    Serial.print("Failed to configure...\n");

    return;
  }
  
  Serial.print("Wi-Fi Setup done!\n");
  Serial.printf("IP Address of Web UI : %s\n", WiFi.softAPIP().toString().c_str());

  return;
}

void server_setup(void) {
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
}

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleSave() {
  if (server.hasArg("ssid") && server.hasArg("psk") && server.hasArg("ntp")) {
    String ssid = server.arg("ssid");
    String psk  = server.arg("psk");
    String ntp  = server.arg("ntp");

    Serial.printf("SSID : %s\nPSK  : %s\nNTP Server : %s\n", ssid.c_str(), psk.c_str(), ntp.c_str());

    prefs.begin("config", false);
    prefs.putString("ssid", ssid);
    prefs.putString("psk" , psk);
    prefs.putString("ntp" , ntp);
    prefs.end();

    server.send(200, "text/html", "Setting data were successfully saved.<br>The system will automatically restart soon...");

    delay(2000);

    ESP.restart();
  } else {
    server.send(400, "text/plain", "Invalid Access.");
  }
}

void get_time_from_server(const char* ntpServer) {
  configTime(0, 0, ntpServer);

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.print("Failed to get time...\n");
    return;
  }

  time_t now = time(NULL);
  Serial.print("UNIX Time : ");
  Serial.println(now);
}


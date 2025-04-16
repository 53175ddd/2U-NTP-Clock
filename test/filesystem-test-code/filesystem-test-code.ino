#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

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
    <input type="submit" value="save">
  </form>
</body>
</html>
)rawliteral";

/* 初期設定時の自身の IP アドレス */
IPAddress local(192, 168, 32, 254);
IPAddress gateway(192, 168, 32, 254);
IPAddress subnet(255, 255, 255, 0);

/* Web サーバ */
WebServer server(WEBSERVER_PORT);
Preferences prefs;

void setup() {
  Serial.begin(BAUDRATE);
  wifi_setup();
  server_setup();
}

void loop() {
  server.handleClient();
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
  Serial.printf("IP Address of this Access Point : %s\n", WiFi.softAPIP().toString().c_str());

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
  if (server.hasArg("ssid") && server.hasArg("psk")) {
    String ssid = server.arg("ssid");
    String psk = server.arg("psk");

    Serial.printf("SSID : %s\nPSK  : %s\n", ssid.c_str(), psk.c_str());

    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("psk", psk);
    prefs.end();

    String response = "Saved your SSID and PSK.<br><a href=\"/\">Back</a>";
    server.send(200, "text/html", response);
  } else {
    server.send(400, "text/plain", "Invalid Access.");
  }
}

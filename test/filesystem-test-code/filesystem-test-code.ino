#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <time.h>
#include <Wire.h>
#include <RTClib.h>

/* Wi-Fi アクセスポイントの設定 */
#define MY_SSID "2U-NTP-Clock-setup"
#define MY_PSK  "esp32setup"

/* Web サーバのポート番号を定義 */
#define WEBSERVER_PORT 80

/* デバッグ通信の速度を定義 */
#define BAUDRATE 115200

/* I2C モジュールのポート定義 */
#define SDA 20
#define SCL 21
#define SQW 40

/* RTC のアドレス */
#define DS1307_I2C_ADDR 0x68

/* Web ページのテンプレート */
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>初期設定</title>
</head>
<body>
  <h2>Wi-Fi 接続 / NTP サーバアドレス / タイムゾーン設定</h2>
  <form action="/save" method="POST">
    SSID: <input type="text" name="ssid"><br>
    PSK: <input type="password" name="psk"><br>
    NTP サーバアドレス: <input type="next" name="ntp" value="ntp.nict.jp"><br>
    タイムゾーン: <input type="next" name="tz" value="JST-9"><br>
    <input type="submit" value="保存">
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

/* RTC 関係 */
RTC_DS1307 rtc;

void setup() {
  Serial.begin(BAUDRATE);

  prefs.begin("config", true);
  String ssid = prefs.getString("ssid", "");
  String psk  = prefs.getString("psk" , "");
  String ntp  = prefs.getString("ntp" , "");
  String tz   = prefs.getString("tz"  , "");
  prefs.end();

  if(ssid == "" || psk == "" || ntp == "" || tz == "") {
    Serial.print("保存された設定が見つかりません。初期設定用 Web サーバを実行します。\n");
    wifi_setup();
    server_setup();
  }else {
    Serial.print("保存されている設定で接続を試みます。\n");

    WiFi.begin(ssid.c_str(), psk.c_str());

    uint32_t t = millis();

    while(WiFi.status() != WL_CONNECTED && millis() - t < 10000) {
      delay(500); Serial.print(".");
    }

    if(WiFi.status() == WL_CONNECTED) {
      Serial.print("NTP Server Address : "); Serial.println(ntp);
      get_time_from_server(tz.c_str(), ntp.c_str());
      Serial.print("\n接続に成功しました！\n");
      Serial.print("取得した IP アドレス : "); Serial.println(WiFi.localIP());
    }else {
      Serial.print("\n接続に失敗しました。\n設定をやり直してください。再起動します。\n");
      prefs.begin("config", false);
      prefs.clear();
      prefs.end();

      delay(1000);

      ESP.restart();
    }

    Serial.print("一度切断します。\n");
    WiFi.disconnect(true);

    Wire.begin(SDA, SCL);

    if(!rtc.begin()) {
      Serial.print("RTC モジュールが見つかりません。\nまもなく自動的に再起動します。\n");

      delay(500);

      ESP.restart();
    }

    lock_rtc();

    Wire.beginTransmission(DS1307_I2C_ADDR);
    Wire.write(0x07);
    Wire.write(0b00010000);
    Wire.endTransmission();

    pinMode(SQW, INPUT);
  }
}

void loop() {
  if(WiFi.getMode() == WIFI_AP) {
    server.handleClient();
  }else {
    static uint8_t sqw = 0;

    sqw = (sqw << 1) + ((digitalRead(SQW) == HIGH) ? 0 : 1);

    if((sqw & 0b00000011) == 0b01) {
      DateTime now = rtc.now();

      Serial.printf("%04d/%02d/%02d %02d:%02d:%02d\n", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
    }

    if((sqw & 0b00000011) == 0b10) {
    }

    delay(10);
  }
}

void wifi_setup(void) {
  WiFi.mode(WIFI_MODE_AP);

  if(WiFi.softAP(MY_SSID, MY_PSK)) {
    Serial.print("アクセスポイントの起動に成功しました。\n");
    Serial.printf("SSID は \"%s\" 、PSK は \"%s\" です。\n", MY_SSID, MY_PSK);
  }else {
    Serial.print("アクセスポイントの起動に失敗しました。\n");

    return;
  }

  if(WiFi.softAPConfig(local, gateway, subnet)) {
    Serial.print("IP アドレスの設定が完了しました。\n");
  }else {
    Serial.print("IP アドレスの設定に失敗しました。\n");

    return;
  }
  
  Serial.print("Wi-Fi の設定が完了しました。\n");
  Serial.printf("設定用 Web サーバのアドレスは http://%s です。\n", WiFi.softAPIP().toString().c_str());

  return;
}

void server_setup(void) {
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.begin();
  Serial.print("Web UI の設定が完了しました。\n");
}

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleSave() {
  if (server.hasArg("ssid") && server.hasArg("psk") && server.hasArg("ntp") && server.hasArg("tz")) {
    String ssid = server.arg("ssid");
    String psk  = server.arg("psk");
    String ntp  = server.arg("ntp");
    String tz   = server.arg("tz");

    Serial.printf("以下の内容で設定を保存します。\nSSID => %s\nPSK => %s\nNTP サーバアドレス => %s\nタイムゾーン => %s\n", ssid.c_str(), psk.c_str(), ntp.c_str(), tz.c_str());

    prefs.begin("config", false);
    prefs.putString("ssid", ssid);
    prefs.putString("psk" , psk);
    prefs.putString("ntp" , ntp);
    prefs.putString("tz"  , tz);
    prefs.end();

    server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>設定完了</title></head><body>入力された情報を保存しました。<br>まもなく再起動します。</body></html>");

    delay(2000);

    ESP.restart();
  } else {
    server.send(400, "text/plain", "Invalid Access.");
  }
}

void get_time_from_server(const char* tz, const char* server) {
  configTzTime("JST-9", server);

  struct tm time_info;
  if (!getLocalTime(&time_info)) {
    Serial.print("Failed to get time...\n");
    return;
  }

  time_t now = time(NULL);
  Serial.print("UNIX Time : ");
  Serial.println(now);
}

void lock_rtc(void) {
  prefs.begin("config", true);
  String ssid = prefs.getString("ssid", "");
  String psk  = prefs.getString("psk" , "");
  String ntp  = prefs.getString("ntp" , "ntp.nict.jp");
  String tz   = prefs.getString("tz"  , "JST-9");
  prefs.end();

  Serial.print("保存されている内容で接続を試みます。\n");

  WiFi.begin(ssid.c_str(), psk.c_str());

  uint32_t t = millis();

  while(WiFi.status() != WL_CONNECTED && millis() - t < 10000) {
    delay(500); Serial.print(".");
  }

  if(WiFi.status() == WL_CONNECTED) {
    Serial.print("\n接続に成功しました！\n");
    Serial.print("取得した IP アドレス : "); Serial.println(WiFi.localIP());
  }

  configTzTime(tz.c_str(), ntp.c_str());

  struct tm time_info;

  if(getLocalTime(&time_info)) {
    rtc.adjust(DateTime(time_info.tm_year + 1900, time_info.tm_mon + 1, time_info.tm_mday, time_info.tm_hour, time_info.tm_min, time_info.tm_sec));
    Serial.print("RTC の時刻を更新しました。\n");
  }

  WiFi.disconnect(true);
}

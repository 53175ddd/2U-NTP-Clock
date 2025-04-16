#include <WiFi.h>

#define WEBSERVER_PORT 80

#define BAUDRATE 115200

IPAddress local(192, 168, 1, 254);
IPAddress gateway(192, 168, 1, 254);
IPAddress subnet(255, 255, 255, 0);

const char *my_ssid = "2U-NTP-Clock-setup";
const char *my_psk  = "setup";

void setup() {
  Serial.begin(BAUDRATE);
  wifi_setup();
}

void loop() {
}

void wifi_setup(void) {
  if(WiFi.softAP(my_ssid, my_psk)) {
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

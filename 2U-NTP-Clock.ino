#include <wifi.h>
#include <Wire.h>

/* 各種ピンの定義 */

#define SW_OK    3
#define SW_UP    5
#define SW_DOWN  1
#define SW_RIGHT 4
#define SW_LEFT  2

#define I2C_SDA 13
#define I2C_SCL 15

#define SQWE_IN 14
#define DOT_OUT 12

#define HC595_OE    40
#define HC595_SER   46
#define HC595_SRCLR 42
#define HC595_RCLK  41
#define HC595_SRCLK 39

/* Wi-Fi アクセスポイントの設定 */
/* "" の中身を変更してください． */

char ssid[] = "YOUR-SSID-HERE";
char psk[]  = "YOUR-PSK-HERE";

uint8_t font[16];

void setup() {
  /* GPIO の初期化 */
  pinMode(SW_OK   ,  INPUT_PULLUP);
  pinMode(SW_UP   ,  INPUT_PULLUP);
  pinMode(SW_DOWN ,  INPUT_PULLUP);
  pinMode(SW_RIGHT,  INPUT_PULLUP);
  pinMode(SW_LEFT ,  INPUT_PULLUP);

  pinMode(SQWE_IN,  INPUT);
  pinMode(DOT_OUT, OUTPUT);  digitalWrite(DOT_OUT,  LOW);

  pinMode(HC595_SER  , OUTPUT);  digitalWrite(HC595_SER  ,  LOW);
  pinMode(HC595_SRCLR, OUTPUT);  digitalWrite(HC595_SRCLR,  LOW);
  pinMode(HC595_RCLK , OUTPUT);  digitalWrite(HC595_RCLK ,  LOW);
  pinMode(HC595_SRCLK, OUTPUT);  digitalWrite(HC595_SRCLK,  LOW);  digitalWrite(HC595_SRCLK, HIGH);
  pinMode(HC595_OE   , OUTPUT);  digitalWrite(HC595_OE   , HIGH);  digitalWrite(HC595_OE   ,  LOW);

  /* I2C の初期化 */
  Wire.setPins(I2C_SDA, I2C_SCL);
  Wire.begin();

  /* RTC の初期化 */
}

void loop() {
  /* ドットの点滅の処理 */
  static uint8_t sq = 0;

  sq = (sq << 1) + ((digitalRead(SQWE_IN) == HIGH) ? 0 : 1);

  if((sq & 0b00001111) == 0b0011) {
    digitalWrite(DOT_OUT, HIGH);
  }else if((sq & 0b00001111) == 0b1100) {
    digitalWrite(DOT_OUT,  LOW);
  }

  /* 時刻表示の処理 */
}

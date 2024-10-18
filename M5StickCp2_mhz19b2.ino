#include <M5StickCPlus2.h>
#include "MHZ19.h"

#define RX_PIN 36                                          // Rx pin which the MHZ19 Tx pin is attached to
#define TX_PIN 26                                          // Tx pin which the MHZ19 Rx pin is attached to
#define BAUDRATE 9600                                      // Device to MH-Z19 Serial baudrate (should not be changed)

#define LCD_MODE_DIGIT 0
#define LCD_MODE_GRAPH 1

#define BRIGHTNESS 30
#define M5_LED 10

MHZ19 myMHZ19;                                             // Constructor for library
HardwareSerial mySerial(1);                                // (ESP32 Example) create device to MH-Z19 serial

bool lcdOn = true;
int lcdMode = LCD_MODE_DIGIT;

bool ledOn = false;
bool ledValue = false;
unsigned long nextLedOperationTime = 0;

unsigned long getDataTimer = 0;

int history[160] = {};
int historyPos = 0;

void setup() {
  M5.begin();
  // M5.Axp.ScreenBreath(BRIGHTNESS);
  M5.Lcd.setBrightness(BRIGHTNESS);


  Serial.begin(9600);                                     // Device to serial monitor feedback

  mySerial.begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN); // (ESP32 Example) device to MH-Z19 serial start
  myMHZ19.begin(mySerial);                                // *Serial(Stream) refence must be passed to library begin().
  myMHZ19.autoCalibration(true);

  M5.Lcd.setRotation(1);
  render();

  pinMode(M5_LED, OUTPUT);
}

void loop() {
  auto now = millis();

  // Aボタン: モードを切り替える
  M5.update();
  if ( M5.BtnA.wasPressed() ) {
    lcdMode = (lcdMode + 1) % 2;
    render();
  }

  // Bボタン: 画面表示を ON/OFF する
  if ( M5.BtnB.wasPressed() ) {
    lcdOn = !lcdOn;

    if (lcdOn) {
      render();
      // M5.Axp.ScreenBreath(BRIGHTNESS);
      M5.Lcd.setBrightness(BRIGHTNESS);

    } else {
      M5.Lcd.setBrightness(0);
      // M5.Axp.ScreenBreath(0);
    }
  }

  if ((ledOn || !ledValue) && nextLedOperationTime <= now) {
    ledValue = !ledValue;
    digitalWrite(M5_LED, ledValue);

    // Lチカ
    if (ledValue) {
      nextLedOperationTime = now + 15000;
    } else {
      nextLedOperationTime = now + 800;
    }
  }

  if (now - getDataTimer >= 30000) {
    /* note: getCO2() default is command "CO2 Unlimited". This returns the correct CO2 reading even
      if below background CO2 levels or above range (useful to validate sensor). You can use the
      usual documented command with getCO2(false) */

    int CO2 = myMHZ19.getCO2();                         // Request CO2 (as ppm)
    // int8_t temp = myMHZ19.getTemperature(false, true);  // Request Temperature (as Celsius)
    int8_t temp = myMHZ19.getTemperature();  // Request Temperature (as Celsius)

    Serial.print("CO2 (ppm): ");
    Serial.print(CO2);
    Serial.print(", Temperature (C): ");
    Serial.println(temp);

    ledOn = CO2 >= 1200;

    // 測定結果の表示
    historyPos = (historyPos + 1) % (sizeof(history) / sizeof(int));
    history[historyPos] = CO2;
    render();

    getDataTimer = now;
  }
}

void render() {
  if (!lcdOn) {
    return;
  }

  // Clear
  int height = M5.Lcd.height();
  int width = M5.Lcd.width();
  M5.Lcd.fillRect(0, 0, width, height, BLACK);

  switch (lcdMode) {
    case LCD_MODE_DIGIT:
      M5.Lcd.drawString("CO2 [ppm]", 12, 0, 2);
      M5.Lcd.drawRightString("    " + (String)history[historyPos], width, 24, 7);
      break;
    case LCD_MODE_GRAPH:
      int len = sizeof(history) / sizeof(int);
      for (int i = 0; i < len; i++) {
        auto value = max(0, history[(historyPos + 1 + i) % len] - 400);
        auto y = min(height, (int)(value * (height / 1200.0)));
        auto color = min(255, (int)(value * (255 / 1200.0)));
        M5.Lcd.drawLine(i, height - y, i, height, M5.Lcd.color565(255, 255 - color, 255 - color));
      }
      break;
  }
}

#include <M5StickCPlus.h>

uint32_t lastToggle = 0;
bool ledOn = false;

void setup() {
  M5.begin();
  Serial.begin(115200);

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(12, 18);
  M5.Lcd.println("M5StickC Plus");
  M5.Lcd.setCursor(12, 48);
  M5.Lcd.println("Upload OK");

  Serial.println("M5StickC Plus test app booted");
}

void loop() {
  M5.update();

  const uint32_t now = millis();
  if (now - lastToggle >= 500) {
    lastToggle = now;
    ledOn = !ledOn;
    digitalWrite(10, ledOn ? LOW : HIGH);

    M5.Lcd.fillRect(12, 82, 220, 24, BLACK);
    M5.Lcd.setCursor(12, 82);
    M5.Lcd.printf("Uptime: %lus", now / 1000);
    Serial.printf("tick %lu\n", now / 1000);
  }
}

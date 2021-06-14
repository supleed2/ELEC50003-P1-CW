#include <Arduino.h>
#include <M5StickC.h>

float pitch, roll, yaw;

void setup() {
  M5.begin();
  M5.IMU.Init();
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, 26, 0);
}
void loop() {
  M5.IMU.getAhrsData(&pitch, &roll, &yaw);
  M5.Lcd.setCursor(0, 45);
  M5.Lcd.printf("%5.2f\n", roll);
  Serial.printf("{\"cH\":%5.2f}\n", roll);
  Serial1.printf("{\"cH\":%5.2f}\n", roll);
  delay(50);
}
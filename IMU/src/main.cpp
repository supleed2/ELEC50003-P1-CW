#include <Arduino.h>
#include <M5StickC.h>
#include <ArduinoJson.h>

float pitch, roll, yaw;
// unsigned long previous_millis;
// unsigned long current_millis;
double yaw_correction = 0;

void setup() {
  M5.begin();
  M5.IMU.Init();
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, 26, 0);
}
void loop() {
  M5.IMU.getAhrsData(&pitch, &roll, &yaw);
  M5.Lcd.setCursor(0, 45);
  M5.Lcd.printf("%5.2f\n", yaw);
  // Serial.printf("{\"cH\":%5.2f}\n", roll);
  // Serial1.printf("{\"cH\":%5.2f}\n", roll);

  // current_millis = millis();
  // float t_diff = (current_millis - previous_millis);
  // previous_millis = current_millis;
  yaw_correction = yaw_correction + (0.041808);

  Serial.println(yaw + yaw_correction);

  DynamicJsonDocument tdoc(1024);
  tdoc["cH"] = yaw;
	serializeJson(tdoc, Serial1);

	

  delay(52);
}


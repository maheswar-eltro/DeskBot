#include <Wire.h>

#define SDA_PIN 0
#define SCL_PIN 1
#define MPU_ADDR 0x68

void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);

  // Wake MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission();
}

void loop() {
  // Read gyro X, Y, Z
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x43);
  Wire.endTransmission(false);

  Wire.requestFrom(MPU_ADDR, 6);

  int16_t gx = (Wire.read() << 8) | Wire.read();
  int16_t gy = (Wire.read() << 8) | Wire.read();
  int16_t gz = (Wire.read() << 8) | Wire.read();

  Serial.print("GX:");
  Serial.print(gx);
  Serial.print("\t");

  Serial.print("GY:");
  Serial.print(gy);
  Serial.print("\t");

  Serial.print("GZ:");
  Serial.println(gz);

  delay(50);
}
//Copyright (c) 2019 aNoŒ¤ ƒvƒŠƒ“‚ğŒ©ç‚é‹Zp
//https://github.com/anoken/purin_wo_mimamoru_gijutsu/

#include <M5StickC.h>
const int motorL_adr = 0x60;
const int motorR_adr = 0x64;
long Speed;
long SpeedL, SpeedR;


//Motor Driver Processing
void motor_drive_i2c_control(int motor_adr, int speed, byte data1) {
  byte regValue = 0x80;
  regValue = abs(speed);
  if (regValue > 100) regValue = 100;
  regValue = regValue << data1;
  if (speed < 0) regValue |= 0x01;    //reverse rotation
  else           regValue |= 0x02;    //Normal rotation

  Wire.beginTransmission(motor_adr);
  Wire.write(0x00);
  Wire.write(regValue);
  Wire.endTransmission(true);
}
void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.setCursor(0, 30, 4);
  M5.Lcd.println("glass porter");
  Wire.begin(33, 32, 10000);    //I2C Setting
  SpeedL = 0;
  SpeedR = 0;
  motor_drive_i2c_control(motorL_adr, (SpeedL), 0x02);
  motor_drive_i2c_control(motorR_adr, (SpeedR), 0x02);
}

void loop() {
  M5.update();

  if (M5.BtnA.wasPressed()) { //Motor Rotation
    SpeedL = 100;     
    SpeedR = -100;      
    motor_drive_i2c_control(motorL_adr, (SpeedL), 0x02);
    motor_drive_i2c_control(motorR_adr, (SpeedR), 0x02);
  }
  if (M5.BtnB.wasPressed()) { //Motor Stop
    SpeedL = 0;       
    SpeedR = 0;       
    motor_drive_i2c_control(motorL_adr, (SpeedL), 0x02);
    motor_drive_i2c_control(motorR_adr, (SpeedR), 0x02);
  }
}

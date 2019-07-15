//Copyright (c) 2019 aNo研 プリンを見守る技術
//https://github.com/anoken/purin_wo_mimamoru_gijutsu/

#include <M5StickC.h>
#include <BLEServer.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <WiFi.h>
#include "SPIFFS.h"
#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"
AudioGeneratorMP3 *mp3;
AudioFileSourceSPIFFS *file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;
int mp3_do = 0;
// Device Name: Maximum 30 bytes
#define DEVICE_NAME "LINE Things Trial M5StickC"

// User service UUID: Change this to your generated service UUID
#define USER_SERVICE_UUID "XXXXXX-XXXX-XXXX-XXXX-XXXXXXXX"
// User service characteristics
#define WRITE_CHARACTERISTIC_UUID "E9062E71-9E62-4BC6-B0D3-35CDCD9B027B"
#define NOTIFY_CHARACTERISTIC_UUID "62FBD229-6EDD-4D1A-B554-5C4E1BB29169"

// PSDI Service UUID: Fixed value for Developer Trial
#define PSDI_SERVICE_UUID "E625601E-9E55-4597-A598-76018A0D293D"
#define PSDI_CHARACTERISTIC_UUID "26E2B12B-85F0-4F3F-9FDD-91D114270E6E"

BLEServer* thingsServer;
BLESecurity *thingsSecurity;
BLEService* userService;
BLEService* psdiService;
BLECharacteristic* psdiCharacteristic;
BLECharacteristic* writeCharacteristic;
BLECharacteristic* notifyCharacteristic;

bool deviceConnected = false;
bool oldDeviceConnected = false;
const int motorL_adr = 0x60;
const int motorR_adr = 0x64;
long Speed;
long SpeedL, SpeedR;

class serverCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};
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
class writeCallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *bleWriteCharacteristic) {
      std::string rxValue = bleWriteCharacteristic->getValue();
      if (rxValue.length() > 0) {
        SpeedL = 0;
        SpeedR = 0;
        if (rxValue[0] = 1) {
          SpeedL = 20;
          SpeedR = 20;
        }
        if (rxValue[0] = 2) {
          SpeedL = -20;
          SpeedR = 20;
        }
        if (rxValue[0] = 3) {
          SpeedL = 20;
          SpeedR = -20;
        }
        if (rxValue[0] = 4) {
          SpeedL = -20;
          SpeedR = -20;
        }
        if (rxValue[0] = 5) {
          mp3_do = 1;
        }
        motor_drive_i2c_control(motorL_adr, (SpeedL), 0x02);
        motor_drive_i2c_control(motorR_adr, (SpeedR), 0x02);

      }
    }

};

void setup() {
  M5.begin();

  BLEDevice::init("");
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_NO_MITM);

  // Security Settings
  BLESecurity *thingsSecurity = new BLESecurity();
  thingsSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_ONLY);
  thingsSecurity->setCapability(ESP_IO_CAP_NONE);
  thingsSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  setupServices();
  startAdvertising();
  pinMode(M5_LED, OUTPUT);
  digitalWrite(M5_LED, HIGH);
  Wire.begin(33, 32, 10000);   //I2CのSDA, SCLポートを設定
  SpeedL = 0;
  SpeedR = 0;
  motor_drive_i2c_control(motorL_adr, (SpeedL), 0x02);
  motor_drive_i2c_control(motorR_adr, (SpeedR), 0x02);
}

void loop() {
  M5.update();
  // Disconnection
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // Wait for BLE Stack to be ready
    thingsServer->startAdvertising(); // Restart advertising
    oldDeviceConnected = deviceConnected;
    M5.Lcd.fillRect(0, 80, 80, 40, BLACK);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.setCursor(0, 80, 2);
    M5.Lcd.println("Not Connect");
  }
  // Connection
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
    M5.Lcd.fillRect(0, 80, 80, 40, BLACK);
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.setCursor(0, 80, 2);
    M5.Lcd.println("Connected");
  }

  if (mp3_do) {
    if (mp3->isRunning()) {
      if (!mp3->loop()) {
        mp3->stop();
        mp3_do = 0;
      }
    }

    else {
      file = new AudioFileSourceSPIFFS("/achirano_okyakusama_karadesu.mp3");
      id3 = new AudioFileSourceID3(file);
      out = new AudioOutputI2S(0, 1);
      out->SetOutputModeMono(true);
      out->SetGain(2.0);
      mp3 = new AudioGeneratorMP3();
      mp3->begin(id3, out);
    }
  }

}

void setupServices(void) {
  // Create BLE Server
  thingsServer = BLEDevice::createServer();
  thingsServer->setCallbacks(new serverCallbacks());

  // Setup User Service
  userService = thingsServer->createService(USER_SERVICE_UUID);
  // Create Characteristics for User Service
  writeCharacteristic = userService->createCharacteristic(WRITE_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
  writeCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
  writeCharacteristic->setCallbacks(new writeCallback());

  notifyCharacteristic = userService->createCharacteristic(NOTIFY_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  notifyCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
  BLE2902* ble9202 = new BLE2902();
  ble9202->setNotifications(true);
  ble9202->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
  notifyCharacteristic->addDescriptor(ble9202);

  // Setup PSDI Service
  psdiService = thingsServer->createService(PSDI_SERVICE_UUID);
  psdiCharacteristic = psdiService->createCharacteristic(PSDI_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ);
  psdiCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);

  // Set PSDI (Product Specific Device ID) value
  uint64_t macAddress = ESP.getEfuseMac();
  psdiCharacteristic->setValue((uint8_t*) &macAddress, sizeof(macAddress));

  // Start BLE Services
  userService->start();
  psdiService->start();
}

void startAdvertising(void) {
  // Start Advertising
  BLEAdvertisementData scanResponseData = BLEAdvertisementData();
  scanResponseData.setFlags(0x06); // GENERAL_DISC_MODE 0x02 | BR_EDR_NOT_SUPPORTED 0x04
  scanResponseData.setName(DEVICE_NAME);

  thingsServer->getAdvertising()->addServiceUUID(userService->getUUID());
  thingsServer->getAdvertising()->setScanResponseData(scanResponseData);
  thingsServer->getAdvertising()->start();
}

/*
  This Arduino code is provided "as is", without any warranty of any kind.
  It may be used, modified, and distributed exclusively for non-commercial purposes.
  Use for commercial purposes is expressly prohibited without prior written consent from the author.
  
  Author: Praga Michele

  License: Non-commercial use only
*/

#include <M5AtomS3.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

int currentSpeedState = 0; // 0 = STOP, 1 = 30%, 2 = 70%, 3 = 100%
int motorSpeed = 0;  // Variabile per memorizzare la velocità corrente

// Dichiarazione della funzione updateDisplay
void updateDisplay();

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      updateDisplay();  // Aggiorna il display quando il client si connette
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      updateDisplay();  // Aggiorna il display quando il client si disconnette
    }
};

void setup() {
  M5.begin();
  M5.Lcd.setRotation(0);
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("BLE Server");

  BLEDevice::init("M5AtomS3_Server");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();

  updateDisplay();  // Inizializza la visualizzazione
}

void loop() {
  M5.update();

  if (deviceConnected) {
    if (M5.BtnA.wasPressed()) {
      String message;
      switch (currentSpeedState) {
        case 0:
          motorSpeed = 30;
          message = "SPEED_30;";
          currentSpeedState = 1;
          break;
        case 1:
          motorSpeed = 70;
          message = "SPEED_70;";
          currentSpeedState = 2;
          break;
        case 2:
          motorSpeed = 100;
          message = "SPEED_100;";
          currentSpeedState = 3;
          break;
        case 3:
          motorSpeed = 0;
          message = "STOP;";
          currentSpeedState = 0;
          break;
      }
      pCharacteristic->setValue(message.c_str());
      pCharacteristic->notify();
      updateDisplay();  // Aggiorna la visualizzazione ogni volta che viene inviato un comando
    }

    if (M5.BtnA.pressedFor(2000)) {
      motorSpeed = 0;
      pCharacteristic->setValue("STOP;");
      pCharacteristic->notify();
      currentSpeedState = 0;
      updateDisplay();  // Aggiorna la visualizzazione
    }
  }

  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }

  delay(10);
}

void updateDisplay() {
    // Cancella lo schermo
    M5.Lcd.fillScreen(TFT_BLACK);

    // Disegna una banda colorata in alto
    M5.Lcd.fillRect(0, 0, M5.Lcd.width(), 20, deviceConnected ? TFT_GREEN : TFT_RED);

    // Disegna una banda colorata in basso
    M5.Lcd.fillRect(0, M5.Lcd.height() - 20, M5.Lcd.width(), 20, deviceConnected ? TFT_GREEN : TFT_RED);

    // Imposta il testo della velocità al centro dello schermo con un carattere grande
    M5.Lcd.setTextSize(4); // Imposta una dimensione del testo più grande
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setCursor((M5.Lcd.width() - M5.Lcd.textWidth(String(motorSpeed) + "%")) / 2, (M5.Lcd.height() - 48) / 2); // Centra il testo
    M5.Lcd.printf("%d%%", motorSpeed);
}

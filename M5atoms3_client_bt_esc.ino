/*
  This Arduino code is provided "as is", without any warranty of any kind.
  It may be used, modified, and distributed exclusively for non-commercial purposes.
  Use for commercial purposes is expressly prohibited without prior written consent from the author.
  
  Author: Praga Michele

  License: Non-commercial use only
*/
#include <M5AtomS3.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <ESP32Servo.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

static BLEUUID serviceUUID(SERVICE_UUID);
static BLEUUID charUUID(CHARACTERISTIC_UUID);

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

const int escPin = 2;
Servo esc;
int motorSpeed = 0;

void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    String message = String((char*)pData);  // Converte in String di Arduino
    Serial.println("Received: " + message); // Usa String di Arduino

    // Rimuovi eventuali caratteri indesiderati
    message.trim();

    if (message.startsWith("SPEED_30")) {
        motorSpeed = 30;
    } else if (message.startsWith("SPEED_70")) {
        motorSpeed = 70;
    } else if (message.startsWith("SPEED_100")) {
        motorSpeed = 100;
    } else if (message.startsWith("STOP")) {
        motorSpeed = 0;
    }
    updateMotorSpeed();
    updateDisplay();
}

class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
        connected = true;
        updateDisplay();
        Serial.println("Connected to server"); // Debugging
    }

    void onDisconnect(BLEClient* pclient) {
        connected = false;
        Serial.println("Disconnected from server"); // Debugging
        // Quando si perde la connessione, imposta la velocità del motore a 0%
        motorSpeed = 0;
        updateMotorSpeed();
        updateDisplay();
    }
};

bool connectToServer() {
    Serial.println("Connecting to server..."); // Debugging
    BLEClient*  pClient  = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(myDevice);
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
        Serial.println("Failed to find service UUID: " + String(serviceUUID.toString().c_str())); // Conversione in String
        pClient->disconnect();
        return false;
    }
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
        Serial.println("Failed to find characteristic UUID: " + String(charUUID.toString().c_str())); // Conversione in String
        pClient->disconnect();
        return false;
    }
    if (pRemoteCharacteristic->canNotify()) {
        pRemoteCharacteristic->registerForNotify(notifyCallback);
        Serial.println("Registered for notifications"); // Debugging
    }
    return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
            Serial.println("Found device with service UUID"); // Debugging
            BLEDevice::getScan()->stop();
            myDevice = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
            doScan = false;
        }
    }
};

void setup() {
    Serial.begin(115200); // Setup serial monitor for debugging
    M5.begin();
    M5.Lcd.setRotation(0);
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setTextSize(2);

    esc.attach(escPin);
    esc.writeMicroseconds(1000);

    BLEDevice::init("");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);

    updateDisplay();
}

void loop() {
    M5.update();

    if (doConnect) {
        if (connectToServer()) {
            updateDisplay();
        } else {
            M5.Lcd.println("Failed to connect");
        }
        doConnect = false;
    }

    if (M5.BtnA.wasPressed()) {
        changeMotorSpeed();
        if (connected) {
            switch (motorSpeed) {
                case 30:
                    pRemoteCharacteristic->writeValue("SPEED_30;");
                    break;
                case 70:
                    pRemoteCharacteristic->writeValue("SPEED_70;");
                    break;
                case 100:
                    pRemoteCharacteristic->writeValue("SPEED_100;");
                    break;
                case 0:
                    pRemoteCharacteristic->writeValue("STOP;");
                    break;
            }
            Serial.println("Sent speed update: " + String(motorSpeed) + "%"); // Debugging
        }
    }

    if (M5.BtnA.pressedFor(2000)) {
        motorSpeed = 0;
        updateMotorSpeed();
        updateDisplay();
        if (connected) {
            pRemoteCharacteristic->writeValue("STOP;");
            Serial.println("Sent stop command"); // Debugging
        }
    }

    if (!connected && doScan) {
        BLEDevice::getScan()->start(0);
    }

    delay(10);
}

void changeMotorSpeed() {
    if (motorSpeed == 0) motorSpeed = 30;
    else if (motorSpeed == 30) motorSpeed = 70;
    else if (motorSpeed == 70) motorSpeed = 100;
    else motorSpeed = 0;

    updateMotorSpeed();
    updateDisplay();
}

void updateMotorSpeed() {
    int pwmValue = map(motorSpeed, 0, 100, 1000, 2000);
    esc.writeMicroseconds(pwmValue);
    Serial.println("Updated motor speed to: " + String(motorSpeed) + "%"); // Debugging
}

void updateDisplay() {
    // Cancella lo schermo
    M5.Lcd.fillScreen(TFT_BLACK);

    // Disegna una banda colorata in alto
    M5.Lcd.fillRect(0, 0, M5.Lcd.width(), 20, connected ? TFT_GREEN : TFT_RED);

    // Disegna una banda colorata in basso
    M5.Lcd.fillRect(0, M5.Lcd.height() - 20, M5.Lcd.width(), 20, connected ? TFT_GREEN : TFT_RED);

    // Imposta il testo della velocità al centro dello schermo con un carattere grande
    M5.Lcd.setTextSize(4); // Imposta una dimensione del testo più grande
    M5.Lcd.setTextColor(TFT_WHITE);
    M5.Lcd.setCursor((M5.Lcd.width() - M5.Lcd.textWidth(String(motorSpeed) + "%")) / 2, (M5.Lcd.height() - 48) / 2); // Centra il testo
    M5.Lcd.printf("%d%%", motorSpeed);
}

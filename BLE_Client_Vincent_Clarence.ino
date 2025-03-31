#include "BLEDevice.h"
#include <M5StickCPlus.h>

// Default Temperature in Celsius
#define temperatureCelsius

// Change the BLE Server name to connect to
#define bleServerName "CSC2106-BLE#01"

/* UUID's of the service, characteristic that we want to read*/
// BLE Service
static BLEUUID bleServiceUUID("01234567-0123-4567-89ab-0123456789ab");

// BLE Characteristics
#ifdef temperatureCelsius
// Temperature Celsius Characteristic
static BLEUUID temperatureCharacteristicUUID("01234567-0123-4567-89ab-0123456789cd");
#else
// Temperature Fahrenheit Characteristic
static BLEUUID temperatureCharacteristicUUID("01234567-0123-4567-89ab-01234567de");
#endif

static BLEUUID ledCharacteristicUUID("01234567-0123-4567-89ab-0123456789bd");

// Battery Voltage Characteristic
static BLEUUID voltageCharacteristicUUID("01234567-0123-4567-89ab-0123456789ef");

// Flags stating if should begin connecting and if the connection is up
static boolean doConnect = false;
static boolean connected = false;

// Address of the peripheral device. Address will be found during scanning...
static BLEAddress* pServerAddress;

// Characteristics that we want to read
static BLERemoteCharacteristic* temperatureCharacteristic;
static BLERemoteCharacteristic* voltageCharacteristic;
static BLERemoteCharacteristic* ledCharacteristic;

// Activate notify
const uint8_t notificationOn[] = { 0x1, 0x0 };
const uint8_t notificationOff[] = { 0x0, 0x0 };

// Variables to store temperature and voltage
char* temperatureStr;
char* voltageStr;
char* ledStr;

// Flags to check whether new temperature and voltage readings are available
boolean newTemperature = false;
boolean newVoltage = false;
boolean newLed = false;

// Function prototypes for callback functions
void temperatureNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
void voltageNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
void ledNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

// Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer(BLEAddress pAddress) {
  BLEClient* pClient = BLEDevice::createClient();

  // Connect to the remote BLE Server.
  if (!pClient->connect(pAddress)) {
    Serial.println("Failed to connect to the server.");
    return false;
  }

  // pClient->connect(pAddress);
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(bleServiceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(bleServiceUUID.toString().c_str());
    return false;
  }

  // Obtain a reference to the characteristics in the service of the remote BLE server.
  temperatureCharacteristic = pRemoteService->getCharacteristic(temperatureCharacteristicUUID);
  voltageCharacteristic = pRemoteService->getCharacteristic(voltageCharacteristicUUID);
  ledCharacteristic = pRemoteService->getCharacteristic(ledCharacteristicUUID);  // Add this line for LED characteristic

  if (temperatureCharacteristic == nullptr || voltageCharacteristic == nullptr || ledCharacteristic == nullptr) {
    Serial.println("Failed to find one or more of our characteristic UUIDs.");
    return false;
  }

  Serial.println(" - Found our characteristics");

  // Assign callback functions for the Characteristics
  temperatureCharacteristic->registerForNotify(temperatureNotifyCallback);
  voltageCharacteristic->registerForNotify(voltageNotifyCallback);
  ledCharacteristic->registerForNotify(ledNotifyCallback);

  return true;
}

// Callback function that gets called, when another device's advertisement has been received
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == bleServerName) {
      advertisedDevice.getScan()->stop();                              // Stop scanning after finding the device
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());  // Get the address
      doConnect = true;                                                // Set the flag to connect
      Serial.println("Device found. Connecting!");
    } else {
      Serial.print(".");  // Print a dot for every scanned device that doesn't match
    }
  };
};

// When the BLE Server sends a new temperature reading with the notify property
void temperatureNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  // store temperature value
  if (pData && length > 0) {
    temperatureStr = strndup((char*)pData, length);  // Ensure safe copy
    newTemperature = true;
  }
}

// When the BLE Server sends a new voltage reading with the notify property
void voltageNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  // store voltage value
  if (pData && length > 0) {
    voltageStr = strndup((char*)pData, length);  // Ensure safe copy
    newVoltage = true;
  }
}

// When the BLE Server sends a new LED state with the notify property
void ledNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  // store LED state
  if (pData && length > 0) {
    ledStr = strndup((char*)pData, length);  // Ensure safe copy
    newLed = true;
  }
}

// Function that prints the latest sensor readings on the OLED display
void printReadings() {
  M5.Lcd.setCursor(0, 20, 2);

  // Check if temperatureStr is null before printing
  if (temperatureStr != nullptr) {
    M5.Lcd.printf("Temperature = ");
    M5.Lcd.printf(temperatureStr);

#ifdef temperatureCelsius
    // Temperature Celsius
    M5.Lcd.printf(" C");
#else
    // Temperature Fahrenheit
    M5.Lcd.printf(" F");
#endif
  } else {
    M5.Lcd.printf("Temperature = Unavailable");
  }

  // Display voltage
  M5.Lcd.setCursor(0, 40, 2);

  if (voltageStr != nullptr) {
    M5.Lcd.printf("Battery Voltage = ");
    M5.Lcd.printf(voltageStr);
    M5.Lcd.printf(" V");
  } else {
    M5.Lcd.printf("Battery Voltage = Unavailable");
  }
}

// Function to read the LED state from the characteristic
void readLEDState() {
  if (ledCharacteristic != nullptr) {
    // Read the LED state
    std::string ledState = ledCharacteristic->readValue();  // Assuming LED state is a string like "ON" or "OFF"
    if (ledStr) free(ledStr);                               // Free old memory before allocating new memory
    ledStr = strdup(ledState.c_str());                      // Store the value in ledStr

    // Display the LED state on the screen
    M5.Lcd.setCursor(0, 60, 2);
    M5.Lcd.printf("\nLED State: ");
    M5.Lcd.printf(ledState.c_str());
  } else {
    M5.Lcd.setCursor(0, 60, 2);
    M5.Lcd.printf("LED characteristic not found!");
  }
}

void setup() {
  M5.Lcd.printf("Running setup()");
  // Start serial communication
  Serial.begin(115200);
  M5.Lcd.printf("Starting BLE Client application...");

  // Initialize M5StickCPlus
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.printf("BLE Client\n", 0);

  // Init BLE device
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 10 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);  // Non-blocking scan with timeout of 5 seconds
}

void loop() {
  // Connect to the server if the flag "doConnect" is set
  if (doConnect == true) {
    if (connectToServer(*pServerAddress)) {
      M5.Lcd.printf("Connected to the BLE Server.\n");
      digitalWrite(M5_BUTTON_HOME, 1);  // Initially set as HIGH
      digitalWrite(BUTTON_B_PIN, 1);    // Initially set as HIGH

      // Activate the Notify property of each Characteristic
      if (temperatureCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))) {
        temperatureCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      }

      if (voltageCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))) {
        voltageCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      }

      if (ledCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))) {
        ledCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      }

      connected = true;
    } else {
      M5.Lcd.printf("Failed to connect to the server. Restart device to scan for nearby BLE server again.");
    }
    doConnect = false;
  }

  M5.update();  // Update the M5StickCPlus buttons

  //if new readings are available, print in the OLED
  if (digitalRead(M5_BUTTON_HOME) == LOW) {
    while (digitalRead(M5_BUTTON_HOME) == LOW)
      ;  // Wait for button release, press and hold is low, let go is high
    // Set back to false to mark as old readings as we just used it
    // it will be set to true accordingly by their respective NotifyCallback functions if there is new readings
    newTemperature = false;
    newVoltage = false;
    newLed = false;
    printReadings();
    readLEDState();
  }

  // Check button press for LED update
  if (digitalRead(BUTTON_B_PIN) == LOW) {
    while (digitalRead(BUTTON_B_PIN) == LOW);  // Wait for button release, press and hold is low, let go is high

    // Toggle LED state on button press
    static char ledState[4];
    // Toggle between "ON" and "OFF" states (or any desired states)
    if (strcmp(ledStr, "ON") == 0) {
      strcpy(ledState, "OFF");
    } else {
      strcpy(ledState, "ON");
    }

    // Send the updated LED state to the server
    ledCharacteristic->writeValue(ledState);  // Send LED state to server
    M5.Lcd.setCursor(0, 100, 2);
    // M5.Lcd.printf("LED State: %s", ledState);
    readLEDState();
  }

  delay(1);  // Delay one second between loops so that no need to press and hold button
}

#include <BLEDevice.h>
#include <BLEServer.h>
//#include <BLEUtils.h>
//#include <BLE2902.h>
#include <M5StickCPlus.h>

//#include <Wire.h>
#define LED_PIN 10

//Default Temperature in Celsius
#define temperatureCelsius

//change to unique BLE server name
#define bleServerName "CSC2106-BLE#01"

float tempC = 25.0;
float tempF;
float vBatt = 5.0;  // initial value
static char status[6];

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 15000;   // update refresh every 15sec

bool deviceConnected = false;
bool ledState = false;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "01234567-0123-4567-89ab-0123456789ab"
#define LED_UUID "01234567-0123-4567-89ab-0123456789bd"

// Temperature Characteristic and Descriptor
#ifdef temperatureCelsius
  BLECharacteristic imuTemperatureCelsiusCharacteristics("01234567-0123-4567-89ab-0123456789cd", BLECharacteristic::PROPERTY_NOTIFY);
  BLEDescriptor imuTemperatureCelsiusDescriptor(BLEUUID((uint16_t)0x2902));
#else
  BLECharacteristic imuTemperatureFahrenheitCharacteristics("01234567-0123-4567-89ab-0123456789de", BLECharacteristic::PROPERTY_NOTIFY);
  BLEDescriptor imuTemperatureFahrenheitDescriptor(BLEUUID((uint16_t)0x2902));
#endif

// Battery Voltage Characteristic and Descriptor
BLECharacteristic axpVoltageCharacteristics("01234567-0123-4567-89ab-0123456789ef", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor axpVoltageDescriptor(BLEUUID((uint16_t)0x2903));

// LED Characteristic 
BLECharacteristic axpLedCharacteristics(LED_UUID, BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE);
BLEDescriptor axpLedDescriptor(BLEUUID((uint16_t)0x2904));

//Setup callbacks onConnect and onDisconnect
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("MyServerCallbacks::Connected...");
    M5.Lcd.setCursor(0, 80, 2);
    M5.lcd.printf("MyServerCallbacks::Connected...");

  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("MyServerCallbacks::Disconnected...");
    M5.Lcd.fillRect(0, 80, 2, 16, BLACK);
    M5.Lcd.setCursor(0, 80, 2);
    M5.lcd.printf("MyServerCallbacks::Disconnected...");
    delay(1000);
    // Restart advertising to allow new connections
    pServer->startAdvertising();
    Serial.println("Advertising restarted...");
    M5.Lcd.fillRect(0, 80, 2, 16, BLACK);
    M5.Lcd.setCursor(0, 80, 2);
    M5.lcd.printf("Advertising restarted...");
  }
};

// Callback for writing to the characteristic 
class MyCharacteristicCallbacks: public BLECharacteristicCallbacks { 
  void onWrite(BLECharacteristic* pCharacteristic) { 
    std::string value = pCharacteristic->getValue(); 
    M5.Lcd.setCursor(0,60,2); 
    M5.Lcd.fillRect(0, 60, 240, 16, BLACK); 
    M5.Lcd.print("LED "); 
    M5.Lcd.println(value.c_str()); 
    if (value == "ON") { 
      digitalWrite(LED_PIN, LOW);  // Turn LED on (assuming active low) 
      // M5.Lcd.println(value.c_str()); 
      Serial.println("LED turned ON");
      ledState = !ledState; 

    } else if (value == "OFF") { 
      digitalWrite(LED_PIN, HIGH);  // Turn LED off 
      // M5.Lcd.println(value.c_str()); 
      Serial.println("LED turned OFF"); 
      ledState = !ledState;
      
    } else { 
      Serial.println("Invalid data received"); 
    } 
 
    Serial.print("Received value: "); 
    Serial.println(value.c_str()); 
    // You can process the received data here 
  } 
};
void setup() {
  // Start serial communication 
  Serial.begin(115200);

  // put your setup code here, to run once:
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.printf("BLE Server", 0);

  // Initialize IMU
  int x = M5.IMU.Init(); //return 0 is ok, return -1 is unknown
  if(x!=0)
    Serial.println("IMU initialisation fail!");  

  pinMode(LED_PIN, OUTPUT);  // Set LED_PIN as OUTPUT
  digitalWrite(LED_PIN, HIGH);  // Ensure LED starts in the OFF state
  
  // Create the BLE Device
  BLEDevice::init(bleServerName);

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *bleService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristics and Create a BLE Descriptor
  // Temperature
  #ifdef temperatureCelsius
    bleService->addCharacteristic(&imuTemperatureCelsiusCharacteristics);
    imuTemperatureCelsiusDescriptor.setValue("IMU Temperature(C)");
    imuTemperatureCelsiusCharacteristics.addDescriptor(&imuTemperatureCelsiusDescriptor);
  #else
    bleService->addCharacteristic(&imuTemperatureFahrenheitCharacteristics);
    imuTemperatureFahrenheitDescriptor.setValue("IMU Temperature(F)");
    imuTemperatureFahrenheitCharacteristics.addDescriptor(&imuTemperatureFahrenheitDescriptor);
  #endif  

  // Battery
  bleService->addCharacteristic(&axpVoltageCharacteristics);
  axpVoltageDescriptor.setValue("AXP Battery(V)");
  axpVoltageCharacteristics.addDescriptor(&axpVoltageDescriptor); 
    
  // LED  
  bleService->addCharacteristic(&axpLedCharacteristics);
  axpLedDescriptor.setValue("LED");
  axpLedCharacteristics.addDescriptor(&axpLedDescriptor); 
  axpLedCharacteristics.setCallbacks(new MyCharacteristicCallbacks());

  // Start the service
  bleService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");

  pinMode(M5_BUTTON_HOME, INPUT);
  pinMode(LED_PIN, OUTPUT);

}

// Function to turn the LED off
void ledOff() {
  digitalWrite(LED_PIN, HIGH);
}

// Function to turn the LED on
void ledOn() {
  digitalWrite(LED_PIN, LOW);
}



void loop() {
  M5.update();  // Update M5StickC status

  // Check for button press
  if (digitalRead(M5_BUTTON_HOME) == LOW) {

    // Toggle LED state
    ledState = !ledState;
    if (ledState) {
      ledOn();
      strcpy(status, "ON ");
    } else {
      ledOff();
      strcpy(status, "OFF");
    }

    // Perform calculations when button is pressed
    if (random(2) > 0)
      tempC += random(10) / 100.0;
    else
      tempC -= random(10) / 100.0;

    // Convert to Fahrenheit if needed
    tempF = 1.8 * tempC + 32;

    // Update battery voltage
    if (vBatt < 1.0)
      vBatt = 5.0;  // Reset to full charge
    else
      vBatt -= 0.01;  // Simulate battery drain

    // Notify connected BLE client (if needed)
    #ifdef temperatureCelsius
      static char temperatureCTemp[6];
      dtostrf(tempC, 6, 2, temperatureCTemp);
      imuTemperatureCelsiusCharacteristics.setValue(temperatureCTemp);
      imuTemperatureCelsiusCharacteristics.notify();
    #else
      static char temperatureFTemp[6];
      dtostrf(tempF, 6, 2, temperatureFTemp);
      imuTemperatureFahrenheitCharacteristics.setValue(temperatureFTemp);
      imuTemperatureFahrenheitCharacteristics.notify();
    #endif

    static char voltageBatt[6];
    dtostrf(vBatt, 6, 2, voltageBatt);
    axpVoltageCharacteristics.setValue(voltageBatt);
    axpVoltageCharacteristics.notify();

    axpLedCharacteristics.setValue(status);
    axpLedCharacteristics.notify();

    // Update LCD with new values
    M5.Lcd.fillRect(0, 20, 2, 16, BLACK);
    M5.Lcd.setCursor(0, 20, 2);
    #ifdef temperatureCelsius
      M5.Lcd.print("Temperature = ");
      M5.Lcd.print(tempC);
      M5.Lcd.println(" C");
    #else
      M5.Lcd.print("Temperature = ");
      M5.Lcd.print(tempF);
      M5.Lcd.println(" F");
    #endif

    M5.Lcd.fillRect(0, 40, 2, 16, BLACK);
    M5.Lcd.setCursor(0, 40, 2);
    M5.Lcd.print("Battery Voltage = ");
    M5.Lcd.print(vBatt);
    M5.Lcd.println(" V");

    M5.Lcd.fillRect(0, 60, 2, 16, BLACK);
    M5.Lcd.setCursor(0, 60, 2);
    M5.Lcd.print("LED = ");
    M5.Lcd.println(status);

    // Print values to Serial (optional for debugging)
    Serial.println("=================================");
    Serial.print("Temperature: ");
    #ifdef temperatureCelsius
      Serial.print(tempC);
      Serial.println(" C");
    #else
      Serial.print(tempF);
      Serial.println(" F");
    #endif
    Serial.print("Battery Voltage: ");
    Serial.print(vBatt);
    Serial.println(" V");
    Serial.print("LED: ");
    Serial.println(status);
    Serial.println("=================================");

    // Debounce the button press
    delay(500);  // Wait to prevent multiple triggers
    while (digitalRead(M5_BUTTON_HOME) == LOW);  // Wait for button release
  }
}


// NODE B
#include "M5StickCPlus.h"
#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

// WiFi & MQTT Broker Configuration
const char* ssid        = "Your_WiFi_SSID";  
const char* password    = "Your_WiFi_Password";
const char* mqtt_server = "192.168.x.x";

#define LED_PIN 10  // GPIO 10 for built-in LED
bool ledState = false;

void setupWifi();
void callback(char* topic, byte* payload, unsigned int length);
void reConnect();

void setup() {
    M5.begin();
    M5.Lcd.setRotation(3);
    M5.Lcd.println("Node B Starting...");
    setupWifi();
    
    pinMode(LED_PIN, OUTPUT);
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
}

void loop() {
    if (!client.connected()) {
        reConnect();
    }
    client.loop();

    // Check if the button is pressed
    M5.update();
    if (M5.BtnA.wasPressed()) {
        M5.Lcd.println("Button B Pressed");
        client.publish("nodeB/buttonPress", "toggle");  // Publish to Node A
    }
}

void setupWifi() {
    delay(10);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
    if (strcmp(topic, "nodeA/buttonPress") == 0) {  // Node A sent a toggle request
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
        // M5.Lcd.fillScreen(ledState ? BLUE : BLACK);
        M5.Lcd.println("LED Toggled by A");
    }
}

void reConnect() {
    while (!client.connected()) {
        String clientId = "M5StackB-";
        clientId += String(random(0xffff), HEX);
        if (client.connect(clientId.c_str())) {
            client.subscribe("nodeA/buttonPress");  // Listen to Node A
        } else {
            delay(5000);
        }
    }
}

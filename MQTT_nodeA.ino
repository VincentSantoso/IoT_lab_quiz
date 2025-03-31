// NODE A
#include "M5StickCPlus.h"
#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

// WiFi & MQTT Broker Configuration
const char* ssid        = "Your_WiFi_SSID";  
const char* password    = "Your_WiFi_Password";  
const char* mqtt_server = "192.168.x.x";  // Replace with your broker's IP

#define LED_PIN 10  // GPIO 10 for built-in LED
bool ledState = false;

void setupWifi();
void callback(char* topic, byte* payload, unsigned int length);
void reConnect();

void setup() {
    M5.begin();
    M5.Lcd.setRotation(3);
    M5.Lcd.println("Node A Starting...");
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
        M5.Lcd.println("Button A Pressed");
        client.publish("nodeA/buttonPress", "toggle");  // Publish to Node B
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
    if (strcmp(topic, "nodeB/buttonPress") == 0) {  // Node B sent a toggle request
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
        // M5.Lcd.fillScreen(ledState ? GREEN : BLACK);
        M5.Lcd.println("LED Toggled by B");
    }
}

void reConnect() {
    while (!client.connected()) {
        String clientId = "M5StackA-";
        clientId += String(random(0xffff), HEX);
        if (client.connect(clientId.c_str())) {
            client.subscribe("nodeB/buttonPress");  // Listen to Node B
        } else {
            delay(5000);
        }
    }
}

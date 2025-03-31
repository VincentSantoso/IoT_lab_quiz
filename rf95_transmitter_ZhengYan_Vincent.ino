// LoRa 9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example LoRa9x_RX

#include <SPI.h>
#include <RH_RF95.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2
#define node_id 1
 
// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 908.0
 
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void(* resetFunc) (void) = 0; //declare reset function at address 0

void setup() 
{
  Serial.begin(9600);
  delay(100);
  
  // Manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address or 0x3D for
    Serial.println(F("SSD1306 allocation failed"));
      for (;;)
      {
          delay(1000);
      }
    }
      // Setup oled display
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setTextColor(WHITE); // Draw white text
    display.setCursor(0, 0);     // Start at top-left corner
    display.clearDisplay();

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    display.fillRect(0, 0, SCREEN_WIDTH, 8, BLACK); 
    display.setCursor(0, 0);
    display.println("Setup Failed");
    display.display();
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 915.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to:"); Serial.println(RF95_FREQ);
  display.fillRect(0, 0, SCREEN_WIDTH, 8, BLACK); 
  display.setCursor(0, 0);
  display.println("Setup Successful");
  display.display();

  // Defaults after init are 915.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(13, false);
}
int16_t packetnum = 0;  // packet counter, we increment per transmission

void sendMessage(uint8_t destID, uint8_t senderID, const char *message) {
    uint8_t payloadLen = strlen(message);
    if (payloadLen > RH_RF95_MAX_MESSAGE_LEN - 5) {
        Serial.println("Message too long!");
        return;
    }

    uint8_t packet[RH_RF95_MAX_MESSAGE_LEN];
    packet[0] = 0xAA;  // Start byte
    packet[1] = destID;
    packet[2] = senderID;
    packet[3] = payloadLen;
    memcpy(&packet[4], message, payloadLen);

    // Calculate checksum (XOR)
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < 4 + payloadLen; i++) {
        checksum ^= packet[i];
    }
    packet[4 + payloadLen] = checksum;

    // Send message
    rf95.send(packet, 5 + payloadLen);
    rf95.waitPacketSent();
    Serial.println("Message sent!");
}

 
void loop()
{
 
  uint8_t data[32];
  sprintf(data, "Hello Sekai %d from %d", int(packetnum++), int(node_id)); 
    
  int max_retries = 3;
  bool ack_received = false;
  
  for (int attempt = 0; attempt < max_retries; attempt++) {
    Serial.println("Sending to rf95_rx");
    // Send a message to rf95_rx
    display.fillRect(0, 0, SCREEN_WIDTH, 8, BLACK); 
    display.setCursor(0, 0);
    display.println("Sending Message");
    display.display();
    Serial.print("Attempt "); Serial.println(attempt + 1);
    sendMessage(2, node_id, (char*)data);  // Send message to Node 2
    //sendMessage(3, node_id, (char*)data);  // Send message to Node 3

    if (rf95.waitAvailableTimeout(1000))
    { 
      // Now wait for a reply
      uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
      uint8_t len = sizeof(buf);

      display.fillRect(0, 0, SCREEN_WIDTH, 8, BLACK); 
      display.setCursor(0, 0);
      display.println("Waiting for Reply");
      display.display();  
      Serial.println("Wait for reply...");   
      // Should be a reply message for us now   
      if (rf95.recv(buf, &len))
      {
        if (strcmp((char*)buf, "Sekai Acknowledged") == 0) {  // Check for correct ACK
          ack_received = true;
          Serial.print("Got ACK:");
          Serial.println((char*)buf);
          Serial.print("RSSI:");
          Serial.println(rf95.lastRssi(), DEC);
          display.fillRect(0, 0, SCREEN_WIDTH, 8, BLACK);
          display.setCursor(0, 0);
          display.println("Message Received");
          display.display();
          break;  // Exit retry loop if ACK is received
        }   
      }
    }
    else
    {
      Serial.println("No ACK, retrying...");
    }
  }
  if(!ack_received)
  {
    Serial.println("No reply received");
  }
  delay(2000);
}
// https://docs.m5stack.com/en/quick_start/m5stickc_plus/arduino
#include <WiFi.h>
#include <WebServer.h>
#include <M5StickCPlus.h>

/* Put your SSID & Password */
const char* ssid = "VINCENTLENOVO";
const char* password = "15R39b01";

#define LED_PIN 10
// read new set of sensors value on button push
float pitch, roll, yaw, temperature, x, y, z;
bool status = false; 

WebServer server(80);

void setup() {
  Serial.begin(115200);

  // put your setup code here, to run once:
  M5.begin();

  int x = M5.IMU.Init(); //return 0 is ok, return -1 is unknown
  if(x!=0)
    Serial.println("IMU initialisation fail!");  

  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.printf("RESTful API", 0);

  WiFi.begin(ssid, password);

  // Setting the hostname
  WiFi.setHostname("group01-stick");

  Serial.print("Start WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("Connecting ..");
  }
  M5.Lcd.setCursor(0, 20, 2);
  M5.Lcd.print("IP: ");
  M5.Lcd.println(WiFi.localIP());

  pinMode(M5_BUTTON_HOME, INPUT);
  pinMode(LED_PIN, OUTPUT);
  delay(100);

  // Adding new endpoints for sensors 


  server.on("/", handle_JsonResponse);
  server.on("/gyro/", gyro);
  server.on("/accel/", accel);
  server.on("/temp/", temp);

  server.on("/led/0", handle_LedOff); 
  server.on("/led/1", handle_LedOn); 
  server.on("/buzzer/0", handle_BuzzerOff); 
  server.on("/buzzer/1", handle_BuzzerOn);

  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
  Serial.print("Connected to the WiFi network. IP: ");
  Serial.println(WiFi.localIP());
}
 
void handle_JsonResponse(){
  String response;
  response += "{ \"imu\": { \"pitch\": ";
  response += String(pitch, 6);
  response += ", \"roll\": ";
  response += String(roll, 6);
  response += ", \"yaw\": ";
  response += String(yaw, 6);
  response += ", \"temperature\": ";
  response += String(temperature, 6);
  response += ", \"x\": ";
  response += String(x, 6);
  response += ", \"y\": ";
  response += String(y, 6);
  response += ", \"z\": ";
  response += String(z, 6);
  response += " } }";

  Serial.println(response);
  digitalWrite(M5_LED, 1);

  server.send(200, "application/json", response);
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

void gyro(){
  String response;
  response += "{ \"pitch\": ";
  response += String(pitch, 6);
  response += ", \"roll\": ";
  response += String(roll, 6);
  response += ", \"yaw\": ";
  response += String(yaw, 6);
  response += " }";
  Serial.println(response);
  server.send(200, "application/json", response);
}
void accel(){
  String response;
  response += ", \"x\": ";
  response += String(x, 6);
  response += ", \"y\": ";
  response += String(y, 6);
  response += ", \"z\": ";
  response += String(z, 6);
  Serial.println(response);
  server.send(200, "application/json", response);
}
void temp(){
  String response;
  response += ", \"temperature\": ";
  response += String(temperature, 6);
  Serial.println(response);
  server.send(200, "application/json", response);
}

void handle_LedOff(){ 
  digitalWrite(LED_PIN, HIGH);
  String response = "{ \"led\": \"off\" }"; 
  Serial.println(response); 
  server.send(200, "application/json", response); 
  }

void handle_LedOn(){ 
  digitalWrite(LED_PIN, LOW);
  String response = "{ \"led\": \"on\" }"; 
  Serial.println(response); 
  server.send(200, "application/json", response); 
  }

void handle_BuzzerOff(){ 
  M5.Beep.tone(0);
  String response = "{ \"buzzer\": \"off\" }"; 
  Serial.println(response); 
  server.send(200, "application/json", response); 
  }

void handle_BuzzerOn(){ 
  M5.Beep.tone(2000);
  String response = "{ \"buzzer\": \"on\" }"; 
  Serial.println(response); 
  server.send(200, "application/json", response); 
  }

bool readGyro(){
  M5.IMU.getAhrsData(&pitch, &roll, &yaw);
  return true;
}

bool readAccel(){
  M5.IMU.getAccelData(&x, &y, &z);
  return true;
}

float readTemperature(){
  float t;
  M5.IMU.getTempData(&t);
  t = (t-32.0)*5.0/9.0;
  return t;
}

uint8_t setup_flag = 1;

bool readSensors() {
  bool status_gyro = readGyro();
  bool status_accel = readAccel();

  if (status_gyro && status_accel) {
    status = true;
    M5.Lcd.setCursor(0, 40, 2);
    M5.Lcd.printf("                                        ");
  }
  Serial.println("Gyro:");
  Serial.print("Pitch[X]: ");
  Serial.println(pitch);
  Serial.print("Roll[Y]: ");
  Serial.println(roll);
  Serial.print("Yaw[Z]: ");
  Serial.println(yaw);

  temperature = readTemperature();
  M5.Lcd.setCursor(0, 60, 2);
  M5.Lcd.printf("Temperature = %2.1f", temperature);
  Serial.print("Temperature: ");
  Serial.println(temperature);

  M5.Lcd.setCursor(0, 80, 2);
  Serial.println("Accel:");
  Serial.print("x: ");
  Serial.println(x);
  Serial.print("y: ");
  Serial.println(y);
  Serial.print("z: ");
  Serial.println(z);
  
  return status;
}
 
void loop() {
  server.handleClient();

  if (setup_flag == 1) {
    M5.Lcd.setCursor(0, 40, 2);
    M5.Lcd.printf("X = %3.2f, Y = %3.2f, Z = %3.2f", pitch, roll, yaw);
    M5.Lcd.setCursor(0, 60, 2);
    M5.Lcd.printf("Temperature = %2.1f", temperature);
    M5.Lcd.setCursor(0, 80, 2);
    M5.Lcd.printf("X = %3.2f, Y = %3.2f, Z = %3.2f", x, y, z);
  }

  if(!setup_flag){
    setup_flag = 1;
    bool status = readSensors();
    if (status)
      Serial.print("\n\rRead Sensors success...\n");
    else
      Serial.print("\n\rRead Sensors failed...\n");
  }

  if(digitalRead(M5_BUTTON_HOME) == LOW){
    setup_flag = 0;
    while(digitalRead(M5_BUTTON_HOME) == LOW);
  }
}
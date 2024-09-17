/* Fill-in your Template ID (only if using Blynk.Cloud) */
#define BLYNK_TEMPLATE_ID "TMPL6N8BU9rn8"
#define BLYNK_TEMPLATE_NAME "testesp32"
#define BLYNK_AUTH_TOKEN "bcsn41hsmep7OAdCSEokU52lJ6OzZWyo"
#define BLYNK_FIRMWARE_VERSION "0.1.0"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <ModbusMaster.h>
#include <SimpleTimer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>

// WiFi credentials
char ssid[] = "Dr.Samart";           //เปลี่ยนชื่อ และรหัสผ่าน Wi-Fi และทำการ upload code ใหม่
char pass[] = "0807999907";

// Blynk authentication
char auth[] = BLYNK_AUTH_TOKEN;

// Modbus Master
ModbusMaster node1;  // Slave ID1: 3IN1 Humidity, Temperature and Light
ModbusMaster node2;  // Slave ID2: SoilMoisture Sensor

// Simple Timer
SimpleTimer timer;

// Define RX,TX
#define RX1 16  //16
#define TX1 17  //17

// 4CH Onboard Relay
#define Relay1_sw1 25
#define Widget_Btn_sw1 V5
#define Relay2_sw2 26
#define Widget_Btn_sw2 V6
#define Relay3_sw3 33
#define Widget_Btn_sw3 V7
#define Relay4_sw4 32
#define Widget_Btn_sw4 V8

// LCD
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Line Notify
const char *line_notify_token = "";  //เปลี่ยน Line token

// Timing variables
unsigned long previousMillis1 = 0;
const unsigned long lineNotifyInterval = 60000;  // 1 minute
unsigned long previousMillisLCD = 0;
const long intervalLCD = 5000;  // Update LCD every 5 seconds

// Temperature Threshold
const float TEMP_THRESHOLD = 35.0;

//=================Blynk Virtual Pin=================//
//V1  Humidity
//V2  Temperature
//V3  Light
//V4  SoilMoisture
//V5 สวิตซ์ 1
//V6 สวิตซ์ 2
//V7 สวิตซ์ 3
//V8 สวิตซ์ 4
//=================Blynk Virtual Pin=================//

//Setup Function
void setup() {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, RX1, TX1);
  node1.begin(1, Serial2);  // Slave ID1: 3IN1 Humidity, Temperature and Light
  node2.begin(2, Serial2);  // Slave ID2: SoilMoisture Sensor

  // LCD
  lcd.begin();
  lcd.backlight();

  // แสดงข้อความเริ่มต้นที่ต้องการให้คงอยู่ตลอด
  displayInitialLCDMessages();  // แสดงข้อความที่ไม่ต้องการให้หายไป

  // Relay Mode
  pinMode(Relay1_sw1, OUTPUT);
  pinMode(Relay2_sw2, OUTPUT);
  pinMode(Relay3_sw3, OUTPUT);
  pinMode(Relay4_sw4, OUTPUT);
  // Initial relay status
  digitalWrite(Relay1_sw1, LOW);
  digitalWrite(Relay2_sw2, LOW);
  digitalWrite(Relay3_sw3, LOW);
  digitalWrite(Relay4_sw4, LOW);

  // Function: Connect to Wi-Fi
  initWiFi();

  timer.setInterval(5000L, update3in1sensor);  // Update sensor data every 5 seconds
  timer.setInterval(5000L, readSensorssoil1);  // Update soil moisture every 5 seconds

  Blynk.config(auth);
  delay(1000);  // Short delay to ensure Wi-Fi and Blynk connection
}

//Loop Function
void loop() {
  Blynk.run();  // Run Blynk connection
  timer.run();  // Run the SimpleTimer to update sensors regularly

  unsigned long currentMillis = millis();

  // Update LCD every 5 seconds
  if (currentMillis - previousMillisLCD >= intervalLCD) {
    previousMillisLCD = currentMillis;
    displaySensorDataOnLCD();  // Update only the sensor data
  }

  // Check for temperature threshold and send Line notify
  if (currentMillis - previousMillis1 >= lineNotifyInterval) {
    previousMillis1 = currentMillis;
    checkTEMPThresholdAndNotify();
  }
}

// Function connect to Wi-Fi
void initWiFi() {
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.println(WiFi.localIP());
}

// Function Check connect to Blynk-IoT
void checkBlynkStatus() {
  if (!Blynk.connected()) {
    Serial.println("Blynk Not Connected");
  }
}

//If blynk connected sync all virtual pin
BLYNK_CONNECTED() {
  Blynk.syncAll();
}

// Function to read 3IN1 sensor: Humidity, Temperature, and Light
void read3in1sensor(float &humi1, float &temp1, int &light1) {
  uint8_t result1;
  humi1 = (node1.getResponseBuffer(0) / 10.0f);
  temp1 = (node1.getResponseBuffer(1) / 10.0f);
  light1 = (int)node1.getResponseBuffer(2);  // แปลง light1 เป็น int

  Serial.println("Get 3IN1 Data");
  result1 = node1.readInputRegisters(0x0000, 3);  // Read 2 registers starting at 1)
  if (result1 == node1.ku8MBSuccess) {
    Serial.print("Humi1: ");
    Serial.println(node1.getResponseBuffer(0) / 10.0f);
    Serial.print("Temp1: ");
    Serial.println(node1.getResponseBuffer(1) / 10.0f);
    Serial.print("Light1: ");
    Serial.println(node1.getResponseBuffer(2));

    Blynk.virtualWrite(V1, humi1);
    Blynk.virtualWrite(V2, temp1);
    Blynk.virtualWrite(V3, light1);

  } else {
    Serial.println("Failed to read from 3IN1 sensor");
  }
}

// สร้างฟังก์ชันใหม่สำหรับ SimpleTimer เพื่ออัปเดตข้อมูลเซ็นเซอร์
void update3in1sensor() {
  float humi1, temp1;
  int light1;
  read3in1sensor(humi1, temp1, light1);  // เรียกใช้ฟังก์ชันเพื่ออ่านค่าจากเซ็นเซอร์ 3IN1
}

// Function Read sensor: SoilMoisture
void readSensorssoil1() {
  uint8_t result2;
  float soil1 = node2.getResponseBuffer(2) / 10.0f;
  Serial.println("Get Soil data");
  result2 = node2.readHoldingRegisters(0x0000, 3);  // Read 2 registers starting at 1)
  if (result2 == node2.ku8MBSuccess) {
    Serial.print("Soil: ");
    Serial.println(node2.getResponseBuffer(2) / 10.0f);
  }
  delay(1000);
  Blynk.virtualWrite(V4, soil1);
}

// BTN ON/OFF Relay1
BLYNK_WRITE(Widget_Btn_sw1) {
  digitalWrite(Relay1_sw1, param.asInt() == 1 ? HIGH : LOW);
}

// BTN ON/OFF Relay2
BLYNK_WRITE(Widget_Btn_sw2) {
  digitalWrite(Relay2_sw2, param.asInt() == 1 ? HIGH : LOW);
}

// BTN ON/OFF Relay1
BLYNK_WRITE(Widget_Btn_sw3) {
  digitalWrite(Relay3_sw3, param.asInt() == 1 ? HIGH : LOW);
}

// BTN ON/OFF Relay1
BLYNK_WRITE(Widget_Btn_sw4) {
  digitalWrite(Relay4_sw4, param.asInt() == 1 ? HIGH : LOW);
}

// Function send line notify
void sendLineNotify(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://notify-api.line.me/api/notify");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Authorization", "Bearer " + String(line_notify_token));

    int httpResponseCode = http.POST("message=" + message);

    if (httpResponseCode > 0) {
      Serial.println(httpResponseCode);
      Serial.println(http.getString());
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("Error in WiFi connection");
  }
}

// Display data on LCD
void displayInitialLCDMessages() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("*=DR.SAMART SINTON=*");

  lcd.setCursor(2, 1);
  lcd.print("Tel:080-799-9907");
}

// แสดงข้อมูลเซ็นเซอร์ในบรรทัดที่ 3 และ 4
void displaySensorDataOnLCD() {
  // ไม่เคลียร์หน้าจอ เพื่อให้ข้อความที่บรรทัด 1 และ 2 ยังคงอยู่
  float humi1 = node1.getResponseBuffer(0) / 10.0f;
  float temp1 = node1.getResponseBuffer(1) / 10.0f;
  int light1 = (int)node1.getResponseBuffer(2);
  float soil1 = node2.getResponseBuffer(2) / 10.0f;

  // แสดงข้อมูลเซ็นเซอร์ในบรรทัดที่ 3 และ 4
  lcd.setCursor(0, 2);  // Start at row 3, column 0
  lcd.print("H:");
  lcd.print(humi1);
  lcd.setCursor(9, 2);
  lcd.print("%");

  lcd.setCursor(11, 2);
  lcd.print("T:");
  lcd.print(temp1);
  lcd.setCursor(19, 2);
  lcd.print("C");

  lcd.setCursor(0, 3);  // Start at row 4, column 0
  lcd.print("L:");
  lcd.print(light1);
  lcd.setCursor(9, 3);
  lcd.print("L");

  lcd.setCursor(11, 3);
  lcd.print("S:");
  lcd.print(soil1);
  lcd.setCursor(19, 3);
  lcd.print("%");
}

// Function to check temperature threshold and notify via Line
void checkTEMPThresholdAndNotify() {
  float temp1 = node1.getResponseBuffer(1) / 10.0f;
  if (temp1 > TEMP_THRESHOLD) {
    String message = "แจ้งเตือนอุณหภูมิสูงกว่าเกณฑ์!\nTEMP: " + String(temp1) + "C\n";
    sendLineNotify(message);
  }
}

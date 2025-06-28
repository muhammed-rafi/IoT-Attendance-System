#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Adafruit_Fingerprint.h>
#include <MFRC522.h>
#include <SPI.h>

// OLED Display Settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C
#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WiFi Credentials
const char* ssid = "Rafi";
const char* password = "rafi1234";

// Fingerprint Sensor Setup
#define FP_RX 16
#define FP_TX 17
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// RFID Setup
#define SS_PIN 5
#define RST_PIN 22
MFRC522 rfid(SS_PIN, RST_PIN);

// Status Flags
bool wifiConnected = false;
bool fingerprintDetected = false;
bool rfidDetected = false;

// Function to Initialize OLED Display
void initOLED() {
  Wire.begin(SDA_PIN, SCL_PIN);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("❌ OLED Initialization Failed!"));
    for(;;) delay(100);
  }
  
  Serial.println(F("✅ OLED Initialized!"));
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 8);
  display.print(F("Initializing..."));
  display.display();
  delay(2000);
  display.clearDisplay();
  display.display(); // Ensure buffer is cleared
}

// Function to Update OLED Display with Status
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 0);
  display.print(F("WiFi: "));
  display.println(wifiConnected ? "Connected" : "Failed");
  
  display.setCursor(0, 16);
  display.print(F("FP: "));
  display.println(fingerprintDetected ? "OK" : "Failed");
  
  display.setCursor(0, 32);
  display.print(F("RFID: "));
  display.println(rfidDetected ? "OK" : "Failed");
  
  display.setCursor(10, 48);
  display.print(F("Running"));
  
  display.display();
  Serial.println(F("Display updated"));
}

// Function to Connect to WiFi
bool connectWiFi() {
  WiFi.mode(WIFI_STA); // Explicitly set WiFi mode
  Serial.print(F("Connecting to WiFi..."));
  WiFi.begin(ssid, password);
  
  for (int i = 0; i < 15; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F("\n✅ WiFi Connected!"));
      return true;
    }
    Serial.print(".");
    delay(1000);
  }
  
  Serial.println(F("\n❌ WiFi Failed!"));
  WiFi.disconnect(); // Clean up
  return false;
}

// Function to Check Fingerprint Sensor
bool checkFingerprintSensor() {
  mySerial.begin(57600, SERIAL_8N1, FP_RX, FP_TX);
  finger.begin(57600);
  delay(1000);

  Serial.println(F("Checking Fingerprint Sensor..."));
  if (finger.verifyPassword()) {
    Serial.println(F("✅ Fingerprint Sensor Detected!"));
    mySerial.end(); // Close serial to avoid conflicts
    return true;
  } else {
    Serial.println(F("❌ Fingerprint Sensor NOT Detected!"));
    mySerial.end();
    return false;
  }
}

// Function to Check RFID Sensor
bool checkRFIDSensor() {
  SPI.begin();
  rfid.PCD_Init();
  delay(1000);
  
  Serial.println(F("Checking RFID Sensor..."));
  if (rfid.PCD_ReadRegister(MFRC522::VersionReg) != 0x00) { 
    Serial.println(F("✅ RFID Reader Detected!"));
    SPI.end(); // Close SPI to avoid conflicts
    return true;
  } else {
    Serial.println(F("❌ RFID Reader NOT Detected!"));
    SPI.end();
    return false;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000); // Wait for serial
  
  initOLED();  // Initialize OLED first
  
  wifiConnected = connectWiFi();
  fingerprintDetected = checkFingerprintSensor();
  rfidDetected = checkRFIDSensor();

  updateDisplay();  // Initial status update
}

void loop() {
  Serial.println(F("Refreshing Display..."));
  updateDisplay();
  delay(2000);
}
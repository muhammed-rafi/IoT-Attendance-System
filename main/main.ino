#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Adafruit_Fingerprint.h>
#include <MFRC522.h>
#include <SPI.h>
#include <HTTPClient.h>

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
const char* scriptURL = "https://script.google.com/macros/s/AKfycbyVhtDWdZ-qPKQVuohssgHMZ0AlsfCQPtqBXr5DGZ6nnVXnGCSl_BlFOmqiUUUETdl2/exec";

// Fingerprint Sensor Setup
#define FP_RX 16
#define FP_TX 17
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// RFID Setup
#define SS_PIN 5    // Slave Select
#define RST_PIN 4   // Reset (Pin 4)
#define MOSI_PIN 23 // SPI MOSI
#define MISO_PIN 19 // SPI MISO
#define SCK_PIN 18  // SPI Clock
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
  delay(1000);
  //display.clearDisplay();
  display.display();
}

// Function to Update OLED Display
void updateDisplay(String message = "") {
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
  
  display.setCursor(0, 48);
  if (message == "") {
    display.print(F("Waiting for FP..."));
  } else {
    display.print(message);
  }
  
  display.display();
  Serial.println(F("Display updated"));
}

// Function to Connect to WiFi
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  Serial.print(F("Connecting to WiFi..."));
  WiFi.begin(ssid, password);
  
  for (int i = 0; i < 30; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F("\n✅ WiFi Connected!"));
      Serial.print(F("IP Address: "));
      Serial.println(WiFi.localIP());
      return true;
    }
    Serial.print(".");
    delay(100);
  }
  
  Serial.println(F("\n❌ WiFi Failed!"));
  WiFi.disconnect();
  return false;
}

// Function to Check Fingerprint Sensor
bool checkFingerprintSensor() {
  mySerial.begin(57600, SERIAL_8N1, FP_RX, FP_TX);
  finger.begin(57600);
  delay(100);

  Serial.println(F("Checking Fingerprint Sensor..."));
  if (finger.verifyPassword()) {
    Serial.println(F("✅ Fingerprint Sensor Detected!"));
    return true; // Keep serial open for scanning in loop
  } else {
    Serial.println(F("❌ Fingerprint Sensor NOT Detected!"));
    mySerial.end();
    return false;
  }
}

// Function to Check RFID Sensor
bool checkRFIDSensor() {
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  rfid.PCD_Init();
  delay(100);
  
  Serial.println(F("Checking RFID Sensor..."));
  if (rfid.PCD_ReadRegister(MFRC522::VersionReg) != 0x00) { 
    Serial.println(F("✅ RFID Reader Detected!"));
    SPI.end();
    return true;
  } else {
    Serial.println(F("❌ RFID Reader NOT Detected!"));
    SPI.end();
    return false;
  }
}

// Function to Send Data to Google Sheets
void sendToGoogleSheets(int fingerprintID) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(scriptURL) + "?id=" + String(fingerprintID);
    
    Serial.print("📡 Sending request to: ");
    Serial.println(url);

    http.begin(url);
    int httpCode = http.GET();

    Serial.print("🔍 HTTP Response Code: ");
    Serial.println(httpCode);

    if (httpCode > 0) {
      String response = http.getString();
      Serial.print("✅ Server Response: ");
      Serial.println(response);
      updateDisplay("Attendance Marked");
    } else {
      Serial.println("❌ Failed to send data!");
      Serial.print("⚠️ HTTP Error: ");
      Serial.println(http.errorToString(httpCode).c_str());
      updateDisplay("Send Failed");
    }
    
    http.end();
  } else {
    Serial.println("❌ WiFi Not Connected!");
    updateDisplay("WiFi Disconnected");
  }
  delay(1000); // Show message for 2 seconds
}

void setup() {
  Serial.begin(115200);
  delay(100);
  
  initOLED();  // Initialize OLED first
  
  wifiConnected = connectWiFi();
  fingerprintDetected = checkFingerprintSensor();
  rfidDetected = checkRFIDSensor();
  
  updateDisplay(); // Initial status update
}

void loop() {
  updateDisplay(); // Show default waiting message
  Serial.println("Waiting for fingerprint...");
  
  if (finger.getImage() == FINGERPRINT_OK) {
    Serial.println("Scanning...");
    updateDisplay("Scanning...");
    delay(100);
    
    if (finger.image2Tz() == FINGERPRINT_OK) {
      if (finger.fingerFastSearch() == FINGERPRINT_OK) { // Fixed typo: fingerFastSearch -> fingerFastSearching
        int id = finger.fingerID;
        Serial.print("Fingerprint ID: ");
        Serial.println(id);
        updateDisplay("Roll No. " + String(id));
        delay(100);
        Serial.println("Marking Attendance");
        sendToGoogleSheets(id);
      } else {
        Serial.println("Fingerprint Not Recognized!");
        updateDisplay("Not Recognized");
        delay(2000);
      }
    } else {
      Serial.println("Image conversion failed!");
      updateDisplay("Scan Failed");
      delay(2000);
    }
  }
  delay(1000); // Reduced delay for responsiveness
}

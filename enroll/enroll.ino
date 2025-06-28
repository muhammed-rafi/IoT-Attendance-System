#include <Adafruit_Fingerprint.h>

// Use Hardware Serial 2 (GPIO16 = RX, GPIO17 = TX)
HardwareSerial mySerial(2);  
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

uint8_t id;

void setup() {
    Serial.begin(115200);  // Debugging Serial Monitor
    mySerial.begin(57600, SERIAL_8N1, 16, 17);  // Init UART2 for fingerprint sensor

    Serial.println("\nFingerprint Sensor Enrollment");

    finger.begin(57600);
    delay(1000);

    if (finger.verifyPassword()) {
        Serial.println("Fingerprint sensor detected!");
    } else {
        Serial.println("Fingerprint sensor NOT found! Check wiring.");
        while (1);
    }

    Serial.println("Sensor Parameters:");
    finger.getParameters();
    Serial.print("Capacity: "); Serial.println(finger.capacity);
    Serial.print("Security Level: "); Serial.println(finger.security_level);
    Serial.print("Baud Rate: "); Serial.println(finger.baud_rate);
}

void loop() {
    Serial.println("\nReady to enroll a fingerprint.");
    Serial.println("Enter ID (1-127) for new fingerprint:");
    id = readNumber();
    if (id == 0) return;

    Serial.print("Enrolling ID #"); Serial.println(id);
    while (!getFingerprintEnroll());
}

// Read user input from Serial Monitor
uint8_t readNumber() {
    uint8_t num = 0;
    while (num == 0) {
        while (!Serial.available());
        num = Serial.parseInt();
    }
    return num;
}

// Enroll fingerprint
uint8_t getFingerprintEnroll() {
    int p = -1;
    Serial.print("Place finger to enroll for ID "); Serial.println(id);

    // Capture first fingerprint image
    while (p != FINGERPRINT_OK) {
        p = finger.getImage();
        if (p == FINGERPRINT_NOFINGER) Serial.print(".");
        else if (p == FINGERPRINT_OK) Serial.println("\nImage captured!");
        else Serial.println("Error capturing image.");
    }

    // Convert image to feature set
    p = finger.image2Tz(1);
    if (p != FINGERPRINT_OK) {
        Serial.println("Error processing image.");
        return p;
    }
    Serial.println("First image stored. Remove finger...");
    delay(2000);

    // Wait for finger removal
    while (finger.getImage() != FINGERPRINT_NOFINGER);
    Serial.println("Now place the same finger again.");
    delay(2000);

    // Capture second fingerprint image
    p = -1;
    while (p != FINGERPRINT_OK) {
        p = finger.getImage();
        if (p == FINGERPRINT_NOFINGER) Serial.print(".");
        else if (p == FINGERPRINT_OK) Serial.println("\nImage captured!");
        else Serial.println("Error capturing image.");
    }

    // Convert second image to feature set
    p = finger.image2Tz(2);
    if (p != FINGERPRINT_OK) {
        Serial.println("Error processing second image.");
        return p;
    }

    // Create fingerprint model
    p = finger.createModel();
    if (p == FINGERPRINT_OK) {
        Serial.println("Fingerprints matched!");
    } else {
        Serial.println("Fingerprint mismatch! Try again.");
        return p;
    }

    // Store the fingerprint
    p = finger.storeModel(id);
    if (p == FINGERPRINT_OK) {
        Serial.println("Fingerprint stored successfully!");
    } else {
        Serial.println("Error storing fingerprint.");
        return p;
    }

    return true;
}

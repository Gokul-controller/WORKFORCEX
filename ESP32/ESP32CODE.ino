#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

// ---------------- WiFi ----------------
const char* WIFI_SSID = "J.....";
const char* WIFI_PASSWORD = "200....";

// Your laptop/backend IP
const char* BACKEND_IP = "10.157.97.186";
const int BACKEND_PORT = 8000;

// ---------------- RFID ----------------
#define RFID_SS  21
#define RFID_RST 22

MFRC522 rfid(RFID_SS, RFID_RST);

// ---------------- LEDs + Buzzer ----------------
#define GREEN_LED 25
#define RED_LED   26
#define BUZZER    27

void beep() {
  digitalWrite(BUZZER, HIGH);
  delay(120);
  digitalWrite(BUZZER, LOW);
}

void connectWiFi() {

  Serial.print("Connecting to WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int count = 0;

  while (WiFi.status() != WL_CONNECTED && count < 30) {
    delay(500);
    Serial.print(".");
    count++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("WiFi CONNECTED");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());

  } else {

    Serial.println("WiFi CONNECTION FAILED");
  }
}

bool checkAccess(String uid) {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return false;
  }

  String url =
    "http://" +
    String(BACKEND_IP) +
    ":" +
    String(BACKEND_PORT) +
    "/attendance/scan/" +
    uid;

  Serial.println();
  Serial.println("ACCESS REQUEST");
  Serial.println(url);

  HTTPClient http;

  http.begin(url);
  http.setTimeout(5000);

  int responseCode = http.GET();

  Serial.print("HTTP CODE: ");
  Serial.println(responseCode);

  if (responseCode != 200) {

    Serial.println("BACKEND ERROR");

    http.end();

    return false;
  }

  String response = http.getString();

  Serial.println("BACKEND RESPONSE:");
  Serial.println(response);

  http.end();

  // Backend returned a successful employee lookup
  return true;
}

void setup() {

  Serial.begin(115200);

  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER, LOW);

  // SPI
  SPI.begin(18, 19, 23);

  // RFID
  pinMode(RFID_SS, OUTPUT);
  digitalWrite(RFID_SS, HIGH);

  rfid.PCD_Init();

  Serial.println();
  Serial.println("==============================");
  Serial.println("WORKFORCEX RFID ACCESS");
  Serial.println("==============================");

  connectWiFi();

  Serial.println("RFID READY");
  Serial.println("SCAN YOUR CARD");
}

void loop() {

  // No card
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  // Cannot read card
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Build UID
  String uid = "";

  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0";
    }

    uid += String(rfid.uid.uidByte[i], HEX);

    if (i < rfid.uid.size - 1) {
      uid += ":";
    }
  }

  uid.toUpperCase();

  Serial.println();
  Serial.println("==============================");
  Serial.println("RFID DETECTED");
  Serial.print("UID: ");
  Serial.println(uid);

  beep();

  bool accessGranted = checkAccess(uid);

  if (accessGranted) {

    Serial.println("ACCESS GRANTED");

    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);

    delay(3000);

    digitalWrite(GREEN_LED, LOW);

  } else {

    Serial.println("ACCESS DENIED");

    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);

    delay(3000);

    digitalWrite(RED_LED, LOW);
  }

  // Stop RFID communication
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(1000);

  Serial.println("READY FOR NEXT SCAN");
}
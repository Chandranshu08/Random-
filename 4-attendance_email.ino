/*
 ======================================================================
 EXP 6 : ESP32 + RFID ATTENDANCE WITH SECURE EMAIL ALERT (SMTP + SHA-256)
 ======================================================================
 CONNECTIONS (RC522 -> ESP32)           Optional indicators
   3.3V  -> 3V3   (NOT 5V!)              Green LED  -> GPIO 32 via 220 ohm -> GND
   GND   -> GND                          Red LED    -> GPIO 25 via 220 ohm -> GND
   RST   -> GPIO 22                      Buzzer +   -> GPIO 33, - -> GND
   SDA/SS-> GPIO 5
   SCK   -> GPIO 18
   MOSI  -> GPIO 23
   MISO  -> GPIO 19
   IRQ   -> not connected

 SETUP STEPS
  1. Arduino IDE -> Boards Manager -> install "esp32". Board: "ESP32 Dev Module".
  2. Library Manager -> install: MFRC522, ESP Mail Client (by Mobizt).
  3. Gmail: Google Account -> Security -> turn on 2-Step Verification -> App passwords ->
     create one -> paste the 16 characters below WITHOUT SPACES.
  4. Fill in WIFI_SSID / WIFI_PASSWORD (phone hotspot, 2.4 GHz), SENDER_EMAIL, SENDER_PASSWORD,
     RECIPIENT_EMAIL.
  5. Upload, open Serial Monitor at 115200, scan card. First scan prints the UID: copy it
     into users[] below and re-upload.
  6. Check the recipient inbox: attendance mail contains time, name, roll no and SHA-256 hash of UID.

 HOW IT WORKS
  Card scanned -> UID read -> compared with users[] -> authorized: check-in / check-out toggled,
  green LED + beep -> time from NTP -> UID hashed (SHA-256) -> email sent via smtp.gmail.com:465.
  Unauthorized: red LED + 4 beeps + alert email.
  Hash is used so the raw UID is never sent in the email.
 ======================================================================
*/
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include "time.h"
#include "mbedtls/md.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ---------- CHANGE THESE ----------
#define WIFI_SSID        "YOUR_WIFI_NAME"
#define WIFI_PASSWORD    "YOUR_WIFI_PASSWORD"
#define SENDER_EMAIL     "your_sender@gmail.com"
#define SENDER_PASSWORD  "xxxxxxxxxxxxxxxx"      // Gmail App Password, no spaces
#define RECIPIENT_EMAIL  "parent_or_teacher@gmail.com"
// ----------------------------------

#define SS_PIN 5
#define RST_PIN 22
#define GREEN_LED 32
#define RED_LED 25
#define BUZZER_PIN 33

MFRC522 mfrc522(SS_PIN, RST_PIN);
SMTPSession smtp;

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;   // IST = 5.5 h * 3600
const int daylightOffset_sec = 0;

struct User { String uid; String name; String roll; bool isPresent; };
User users[] = {
  {"FE 9C FA 03", "Student One", "IOT38", false},   // <- put your card UID + name
  {"AA BB CC DD", "Student Two", "IOT39", false}
};
const int numUsers = sizeof(users) / sizeof(users[0]);

String getCurrentTime() {
  struct tm t;
  if (!getLocalTime(&t)) return "Time Sync Error";
  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
  return String(buf);
}

String sha256(String payload) {
  byte out[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const unsigned char*)payload.c_str(), payload.length());
  mbedtls_md_finish(&ctx, out);
  mbedtls_md_free(&ctx);
  String s = "";
  for (int i = 0; i < 32; i++) { char h[3]; sprintf(h, "%02x", out[i]); s += h; }
  return s;
}

bool sendEmail(String subject, String body) {
  Session_Config config;
  config.server.host_name = "smtp.gmail.com";
  config.server.port = 465;
  config.login.email = SENDER_EMAIL;
  config.login.password = SENDER_PASSWORD;
  config.login.user_domain = "";

  SMTP_Message msg;
  msg.sender.name = "ESP32 Attendance";
  msg.sender.email = SENDER_EMAIL;
  msg.subject = subject;
  msg.addRecipient("Receiver", RECIPIENT_EMAIL);
  msg.text.content = body.c_str();

  if (!smtp.connect(&config)) { Serial.println("SMTP connect failed: " + smtp.errorReason()); return false; }
  if (!MailClient.sendMail(&smtp, &msg)) { Serial.println("Send failed: " + smtp.errorReason()); smtp.closeSession(); return false; }
  smtp.closeSession();
  Serial.println("Email sent successfully!");
  return true;
}

void beep(int times, int ms) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH); delay(ms);
    digitalWrite(BUZZER_PIN, LOW);  delay(ms);
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);   // avoids brownout resets when WiFi starts
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  pinMode(GREEN_LED, OUTPUT); pinMode(RED_LED, OUTPUT); pinMode(BUZZER_PIN, OUTPUT);

  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { Serial.print("."); delay(500); }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  delay(2000);
  Serial.println("Time: " + getCurrentTime());
  Serial.println("SYSTEM READY. Scan RFID card...");
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += (mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.trim(); uid.toUpperCase();
  String ts = getCurrentTime();
  Serial.println("\n[CARD] UID: " + uid + "  Time: " + ts);

  bool found = false;
  for (int i = 0; i < numUsers; i++) {
    if (uid == users[i].uid) {
      found = true;
      users[i].isPresent = !users[i].isPresent;
      String action = users[i].isPresent ? "CHECK-IN" : "CHECK-OUT";
      Serial.println("Authorized: " + users[i].name + " -> " + action);
      digitalWrite(GREEN_LED, HIGH); beep(1, 200); digitalWrite(GREEN_LED, LOW);

      String body = "Time: " + ts + "\nName: " + users[i].name +
                    "\nRoll No: " + users[i].roll + "\nAction: " + action +
                    "\n\n--- SECURITY ---\nUID SHA-256: " + sha256(uid) + "\n";
      Serial.println("UID hash: " + sha256(uid));
      sendEmail("[Attendance] " + users[i].name + " " + action, body);
      break;
    }
  }

  if (!found) {
    Serial.println("UNAUTHORIZED CARD");
    digitalWrite(RED_LED, HIGH); beep(4, 150); digitalWrite(RED_LED, LOW);
    String body = "Time: " + ts + "\nUnauthorized access attempt\nUID SHA-256: " + sha256(uid) + "\n";
    sendEmail("[SECURITY ALERT] Unauthorized Card", body);
  }

  Serial.println("SYSTEM READY. Scan RFID card...");
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

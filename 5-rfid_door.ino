/*
 ======================================================================
 EXP 5 : RFID CONTROLLED DOOR SYSTEM USING ESP32 (servo + LEDs + buzzer + Serial Monitor)
 ======================================================================
 CONNECTIONS (RC522 -> ESP32)
   3.3V  -> 3V3   (NOT 5V!)
   GND   -> GND
   RST   -> GPIO 22
   SDA/SS-> GPIO 5
   SCK   -> GPIO 18
   MOSI  -> GPIO 23
   MISO  -> GPIO 19
 OTHER PARTS
   Servo: red -> 5V (VIN), brown/black -> GND, orange/yellow (signal) -> GPIO 4
          (your lab table says GPIO 13 but the working code used GPIO 4 - match whichever you wire)
   Green LED anode -> GPIO 32 via 220 ohm, cathode -> GND
   Red LED anode   -> GPIO 25 via 220 ohm, cathode -> GND
   Buzzer +        -> GPIO 33, buzzer - -> GND
 (If you use an Arduino UNO instead: SS=10, RST=9, MOSI=11, MISO=12, SCK=13, 3.3V, use #include <Servo.h>)

 SETUP STEPS
  1. Install ESP32 board package; Library Manager: MFRC522, ESP32Servo.
  2. Board "ESP32 Dev Module", select COM port, upload.
  3. Serial Monitor at 115200 baud.
  4. Scan your card. The UID prints even if denied. Copy it into users[] and upload again.

 LOGIC
  Scan -> read UID -> compare with users[]
   Authorized: green LED ON, short beep, servo to 90 (door open), wait 3 s, back to 0 (closed).
               Serial prints name, roll no, CHECK-IN / CHECK-OUT.
   Denied    : red LED ON + long beep for 1.5 s, door stays closed, Serial prints "Access Denied".
 ======================================================================
*/
#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

#define SS_PIN 5
#define RST_PIN 22
#define GREEN_LED 32
#define RED_LED 25
#define BUZZER_PIN 33
#define SERVO_PIN 4

MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo doorServo;

struct User { String uid; String name; String roll; bool isPresent; };
User users[] = {
  {"FE 9C FA 03", "Student One", "IOT38", false},   // <- put your card UID + name
  {"AA BB CC DD", "Student Two", "IOT39", false}
};
const int numUsers = sizeof(users) / sizeof(users[0]);

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  pinMode(GREEN_LED, OUTPUT); pinMode(RED_LED, OUTPUT); pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(GREEN_LED, LOW); digitalWrite(RED_LED, LOW); digitalWrite(BUZZER_PIN, LOW);
  doorServo.attach(SERVO_PIN);
  doorServo.write(0);                       // door closed
  Serial.println("System Ready. Tap an RFID tag...");
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
  Serial.println("Scanned Card UID: " + uid);

  bool granted = false;
  for (int i = 0; i < numUsers; i++) {
    if (uid == users[i].uid) {
      granted = true;
      Serial.println("-------------------------");
      Serial.println("Name: " + users[i].name);
      Serial.println("Roll No: " + users[i].roll);
      if (!users[i].isPresent) { Serial.println("Status: CHECK-IN SUCCESSFUL (Welcome!)"); users[i].isPresent = true; }
      else                     { Serial.println("Status: CHECK-OUT SUCCESSFUL (Goodbye!)"); users[i].isPresent = false; }
      Serial.println("-------------------------");

      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(BUZZER_PIN, HIGH); delay(200); digitalWrite(BUZZER_PIN, LOW);   // short beep
      Serial.println("Door Opening...");
      doorServo.write(90);
      delay(3000);                                                                  // open for 3 s
      Serial.println("Door Closing...");
      doorServo.write(0);
      digitalWrite(GREEN_LED, LOW);
      break;
    }
  }

  if (!granted) {
    Serial.println("Access Denied! Unregistered Tag.");
    doorServo.write(0);
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);          // long beep
    delay(1500);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  }

  mfrc522.PICC_HaltA();                      // stop reading same card repeatedly
  mfrc522.PCD_StopCrypto1();
}

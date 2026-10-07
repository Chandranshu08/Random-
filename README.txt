IISC PRACTICAL ORAL - 7 OCT 2026 - QUICK GUIDE (1 hour exam)
=============================================================
Folder map (each file has aim, connections/variables, steps, code, expected output):
 1_PLC_Traffic_Light/traffic_light.txt            -> Exp 2
 2_PLC_Conveyor_Warning_Lamp/conveyor_lamp.txt    -> Exp 3
 3_PLC_Motor_10s_ON_10s_OFF/motor_loop.txt        -> Start button, 10s run, 10s idle, loop
 4_ESP32_RFID_Email_Attendance/attendance_email.ino -> Exp 6
 5_RFID_Door_Access/rfid_door.ino                 -> Exp 5
 6_Python_Temperature_Analytics/analytics.py      -> Exp 7 style, any dataset

BEFORE THE EXAM CHECKLIST
 - Arduino IDE: ESP32 board package installed (Boards Manager -> "esp32" by Espressif)
 - Libraries (Library Manager): MFRC522 (GithubCommunity), ESP32Servo, ESP Mail Client (Mobizt)
 - Python: pip install pandas numpy matplotlib seaborn
 - Know your card UID (scan once, Serial Monitor at 115200 prints it, copy into users[])
 - Gmail needs 2-Step Verification ON + a 16-char App Password (not your normal password)
 - Phone hotspot for WiFi (2.4 GHz only; ESP32 does not do 5 GHz)

SECURITY WARNING
 Your lab PDF has a real Gmail app password and WiFi password printed in the code.
 Revoke that app password (Google Account -> Security -> App passwords) and make a new one.
 Never print it in a submission.

TIME PLAN (60 min): do whichever the examiner gives; PLC ones take ~10 min, ESP32 ones ~15-20 min,
Python ~15 min.

VIVA QUICK ANSWERS
 PLC: Industrial ruggedized digital computer for real-time control. Parts: CPU, power supply,
   memory, input module, output module, programming device/communication ports.
 I/O modules: Input module converts field signals (push buttons, sensors, 24V) to logic levels
   for the CPU. Output module converts CPU decisions to drive motors, lamps, contactors.
 Ladder logic: graphical language that looks like relay circuits. Left/right rails, rungs,
   contacts (NO/NC) = conditions, coils = outputs. Scanned left-to-right, top-to-bottom, repeatedly.
 TON timer: on-delay. IN true -> ET counts up -> when ET = PT, Q goes true. IN false resets it.
 NO contact passes power when its bit = 1. NC passes when bit = 0. (S) = set/latch coil.
 RFID: reader emits RF field, tag replies with UID. Passive tag (no battery, powered by reader
   field), Active tag (battery, long range), Semi-passive/BAP (battery for chip, passive comms).
   Frequency: LF 125 kHz, HF 13.56 MHz (RC522 uses this), UHF 860-960 MHz.
 RFID applications: access control, attendance, supply chain/inventory, toll collection
   (FASTag), contactless payment/ticketing, livestock/pet tracking, library management.
 RC522 talks to ESP32 over SPI: SDA(SS)=5, SCK=18, MOSI=23, MISO=19, RST=22, 3.3V (NOT 5V!).
 Servo: PWM signal sets angle 0-180 deg. Powered from 5V.
 SMTP: protocol to send mail. Gmail: smtp.gmail.com, port 465 (SSL) or 587 (STARTTLS).
 Hash vs encryption: SHA-256 is one-way (cannot be reversed), encryption (XOR/AES) is reversible
   with a key. Hash proves the data is unchanged without exposing the raw UID.
 NTP: ESP32 gets real time from internet (pool.ntp.org). IST offset = 19800 s.
 Data analytics steps: collect -> preprocess (missing/duplicates) -> EDA -> analysis -> report.
 Outlier != error: could be a real event. IQR rule: below Q1-1.5*IQR or above Q3+1.5*IQR.

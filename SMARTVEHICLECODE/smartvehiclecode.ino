#define BLYNK_TEMPLATE_ID "TMPL3Z9A8QJ_O"
#define BLYNK_TEMPLATE_NAME "smart vehicle"
#define BLYNK_AUTH_TOKEN "BQoWv0mgQ1wvqrT2CRB0r0TELTS5j2y4"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

char ssid[] = "iot@12345";
char pass[] = "iot@12345";

#define SS_PIN 5
#define RST_PIN 4
MFRC522 rfid(SS_PIN, RST_PIN);

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Motor pins
#define ENA 26
#define IN1 14
#define IN2 27

// Sensors
#define MQ135 34
#define POT 33

// Mode Switch
#define MODE_SWITCH 25

String validUID = "f9433e7";
int gasThreshold = 2000;

bool accessGranted = false;

void setup() {
  Serial.begin(115200);

  // I2C
  Wire.begin(21, 22, 100000);
  delay(100);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("SMART VEHICLE");
  delay(1500);

  // WiFi + Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // RFID
  SPI.begin(18, 19, 23, 5);
  rfid.PCD_Init();

  // Motor
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  ledcAttach(ENA, 5000, 8);

  // Switch
  pinMode(MODE_SWITCH, INPUT);

  lcd.clear();
  lcd.print("Select Mode");
}

void loop() {
  Blynk.run();

  bool learnerMode = (digitalRead(MODE_SWITCH) == HIGH);

  int potValue = analogRead(POT);
  int potPercent = map(potValue, 0, 4095, 0, 100);

  // =======================
  // 🔴 NORMAL MODE
  // =======================
  if (!learnerMode) {

    if (!accessGranted) {
      lcd.setCursor(0, 0);
      lcd.print("Normal Mode   ");
      lcd.setCursor(0, 1);
      lcd.print("Scan License  ");

      if (!rfid.PICC_IsNewCardPresent()) return;
      if (!rfid.PICC_ReadCardSerial()) return;

      String uid = "";
      for (byte i = 0; i < rfid.uid.size; i++) {
        uid += String(rfid.uid.uidByte[i], HEX);
      }

      if (uid != validUID) {
        lcd.setCursor(0, 0);
        lcd.print("Invalid Card  ");
        lcd.setCursor(0, 1);
        lcd.print("Access Denied ");
        Blynk.virtualWrite(V3, 0);
        delay(1500);
        return;
      }

      lcd.setCursor(0, 0);
      lcd.print("License OK    ");
      lcd.setCursor(0, 1);
      lcd.print("Checking Gas  ");
      Blynk.virtualWrite(V3, 1);

      delay(2000);

      int gas = analogRead(MQ135);
      Blynk.virtualWrite(V0, map(gas, 0, 4095, 0, 1000));

      if (gas > gasThreshold) {
        lcd.setCursor(0, 0);
        lcd.print("Alcohol Found ");
        lcd.setCursor(0, 1);
        lcd.print("Dont Drive    ");
        Blynk.virtualWrite(V1, 1);
        return;
      }

      Blynk.virtualWrite(V1, 0);
      accessGranted = true;

      lcd.setCursor(0, 0);
      lcd.print("Engine Start  ");
      lcd.setCursor(0, 1);
      lcd.print("Ready         ");
      delay(1500);
    }

    // Motor ALWAYS RUN
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    ledcWrite(ENA, 200); // fixed speed

    lcd.setCursor(0, 0);
    lcd.print("Driving Mode  ");

    if (potPercent > 50) {
      lcd.setCursor(0, 1);
      lcd.print("High Speed    ");
      Blynk.virtualWrite(V4, 1);
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Normal Speed  ");
      Blynk.virtualWrite(V4, 0);
    }

    Blynk.virtualWrite(V5, "Speed:" + String(potPercent) + "%");
  }

  // =======================
  // 🟢 LEARNER MODE
  // =======================
  else {

    lcd.setCursor(0, 0);
    lcd.print("Learner Mode  ");

    if (potPercent > 50) {
      lcd.setCursor(0, 1);
      lcd.print("Speed Limit!  ");
      ledcWrite(ENA, 0); // STOP MOTOR
      return;
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Safe Driving  ");
    }

    // Motor RUN
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    ledcWrite(ENA, 200);
  }

  delay(300);
}
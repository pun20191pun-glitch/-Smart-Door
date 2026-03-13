#include <Keypad.h>

// ----- Keypad Setup -----
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '4', '7', '*'},
  {'2', '5', '8', '0'},
  {'3', '6', '9', '#'},
  {'A', 'B', 'C', 'D'}
};
byte rowPins[ROWS] = {9, 8, 7, 6};
byte colPins[COLS] = {5, 4, 3, 2};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ----- Relay & Button Setup -----
const int relayPin = 11;     // รีเลย์
const int buttonPin = 10;    // ปุ่มกดติด-ปล่อย

String inputPassword = "";
const String correctPassword = "1111";

void setup() {
  Serial.begin(115200);

  pinMode(relayPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);   // ปุ่มกดติด-ปล่อย

  digitalWrite(relayPin, HIGH); // ปิดรีเลย์เริ่มต้น

  Serial.println("ระบบเริ่มทำงาน: Keypad และปุ่มขา 10 พร้อมใช้งาน");
}

void loop() {
  // --- ขา 10 ปุ่มกดติด-ปล่อย ---
  if (digitalRead(buttonPin) == LOW) {
    digitalWrite(relayPin, LOW); // กด → เปิดรีเลย์
  } else {
    digitalWrite(relayPin, HIGH); // ปล่อย → ปิดรีเลย์
  }

  // --- Keypad ---
  char key = keypad.getKey();
  if (key != NO_KEY) {
    Serial.print("กด: ");
    Serial.println(key);

    if (key == '#') {
      if (inputPassword == correctPassword) {
        Serial.println("✅ รหัสถูกต้อง - เปิดรีเลย์ 10 วิ");
        digitalWrite(relayPin, LOW);
        delay(10000); // ระหว่างนี้ Keypad และขา10 ไม่ตอบสนอง
        digitalWrite(relayPin, HIGH);
      } else {
        Serial.println("❌ รหัสผิด");
      }
      inputPassword = "";
    } 
    else if (key == '*') {
      inputPassword = "";
      Serial.println("🔁 ล้างรหัสแล้ว");
    } 
    else {
      inputPassword += key;
    }
  }
}

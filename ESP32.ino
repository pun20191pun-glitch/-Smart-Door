#include <Keypad.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ====== Keypad Setup ======
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '4', '7', '*'},
  {'2', '5', '8', '0'},
  {'3', '6', '9', '#'},
  {'A', 'B', 'C', 'D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ====== Relay & Button ======
const int relayPin = 15;     
const int buttonPin = 4;     

String inputPassword = "";
const String correctPassword = "1111";

// ====== WiFi & MQTT ======
const char* ssid = "pen kuy ri";
const char* password = "bell359273665";

const char* mqtt_server = "10.80.3.57"; 
const int mqtt_port = 1883;
const char* mqtt_client_id = "ESP32Client";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastReconnectAttempt = 0;

// ====== Relay Control Timer ======
bool relayTimerActive = false;
unsigned long relayOffTime = 0;

// ====== Button Debounce ======
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// ====== ฟังก์ชัน MQTT ======
void sendStatus(String status) {
  if (client.connected()) {
    client.publish("esp32/status", status.c_str());
  }
}

void startRelayTimer(unsigned long duration) {
  relayTimerActive = true;
  relayOffTime = millis() + duration;
  digitalWrite(relayPin, LOW);
  Serial.println("Relay ON (timer)");
}

void handleRelayTimer() {
  if (relayTimerActive && millis() >= relayOffTime) {
    digitalWrite(relayPin, HIGH);
    relayTimerActive = false;
    Serial.println("Relay OFF (timer)");
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.print("Received: ");
  Serial.println(msg);

  String command = "";
  DynamicJsonDocument doc(200);
  if (deserializeJson(doc, msg) == DeserializationError::Ok) {
    command = doc["msg"].as<String>();
  } else {
    command = msg;
  }

  if (command == "BLINK") {
    Serial.println("🚀 BLINK -> รีเลย์ทำงาน 2 วิ");
    startRelayTimer(2000);  
    sendStatus("Relay ON 2s (BLINK)");
  }
}

bool reconnect() {
  if (client.connect(mqtt_client_id)) {
    Serial.println("connected to MQTT!");
    client.subscribe("arduino/data");
    return true;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  digitalWrite(relayPin, HIGH);

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  lastReconnectAttempt = 0;

  Serial.println("ระบบพร้อม: Keypad + ปุ่ม + Relay + MQTT (topic=arduino/data)");
}

// ====== ปุ่มกด ======
void handleButton() {
  bool reading = digitalRead(buttonPin);
  if (reading != lastButtonState) lastDebounceTime = millis();

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW) {
      digitalWrite(relayPin, LOW);  // กด → เปิดรีเลย์
    } else {
      if (!relayTimerActive) digitalWrite(relayPin, HIGH);  // ปล่อย → ปิดถ้า Timer ไม่ทำงาน
    }
  }
  lastButtonState = reading;
}

// ====== Loop ======
void loop() {
  // MQTT
  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      if (reconnect()) lastReconnectAttempt = 0;
    }
  } else {
    client.loop();
  }

  handleRelayTimer();
  handleButton();

  // Keypad
  char key = keypad.getKey();
  if (key != NO_KEY) {
    Serial.print("กด: ");
    Serial.println(key);

    if (key == '#') {
      if (inputPassword == correctPassword) {
        Serial.println("✅ รหัสถูกต้อง - เปิดรีเลย์ 3 วิ");
        startRelayTimer(3000);  
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

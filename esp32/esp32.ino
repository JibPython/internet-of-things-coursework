// for wifi 
#include <WiFi.h>
#include <credentials.h>

// for mqtt
#include <PubSubClient.h>
const char* mqtt_server = "io.adafruit.com";
WiFiClient espClient;
PubSubClient client(espClient);

// for creating timer to avoid ada throttling
unsigned long startMillis;  
unsigned long currentMillis;
const unsigned long period = 2000;

// for dht11
#include <DHT.h>

// for rfid
#include <SPI.h>
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

// setting up the 1602 lcd display
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SDA_PIN 21
#define SCL_PIN 22

LiquidCrystal_I2C lcd(0x27, 16, 2);

// for handling people inside the room
#include <vector>
#include <ctime>
#include <algorithm>
using namespace std;
// .push_back("") to add to back to vector
// .pop_back("") to remove last vector item
vector<String> uids_in_room = {};
// "97770d01"
vector<String> acceptable_uids = {"97770d01"};

// setting up the 1602 lcd row values
String top_row_lcd = "                ";
String bottom_row_lcd = "                ";

// setting up the RFID module
MFRC522DriverPinSimple ss_pin(32);
MFRC522DriverSPI driver{ss_pin};
MFRC522 mfrc522{driver};

// setting up motion sensor
const int motion_pin = 14;
struct MovementTimes {time_t start_time; time_t end_time;};

// setting up the sound sensor
const int sound_pin = 4;

// setting up the ultrasonic sensor
const int ultra_echo_pin = 18;
const int ultra_trig_pin = 5;
long duration;

// setting up the dht11 sensor
#define Type DHT11
const int dht_pin = 13;
DHT dht(dht_pin, Type);

// setting up the passive buzzer
const int buzzer_pin = 33;


void setup() {
  Serial.begin(115200);
  // triggering the timer to start
  startMillis = millis();

  // connecting to the wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");

  while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(100);
  }
  
  // connecting to adafruit 
  client.setServer(mqtt_server, 1883);

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  // setting up the 1602 lcd display 
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // setting up the RFID module
  SPI.begin(27, 19, 23, 32); // SCK, MISO, MOSI, SS
  mfrc522.PCD_Init();

  // setting up the motion sensor
  pinMode(motion_pin, INPUT);

  // setting up the sound sensor
  pinMode(sound_pin, INPUT);

  // setting up the ultrasonic sensor
  pinMode(ultra_trig_pin, OUTPUT);
  pinMode(ultra_echo_pin, INPUT);

  // setting up the DHT11 sensor
  dht.begin();

  // setting up the passive buzzer
  pinMode(buzzer_pin, OUTPUT);
}

void loop() {
  maintain_mqtt();

  rfid_check();

  // motion_check();

  // sound_check();

  // ultrasonic_check();

  temperature_check();

  delay(50);
}
 

void rfid_check() {
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
	if (!mfrc522.PICC_IsNewCardPresent()) {
		Serial.println("FAILED AT STOP 1");
    return;
	}

	// Select one of the cards.
	if (!mfrc522.PICC_ReadCardSerial()) {
    Serial.println("FAILED AT STOP 2");
		return;
	}

  Serial.println("GOT TO HERE");


  const String uid = get_rfid_uid();
  time_t current_time = get_current_time();
  bool is_rfid_acceptable = get_rfid_acceptable(uid);
  bool is_uid_in_room = get_uids_in_room(uid);

  // acceptable user trying to enter room
  if (is_rfid_acceptable && !is_uid_in_room) {
    uids_in_room.push_back(uid);

    vector<String> lcd_text = {"authorised user", "entered room"};
    send_lcd_message(lcd_text, 5000);

    Serial.println("acceptable user trying to enter room");

    const String alert_message = "Authorised user entered the room.\nWith the uid: " + uid;
    high_priority("rfid", alert_message);

    const String inside_warehouse = get_people_inside();
    high_priority("inside", inside_warehouse);
  }
  // acceptable user trying to leave room
  else if (is_rfid_acceptable && is_uid_in_room) {
    remove_uid_from_room(uid);

    vector<String> lcd_text = {"authorised user", "left room"};
    send_lcd_message(lcd_text, 5000);

    Serial.println("acceptable user trying to leave room");

    const String alert_message = "Authorised user left the room.\nWith the uid: " + uid;
    high_priority("rfid", alert_message);

    const String inside_warehouse = get_people_inside();
    high_priority("inside", inside_warehouse);
  }
  // unacceptable user trying to interact with RFID
  else if (!is_rfid_acceptable && uid != "") {

    vector<String> lcd_text = {"RFID not in", "the database"};
    send_lcd_message(lcd_text, 5000);

    activate_buzzer(1);

    Serial.println("unacceptable user trying to interact with RFID");

    const String alert_message = "Unauthorised user tried interacting with the RFID scanner.\nWith the uid: " + uid;
    high_priority("rfid", alert_message);

    const String inside_warehouse = get_people_inside();
    high_priority("inside", inside_warehouse);
  }
}

void motion_check() {
  const int movement_detected = digitalRead(motion_pin);

  if (movement_detected && uids_in_room.size() == 0) {
    tone(buzzer_pin, 300);
    auto [start_time, end_time] = get_movement_times();
    noTone(buzzer_pin);

    Serial.print("start time: ");
    Serial.print(start_time);
    Serial.print(", end time: ");
    Serial.println(end_time);

    vector<String> lcd_text = {"movement whilst", "room is empty"};
    send_lcd_message(lcd_text, 5000);
  }
  else if (movement_detected) {
    auto [start_time, end_time] = get_movement_times();

    Serial.print("start time: ");
    Serial.print(start_time);
    Serial.print(", end time: ");
    Serial.println(end_time);
  }
}

void sound_check() {
  int detected_sound = digitalRead(sound_pin);

  if (detected_sound && uids_in_room.size() == 0) {
    vector<String> lcd_text = {"sound whilst", "room is empty"};
    send_lcd_message(lcd_text, 5000);
  }
}

void ultrasonic_check() {
  int distance = get_distance(); 

  Serial.println(distance);

  const int close = 30;
  const int close_freq = 300;
  const int very_close = 18;
  const int very_close_freq = 400;

  // this variable makes it so that the tone of the buzzer only changes if the distance type changes (close, very_close) 
  // from the previous state so the sound doesn't sound weird.
  String previous_distance = "";

  while (distance <= close) {
    if (distance <= very_close) {
      if (previous_distance == "" || previous_distance == "close") {
        tone(buzzer_pin, very_close_freq);
        previous_distance = "very_close";
      } 
    }
    else {
      if (previous_distance == "" || previous_distance == "very_close") {
        tone(buzzer_pin, close_freq);
        previous_distance = "close";
      } 
    }
    distance = get_distance(); 
    Serial.println(distance);
  }

  noTone(buzzer_pin);
}

void temperature_check() {
  // temperature in celsius
  float temp_c = dht.readTemperature();
  float min_threshold = 8.00;
  float max_threshold = 30.00;
  Serial.print("Temperature: ");
  Serial.println(temp_c);
  
  normal_priority("temperature", temp_c);

  if (temp_c > max_threshold) {
    tone(buzzer_pin, 300);

    vector<String> lcd_text = {"temperature too", "high"};
    send_lcd_message(lcd_text, 5000);

    noTone(buzzer_pin);
    high_priority("alert", "Temperature is above the maximum threshold.");
  }
  else if (temp_c < min_threshold) {
    tone(buzzer_pin, 300);

    vector<String> lcd_text = {"temperature too", "low"};
    send_lcd_message(lcd_text, 5000);

    noTone(buzzer_pin);
    high_priority("alert", "Temperature is below the minimum threshold.");
  }

}

String get_rfid_uid() {
  String uid_string = "";
  for (uint8_t i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      uid_string += "0";
    }
    uid_string += String(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println("Current uid: " + uid_string);
  return uid_string;
}

// finds out if argument is in acceptable_uids vector
bool get_rfid_acceptable(String uid) {
  return std::find(acceptable_uids.begin(), acceptable_uids.end(), uid) != acceptable_uids.end();
}

// finds out if the uid is currently inside the room
bool get_uids_in_room(String uid) {
  return std::find(uids_in_room.begin(), uids_in_room.end(), uid) != uids_in_room.end();
}

time_t get_current_time() {
  return time(NULL);
}

void send_lcd_message(vector<String> text, int timeout) {
  lcd.setCursor(0, 0);
  lcd.print(text.front());
  lcd.setCursor(0, 1);
  lcd.print(text.back());

  delay(timeout);
  lcd.clear();
}

void remove_uid_from_room(String uid) {
  auto iterator = std::find(uids_in_room.begin(), uids_in_room.end(), uid);
  if (iterator != uids_in_room.end()) uids_in_room.erase(iterator);
}

void activate_buzzer(int no_loops) {
  for (int i = no_loops; i > 0; i--) {
    tone(buzzer_pin, 300);
    delay(500);
    noTone(buzzer_pin);
    delay(500);
  }
}

// this function gets the start and end movement times for the motion sensor
MovementTimes get_movement_times() {
  const time_t start_time = get_current_time(); 
  int movement_detected = digitalRead(motion_pin);
  while (movement_detected) {
    int current_movement = digitalRead(motion_pin);
    if (!current_movement) {movement_detected = !movement_detected;}
    delay(50);
  }
  const time_t end_time = get_current_time();
  return {start_time, end_time};
}

// this returns the distance for the ultrasonic sensor
int get_distance() {
  digitalWrite(ultra_trig_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(ultra_trig_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(ultra_trig_pin, LOW);

  duration = pulseIn(ultra_echo_pin, HIGH);
  return (duration * 0.034 / 2); 
}

void maintain_mqtt() {
  if (!client.connected()) {
    reconnect();
  }
  // keep MQTT alive
  client.loop();
}

// ensures that esp32 stays connected to broker
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting to create an MQTT connection...");
    if (client.connect("esp32", adafruit_username, adafruit_key)) {
      Serial.println("connected");
      client.subscribe("JibPython/feeds/output");
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void save_details(String route, float msg) {
  const char* formatted_msg = to_string(msg).c_str();
  if (route == "temperature") {
    client.publish("JibPython/feeds/temperature", formatted_msg);
  }
  else if (route == "rfid") {
    client.publish("JibPython/feeds/alert", formatted_msg);
    Serial.println(formatted_msg);
  }
  else if (route == "inside") {
    client.publish("JibPython/feeds/peopleinroom", formatted_msg);
    Serial.println(formatted_msg);
  }
}

void save_details(String route, String msg) {
  const char* formatted_msg = msg.c_str();
  if (route == "temperature") {
    client.publish("JibPython/feeds/temperature", formatted_msg);
  }
  else if (route == "rfid") {
    client.publish("JibPython/feeds/alert", formatted_msg);
    Serial.println(formatted_msg);
  }
  else if (route == "inside") {
    client.publish("JibPython/feeds/peopleinroom", formatted_msg);
    Serial.println(formatted_msg);
  }
}

// returns true if enough time has passed since last publish and updates timer. Used for normal priority.
bool time_passed() {
  currentMillis = millis();
  if (currentMillis - startMillis >= period) {
    startMillis = currentMillis;
    return true;
  }
  else {
    return false;
  }
}

// used for events that MUST be published to adafruit
void high_priority(String route, auto data) {
  currentMillis = millis();
  while (currentMillis - startMillis < period) { currentMillis = millis(); client.loop(); }
  startMillis = currentMillis;

  if (!client.connected()) {
    reconnect();
  }
  
  save_details(route, data);
}

// used for events that don't have to always publish latest data
void normal_priority(String route, auto data) {
  if (time_passed()) {
    save_details(route, data);
  }
}

// returns a text formatted of people who are inside the house
String get_people_inside() {
  String temp_msg = "";

  if (uids_in_room.size() > 0) { 
    for (int i=0; i < uids_in_room.size(); i++) {
      temp_msg += "\n" + uids_in_room[i];
    }
  }
  else {
    temp_msg = "There is currently no one inside.\n";
  }

  return temp_msg;
}

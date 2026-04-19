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

// setting up the passive buzzer
const int buzzer_pin = 33;


void setup() {
  Serial.begin(115200);

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

  // setting up the passive buzzer
  pinMode(buzzer_pin, OUTPUT);
}

void loop() {

  // rfid_check();

  motion_check();

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
  }
  // acceptable user trying to leave room
  else if (is_rfid_acceptable && is_uid_in_room) {
    remove_uid_from_room(uid);

    vector<String> lcd_text = {"authorised user", "left room"};
    send_lcd_message(lcd_text, 5000);

    Serial.println("acceptable user trying to leave room");
  }
  // unacceptable user trying to interact with RFID
  else if (!is_rfid_acceptable && uid != "") {
    vector<String> lcd_text = {"RFID not in", "the database"};
    send_lcd_message(lcd_text, 5000);

    activate_buzzer(1);

    Serial.println("unacceptable user trying to interact with RFID");
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
    noTone(2);
    delay(500);
  }
}

// this function gets the start and end movement times for the motion sensor
MovementTimes get_movement_times() {
  const time_t start_time = get_current_time(); 
  int movement_detected = digitalRead(motion_pin);
  while (movement_detected) {
    int current_movement = digitalRead(motion_pin);
    if (movementcurrent_movement) {movement_detected = !movement_detected;}
    delay(50);
  }
  const time_t end_time = get_current_time();
  return {start_time, end_time};
}
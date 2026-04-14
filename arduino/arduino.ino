
#include <LiquidCrystal.h>

// setting up the Liquid 
const int rs = 5, en = 6, d4 = 7, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// setting up the ultrasound sensor
#define echoPin 9
#define trigPin 10
long duration; 
int distance; 

// setting up the motion sensor 
const int motionPin = 4;

void setup() {
  Serial.begin(115200);
  // for motion sensor
  pinMode(motionPin, INPUT);
  // for ultrasound sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  // for lcd display
  lcd.begin(16, 2);
  lcd.print("hello, world!");
}

void loop() {
  // LCD CODE
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  lcd.print(millis() / 1000);

  // ULTRASOUND CODE
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH, 30000);

  /*
  if (duration == 0) {
    Serial.println("No echo");
  } else {
    distance = duration * 0.034 / 2;
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  }

  delay(60);
  */

  // MOTION SENSOR CODE
  int HandMotion = digitalRead(motionPin);

  if (HandMotion) {
    Serial.println("Hand Motion detected");
  } else {
    Serial.println("===nothing moves");
  }
  delay(100);

}
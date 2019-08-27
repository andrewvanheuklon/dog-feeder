#include <StringSplitter.h>
#include <Servo.h>
#include <AccelStepper.h>
#include "HX711.h"

const int TRAY_SWITCH = 45;
const int INPUT_BUTTON = 5;

const int VIBRATE_RELAY = 14;
const int VIBRATE_MS = 1000;

const int BOWL_STEPPER_STEP = 36;
const int BOWL_STEPPER_DIRECTION = 37;
const int BOWL_STEPPER_SLEEP = 35;

const int HOPPER_STEPPER_STEP = 52;
const int HOPPER_STEPPER_DIRECTION = 53;
const int HOPPER_STEPPER_SLEEP = 51;
const int HOPPER_SPEED = 15000;
// others

const int DOOR_SERVO = 6;
const int DOOR_OPENED = 2;
const int DOOR_CLOSED = 104;
const int DOOR_DELAY_STEP = 1;
const float DOOR_ACCEL_AMOUNT = .0009;
const float DOOR_START_DP = .00001;

const int STEPPER_SPEED_OUT = 15000;
const int STEPPER_ACCEL_OUT = 5000;

const int STEPPER_SPEED_IN = 3000;
const int STEPPER_ACCEL_IN = 2000;

const int OUT_POS = -6900;
const int IN_POS = -30;
const int CENTERED_POS = -300;

const int RED = 10;
const int BLUE = 12;
const int GREEN = 11;
const int WHITE = 99;

int redValue = 0;
int blueValue = 0;
int greenValue = 0;

const int MOTION_SENSOR = 17;

const int HX711_DT = 29;
const int HX711_SK = 28;
float calibrationFactor = -1950;
HX711 scale;

String command = "";

Servo doorServo;
boolean isDoorOpen = false;
boolean isBowlOut = false;

AccelStepper stepper(AccelStepper::DRIVER, BOWL_STEPPER_STEP, BOWL_STEPPER_DIRECTION);
AccelStepper hopper(AccelStepper::DRIVER, HOPPER_STEPPER_STEP, HOPPER_STEPPER_DIRECTION);

int pos = 200;
unsigned long piCheckInMillis = millis();
const long CHECK_IN_WARN_MS = 60000;

void setup() {
  Serial.begin(9600);

  //BOWL STEPPER
  stepper.setMaxSpeed(8000);
  stepper.setAcceleration(3000);
  stepper.setEnablePin(BOWL_STEPPER_SLEEP);
  stepper.disableOutputs();

  //HOPPER
  hopper.setMaxSpeed(HOPPER_SPEED);
  hopper.setAcceleration(10000);
  hopper.setEnablePin(HOPPER_STEPPER_SLEEP);
  hopper.disableOutputs();

  //SWITCHES
  pinMode(TRAY_SWITCH, INPUT_PULLUP);
  pinMode(INPUT_BUTTON, INPUT_PULLUP);

  //LEDS
  pinMode(RED, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(GREEN, OUTPUT);

  pinMode(MOTION_SENSOR, INPUT);
  pinMode(VIBRATE_RELAY, OUTPUT);

  delay(1000);
  scale.begin(HX711_DT, HX711_SK);
  scale.set_scale();
  scale.tare();
  scale.set_scale(calibrationFactor);

  initPositions();

  //testMotionSensor();
}

void initPositions() {
  calibratePosition();
  bowlIn();

  doorServo.attach(DOOR_SERVO);
  doorServo.write(DOOR_CLOSED);
  delay(1000);
  doorServo.detach();
}

void testLoop() {
  while (1) {
    waitForButtonPress(false);
    testHopper();
    calibratePosition();
    openDoor();
    bowlOut();
    waitForButtonPress(true);
    bowlIn();
    closeDoor();
  }
}

void testAll() {
  colorOn(WHITE);

  calibratePosition();
  openDoor();
  bowlOut();
  bowlIn();
  closeDoor();
  lightsOff();
}

void waitForButtonPress(bool colors) {
  if (colors)
    colorOn(RED);

  while (digitalRead(INPUT_BUTTON) == HIGH) {
    delay(10);
  }

  blinkColor(GREEN);
}

void testMotionSensor() {
  openDoor();
  bowlOut();

  while (digitalRead(INPUT_BUTTON) == HIGH) {

    if (digitalRead(MOTION_SENSOR) == HIGH) {
      toColor(0, 150, 0);
    } else {
      toColor(150, 0, 0);
    }
    delay(100);
  }

  toColor(0, 0, 0);

  bowlIn();
  closeDoor();

}

void testColors() {
  blinkColor(RED);
  blinkColor(GREEN);
  blinkColor(BLUE);
}

void testSwitch(int switchPin) {
  colorOn(RED);
  while (digitalRead(switchPin) == HIGH) {
    delay(10);
  }
  colorOn(GREEN);
  delay(2000);
  lightsOff();
}

void blinkColor(int color) {
  lightsOff();
  analogWrite(color, 255);
  delay(1000);
  analogWrite(color, 0);
}

void colorOn(int color) {
  lightsOff();
  if (color == WHITE) {
    analogWrite(RED, 255);
    analogWrite(BLUE, 255);
    analogWrite(GREEN, 255);
  } else {
    analogWrite(color, 255);
  }
}

void lightsOff() {
  analogWrite(RED, 0);
  analogWrite(BLUE, 0);
  analogWrite(GREEN, 0);
}

void toColor(int r, int g, int b) {
  redValue = r;
  greenValue = g;
  blueValue = b;

  analogWrite(RED, r);
  analogWrite(GREEN, g);
  analogWrite(BLUE, b);
}

void fadeToColor(int targetR, int targetG, int targetB) {
  bool done = false;

  int red = redValue;
  int green = greenValue;
  int blue = blueValue;

  while (!done || digitalRead(INPUT_BUTTON) == LOW) {
    if (redValue != targetR) {
      red += ((targetR - redValue) / abs(targetR - redValue));
    }
    if (greenValue != targetG) {
      green += ((targetG - greenValue) / abs(targetG - greenValue));
    }
    if (blueValue != targetB) {
      blue += ((targetB - blueValue) / abs(targetB - blueValue));
    }
    delay(10);
    toColor(red, green, blue);
    done = (red == targetR) && (blue == targetB) && (green == targetG);
  }
}

void testHopperCycles(int num) {
  hopper.enableOutputs();
  hopper.setSpeed(HOPPER_SPEED);
  while (num > 0) {
    hopper.runSpeed();
    //Serial.println("h");
    num--;
    delay(1);
  }
  hopper.disableOutputs();
}

void testHopper() {
  colorOn(GREEN);
  hopper.enableOutputs();
  hopper.setCurrentPosition(0);
  hopper.setSpeed(HOPPER_SPEED);
  hopper.moveTo(10000);
  vibrateOn();

  int val = digitalRead(INPUT_BUTTON);
  while (val == HIGH) {
    val = digitalRead(INPUT_BUTTON);

    hopper.run();
  }
  vibrateOff();

  hopper.setCurrentPosition(0);
  hopper.runToNewPosition(-200);
  hopper.disableOutputs();
  blinkColor(RED);

}

void openDoor() {
  doorServo.attach(DOOR_SERVO);

  float amount = DOOR_START_DP;

  for (float pos = DOOR_CLOSED; pos >= DOOR_OPENED; pos -= amount) {
    doorServo.write(pos);
    delay(DOOR_DELAY_STEP);
    amount += DOOR_ACCEL_AMOUNT;
  }
  doorServo.write(DOOR_OPENED);
  delay(200);

  doorServo.detach();
  isDoorOpen = true;
}

void closeDoor() {
  if (isBowlOut) {
    bowlIn();
  }
  doorServo.attach(DOOR_SERVO);

  float amount = DOOR_START_DP;

  for (float pos = DOOR_OPENED; pos <= DOOR_CLOSED; pos += amount) {
    doorServo.write(pos);
    delay(DOOR_DELAY_STEP);
    amount += DOOR_ACCEL_AMOUNT;
  }

  doorServo.write(DOOR_CLOSED);
  delay(200);

  doorServo.detach();
  isDoorOpen = false;
}

void setStepperOutSpeed() {
  stepper.setSpeed(STEPPER_SPEED_OUT);
  stepper.setAcceleration(STEPPER_ACCEL_OUT);
}

void setStepperInSpeed() {
  stepper.setSpeed(STEPPER_SPEED_IN);
  stepper.setAcceleration(STEPPER_ACCEL_IN);
}

void bowlOut() {
  if (!isDoorOpen) {
    openDoor();
  }
  stepper.enableOutputs();
  setStepperOutSpeed();
  stepper.runToNewPosition(OUT_POS);
  stepper.disableOutputs();
  isBowlOut = true;
}

void bowlIn() {
  stepper.enableOutputs();
  setStepperInSpeed();
  stepper.runToNewPosition(IN_POS);
  stepper.runToNewPosition(CENTERED_POS);
  stepper.disableOutputs();
  isBowlOut = false;
}

void vibrateOn() {
  digitalWrite(VIBRATE_RELAY, HIGH);
}

void vibrateOff() {
  digitalWrite(VIBRATE_RELAY, LOW);
}

void hopperAndMeasure(int targetWeight) {
  hopper.enableOutputs();
  hopper.setCurrentPosition(0);
  hopper.setSpeed(HOPPER_SPEED);
  hopper.moveTo(1000);

  scale.tare();
  delay(1000);

  vibrateOn();

  float weight = 0;
  float speedDelay = 1.2;
  bool done = false;
  int lightMapped = 0;


  while (!done) {
    digitalWrite(HOPPER_STEPPER_DIRECTION, HIGH);
    for (int i = 0; i < 300; i++) {
      digitalWrite(HOPPER_STEPPER_STEP, HIGH);
      delay(speedDelay);
      digitalWrite(HOPPER_STEPPER_STEP, LOW);
      delay(speedDelay);
    }
    // Back up a little to avoid jams
    digitalWrite(HOPPER_STEPPER_DIRECTION, LOW);
    for (int i = 0; i < 80; i++) {
      digitalWrite(HOPPER_STEPPER_STEP, HIGH);
      delay(1);
      digitalWrite(HOPPER_STEPPER_STEP, LOW);
      delay(1);
    }

    weight = round(scale.get_units(2) * -1);
    lightMapped = map(weight, 0, targetWeight, 0, 255);
    toColor(lightMapped, lightMapped, lightMapped);

    if (digitalRead(INPUT_BUTTON) == LOW || weight > targetWeight) {
      done = true;
    }
  }
  digitalWrite(HOPPER_STEPPER_DIRECTION, LOW);
  vibrateOff();
  lightsOff();

  hopper.setCurrentPosition(0);
  hopper.runToNewPosition(-250);
  hopper.disableOutputs();
}

void calibratePosition() {
  stepper.enableOutputs();
  delay(200);

  const int MAX_STEPS = 8000;
  const int STEP_PER_CYCLE = 10;
  digitalWrite(BOWL_STEPPER_DIRECTION, HIGH);
  int stepCount = 0;
  bool done = false;

  while (!done && stepCount < MAX_STEPS) {
    int sensor = digitalRead(TRAY_SWITCH);

    if (sensor == LOW) {
      stepper.setCurrentPosition(0);
      digitalWrite(BOWL_STEPPER_DIRECTION, LOW);
      done = true;
    } else {
      digitalWrite(BOWL_STEPPER_STEP, HIGH);
      delay(1);
      digitalWrite(BOWL_STEPPER_STEP, LOW);
      delay(1);
      stepCount++;
    }
  }
  stepper.disableOutputs();
}

void waitForMotion() {
  bool detected = false;
  while (!detected) {
    if (digitalRead(MOTION_SENSOR) == HIGH || digitalRead(INPUT_BUTTON) == LOW) {
      detected = true;
    }
    delay(100);
  }
}

void waitForLackOfMotion() {
  bool detected = true;
  const int retries = 5;
  const int delayMs = 2000;
  int curTry = 0;

  while (detected) {
    if (digitalRead(MOTION_SENSOR) == LOW) {
      curTry++;
      if (curTry >= retries) {
        detected = false;
      } else {
        delay(delayMs);
      }
    } else {
      //Start over if motion detected
      curTry = 0;
    }
    if (digitalRead(INPUT_BUTTON) == LOW) {
      detected = false;
    }
    delay(100);
  }
}

void feedRoutine(int amount) {
  calibratePosition();
  bowlIn();
  hopperAndMeasure(amount);
  toColor(250, 250, 250);
  openDoor();
  bowlOut();
  waitForMotion();
  fadeToColor(150, 150, 150);
  delay(1000);
  waitForLackOfMotion();
  fadeToColor(0, 0, 0);
  bowlIn();
  closeDoor();
}

void handleActionButton() {
  if (isBowlOut) {
    bowlIn();
    closeDoor();
  } else {
    openDoor();
    bowlOut();
  }
}

void loop() {

  delay(100);

  //Blink Red if the pi service has not checked in with the arduino
  if (millis() - piCheckInMillis > CHECK_IN_WARN_MS) {
    toColor(0, 0, 0);
    fadeToColor(255, 0, 0);
    fadeToColor(0, 0, 0);
    piCheckInMillis = millis();
  }
  
  if (digitalRead(INPUT_BUTTON) == LOW) {
    handleActionButton();
  }
  if (Serial.available() > 0) {
    command = Serial.readStringUntil('\n');
  }
  if (command.length() > 0) {

    Serial.print("Command: ");
    Serial.println(command);
    StringSplitter *splitter = new StringSplitter(command, ':', 5);
    String type = splitter->getItemAtIndex(0);
    int itemCount = splitter->getItemCount();
    //Serial.println("Item count: " + String(itemCount));

    if (type == "h") {
      //Hello from the Pi.
      fadeToColor(0, 0, 255);
      delay(1000);
      fadeToColor(0, 0, 0);
    }
    if (type == "c") {
      //Checkin from the pi
      piCheckInMillis = millis();
    }

    //---------
    // FEED
    //---------
    if (type == "f") {
      int amount = splitter->getItemAtIndex(1).toInt();
      Serial.print("Feed ");
      Serial.println(amount);

      feedRoutine(amount);
    }

    //---------
    // LIGHT
    //---------
    if (type == "l") {
      if (itemCount == 4) {
        int r = splitter->getItemAtIndex(1).toInt();
        int g = splitter->getItemAtIndex(2).toInt();
        int b = splitter->getItemAtIndex(3).toInt();

        fadeToColor(r, g, b);

      }
    }

    if (type == "m") {
      testMotionSensor();
    } else if (type == "b") {
      String val = splitter->getItemAtIndex(1);
      if (val == "1") {
        bowlOut();
      }
      else if (val == "0") {
        bowlIn();
      }
    }
    if (type == "d") {
      String val = splitter->getItemAtIndex(1);
      if (val == "1") {
        openDoor();
      } else if (val == "0") {
        closeDoor();
      }
    }
    command = "";

  }

}
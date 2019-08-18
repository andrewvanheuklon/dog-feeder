#include <Servo.h>
#include <AccelStepper.h>

const int TRAY_SWITCH = 45;
const int INPUT_BUTTON = 5;

const int VIBRATE_MOSFET = 4;
const int VIBRATE_MS = 2000;

const int BOWL_STEPPER_STEP = 36;
const int BOWL_STEPPER_DIRECTION = 37;
const int BOWL_STEPPER_SLEEP = 35;

const int HOPPER_STEPPER_STEP = 52;
const int HOPPER_STEPPER_DIRECTION = 53;
const int HOPPER_STEPPER_SLEEP = 51;
const int HOPPER_SPEED = 400;
// others

const int DOOR_SERVO = 6;
const int DOOR_OPENED = 0;
const int DOOR_CLOSED = 104;

const int STEPPER_SPEED_OUT = 15000;
const int STEPPER_ACCEL_OUT = 5000;

const int STEPPER_SPEED_IN = 3000;
const int STEPPER_ACCEL_IN = 2000;

const int OUT_POS = -6900;
const int IN_POS = -30;
const int CENTERED_POS = -300;

const int RED = 11;
const int BLUE = 10;
const int GREEN = 12;

const int MOTION_SENSOR = 17;

Servo doorServo;
boolean isDoorOpen = false;

AccelStepper stepper(AccelStepper::DRIVER, BOWL_STEPPER_STEP, BOWL_STEPPER_DIRECTION);
AccelStepper hopper(AccelStepper::DRIVER, HOPPER_STEPPER_STEP, HOPPER_STEPPER_DIRECTION);

int pos = 200;

void setup() {
  Serial.begin(9600);
  //BOWL STEPPER
  stepper.setMaxSpeed(8000);
  stepper.setAcceleration(3000);
  stepper.setEnablePin(BOWL_STEPPER_SLEEP);
  stepper.disableOutputs();

  hopper.setMaxSpeed(HOPPER_SPEED);
  hopper.setAcceleration(1000);
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

  //testHopper();

  //calibratePosition();
  //testColors();
  //testMotionSensor();
  //vibrate();
  //closeDoor();
  //delay(2000);
  openDoor();
  delay(3000);
  closeDoor();
  delay(3000);
  openDoor();
}

void testMotionSensor() {
  while(1) {
    int val = digitalRead(MOTION_SENSOR);
    if (val == HIGH) {
      colorOn(GREEN);
    } else {
      colorOn(RED);
    }
    delay(100);
  }
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
  analogWrite(color, 255);
}

void lightsOff() {
  analogWrite(RED, 0);
  analogWrite(BLUE, 0);
  analogWrite(GREEN, 0);
}

void vibrate() {
  digitalWrite(VIBRATE_MOSFET, HIGH);
  delay(VIBRATE_MS);
  digitalWrite(VIBRATE_MOSFET, LOW);
}

void testHopper() {
  colorOn(GREEN);
  hopper.enableOutputs();
  hopper.setSpeed(HOPPER_SPEED);

  int val = digitalRead(INPUT_BUTTON);
  while (val == HIGH) {
    val = digitalRead(INPUT_BUTTON);

    hopper.runSpeed();
  }
 
  hopper.setCurrentPosition(0);
  hopper.runToNewPosition(-200);
  hopper.disableOutputs();
  blinkColor(RED);
  
}

void openDoor() {
  doorServo.attach(DOOR_SERVO);
  doorServo.write(DOOR_OPENED);
  delay(700);
  doorServo.detach();
  isDoorOpen = true;
}

void closeDoor() {
  doorServo.attach(DOOR_SERVO);
  doorServo.write(DOOR_CLOSED);
  delay(700);
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
  stepper.enableOutputs();
  setStepperOutSpeed();
  stepper.runToNewPosition(OUT_POS);
  stepper.disableOutputs();
}

void bowlIn() {
  stepper.enableOutputs();
  setStepperInSpeed();
  stepper.runToNewPosition(IN_POS);
  stepper.runToNewPosition(CENTERED_POS);
  stepper.disableOutputs();
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

void loop() {
  // put your main code here, to run repeatedly:
  //Serial.println(digitalRead(MOTION_SENSOR));
  //delay(100);
}
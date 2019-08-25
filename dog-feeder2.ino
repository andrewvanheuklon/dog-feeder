
#include "HX711.h";
#include <Servo.h>
#include <math.h>

#define DOUT  11
#define CLK  10

Servo hopperMotor;

HX711 scale(DOUT, CLK);

float calibration_factor = -450;
float dispenseWeight = 0.0;

const int recordCount = 20;
float fillWeight = 0.0;
float lastReadings[recordCount];
int lastReadingsIndex = 0;
float totalReadCount = 0;
int diffThreashold = 4; //If all last readings are within this number of eachother, we are empty


int stopMs = 1455;
int fullSpeedMs = 1900;
int halfSpeedMs = 1600;
int backUpSpeedMs = 1290;

int slowSpeedOffMs = 2000;
int slowSpeedOnMs = 500;

float totalFoodGrams = 0.0;

int state = 0;
String command = "";
boolean isSlowFeed = false;
boolean hasLidBeenOpened = false;
boolean isLidOpen = false;

int prevLidValue = HIGH;

void setup() {
  Serial.begin(9600);

  pinMode(8, INPUT_PULLUP);
  prevLidValue = digitalRead(8);
  isLidOpen = prevLidValue == LOW;
  Serial.print("Is lid open: ");
  Serial.println(isLidOpen ? "true" : "false");
  
  //Set all the readings to -1
  resetLastReadings();
  
  pinMode(6, OUTPUT);

  scale.set_scale();
  scale.tare(); //Reset the scale to 0

  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);
  scale.set_scale(calibration_factor); //Adjust to this calibration factor

  hopperMotor.attach(9);
  waitForCommands();

  scale.tare();
 
}

//******************************
// . INTIATE FOOD DISPENSING
//******************************
void initDispenseFood(int grams, boolean slow) {
  dispenseWeight = grams;

  isSlowFeed = slow;
  Serial.print("Food dispensing.. ");
  scale.tare();
  Serial.print("Scale tared..");
  delay(200);
  resetLastReadings();
  
  hopperMotor.writeMicroseconds(fullSpeedMs);
  Serial.print("Full speed set");
  Serial.println();
  Serial.print("UPDATE:dispensingNowTargetGrams:");
  Serial.println(grams);
  
  Serial.println("UPDATE:state:1");
  state = 1;
}

//******************************
// . LOW FOOD WARNING
//******************************
void lowFoodWarning() {
  tone(6, 2000);
  delay(1000);
  noTone(6);
}

//******************************
// . STOP AND WAIT FOR COMMANDS
//******************************
void waitForCommands() {
  Serial.println("Waiting for commands");
  hopperMotor.writeMicroseconds(stopMs);
  Serial.println("UPDATE:state:0");
  state = 0;
}


//******************************
// . FILLING FOOD TRANSITION
//******************************
void transitionToFillingFoodState() {
  Serial.println("UPDATE:state:2");
  state = 2;
  Serial.println("Listening for food filling");

  fillWeight = 0;
  scale.tare();
  delay(200);
}


//*******************************
//    WARNING IF LOW FOOD
//********************************
void warnIfLow() {

  if (totalFoodGrams < 140.0) {
     Serial.println("Below two feedings! Warning!");
     lowFoodWarning();
  }
  if (totalFoodGrams < 64) {
    Serial.println("MIGHT NOT BE ENOUGH FOOD FOR NEXT FEEDING!!");
    lowFoodWarning();
  }
}

//*******************************
//    RESET LAST READINGS
//********************************
void resetLastReadings() {
  Serial.println("Reseting last readings");
  for (int i = 0; i <  recordCount; i++) {
   lastReadings[i] = -1; 
  }
  lastReadingsIndex = 0;
  totalReadCount = 0;
}

//******************************
//------------------------------
//
// . L O O P
//
//------------------------------
//******************************
void loop() {
  
//   Serial.print("Loop state ");
//   Serial.print(state);
//   Serial.println();
  //------------------------
  // WAITING FOR COMMANDS
  //------------------------
  int lidSensor = digitalRead(8);
  
  if (prevLidValue == HIGH && lidSensor == LOW) {
    Serial.println("Lid opened");
    Serial.println("UPDATE:isLidOpen:1");
    isLidOpen = true;
    
    hasLidBeenOpened = true;

    if (state == 0) {
        //We are just waiting for commands, so transition to filling state
        transitionToFillingFoodState();
    }

  }
  if (prevLidValue == LOW && lidSensor == HIGH) {
    Serial.println("Lid closed");
    Serial.println("UPDATE:isLidOpen:0");
    isLidOpen = false;
    
    if (hasLidBeenOpened) {
      //Set the total again, wait for commands. Need this incase lid was open when turned on
      Serial.print("Add weight ");
      Serial.print(fillWeight);
      Serial.print(" grams to the total: ");
      Serial.println(totalFoodGrams);
      //Add the fill weight to the total
      if (abs(fillWeight) < 10) {
         Serial.println("Not enough weight change. Not changing total weight"); 
      } else {
        totalFoodGrams = totalFoodGrams + fillWeight;
        Serial.print("UPDATE:lastFillAmount:");
        Serial.println(fillWeight);
      }
      Serial.println(totalFoodGrams);
      Serial.print("UPDATE:totalWeight:");
      Serial.println(totalFoodGrams);
      
      waitForCommands();
    }
  }
  prevLidValue = lidSensor;
  
  if (state == 0) {
    delay(100);
    if (Serial.available() > 0) {
      command = Serial.readStringUntil('\n');
    }
    if (command.length() > 0) {
      String commandChar = command.substring(0, 1);
      //Get the command


      Serial.println(commandChar);

      //--------------------
      // .  DISPENSE
      //--------------------
      if (commandChar == "d") {
        Serial.println("Dispense command");
        String amount = command.substring(1);
        Serial.print("Amount: ");
        Serial.print(amount);
        Serial.println();

        initDispenseFood(amount.toInt(), false);
      }
      //--------------------
      // .  SLOW FEED
      //--------------------
      if (commandChar == "f") {
        Serial.println("Slowfeed command");
        String amount = command.substring(1);
        Serial.print("Amount: ");
        Serial.print(amount);
        Serial.println();

        initDispenseFood(amount.toInt(), true);
      }
      //--------------------
      // .  REPORT
      //--------------------
      else if (commandChar == "r") {
        Serial.println("REPORT");
      }
      //--------------------
      // .  STOP
      //--------------------
      else if (commandChar == "s") {
        Serial.println("STOP recieved!");
        waitForCommands();
      //--------------------
      // .  SET STOP MS
      //--------------------  
      } else if (commandChar == "t") {
        Serial.println("Changing stopMs");
        String val = command.substring(1);
        Serial.print("New value: ");
        Serial.print(val);
        Serial.println();
        stopMs = val.toInt();
        waitForCommands();
      
      //--------------------
      // .  SET HALF SPEED MS
      //--------------------  
      } else if (commandChar == "h") {
        Serial.println("Changing halfSpeed");
        String val = command.substring(1);
        Serial.print("New value: ");
        Serial.print(val);
        Serial.println();
        halfSpeedMs = val.toInt();
        waitForCommands();
      
        //--------------------
      // .  SETSLOW SPEED OFF TIME
      //--------------------  
      } else if (commandChar == "l") {
        Serial.println("Changing Slow speed OFF time (ms)");
        String val = command.substring(1);
        Serial.print("New value: ");
        Serial.print(val);
        Serial.println();
        slowSpeedOffMs = val.toInt();
        waitForCommands();
      
      //--------------------
      // .  SET SLOW SPEED ON TIME
      //--------------------  
      } else if (commandChar == "o") {
        Serial.println("Changing Slow speed ON time (ms)");
        String val = command.substring(1);
        Serial.print("New value: ");
        Serial.print(val);
        Serial.println();
        slowSpeedOnMs = val.toInt();
        waitForCommands();
      //--------------------
      //    INIT WEIGHT
      //--------------------
      } else if (commandChar == "i") {
        Serial.println("Changing initial food grams");
        String val = command.substring(1);
        Serial.print("New value: ");
        Serial.print(val);
        Serial.println();
        totalFoodGrams = val.toInt();
        Serial.print("UPDATE:totalWeight:");
        Serial.println(totalFoodGrams);
        waitForCommands();
  
      //--------------------
      //    WARN IF LOW
      //--------------------
      } else if (commandChar == "w") {
        Serial.println("Warn if low");
        warnIfLow();
        
        waitForCommands();
      }
      

      command = "";
    } else {
      //Just idle for 1/2 sec
      delay(100);
    }

    //-------------------------
    //DISPENSING FOOD
    //-------------------------
  } else if (state == 1) {
    Serial.print("State 1: ");
    scale.set_scale(calibration_factor); //Adjust to this calibration factor
    Serial.print(" Calibration set - ");
    
    
    
    if (isSlowFeed) {
      Serial.print("  : Slow feeding");
      Serial.println();
      hopperMotor.writeMicroseconds(stopMs);
      Serial.println("Stop ms set for delay..");
      delay(slowSpeedOffMs);
      Serial.println("Before full speed");
      hopperMotor.writeMicroseconds(fullSpeedMs);
      Serial.print("After Full speeds");
      delay(slowSpeedOnMs);
      Serial.print("... continue with loop");
      Serial.println();
    }

    float weight = round(scale.get_units());
    Serial.print(weight);

    Serial.println("Checking if done..");
    Serial.print("Weight: ");
    Serial.println(weight);
    Serial.println("  compared to: ");
    Serial.println(dispenseWeight * -1);
    
    Serial.print("UPDATE:dispensingNowGrams:");
    Serial.println((weight * -1));
    
    float tempTotal = totalFoodGrams + weight;
    Serial.print("temp total: ");
    Serial.println(tempTotal);

    boolean isDone = weight <= dispenseWeight * -1;
    boolean isEmpty = false;
    
    lastReadings[lastReadingsIndex] = weight;
    
    lastReadingsIndex = lastReadingsIndex == recordCount - 1 ? 0 : lastReadingsIndex + 1;
    totalReadCount++;
    
    for (int i = 0; i < recordCount; i++) {
        //Print all the values
        if (i > 0) {
          Serial.print(", ");
        }
        if (i == lastReadingsIndex) {
          Serial.print("@");
        }
        Serial.print(lastReadings[i]);
    }
    Serial.println();
    Serial.print("Total readings: ");
    Serial.println(totalReadCount);
    
    if (totalReadCount > 20) {
      
      isEmpty = false;
      float lowest = 10000;
      float highest = 0;
      float spread = -1.0;
      
      Serial.println("Total read count is hit. Checking if empty");
      for(int i = 0; i < recordCount; i++) {

        float w = abs(weight);
        float r = abs(lastReadings[i]);
        if (r < lowest) {
         lowest = r; 
        }
        if (r > highest) {
          highest = r;
        }
      }
        spread = highest - lowest;
        Serial.print("Spread: ");
        Serial.print("high: ");
        Serial.print(highest);
        Serial.print("  low: ");
        Serial.print(lowest);
        Serial.print(" diff: ");
        Serial.println(spread);
        boolean lowSpread = spread < 4;
        
         if ( lowSpread ) {
           //empty
           Serial.println("Spread less than 4, setting empty to true");
           isEmpty = true;
         }
      
      
      
      isDone = isDone || isEmpty;
    }
    
    Serial.print("   is DONE: ");
    Serial.print(isDone);
    Serial.println();
    
    if (isDone) {
      if (isEmpty) {
        Serial.println("Stopping because we are empty. Setting weight to 0");
        
        totalFoodGrams = 0.0;
        
      }
      Serial.println("!! Dispense weight reached!");
      resetLastReadings();
      
      hopperMotor.writeMicroseconds(backUpSpeedMs);
      Serial.println("Wrote microseconds");
      Serial.println(backUpSpeedMs);
      delay(500);
      hopperMotor.writeMicroseconds(stopMs);
      Serial.println("Wrote microseconds");
      Serial.println(stopMs);
      delay(2000);
      Serial.println("Final weight despensed: ");
      delay(1000);
      //Get new weight
      weight = round(scale.get_units());
      Serial.print("UPDATE:dispensed:");
      Serial.println((weight * -1));
      
      Serial.print(weight);
      Serial.println();
      Serial.print("Old total food: ");
      Serial.print(totalFoodGrams);
      
      totalFoodGrams = totalFoodGrams + weight;
      Serial.print("New calculated total: ");
      Serial.println(totalFoodGrams);
      
      if (totalFoodGrams < 0) {
         Serial.println("Less than 0, setting to 0");
         totalFoodGrams = 0; 
      }
      Serial.println();

      Serial.print("New total: ");
      Serial.println(totalFoodGrams);
      
      Serial.print("UPDATE:totalWeight:");
      Serial.println(totalFoodGrams);

      // CHECK FOR LOW FOOD ------
      
      // --------------------------
      waitForCommands();
    } else {
      Serial.println("Weight not reached");
    }
    Serial.println("end state 1");

    //-------------------------
    //FILLING FOOD
    //-------------------------
  } else if (state == 2) {
    scale.set_scale(calibration_factor); //Adjust to this calibration factor
    
    fillWeight = round(scale.get_units());
    
    Serial.print("UPDATE:fillingNowGrams:");
    Serial.println(fillWeight);
    //Serial.print("Filling food .. ");
   // Serial.println(fillWeight);
    
  }

}


/*
 * NightShift
 * Electromechanical bicycle rear derailleur project
 *
 * Notes:
 * Servo & Arduino need to be in same GND connection
 * Does not work without attach/detach
 * With this sketch, hall sensor TLE4905L is normally closed and reports ON if no magnet is present
 */

#include <Servo.h>
#include "ButtonState.h"

/* Servo position measured min / max = 177 / 368 @ 180deg
 * Servo position min / max map adjusted = 177 / 365 @ 180deg
 *
 * Servo position measured min / max = 58 / 207 @ derailleur min/max range
 */
#define SERVO_POSITION_PIN A7     // servo position feedback pin = wiper

/*
 * P-Fet gate, pull low to switch fet/servo on
 * This is also needed when reading servo's internal potentiometer 
 * to put +5v to servo pot left side, other side to GND while reading the wiper pin
 */
#define SERVO_POWER_TOGGLE_PIN 5  // fet gate pin

/*
 * Arduino internal led
 */
#define SERVO_POWER_TOGGLE_LED 13 // duplicate fet gate signal to led

/*
 * Servo control line (50Hz pulse, 1-2ms)
 */
#define SERVO_PIN 6

/*
 * Hall sensor pins for detecting chain rotation & direction
 */
#define HALL_1_PIN 2              // Hall-sensor no. 1 (interrupt pin INT0)
#define HALL_2_PIN 7              // Hall-sensor no. 2

/**
 * Control buttons pin, built with resistor tree
 */
#define BUTTON_ANALOG_INPUT A6


Servo testServo;
int servoPos;       // servo initial position
int movement = 5;  // amount to move
boolean increase = true; // servo position test variable to indicate direction
int servo_low_limit = 58;  // servo/derailleur physical LOW limit, potentiometer raw value
int servo_high_limit = 314; // servo/derailleur physical HIGH limit, potentiometer raw value
int servo_low_limit_angle, servo_high_limit_angle; // servo/derailleur limit angles
int gearcount = 7;  // gears in bike

volatile byte clockwise = false; // direction info from hall sensors

int buttonValue = 0;  // button value, read from buttons
int noButtonsPressedTreshold = 400; // treshold value, under this we're sure no buttons are pressed

ButtonState buttonState, lastButtonState = NO_BUTTONS_PRESSED;

/*
 * Timer variables for various events, to avoid usage of delay()
 */ 
unsigned long timerInterval, timerServoOn, timerGeneralDelay, timerHallSensor;
boolean hallInterruptFired = false;

/** 
 * Method definitions
 */
boolean millisDelay(unsigned long timeToWaitMS);
void blink(int pin, int count);
void servoOn();
void servoOff();
void gearUp();
void gearDown();
int servoRawToDegrees(int measurement);
int servoRawToGear(int measurement, int gears);
void reportHall();
boolean millisDelay(unsigned long timeToWaitMS);
ButtonState readButtons();
void buttonInputTest(ButtonState state);


/**
 * Method to blink led
 */
void blink(int pin, int count) {
  for(int i = 0; i<count; i++) {
  pinMode(pin, OUTPUT); // set pin to output
  digitalWrite(pin, HIGH); // pull led pin high
  delay(200);
  digitalWrite(pin, LOW); // pull led pin low
  delay(200);
  }
}

/*
 * Method to turn servo p-fet on and attach servo
 */
void servoOn() {
  digitalWrite(SERVO_POWER_TOGGLE_PIN, LOW);  // turn servo on (pull p-fet gate to gnd)
  digitalWrite(SERVO_POWER_TOGGLE_LED, HIGH);  // turn servo indicator led on  
  Serial.println("Servo power turned ON");
  pinMode(SERVO_PIN, OUTPUT);                 // not necessary, as servo.attach does this too
  testServo.attach(SERVO_PIN, 1000, 2000);    // setup servo with pwm range of 1000-2000 µs (default servo lib values = 544-2400 µs)
  //if (testServo.attached()) {Serial.println("Servo attached"); } else { Serial.println("Servo detached"); }
}

/*
 * Method to turn servo p-fet off and detach servo
 */
void servoOff() {
  testServo.detach();
  pinMode(SERVO_PIN, INPUT);
  //if (testServo.attached()) {Serial.println("Servo attached"); } else { Serial.println("Servo detached"); }
  digitalWrite(SERVO_POWER_TOGGLE_PIN, HIGH); // turn servo power off
  digitalWrite(SERVO_POWER_TOGGLE_LED, LOW); // turn servo led off
  Serial.println("Servo power turned OFF");
}


/**
 * Method to shift gear upwards
 */ 
void gearUp() {

  servoOn();
  
  if (servoPos >= servo_high_limit_angle ) {
    Serial.println("Already at highest gear!");
  } else {
    servoPos = servoPos + 10;
  }

  Serial.print("Moving servo to position: ");
  Serial.println(servoPos);
  
  testServo.write(servoPos);  // write value to servo
  delay(200);
//  millisDelay(600UL); // 600ms wait for servo to reach position

  servoOff();  
 
}

/**
 * Method to shift gear downwards
 */ 
void gearDown() {
  servoOn();
  if (servoPos <= servo_low_limit_angle) {
    Serial.println("Already at highest gear!"); 
  } else {
    servoPos = servoPos - 10;
  }

  Serial.print("Moving servo to position: ");
  Serial.println(servoPos);
  
  testServo.write(servoPos);  // write value to servo    
  delay(200);
//  millisDelay(600UL); // 600ms wait for servo to reach position
    
  servoOff();  
}

/** 
 * Function to map servo potentiometer data to degrees
 * assumption that servo rotates 180deg, min value = 177 & max value = 365
 * assumption that servo rotates 180deg, min value = 56 & max value = 439
 * Note! Derailleur range min value = 58 & max value = 207
 */
int servoRawToDegrees(int measurement) {
  return map(measurement, 180, 320, 0, 180); // map servo position raw data from 177-365 => 0-180
}

/** 
 * Function to map servo potentiometer data to degrees
 * measured when connected with derailleur, min value = 58 & max value = 207
 * gears = no. of gears in bike
 */
int servoRawToGear(int measurement, int gears) {
  return map(measurement, 58, 207, gears, 1); // map servo position raw data from 177-365 => gearcount - gear 1 (derailleur moves in "negative")
}

/**
 * Helper method to read & print hall sensor values
 */
void reportHall() {
  if (digitalRead(HALL_1_PIN)) {Serial.println(" - HALL 1 OFF"); } else { Serial.println(" - HALL 1 ON"); }
  if (digitalRead(HALL_2_PIN)) {Serial.println(" - HALL 2 OFF"); } else { Serial.println(" - HALL 2 ON"); }
   
  if (clockwise) {Serial.println(" - HALL rotation clockwise"); } else { Serial.println(" - HALL rotation counter-clockwise"); }
}

/**
 * Delay function
 * parameter in = 400UL, for example creates a 400ms delay
 */
boolean millisDelay(unsigned long timeToWaitMS) {
  timerGeneralDelay = millis();  
  while (!((millis() - timerServoOn) >= timeToWaitMS)) {
    // timeToWaitMS wait loop for servo to reach position
  }
}

/** 
 * Method to read & determine what buttons user has pressed, if any
 */
ButtonState readButtons() {
  buttonValue = analogRead(BUTTON_ANALOG_INPUT);    // read the input pin
  // Serial.print("button value = "); Serial.println(buttonValue);
  int tmpVal = buttonValue;
  if (buttonValue <= noButtonsPressedTreshold) {
    return NO_BUTTONS_PRESSED;    
  } 
  else {
    
    if (buttonValue >= 580 && buttonValue <= 620) {
      //Serial.print("Possible button UP, value = "); Serial.println(val);
      
      millisDelay(20);
      
      tmpVal = analogRead(BUTTON_ANALOG_INPUT);
      //Serial.print("Possible button UP, 2nd value = "); Serial.println(tmpVal);
      if (tmpVal < buttonValue+5 && tmpVal > buttonValue-5 ) { // take another measurement to see if value is rising or falling fast enough to skip
        buttonValue = tmpVal;
        return BTN_UP_PRESSED;
      }
    }
    
    if (buttonValue >= 680 && buttonValue <= 725) {
      //Serial.print("Possible button DOWN, value = "); Serial.println(val);
      
      millisDelay(20);
      
      tmpVal = analogRead(BUTTON_ANALOG_INPUT);
      //Serial.print("Possible button DOWN, 2nd value = "); Serial.println(tmpVal);
      if (tmpVal < buttonValue+5 && tmpVal > buttonValue-5) { // take another measurement to see if value is still rising
        buttonValue = tmpVal;
        return BTN_DN_PRESSED;
      }
    }  
    
    if (buttonValue >= 770) { // && buttonValue <= 830) {
      return BOTH_BUTTONS_PRESSED;
    }
    //  Serial.println(val);             // debug value
  }
}

/**
 * Button state machine
 * Possible states are listed in ButtonState.h
 * States are BTN_DN_PRESSED | BTN_UP_PRESSED | BOTH_BUTTONS_PRESSED | NO_BUTTONS_PRESSED
 * Valid state transfers:
 *   NO_BUTTONS_PRESSED
 *      -> BTN_DN_PRESSED
 *      -> BTN_UP_PRESSED
 *      -> BOTH_BUTTONS_PRESSED
 *   BTN_DN_PRESSED
 *      -> BOTH_BUTTONS_PRESSED
 *      -> NO_BUTTONS_PRESSED
 *   BTN_UP_PRESSED
 *      -> BOTH_BUTTONS_PRESSED
 *      -> NO_BUTTONS_PRESSED
 *  
 */
void buttonInputTest(ButtonState state) {

  switch (state) {
  case BTN_DN_PRESSED:
    if (state != lastButtonState) {
      switch (lastButtonState) {
        case BOTH_BUTTONS_PRESSED:
          Serial.print("Button UP released, Button DOWN still pressed (value="); 
          break;
        case NO_BUTTONS_PRESSED:
          Serial.print("Button DOWN pressed (value=");          
          break;
      }  
      Serial.print(buttonValue); 
      Serial.println(")");
      increase = true;
    }
    break;
  case BTN_UP_PRESSED:
    if (state != lastButtonState) {
      switch (lastButtonState) {
        case BOTH_BUTTONS_PRESSED:
          Serial.print("Button DOWN released, Button UP still pressed (value="); 
          break;
        case NO_BUTTONS_PRESSED:          
          Serial.print("Button UP pressed (value=");           
          break;
      }
      Serial.print(buttonValue); 
      Serial.println(")");
      increase = false;
    }
    break;
  case BOTH_BUTTONS_PRESSED:
    if (state != lastButtonState) {
      switch (lastButtonState) {
        case BTN_DN_PRESSED:
          Serial.print("BOTH buttons pressed, modifier button is DOWN (value="); 
          break;
        case BTN_UP_PRESSED:
          Serial.print("BOTH buttons pressed, modifier button is UP (value="); 
          break;
        default:
          Serial.print("BOTH buttons pressed (value=");
          break;          
      }
      Serial.print(buttonValue); 
      Serial.println(")");
      break;
    }
  default:
  case NO_BUTTONS_PRESSED:
    if (state != lastButtonState) {
      switch (lastButtonState) {
        case BTN_DN_PRESSED:
          Serial.print("Button DOWN released (value="); 
          
          gearDown();
          
          break;
        case BTN_UP_PRESSED:
          Serial.print("Button UP released (value="); 
          
          gearUp();
          
          break;
        case BOTH_BUTTONS_PRESSED:
          Serial.print("Both buttons released (value="); 
          break;
      }
      Serial.print(buttonValue); 
      Serial.println(")");
    }
    break;
  }
}

/**
 * Arduino setup method
 * Variable / port initialization
 */
void setup() {
  
  Serial.begin(9600);
      
  blink(SERVO_POWER_TOGGLE_LED, 1);          // blink led 1 times
  
  pinMode(HALL_1_PIN, INPUT);                // set Hall sensor no. 1 pin to input
  pinMode(HALL_2_PIN, INPUT);                // set Hall sensor no. 2 pin to input
  
  // Turn pullup resistors ON => HALL sensors report ON if no magnet is present
  digitalWrite(HALL_1_PIN, HIGH);
  digitalWrite(HALL_2_PIN, HIGH);
  
  // Attach interrupt (INT0) to HALL_1_PIN
  attachInterrupt(0, checkRotation, FALLING);
  Serial.println("Hall sensor setup done");
  
  blink(SERVO_POWER_TOGGLE_LED, 2);          // blink led 2 times
  
  pinMode(SERVO_POWER_TOGGLE_PIN, OUTPUT);   // set FET pin to output
  pinMode(SERVO_POWER_TOGGLE_LED, OUTPUT);   // set led pin to output
  digitalWrite(SERVO_POWER_TOGGLE_PIN, LOW);  // turn servo on (pull p-fet gate to gnd) = push +5v to pot
  pinMode(SERVO_POSITION_PIN, INPUT);      // set servo position pin to input
  testServo.attach(SERVO_PIN, 1000, 2000);    // setup servo with pwm range of 1000-2000 µs (default servo lib values = 544-2400 µs)
  delay(20);
  
  int raw = analogRead(SERVO_POSITION_PIN);
  servoPos = servoRawToDegrees(raw);    // read servo initial position !! TODO: needs adjustment
  servoPos = 120; // just set it to some value to get a working system...
  
  Serial.print("Servo pin setup done, initial position = ");
  Serial.print(servoPos);
  Serial.print(", raw value from servo = ");
  Serial.println(raw);
  
  blink(SERVO_POWER_TOGGLE_LED, 3);          // blink led 3 times
  
  servo_low_limit_angle = servoRawToDegrees(servo_low_limit);
  servo_high_limit_angle = servoRawToDegrees(servo_high_limit);
  
  Serial.print("Servo low limit (58) to degrees = ");
  Serial.println(servo_low_limit_angle);
  Serial.print("Servo high limit (207) to degrees = ");
  Serial.println(servo_high_limit_angle);
  
  timerInterval = millis();
  Serial.println("Servo power toggle test");
}

/**
 * Arduino main loop
 */
void loop() {
  
  buttonState = readButtons();
  
  if (hallInterruptFired) {
    timerHallSensor = millis(); // reset timer
    hallInterruptFired = false;
  }  
  
  buttonInputTest(buttonState);  
  
  lastButtonState = buttonState; // save last known button state
  
  if ((millis() - timerInterval) >= 300UL) { // 0.3sec delay without delay() function   
    timerInterval = millis(); // reset timer
  }
}

/**
 * HW Interrupt subroutine for INT0 (HALL_1_PIN), attached to FALLING edge
 * Reads hall sensor no. 2 and determines the direction magnet is travelling
 *  - if HALL2 is high, clockwise
 *  - otherwise the direction is counter-clockwise
 */
void checkRotation() {
  
  if (digitalRead(HALL_2_PIN)) {
    clockwise = true;    
  }
  else {
    clockwise = false;
  }
  hallInterruptFired = true;
}

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "Derailleur.h"
#include "Servo.h" // ugly forced include in linker settings
#include "NightShift_Util.h"


#define PIN_SERVO_PWM 6
#define PIN_SERVO_POWER_TOGGLE 5
#define PIN_SERVO_POWER_LED 13
#define PIN_SERVO_WIPER_POSITION A7

Servo _servo;
int _servoPos;       // servo initial position
int servo_low_limit = 58;  // servo/derailleur physical LOW limit, potentiometer raw value
int servo_high_limit = 314; // servo/derailleur physical HIGH limit, potentiometer raw value
int servo_low_limit_angle, servo_high_limit_angle; // servo/derailleur limit angles

Derailleur::Derailleur()
	: _derailleur_gearCount(7)
	, _derailleur_currentGear(3) // start from middle
{
	// todo: init servo	
}

//void Derailleur::attach(int servoPowerPin) {	
//}

void Derailleur::setGearCount(int gears) {
	this->_derailleur_gearCount = gears;
}

int Derailleur::getGearCount() {
	return this->_derailleur_gearCount;
}

int Derailleur::getCurrentGear() {
	return this->_derailleur_currentGear;
}

/**
* Method to set up servo
*/
bool Derailleur::initServo(bool hasFeedbackLoop, bool cutPowerWhenIdle) {
	bool servoFeedBack = hasFeedbackLoop;
	bool servoPowerCut = cutPowerWhenIdle;

	pinMode(PIN_SERVO_POWER_TOGGLE, OUTPUT);   // set FET pin to output
	pinMode(PIN_SERVO_POWER_LED, OUTPUT);   // set led pin to output
	digitalWrite(PIN_SERVO_POWER_TOGGLE, LOW);  // turn servo on (pull p-fet gate to gnd) = push +5v to pot
	pinMode(PIN_SERVO_WIPER_POSITION, INPUT);      // set servo position pin to input
	_servo.attach(PIN_SERVO_PWM, 1000, 2000);    // setup servo with pwm range of 1000-2000 �s (default servo lib values = 544-2400 �s)
	delay(20);

	int raw = analogRead(PIN_SERVO_WIPER_POSITION);
	_servoPos = this->servoRawToDegrees(raw);    // read servo initial position !! TODO: needs adjustment, maybe do some averaging
	_servoPos = 120; // just set it to some value to get a working system...

	Serial.print("Servo pin setup done, initial position = ");
	Serial.print(_servoPos);
	Serial.print(", raw value from servo = ");
	Serial.println(raw);

	NightShift_Util::blink(PIN_SERVO_POWER_LED, 3);          // blink led 3 times

	servo_low_limit_angle = servoRawToDegrees(servo_low_limit);
	servo_high_limit_angle = servoRawToDegrees(servo_high_limit);

	Serial.print("Servo low limit (58) to degrees = ");
	Serial.println(servo_low_limit_angle);
	Serial.print("Servo high limit (207) to degrees = ");
	Serial.println(servo_high_limit_angle);

	//timerInterval = millis();
	Serial.println("Servo power toggle test");

	return true;
}

/*
* Method to change gear up by requested amount
*/
void Derailleur::gearUp(int count) {
	// current = 3|3, max = 8|8, count = 2|6
	// maxAllowedUp = max - current = 5|5
	// target = current + count = 5|9
	// Serial.print("gearUp() - count: ");
	// Serial.print(count);

	int maxAllowedUp = _derailleur_gearCount - _derailleur_currentGear;
	int target = _derailleur_currentGear + count;
	// Serial.print(" maxUp: ");
	// Serial.print(maxAllowedUp);
	// Serial.print(" targetGear: ");

	target = min(target, _derailleur_gearCount);
	// Serial.println(target);

	count = min(count, maxAllowedUp);
	// Serial.print("Adjusted count: ");
	// Serial.println(count);

	for (int i = 0; i < count; i++)
	{
		_derailleur_currentGear++;
		NightShift_Util::sendSerialCurrentGear(_derailleur_currentGear);
	}
}

/*
* Method to change gear down by requested amount
*/
void Derailleur::gearDown(int count) {
	// current = 3|3|1, max = 8|8|8, count = 2|5|1
	// maxAllowedDown = current - 1 = 2|2|0
	// target = max(current - count, 1) = 1|1|0
	// Serial.print("gearDown() - count: ");
	// Serial.print(count);

	int maxAllowedDown = _derailleur_currentGear - 1;
	int target = _derailleur_currentGear - count;

	// Serial.print(" maxDown: ");
	// Serial.print(maxAllowedDown);
	// Serial.print(" targetGear: ");

	target = max(target, 1);
	// Serial.println(target);

	if (count > maxAllowedDown) {
		count = maxAllowedDown;
	}

	// Serial.print("Adjusted count: ");
	// Serial.println(count);

	for (int i = 0; i < count; i++)
	{
		_derailleur_currentGear--;
		NightShift_Util::sendSerialCurrentGear(_derailleur_currentGear);
	}

}

/*
* Method to change gear to target
*/
void Derailleur::gearTo(int targetGear) {
	// current= 3|6, max=8|8, target = 2|9
	// gears to shift (down) = current - target = 1|-3
	// gears to shift (up) = target - current = -1|3
	// maxAllowedDown = current - 1 (first gear) = 2|5
	// maxAllowedUp = max - current = 5|2
	// Serial.print("gearTo() - target:");
	// Serial.print(targetGear);
	// Serial.print(" current:");
	// Serial.println(_derailleur_currentGear);

	int maxAllowedDown = _derailleur_currentGear - 1;
	int maxAllowedUp = _derailleur_gearCount - _derailleur_currentGear;

	// up
	if (targetGear > _derailleur_currentGear) {
		// Serial.print("going UP ");
		int upShifts = targetGear - _derailleur_currentGear;
		upShifts = min(upShifts, maxAllowedUp);

		// Serial.print(upShifts);
		// Serial.println(" times");

		gearUp(upShifts);
	}

	// down
	else if (targetGear < _derailleur_currentGear) {
		// Serial.print("going DOWN ");
		int downShifts = _derailleur_currentGear - targetGear;
		downShifts = min(downShifts, maxAllowedDown);

		// Serial.print(downShifts);
		// Serial.println(" times");

		gearDown(downShifts);
	}
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

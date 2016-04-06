
/*
* NightShift v3
* Electromechanical bicycle rear derailleur project
*/

#include "NightShift_Util.h"
#include "Derailleur.h"
#include <Bounce2.h>
#include <WString.h>

#define SOFTWARE_VERSION 3

#define PIN_GEARUP 2
#define PIN_GEARDOWN 3

#define DEBOUNCE_MS 5

#define SERIAL_BAUDRATE 9600
#define SERIAL_DATA_SIZE 50

byte buttonPins[] = { PIN_GEARUP, PIN_GEARDOWN };
#define NUMBUTTONS sizeof(buttonPins)

Derailleur derailleur;
int _gearsToSwitch = 1; // how many gears to switch

unsigned long _modeChangeTimer;
unsigned long _modeChangeInterval = 4000L; // 4 sec
bool _queueModeChange = false;
bool _modeJustChanged = false;

enum OperationMode {
	MODE_DRIVE,
	MODE_ADJUST,
	MODE_SERIALSETUP
};

OperationMode _operationMode = MODE_DRIVE;

Bounce btn_up;
Bounce btn_down;

String _serialData = "";
bool _serialStringComplete = false;

void setup()
{	
	if (initSerial()) {
		Serial.print("NightShift v");
		Serial.println(SOFTWARE_VERSION);
	}

	if (initButtons()) {
		Serial.println("Button init ready");
	}

	if (initDerailleur()) {
		Serial.println("Derailleur init ready");
	}
	printSerialCommands();
}

/**
 * Method to init serial connection
 */
bool initSerial() {
	Serial.begin(SERIAL_BAUDRATE);
	while (!Serial) {
		// wait for serial port to connect (native usb)
		;
	}

	_serialData.reserve(SERIAL_DATA_SIZE);

	return true;
}
/**
 * Button setup method
 */
bool initButtons() {
	for (byte i = 0; i < NUMBUTTONS; i++) {
		pinMode(buttonPins[i], INPUT_PULLUP);
	}

	btn_up = Bounce();
	btn_up.attach(PIN_GEARUP);
	btn_up.interval(DEBOUNCE_MS);

	btn_down = Bounce();
	btn_down.attach(PIN_GEARDOWN);
	btn_down.interval(DEBOUNCE_MS);

	ResetModeChangeTimer();

	return true;
}

bool initDerailleur() {
	derailleur = Derailleur();
	return true;
}

void printSerialCommands() {
	Serial.println("\nSerial connection help:");
	Serial.println("'H' = Print this help message");
	Serial.println("'I' = Init serial setup/control mode");
	Serial.println("\nIn serial setup/control mode:");
	Serial.println("'G#' = Change cassette gear count to #");
	Serial.println("'C#' = Change current gear to #");
	Serial.println("'AI' = Adjust derailleur position Inwards for current gear");
	Serial.println("'AO' = Adjust derailleur position Outwards for current gear");	
	Serial.println("'D' = Drive mode, exit serial setup/control mode");
	Serial.println("All successful commands get 'O' as an response.");
	Serial.println("Newline char '\\n' should follow every command.\n");
}

/**
 * ******** INIT METHODS & SETUP END HERE *************
 */

/**
 * Method to update button states (could maybe be triggered with timer interrupt?)
 */
void updateBtnState() {	
	btn_up.update();
	btn_down.update();
}

/** 
 * Main loop
 */
void loop()
{
	// update button states
	updateBtnState();
	checkModeChangeFromButtons();
	checkSerialCommands();
	performCurrentModeOperations();
}

/**
 * Convenience method to reset mode change timer
 */
void ResetModeChangeTimer() {
    Serial.println("Mode change timer reset");
    _modeChangeTimer = millis();
}

/**
 * Method to check if operation mode should be changed between DRIVE and ADJUST.
 * Sets _queueModeChange -boolean flag (if not already set)
 * and returns true, if mode should be changed
 */
bool checkOperationModeChange() {

	// !btn_X.read() == 1 when released (pin pulled up)
	if (!btn_up.read() && !btn_down.read()) {
		//Serial.println("checkOperationModeChange() - both buttons pressed");
		
		// has timer wrapped around? if, just reset
		if (millis() < _modeChangeTimer) {
			ResetModeChangeTimer();			
		}
		//Serial.print("millis(): ");
		//Serial.print(millis());
		//Serial.print("_mcTimer: ");
		//Serial.print(_modeChangeTimer);
		//Serial.print(" _mcInterval: ");
		//Serial.println(_modeChangeInterval);
		
		if ((_modeChangeTimer + _modeChangeInterval) > millis()) {
			// not enough time passed
			return false;
		}

		// if both buttons pressed long enough, reset timer and set mode change flag, if not already set
		if (!_queueModeChange) {
			Serial.println("checkOperationModeChange() - mode change queued");
			ResetModeChangeTimer();
			_queueModeChange = true;
		}
		return true;
	}

	// both buttons not pressed
	ResetModeChangeTimer();
	return false;
}

/**
 * Method to handle long double button press and to queue 
 * operation mode change.
 */
void checkModeChangeFromButtons() {

	if (checkOperationModeChange() || _queueModeChange) {
		// mode change queue flag is set, check if buttons are released yet
		// Serial.println("checkModeChangeFromButtons() - mode change flag set, checking buttons:");
		
		// btn_x.read() = 1 if released, 0 if pressed (input_pullup)
		if (btn_up.read() && btn_down.read()) {
			Serial.print("Buttons released, changing mode to ");
			if (_operationMode == MODE_DRIVE) {
				_operationMode = MODE_ADJUST;
				Serial.println("MODE_ADJUST");
			}
			else if (_operationMode == MODE_ADJUST) {
				_operationMode = MODE_DRIVE;
				Serial.println("MODE_DRIVE");
			}
			_queueModeChange = false;
			_modeJustChanged = true;
		}
	}
}

/**
 * Method to check current operation mode
 * and act accordingly
 */
void performCurrentModeOperations() {
	switch (_operationMode)
	{
	case MODE_DRIVE:
		driveModeFunctions();
		break;
	case MODE_ADJUST:
		adjustModeFunctions();
		break;
	case MODE_SERIALSETUP:
		serialSetupModeFunctions();
		break;
	default:
		break;
	}	
}

/**
 * Method to check if a command has been received through
 * serial port and to store it for later use
 */
void checkSerialCommands() {
	// if operation mode is SERIALSETUP, serial data is handled
	// in serialSetupModeFunctions(). Only the first mode change is handled here
	if (_serialStringComplete && _operationMode != MODE_SERIALSETUP) {
		
		//Serial.print("RECV: '");
		//Serial.print(_serialData); // echo data back to sender
		//Serial.print("', length: ");
		//Serial.println(_serialData.length());

		// init connection
		if (_serialData == "I\n") {
			_operationMode = MODE_SERIALSETUP;
			Serial.println("Mode: serial setup");
			NightShift_Util::sendSerialGearCount(derailleur.getGearCount());			
			NightShift_Util::sendSerialCurrentGear(derailleur.getCurrentGear());			
			NightShift_Util::sendSerialOK(); // ok, ready
		}
		// print help message
		else if (_serialData == "H\n") {
			printSerialCommands();
			NightShift_Util::sendSerialOK();
		}
		// clear the string and reset flag
		_serialData = "";
		_serialStringComplete = false;
	}
}

/**
 * Method to check if serial port has any input.
 * serialEvent should be triggered automatically,
 * but apparently one possible target board (Pro Mini) has some
 * problems with this, this method may be have to triggered manually
 * from loop() (in this case, modify while loop) or with timer interrupt
 */
void serialEvent() {
	while (Serial.available()) {		
		char inChar = (char)Serial.read();	

		//_serialData = Serial.readStringUntil('\n'); // test this
		// _serialStringComplete = true;
		
		_serialData += inChar;

		// if the incoming character is a newline, set a flag
		// so the main loop can do something about it:
		if (inChar == '\n') {
			_serialStringComplete = true;
		}
	}
}

/**
 * Method to handle control messages from serial connection (MODE_SERIALSETUP)
 */
void serialSetupModeFunctions() {

	if (_serialStringComplete && _operationMode == MODE_SERIALSETUP) {

	// disconnect
	if (_serialData == "D\n") {
		_operationMode = MODE_DRIVE;
		Serial.println("Mode: drive");
		NightShift_Util::sendSerialOK();
	}
	// display help message
	else if (_serialData == "H\n") {
		printSerialCommands();
		NightShift_Util::sendSerialOK();
	}
	// change current gear
	else if (_serialData.substring(0, 1) == "C") {
		int gotoGear = _serialData.substring(1, _serialData.length()).toInt();
		
		Serial.print("Change current gear to: ");		
		Serial.println(gotoGear);
				
		derailleur.gearTo(gotoGear);
		NightShift_Util::sendSerialOK();
	}

	// change gear count
	else if (_serialData.substring(0, 1) == "G") {
		int newGearCount = _serialData.substring(1, _serialData.length()).toInt();
		Serial.print("Change gear count to: ");
		Serial.println(newGearCount);

		// new gear count is smaller than current gear, change gear
		if (derailleur.getCurrentGear() > newGearCount){
			derailleur.gearTo(newGearCount);
			NightShift_Util::sendSerialCurrentGear(derailleur.getCurrentGear());
		}
		derailleur.setGearCount(newGearCount);
				
		NightShift_Util::sendSerialGearCount(derailleur.getGearCount());
		NightShift_Util::sendSerialOK();
	}

	// adjust current gear
	else if (_serialData.substring(0, 1) == "A") {
		Serial.print("Adjusting derailleur position ");
		// inwards
		if (_serialData.substring(1, _serialData.length() - 1) == "I") {
			Serial.print("INWARDS");
			// adjust, store				
		}
		// outwards
		if (_serialData.substring(1, _serialData.length() - 1) == "O") {
			Serial.print("OUTWARDS");
			// adjust, store
		}
		Serial.println();
		NightShift_Util::sendSerialOK();
	}

	// clear the string and reset flag
	_serialData = "";
	_serialStringComplete = false;

	}
}

/**
 * Method for operating in drive mode (MODE_DRIVE):
 * Button checks, gear changes, etc.
 */
void driveModeFunctions() {
	// check if mode was just changed to skip unintentional
	// gear changes from mode change button release
	if (!_modeJustChanged) {

		// handle gear changes
		if (btn_up.rose()) {
			// Serial.print("driveModeFunctions() - btnUP released");

			// up released while down pressed, increment queue gear change counter
			if (!btn_down.read()) {
				// Serial.println(" while btnDOWN still pressed");

				_gearsToSwitch++;
				// _modeChangeTimer = millis(); // just playing safe, reset mode change counter
			}
			else {
				// up button released, change gear
				// Serial.println(", changing gear(s)");
				derailleur.gearUp(_gearsToSwitch);
				_gearsToSwitch = 1;
			}
		}

		if (btn_down.rose()) {
			// Serial.print("driveModeFunctions() - btnDOWN released");

			if (!btn_up.read()) {
				// Serial.println(" while btnUP still pressed");
				_gearsToSwitch++;
			}
			else {
				// Serial.println(", changing gear(s)");
				derailleur.gearDown(_gearsToSwitch);
				_gearsToSwitch = 1;
			}
		}
	}
	else {
		_modeJustChanged = false;
		_gearsToSwitch = 1;
	}
}

/*
 * Method to handle drive-time adjustments (MODE_ADJUST)
 */
void adjustModeFunctions() {

}
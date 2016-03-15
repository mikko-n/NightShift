// 
// Helper functions namespace class
// 

#include "NightShift_Util.h"

namespace NightShift_Util {

	void blink(int pin, int count) {
		for (int i = 0; i < count; i++) {
			pinMode(pin, OUTPUT); // set pin to output
			digitalWrite(pin, HIGH); // pull led pin high
			delay(200);
			digitalWrite(pin, LOW); // pull led pin low
			delay(200);
		}
	}

	void sendSerialOK() {
		Serial.println("O");
	}

	void sendSerialGearCount(int gears) {
		Serial.print("G"); // gear count
		Serial.println(gears);
	}

	void sendSerialCurrentGear(int gear) {
		Serial.print("C"); // current gear
		Serial.println(gear);
	}
	
	
}
// NightShift_Util.h

#ifndef _NIGHTSHIFT_UTIL_h
#define _NIGHTSHIFT_UTIL_h
#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

namespace NightShift_Util {

	void blink(int pin, int count);
	void sendSerialOK();
	void sendSerialGearCount(int gears);
	void sendSerialCurrentGear(int gear);
	void sendSerialGearPosition(int position);
};


#endif


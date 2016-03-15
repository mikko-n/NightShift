// Derailleur.h

#ifndef _DERAILLEUR_h
#define _DERAILLEUR_h

class Derailleur
{
public:
	Derailleur();
	//void attach(int servoPwmPin);
	//void attach(int servoPwmPin, int servoFeedbackPin);
	void setGearCount(int gears);

	void gearTo(int targetGear);
	void gearUp(int count);
	void gearDown(int count);

	int getGearCount();
	int getCurrentGear();
protected:
	int _derailleur_gearCount;  // gears in bike
	int _derailleur_currentGear; // current gear
	bool initServo(bool hasFeedbackLoop, bool cutPowerWhenIdle);
	int servoRawToDegrees(int measurement);	
};

#endif

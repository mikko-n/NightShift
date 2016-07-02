// Derailleur.h

#ifndef _DERAILLEUR_h
#define _DERAILLEUR_h

#define DERAILLEUR_MAX_GEARCOUNT 50
#define DERAILLEUR_INITGEARCOUNT 7

class Derailleur
{
public:
	Derailleur();
	//void attach(int servoPwmPin);
	//void attach(int servoPwmPin, int servoFeedbackPin);
	void setGearCount(int gears);
    bool setCurrentGearPosition(int position);
    
	void gearTo(int targetGear);
	void gearUp(int count);
	void gearDown(int count);

	int getGearCount();
	int getCurrentGear();
    int getCurrentGearPosition();
    int getGearPosition(int gear);
protected:
    int _derailleur_gearPositions[DERAILLEUR_MAX_GEARCOUNT]; // large-ish array for gear positions
	int _derailleur_gearCount;  // gears in bike
	int _derailleur_currentGear; // current gear
	bool initServo(bool hasFeedbackLoop, bool cutPowerWhenIdle);
	int servoRawToDegrees(int measurement);	
};

#endif

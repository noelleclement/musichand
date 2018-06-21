#pragma once

#include <string>

#include "sensor.h"
#include "instrument.h"
#include "instrumentHashMap.h"


class MusicHand{
	
public:
	string musicHandID;
	HashMap instrumentHashMap;
	Instrument* currentInstrument;
	Sensor* sensor;

	MusicHand(string id);
	void bluetoothCheck();


};
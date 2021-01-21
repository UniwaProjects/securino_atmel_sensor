/*
Encapsulates the fuctions for the battery readings.
*/

#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

namespace sensor
{
	// Minimum analog read for a healthy battery charge
	// for a 6V battery pack
	const uint16_t battery_voltage_threshold = 617;

	class BatteryMonitor
	{
	public:
		BatteryMonitor(BatteryMonitor const &) = delete;
		void operator=(BatteryMonitor const &) = delete;
		// Methods
		static BatteryMonitor *getInstance();
		void init(uint8_t voltage_pin);
		bool isLow();

	private:
		// Methods
		BatteryMonitor();
		// Variables
		static BatteryMonitor *m_instance;
		uint8_t m_voltage_pin;
	};
} // namespace sensor
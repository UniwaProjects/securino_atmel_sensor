#include "BatteryMonitor.h"

sensor::BatteryMonitor *sensor::BatteryMonitor::m_instance = nullptr;

sensor::BatteryMonitor *sensor::BatteryMonitor::getInstance()
{
	if (m_instance == nullptr)
	{
		m_instance = new BatteryMonitor();
	}
	return m_instance;
}

sensor::BatteryMonitor::BatteryMonitor() {}

// Sets the internal voltage pin variable and initiazes that pin
// as an input.
void sensor::BatteryMonitor::init(uint8_t voltage_pin)
{
	m_voltage_pin = voltage_pin;
	pinMode(m_voltage_pin, INPUT);
}

// Performs an analog read to the voltage divider at designated pin.
// Returns true if the battery is bellow the threshold and true otherwise.
bool sensor::BatteryMonitor::isLow()
{
	uint16_t battery_voltage = analogRead(m_voltage_pin);
	Serial.println("Battery: " + String(battery_voltage));
	if (battery_voltage <= battery_voltage_threshold)
	{
		return true;
	}
	return false;
}
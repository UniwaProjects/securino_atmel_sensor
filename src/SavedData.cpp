#include "SavedData.h"

sensor::SavedData *sensor::SavedData::m_instance = nullptr;

sensor::SavedData *sensor::SavedData::getInstance()
{
	if (m_instance == nullptr)
	{
		m_instance = new SavedData();
	}
	return m_instance;
}

sensor::SavedData::SavedData() {}

// Initialized the EEPROM the first time the controller boots
void sensor::SavedData::initializeMemory()
{
	uint8_t value = EEPROM.read(memoryInitAddress);
	if (value == memoryInitValue)
	{
		return;
	}

	saveDeviceId(0);
	saveSessionId(0);
	saveSensorId(0);
	EEPROM.write(memoryInitAddress, memoryInitValue);
}

// Saves the device id as a char array.
void sensor::SavedData::saveDeviceId(uint32_t device_id)
{
	char buffer[device_id_length + 1];
	memset(buffer, 0, device_id_length + 1);
	sprintf(buffer, "%lu", device_id);
	Serial.print(buffer);
	Serial.print(" ");
	for (int i = 0; i < device_id_length; i++)
	{
		Serial.print(device_id_address + i);
		Serial.print("(" + String(buffer[i]) + ")");

		Serial.print(", ");
		EEPROM.write(device_id_address + i, buffer[i]);
	}
	Serial.println();
}

// Reads the device id from the specified address.
// Returns it as an unsigned long integer.
uint32_t sensor::SavedData::readDeviceId()
{
	char buffer[device_id_length + 1] = {0};
	for (int i = 0; i < device_id_length; i++)
	{
		char c = (char)EEPROM.read(device_id_address + i);
		if (c == '\0')
		{
			break;
		}
		buffer[i] = c;
	}
	return strtoul(buffer, NULL, 0);
}

// Saves the session id as a char array.
void sensor::SavedData::saveSessionId(uint16_t session_id)
{
	char buffer[session_id_length + 1];
	memset(buffer, 0, session_id_length + 1);
	sprintf(buffer, "%u", session_id);
	Serial.print(buffer);
	Serial.print(" ");

	for (int i = 0; i < session_id_length; i++)
	{
		Serial.print(session_id_address + i);
		Serial.print("(" + String(buffer[i]) + ")");

		Serial.print(", ");
		EEPROM.write(session_id_address + i, buffer[i]);
	}
	Serial.println();
}

// Reads the session id from the specified address.
// Returns it as an unsigned integer.
uint16_t sensor::SavedData::readSessionId()
{
	char buffer[session_id_length + 1] = {0};
	for (int i = 0; i < session_id_length; i++)
	{
		char c = (char)EEPROM.read(session_id_address + i);
		if (c == '\0')
		{
			break;
		}
		buffer[i] = c;
	}
	return strtoul(buffer, NULL, 0);
}

// Saves the sensor id as a char array.
void sensor::SavedData::saveSensorId(uint8_t sensor_id)
{
	char buffer[sensor_id_length + 1];
	memset(buffer, 0, sensor_id_length + 1);
	sprintf(buffer, "%hu", sensor_id);
	Serial.print(buffer);
	Serial.print(" ");
	for (int i = 0; i < sensor_id_length; i++)
	{
		Serial.print(sensor_id_address + i);
		Serial.print("(" + String(buffer[i]) + ")");

		Serial.print(", ");
		EEPROM.write(sensor_id_address + i, buffer[i]);
	}
	Serial.println();
}

// Reads the sensor id from the specified address.
// Returns it as an unsigned short integer.
uint8_t sensor::SavedData::readSensorId()
{
	char buffer[sensor_id_length + 1] = {0};
	for (int i = 0; i < sensor_id_length; i++)
	{
		char c = (char)EEPROM.read(sensor_id_address + i);
		if (c == '\0')
		{
			break;
		}
		buffer[i] = c;
	}
	return strtoul(buffer, NULL, 0);
}
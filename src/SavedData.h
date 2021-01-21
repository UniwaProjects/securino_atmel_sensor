/*
Handles the communication with the EEPROM and the addresses that
the information is stored.
*/

#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <EEPROM.h>

namespace sensor
{
	// Address of the memory init cookie
	const uint16_t memoryInitAddress = 512;
	const uint8_t memoryInitValue = 128;
	// Addresses of stored information and length of said information.
	const uint8_t device_id_address = 0;
	const uint8_t device_id_length = 10;
	const uint8_t session_id_address = device_id_address + device_id_length;
	const uint8_t session_id_length = 5;
	const uint8_t sensor_id_address = session_id_address + session_id_length;
	const uint8_t sensor_id_length = 3;

	class SavedData
	{
	public:
		SavedData(SavedData const &) = delete;
		void operator=(SavedData const &) = delete;
		// Methods
		static SavedData *getInstance();
		void initializeMemory();
		void saveDeviceId(uint32_t device_id);
		uint32_t readDeviceId();
		void saveSessionId(uint16_t session_id);
		uint16_t readSessionId();
		void saveSensorId(uint8_t device_id);
		uint8_t readSensorId();

	private:
		// Methods
		SavedData();
		// Variables
		static SavedData *m_instance;
	};
} //  namespace sensor
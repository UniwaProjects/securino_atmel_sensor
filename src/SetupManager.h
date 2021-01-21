/*
Handles the setup process after connecting the usb cable
and pressing the setup button. Uses the I2C protocol to communicate
via the usb cable and receive the main device's ids.
*/

#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <Wire.h>

namespace sensor
{
	// Response of the main device.
	typedef enum setup_outcome_t
	{
		ok = 1,
		error = 2
	} setup_outcome_t;
	// I2C communication address
	const int address = 8;
	const uint16_t setup_timeout = 15;
	// I2C receive buffer size, the number is derived from:
	// Max device id: 4294967295 = 10 chars
	// Max session id: 65535 = 5 chars
	// Max sensor id: 255 (byte) = 3 chars
	// = 18 characters total, 2 commas + 1 end char
	const uint8_t buffer_size = 21;
	// A custom struct for returning all of the IDs
	typedef struct ReceivedId
	{
		uint32_t parent_device_id = 0;
		uint16_t session_id = 0;
		uint8_t sensor_id = 0;
	} ReceivedId;

	class SetupManager
	{
	public:
		SetupManager(SetupManager const &) = delete;
		void operator=(SetupManager const &) = delete;
		// Methods
		static SetupManager *getInstance();
		static bool m_setup;
		static uint8_t m_bind_response;
		void init(uint8_t button_pin, uint8_t request_response);
		bool enterInstallMode();
		bool getButtonPress();
		bool isSetup();
		ReceivedId getReceivedIds();

	private:
		// Methods
		SetupManager();
		static void receiveEvent(int length);
		static void requestEvent();
		// Variables
		static int m_button_pin;
		static uint8_t m_receive_requests;
		static SetupManager *m_instance;
		static volatile char m_buffer[buffer_size];
		static uint8_t m_request_response;
	};
} //  namespace sensor
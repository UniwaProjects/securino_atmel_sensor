/*
Manages the radio communications using the rf24 library.
*/

#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include "common/sensortypes.h"
// Radio libraries
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

namespace sensor
{
	// Response of the main device.
	typedef enum response_t
	{
		response_error = -1,
		response_disarmed = 0,
		response_armed = 1,
		response_wrong_device = 2
	} response_t;
	// Radio constants
	const uint64_t addresses[2] = {0xABCDABCD71LL, 0x544d52687CLL};
	const uint8_t channel = 125; // Sets the frequency to 2525Mhz, above the Wifi range
	// Max used from the rf24 library from the setRetries function
	const uint8_t max_retries = 15;
	// The min and max delay microseconds, the range is the same with the one
	// used from the rf24 library for the setRetries function.
	const uint16_t min_delay = 250;
	const uint16_t max_delay = 4000;

	class RadioManager
	{
	public:
		RadioManager(RadioManager const &) = delete;
		void operator=(RadioManager const &) = delete;
		// Methods
		static RadioManager *getInstance();
		void init(uint8_t ce_pin, uint8_t csn_pin);
		response_t send(const sensortypes::SensorMessage &message, bool hasNoTimeout);
		bool wasSent();

	private:
		// Methods
		RadioManager();
		// Variables
		static RadioManager *m_instance;
		RF24 *m_radio;
		bool m_sent; // True if the last message was sent
	};
} // namespace sensor
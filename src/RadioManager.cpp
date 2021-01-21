#include "RadioManager.h"

sensor::RadioManager *sensor::RadioManager::m_instance = nullptr;

sensor::RadioManager *sensor::RadioManager::getInstance()
{
	if (m_instance == nullptr)
	{
		m_instance = new RadioManager();
	}
	return m_instance;
}

sensor::RadioManager::RadioManager()
{
	m_sent = false;
}

// Initialize radio communications.
void sensor::RadioManager::init(uint8_t ce_pin, uint8_t csn_pin)
{
	// Create the radio object and set some settings.
	m_radio = new RF24(ce_pin, csn_pin);
	m_radio->begin();
	m_radio->setPALevel(RF24_PA_MAX);							 // High power consumption and more dbs.
	m_radio->setChannel(channel);								 // See comments on channel constant.
	m_radio->setAutoAck(true);									 // Ensure autoACK is enabled.
	m_radio->enableAckPayload();								 // Allow optional ack payloads.
	m_radio->setPayloadSize(sizeof(sensortypes::SensorMessage)); // Payload is equal to the message size
	
	// Open the pipes for reading and writing.
	m_radio->openWritingPipe(addresses[0]);
	m_radio->openReadingPipe(1, addresses[1]);
	m_radio->startListening();
}

// Sends the message passed on the arguements and returns the response. If not sent, the message
// is resent on random intervals between limits, for up to the set number of retries.
// If the no timeouts arguement is true, the message will be resent until received.
sensor::response_t sensor::RadioManager::send(const sensortypes::SensorMessage &message, bool hasNoTimeout)
{
	m_radio->powerUp();
	m_radio->stopListening();

	// Delay a random ammount of seconds, that way is improbable that the
	// message with colide again with another sensor, as that sensor will
	// delay random microseconds too.
	bool sent = false;
	uint8_t retries = 0;
	do
	{
		sent = m_radio->write(&message, sizeof(message));
		if (!sent)
		{
			delayMicroseconds(random(min_delay, max_delay));
			retries++;
		}
	} while (!sent && (retries < max_retries || hasNoTimeout));

	Serial.print("Sent: ");
	Serial.print(sent ? "True" : "False");
	Serial.print(" -- Retries: ");
	Serial.println(retries);

	// If the message was successfully sent, get the ack payload.
	int8_t response = response_error;
	if (sent)
	{
		// m_radio->startListening();

		// If a payload is received.
		if (m_radio->isAckPayloadAvailable())
		{
			// Read the response and flush the rx if it is an error.
			m_radio->read(&response, sizeof(response));
			if (response == response_error)
			{
				// Without flushing the rx register, in case of a failed ack
				// it will fail clearing it and fail all the next attempts to send anything.
				// A bad ack can be received due to interference from other sensors.
				m_radio->flush_rx();
			}
		}
	}
	m_radio->powerDown();
	m_sent = sent;
	return (response_t)response;
}

// Returns the sent flag for the last message attempt.
bool sensor::RadioManager::wasSent()
{
	return m_sent;
}
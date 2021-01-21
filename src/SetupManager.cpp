#include "SetupManager.h"
#include "common/Timer.h"

// Variables
sensor::SetupManager *sensor::SetupManager::m_instance = nullptr;
volatile char sensor::SetupManager::m_buffer[buffer_size] = {0};
uint8_t sensor::SetupManager::m_request_response = 0;
uint8_t sensor::SetupManager::m_receive_requests = 0;
uint8_t sensor::SetupManager::m_bind_response = 0;
bool sensor::SetupManager::m_setup = false;
int sensor::SetupManager::m_button_pin = 0;

// Methods
sensor::SetupManager *sensor::SetupManager::getInstance()
{
	if (m_instance == nullptr)
	{
		m_instance = new SetupManager();
	}
	return m_instance;
}

// Attaches pointers to the on receive and request methods for the wire communication
sensor::SetupManager::SetupManager()
{
	// Attach the receive event to functions
	Wire.onReceive(receiveEvent);
	Wire.onRequest(requestEvent);
}

// Initliazes the pin mode for the install button and attaches the interrupt.
// Additionally sets the request response.
void sensor::SetupManager::init(uint8_t button_pin, uint8_t request_response)
{
	m_button_pin = button_pin;
	// Attach an interrupt to the setup button
	pinMode(m_button_pin, INPUT);
	// Set the response bit, which is the type of the sensor
	m_request_response = request_response;
}

// Enters the install mode, during which the sensor awaits for the ids to
// be received, halting its operation until then, or until the button is pressed again.
bool sensor::SetupManager::enterInstallMode()
{
	Timer m_setup_timer(setup_timeout);
	m_receive_requests = 0;
	m_bind_response = 0;
	// Wire.begin needs to be called every time, as the sleep function
	// deactivates the I2C pullup capacitors.
	Wire.begin(address);
	// While the incoming buffer is empty.
	// When the buffer is not empty then the receive loop takes control.
	while (m_buffer[0] == 0)
	{
		// Escape if timeout
		if (m_setup_timer.timeout())
		{
			m_setup = false;
			return false;
		}
	}

	// Exit install mode
	m_setup = false;
	return true;
}

// Returns true if the button is pressed
bool sensor::SetupManager::getButtonPress()
{
	return digitalRead(m_button_pin) == LOW;
}

// Returns true if the setup mode turns true,
// when the button is pressed.
bool sensor::SetupManager::isSetup()
{
	bool button_pressed = digitalRead(m_button_pin) == LOW;
	if (button_pressed)
	{
		m_setup = !m_setup;
	}
	return m_setup;
}

// Returns the received ids and empties the buffer.
sensor::ReceivedId sensor::SetupManager::getReceivedIds()
{
	// Intialize an empty object
	ReceivedId received_ids;

	// The max word length is the device id, which is 11 and a 0 for end of array.
	char result_buffer[12] = {0};
	uint8_t data_index = 0;
	uint8_t result_index = 0;
	char current_char;

	// Get the device id
	while (1)
	{
		current_char = m_buffer[data_index];
		if (current_char == ',')
		{
			data_index++; // skip char
			break;
		}
		result_buffer[result_index] = current_char;
		result_buffer[result_index + 1] = '\0';
		result_index++;
		data_index++;
	}

	// Save the id and zero the index
	received_ids.parent_device_id = strtoul(result_buffer, NULL, 0);
	result_index = 0;

	// Get the session id
	while (1)
	{
		current_char = m_buffer[data_index];
		if (current_char == ',')
		{
			data_index++; // skip char
			break;
		}
		result_buffer[result_index] = current_char;
		result_buffer[result_index + 1] = '\0';
		result_index++;
		data_index++;
	}

	// Save the id and zero the index
	received_ids.session_id = atoi(result_buffer);
	result_index = 0;

	// Get session id
	while (1)
	{
		if (current_char == '\0')
		{
			break;
		}
		current_char = m_buffer[data_index];
		result_buffer[result_index] = current_char;
		result_buffer[result_index + 1] = '\0';
		result_index++;
		data_index++;
	}

	// Save the id
	received_ids.sensor_id = atoi(result_buffer);

	// Clear the buffer
	for (uint8_t i = 0; i < buffer_size; i++)
	{
		m_buffer[i] = 0;
	}
	Serial.println("Recevied Ids: " + String(received_ids.parent_device_id) + ", " + String(received_ids.session_id) + ", " + String(received_ids.sensor_id));
	return received_ids;
}

// On the receive event, red the I2C buffer char by char and transfer it to
// the internal buffer.
void sensor::SetupManager::receiveEvent(int length)
{
	uint8_t data_index = 0;
	while (Wire.available())
	{
		m_buffer[data_index] = (char)Wire.read();
		data_index++;
	}
}

// On the request event, respond with the device id.
void sensor::SetupManager::requestEvent()
{
	if (m_receive_requests == 0)
	{
		Serial.println("Sent:" + String(m_request_response));

		Wire.write(m_request_response);
	}
	else
	{
		if (m_bind_response > 0)
		{
			Serial.println("Sent:" + String(m_bind_response));
			Wire.write(m_bind_response);
			// Set it to 0 to not be sent again
			m_bind_response = 0;
		}
	}
	m_receive_requests++;
}
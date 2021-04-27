#include <LowPower.h>
#include "RadioManager.h"
#include "BatteryMonitor.h"
#include "SetupManager.h"
#include "SavedData.h"
#include "common/Timer.h"
#include "common/sensortypes.h"

#define DEBUG

#pragma region Constants
// Pin constants
const uint8_t voltage_pin = A1;
const uint8_t sensor_pin = 2;
const uint8_t led_pin = 3;
const uint8_t button_pin = 4;
const uint8_t sensor_type_pin = 5;
const uint8_t ce_pin = 9;
const uint8_t csn_pin = 10;

// Sleep cycles
const uint8_t sleep_cycles_armed = 24;
const period_t sleep_cycle_armed = SLEEP_1S;
const uint8_t sleep_cycles_disarmed = 3;
const period_t sleep_cycle_disarmed = SLEEP_8S;

// Time between led blinks and blink codes
const uint16_t led_interval = 200;
const uint8_t setup_success_blinks = 3;
const uint8_t setup_failed_blinks = 1;
const uint8_t signal_lost_blinks = 5;
#pragma endregion

#pragma region Global
// Class Instances
sensor::BatteryMonitor *g_battery = sensor::BatteryMonitor::getInstance();
sensor::RadioManager *g_radio = sensor::RadioManager::getInstance();
sensor::SetupManager *g_setup = sensor::SetupManager::getInstance();
sensor::SavedData *g_data = sensor::SavedData::getInstance();

// Variables
volatile uint8_t g_state;
bool g_is_armed;
sensortypes::SensorMessage g_message;
#pragma endregion

#pragma region Forward Declarations
void updateSensorState();
void changeArmStatus(bool);
void mcuSleep();
void bindSensor();
void sensorTriggerEvent();
void sendData(bool);
#pragma endregion

void setup()
{
#ifdef DEBUG
	// Initialzie serial
	Serial.begin(115200);
#endif

	// Set the pinmodes
	pinMode(sensor_pin, INPUT);
	pinMode(sensor_type_pin, INPUT);
	pinMode(led_pin, OUTPUT);

	// Initialize the device EEPROM memory
	g_data->initializeMemory();

	// Initialize battery manager and get the sensor state
	g_battery->init(voltage_pin);
	updateSensorState();

	// Collect data from EEPROM for the message
	g_message.parent_device_id = g_data->readDeviceId();
	g_message.session_id = g_data->readSessionId();
	g_message.sensor_id = g_data->readSensorId();
	g_message.type = digitalRead(sensor_type_pin) == HIGH ? sensortypes::type_pir : sensortypes::type_magnet;
	g_message.state = (sensortypes::sensor_state_t)g_state;

	// Random seed is unique for each sensor in the network, based on the unique sensor id.
	randomSeed(g_message.sensor_id);

	// Default arm status is disarmed
	changeArmStatus(false);

	// Intialize Radio
	g_radio->init(ce_pin, csn_pin);

	// Initialize the class that handles cable setup with main device
	g_setup->init(button_pin, sensortypes::type_pir);

#ifdef DEBUG
	Serial.println("Sensor type: " + String(g_message.type));
	Serial.println("Loaded Ids: " + String(g_message.parent_device_id) + ", " + String(g_message.session_id) + ", " + String(g_message.sensor_id));
#endif
}

void loop()
{
	// If the button is pressed, intialize sensor setup
	if (g_setup->isSetup())
	{
		bindSensor();
	}

	// For triggered state, send message and change arm status to avoid more triggers, else just ping
	if (g_state == sensortypes::state_triggered)
	{
		sendData(true);
		changeArmStatus(false);
	}
	else
	{
		sendData(false);
	}

	// Update the state to ping or battery low
	updateSensorState();

	// Sleep the mcu, with cycles determined by the armed state
	mcuSleep();
}

// Blinks led given times
void blinkLed(uint8_t times)
{
	for (uint8_t i = 0; i < times; i++)
	{
		delay(led_interval);
		digitalWrite(led_pin, HIGH);
		delay(led_interval);
		digitalWrite(led_pin, LOW);
	}
}

// Gets the required ids from the main device, by cable.
// Blinks the led if it was a success, saves the new ids to eeprom and
// reinitializes the random seed based on the new sensor id.
void bindSensor()
{
	Serial.println("Setup started");

	// Led stays lit during setup
	digitalWrite(led_pin, HIGH);

	bool isInstalled = g_setup->enterInstallMode();

	// Led turns off after install
	digitalWrite(led_pin, LOW);

	Serial.println("Setup ended");

	// If the install mode returns false (due to canceling), exit
	if (!isInstalled)
	{
		blinkLed(setup_failed_blinks);
		return;
	}

	// Get the received ids
	sensor::ReceivedId received_ids = g_setup->getReceivedIds();
	bool is_same_device_id = received_ids.parent_device_id == g_message.parent_device_id;
	bool is_same_session_id = received_ids.session_id == g_message.session_id;
	if (is_same_device_id && is_same_session_id)
	{
		g_setup->m_bind_response = sensor::setup_outcome_t::error;
		blinkLed(setup_failed_blinks);
		return;
	}

	g_setup->m_bind_response = sensor::setup_outcome_t::ok;

	// Save ids to EEPROM
	g_data->saveDeviceId(received_ids.parent_device_id);
	g_data->saveSessionId(received_ids.session_id);
	g_data->saveSensorId(received_ids.sensor_id);

	// Change the ids of the message
	g_message.parent_device_id = received_ids.parent_device_id;
	g_message.session_id = received_ids.session_id;
	g_message.sensor_id = received_ids.sensor_id;

	// Reinitialize the seed with the new sensor id, now this seed is unique for this alarm system
	randomSeed(g_message.sensor_id);

	// Blink success
	blinkLed(setup_success_blinks);
#ifdef DEBUG
	Serial.println("Saved Ids: " + String(g_message.parent_device_id) + ", " + String(g_message.session_id) + ", " + String(g_message.sensor_id));
	Serial.flush();
#endif
}

// Updates the global state based on the battery reading.
void updateSensorState()
{
	if (g_battery->isLow())
	{
		g_state = sensortypes::state_battery_low;
	}
	else
	{
		g_state = sensortypes::state_ping;
	}
}

// Enables or disables the interrupt 0, based on the alarm arm state.
void changeArmStatus(bool new_status)
{
	if (new_status != g_is_armed)
	{
		// If the alarm is armed
		g_is_armed = new_status;
		if (g_is_armed)
		{
			// Clear interrupt flag for pin D2
			EIFR |= (1 << INT0);
			// And attach an interrupt that sets state to triggered if movement is detected.
			attachInterrupt(digitalPinToInterrupt(sensor_pin), sensorTriggerEvent, FALLING);
		}
		else
		{
			// Else detach the interrupt.
			detachInterrupt(0);
		}
	}
}

// Is called only by the the interrupt, sets the state to triggered and detaches
// the interrupt to prevent the firing of multiple interupts disrupting the program flow.
void sensorTriggerEvent()
{
	detachInterrupt(0);
	g_state = sensortypes::state_triggered;
#ifdef DEBUG
	Serial.println("Trigger");
	Serial.flush();
#endif
}

// Puts the mcu to a sleep cycle determined by the arm state. Shorter sleep cycles are selected
// for armed state and longer for disarmed state.
void mcuSleep()
{
#ifdef DEBUG
	Serial.flush();
#endif
	if (g_is_armed)
	{
		// Shorter sleep cycles and sending in between if triggered
		for (uint8_t i = 0; i < sleep_cycles_armed; i++)
		{
			// If the interrupt triggered the state
			if (g_state == sensortypes::state_triggered)
			{
				return;
			}
			LowPower.powerDown(sleep_cycle_armed, ADC_OFF, BOD_OFF);
		}
	}
	else
	{
		// Long sleep cycles
		for (uint8_t i = 0; i < sleep_cycles_disarmed; i++)
		{
			if (g_setup->getButtonPress())
			{
				return;
			}
			LowPower.powerDown(sleep_cycle_disarmed, ADC_OFF, BOD_OFF);
		}
	}
}

void sendData(bool hasNoTimeout)
{
	// Update the state and send the message
	g_message.state = (sensortypes::sensor_state_t)g_state;
	sensor::response_t response = g_radio->send(g_message, hasNoTimeout);
	// Change state based on response
	switch (response)
	{
	case sensor::response_error:
		// Do nothing for error
		break;
	case sensor::response_disarmed:
		changeArmStatus(false);
		break;
	case sensor::response_armed:
		changeArmStatus(true);
		break;
	case sensor::response_wrong_device:
		// Do nothing for wrong device
		break;
	}
	// Blink for radio lost if not on setup mode (only one led)
	if (!g_radio->wasSent() && !g_setup->m_setup)
	{
		blinkLed(signal_lost_blinks);
	}

#ifdef DEBUG
	// Ack 0 is disarmed, 1 is armed, 255 is blank payload
	// Ack 72 is collision during ack on NANO, 200 on MINI
	Serial.print("Response: ");
	Serial.print(response);
	Serial.print(", Armed: ");
	Serial.print(g_is_armed);
	Serial.print(", State: ");
	Serial.println(g_state);
	Serial.flush();
#endif
}
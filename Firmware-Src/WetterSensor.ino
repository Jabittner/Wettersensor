#include "WetterSensor.h"
#include "Register.h"															// configuration sheet

//- load library's --------------------------------------------------------------------------------------------------------
#include <Wire.h>																// i2c library, needed for bmp085 or bmp180
#include <TSL2561.h>
#include <Sensirion.h>
#include <BMP085.h>
#include <Buttons.h>															// remote buttons library
#include <Sensor_SHT10_BMP085_TSL2561.h>

// homematic communication
HM::s_jumptable jTbl[] = {														// jump table for HM communication
	// byte3, byte10, byte11, function to call									// 0xff means - any byte
	 { 0x11,  0x04,  0x00,    cmdReset },										// Reset message
	 { 0x01,  0xFF,  0x06,    cmdConfigChanged },								// Config end message
	 { 0x01,  0xFF,  0x0E,    cmdStatusRequest },
	 { 0x00 }
};

Buttons button[1];																// declare remote button object

Sensirion sht10;
BMP085 bmp085;
TSL2561 tsl2561;

Sensors_SHT10_BMP085_TSL2561 sensTHPL;

// main functions
void setup() {

	// We disable the Watchdog first
	wdt_disable();

	#ifdef SER_DBG
		Serial.begin(57600);													// serial setup
		Serial << F("Starting sketch...\n");									// ...and some information
		Serial << F("freeMem: ") << freeMem() << F(" byte") <<'\n';
	#endif

	#if USE_ADRESS_SECTION == 1
		getDataFromAddressSection(devParam, 1,  ADDRESS_SECTION_START + 0, 12);	// get device type (model-ID) and serial number from bootloader section at 0x7FF0 and 0x7FF2
		getDataFromAddressSection(devParam, 17, ADDRESS_SECTION_START + 12, 3);	// get device address stored in bootloader section at 0x7FFC

//		Serial.begin(57600);
//		Serial << F("Device type from Bootloader: ") << pHex(&devParam[1],  2) << '\n';
//		Serial << F("Serial from Bootloader: ")      << pHex(&devParam[3], 10) << '\n';
//		Serial << F("Addresse from Bootloader: ")    << pHex(&devParam[17], 3) << '\n';
	#endif

	hm.cc.config(10,11,12,13,2,0);												// CS, MOSI, MISO, SCK, GDO0, Interrupt

	// setup battery measurement
	hm.battery.config(
		BATTERY_MODE_EXTERNAL_MESSUREMENT, 7, 1, BATTERY_MEASSUREMENT_FACTOR, 10000
	);

	hm.init();																	// initialize the hm module

	button[0].regInHM(0, &hm);													// register buttons in HM per channel, handover HM class pointer
	button[0].config(8, NULL);													// configure button on specific pin and handover a function pointer to the main sketch

	sensTHPL.regInHM(1, &hm);													// register sensor class in hm
	sensTHPL.config(A4, A5, 0, &sht10, &bmp085, &tsl2561);						// data pin, clock pin and timing - 0 means HM calculated timing, every number above will taken in milliseconds

	byte rr = MCUSR;
	MCUSR =0;

	cmdConfigChanged(0, 0);

	// initialization done, we blink 3 times
	hm.statusLed.config(4, 4);													// configure the status led pin
	hm.statusLed.set(STATUSLED_BOTH, STATUSLED_MODE_BLINKFAST, 3);

	/** Debug start */
//	pinMode(5, OUTPUT);
//	pinMode(6, OUTPUT);
//	digitalWrite(5, 1);
	/** Debug end */
}

void getDataFromAddressSection(uint8_t *buffer, uint8_t bufferStartAddress, uint16_t sectionAddress, uint8_t dataLen) {
	for (unsigned char i = 0; i < dataLen; i++) {
		buffer[(i + bufferStartAddress)] = pgm_read_byte(sectionAddress + i);
	}
}

void getPgmSpaceData(uint8_t *buffer, uint16_t address, uint8_t len) {
	for (unsigned char i = 0; i < len; i++) {
		buffer[i] = pgm_read_byte(address+i);
	}
}

void loop() {
	hm.poll();																	// poll the HM communication
}

void cmdReset(uint8_t *data, uint8_t len) {
	#ifdef SER_DBG
		Serial << F("reset, data: ") << pHex(data,len) << '\n';
	#endif

	hm.send_ACK();																// send an ACK
	if (data[1] == 0) {
		hm.reset();
//		hm.resetWdt();
	}
}

void cmdConfigChanged(uint8_t *data, uint8_t len) {
	// low battery level
	uint8_t lowBatVoltage = regs.ch0.l0.lowBatLimit;
	lowBatVoltage = (lowBatVoltage < BATTERY_MIN_VOLTAGE) ? BATTERY_MIN_VOLTAGE : lowBatVoltage;
	lowBatVoltage = (lowBatVoltage > BATTERY_MAX_VOLTAGE) ? BATTERY_MAX_VOLTAGE : lowBatVoltage;
	hm.battery.setMinVoltage(lowBatVoltage);

	// led mode
	hm.setLedMode((regs.ch0.l0.ledMode) ? LED_MODE_EVERYTIME : LED_MODE_CONFIG);

	// power mode for HM device
//	hm.setPowerMode(POWER_MODE_SLEEP_WDT);
	hm.setPowerMode((regs.ch0.l0.burstRx) ? POWER_MODE_BURST : POWER_MODE_SLEEP_WDT);
//	hm.setPowerMode(POWER_MODE_ON);

	// set max transmit retry
	uint8_t transmDevTryMax = regs.ch0.l0.transmDevTryMax;
	transmDevTryMax = (transmDevTryMax < 1) ? 1 : transmDevTryMax;
	transmDevTryMax = (transmDevTryMax > 10) ? 10 : transmDevTryMax;
	dParm.maxRetr = transmDevTryMax;

	// set altitude
	int16_t altitude = regs.ch0.l0.altitude[1] | (regs.ch0.l0.altitude[0] << 8);
	altitude = (altitude < -500) ? -500 : altitude;
	altitude = (altitude > 10000) ? 10000 : altitude;
	sensTHPL.setAltitude(altitude);

	#ifdef SER_DBG
		Serial << F("config changed, data: ") << pHex(data,len) << '\n';
//		Serial << F("lowBatLimit: ") << regs.ch0.l0.lowBatLimit << '\n';
//		Serial << F("ledMode: ") << regs.ch0.l0.ledMode << '\n';
//		Serial << F("burstRx: ") << regs.ch0.l0.burstRx << '\n';
//		Serial << F("transmDevTryMax: ") << transmDevTryMax << '\n';
//		Serial << F("altitude: ") << altitude << '\n';
	#endif
}

void cmdStatusRequest(uint8_t *data, uint8_t len) {
	Serial << F("status request, data: ") << pHex(data,len) << '\n';
	_delay_ms(50);
}

void HM_Remote_Event(uint8_t *data, uint8_t len) {
	Serial << F("remote event, data: ") << pHex(data,len) << '\n';
}

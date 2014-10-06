#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <Wire.h>																// i2c library, needed for bmp085 or bmp180

/**
 * Must set in AskSinMain.h to !!!
 *
 * Set to 1 to read address data from end of bootloader section
 * Set to 0 to read address data from program space
 */
#define USE_ADRESS_SECTION     1

#define ADDRESS_SECTION_START  0x7FF0											// Start address of data in adress section at and of bootloader

/**
 * Activate for serial debug infos
 */
#define SER_DBG

#define FIRMWARE_VERSION     0x0D												// 0.13

/*
 * Address data if we don't read them from bootloader section
 */
#if USE_ADRESS_SECTION == 0
	//#define DEVICE_TYPE        0xF1, 0x02										// The model-ID    0xF1 0x01 = DIY (HB-UW-Sen-THPL-O)
	#define DEVICE_TYPE          0xF1, 0x01										// The model-ID    0xF1 0x01 = DIY (HB-UW-Sen-THPL-I)
	#define DEVICE_SERIAL        'H','B','0','D','e','f','a','u','l','t'		// The serial 10 bytes, needed for pairing   (Default for serial flash tool)
	#define DEVICE_ADDRESS       0xAB, 0xCD, 0xEF								// The HM-ID 3 bytes, needed for pairing     (Default for flash tool)
#endif

#define BATTERY_MEASSUREMENT_FACTOR 0.164										// Faktor Spannungsteiler externe Batteriemessung
#define BATTERY_MIN_VOLTAGE         10											// minimal configuable low battery voltage level
#define BATTERY_MAX_VOLTAGE         50											// maximal configuable low battery voltage level

void getPgmSpaceData(uint8_t *buffer, uint16_t address, uint8_t len);
void getDataFromAddressSection(uint8_t *buffer, uint8_t bufferStartAddress, uint16_t sectionAddress, uint8_t dataLen);

void cmdConfigChanged(uint8_t *data, uint8_t len);
void cmdReset(uint8_t *data, uint8_t len);
void cmdStatusRequest(uint8_t *data, uint8_t len);

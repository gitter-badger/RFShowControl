/*
 * FastSPI_LED2Receiver  RFPixel Control Receiver Sketch for handling the FAST_SPI2 Release candidate.
 *
 *		Input: nRF
 *		Output: Multiple Pixel Types (configurable below)
 *
 * Created on: Mar 2013
 * Updated 3/18/2014 - Added FastLED v2 release
 * Author: Greg Scull, komby@komby.com
 *
 * Updated: May 18, 2014 - Mat Mrosko, Materdaddy, rfpixelcontrol@matmrosko.com
 *
 * License:
 *		Users of this software agree to hold harmless the creators and
 *		contributors of this software.  By using this software you agree that
 *		you are doing so at your own risk, you could kill yourself or someone
 *		else by using this software and/or modifying the factory controller.
 *		By using this software you are assuming all legal responsibility for
 *		the use of the software and any hardware it is used on.
 *
 *		The Commercial Use of this Software is Prohibited.
 */

#include <Arduino.h>
#include <EEPROM.h>
#include <FastSPI_LED2.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <SPI.h>

#include "IPixelControl.h"
#include "printf.h"
#include "RFPixelControl.h"



/********************* START OF REQUIRED CONFIGURATION ***********************/
// This configuration section is required whether you're using OTA or not.
//
// If you're using OTA, these values need to be what you configured your
// OTA configuration node to send packets on.
//
// If you're not using OTA, these values are for the actual data coming over
// your wireless komby network.
//

// DATA_RATE Description:
//		If you're using OTA, this most likely needs to be set to RF24_250KBPS.  That
//		is the speed your OTA configuration packets are transmitted at if you didn't
//		customize the OTA configuration sketch.  The OTA packets you send from the
//		configuration web page set the data rate for the RF data your RF transmitter
//		is configured for.
//
//		If you're not using OTA, set this to whatever your RF transmitter sketch was
//		set to when configuring that sketch.
//
// Valid Values:
//		RF24_250KBPS
//		RF24_1MBPS
//
// More Information:
//		http://learn.komby.com/wiki/Configuration:Data_Rate
//
#define DATA_RATE					RF24_250KBPS

// NRF_TYPE Description:
//		What board are you using to connect your nRF24L01+?
//
// Valid Values:
//		RF1 - Most RF1 Devices including RF1_12V devices
//		MINIMALIST_SHIELD - Minimalist shield designed to go atop a standard arduino
//		WM_2999_NRF - WM299 specific board
//		RFCOLOR_2_4 - RFColor24 device to control GECEs
//
// More Information:
//		http://learn.komby.com/wiki/46/rfpixelcontrol-nrf_type-definitions-explained
//
#define NRF_TYPE					RF1

// PIXEL_PROTOCOL Description:
//		What type of pixels are you trying to control?
//
// Valid Values:
//		LPD_8806
//		WS_2801
//		SM_16716
//		TM_1809
//		TM_1803
//		UCS_1903
//		WS_2811
//
// More Information:
//		http://learn.komby.com/wiki/Configuration:Pixel_Protocol
#define PIXEL_PROTOCOL				WS_2811

// PIXEL_DATA_PIN Description:
//		Arduino (or clone) output pin that will go to the data line of
//		your pixel strand.  The default, and bottom data pin for the
//		RF1 devices is 2.
//
// Valid Values:
//		Any arduino output pin, typically ~1-16
//
// More Information:
//		http://learn.komby.com/wiki/Configuration:Pixel_Data_Pin
#define PIXEL_DATA_PIN				2

// PIXEL_CLOCK_PIN Description:
//		Arduino (or clone) output pin that drives the clock line of
//		your pixel strand.  The default, and bottom clock pin for the
//		RF1 devices is 4.
//
// Valid Values:
//		Any arduino output pin, typically ~1-16
//
// More Information:
//		http://learn.komby.com/wiki/Configuration:Pixel_Clock_Pin
#define PIXEL_CLOCK_PIN				4

// PIXEL_COLOR_ORDER Description:
//		This is the order your pixel string expects the data to be sent.
//
// Valid Values:
//		RGB
//		RBG
//		GRB
//		GBR
//		BRG
//		BGR
//
// More Information:
//		http://learn.komby.com/wiki/Configuration:Pixel_Color_Order
#define PIXEL_COLOR_ORDER			RGB

// OVER_THE_AIR_CONFIG_ENABLE Description:
//		If you're using Over-The-Air configuration, set this value to 1 and skip
//		the NON-OTA CONFIGURATION section below.  Otherwise, leave this at 0 and
//		skip the OTA CONFIGURATION section below.
//
//		OTA makes it so you can make an RF node that can re-program your RF1s in
//		the field.  This means the RF1s will search for a configuration broadcast
//		for 5 seconds after power-on.  If no broadcast is found, it will read it's
//		configuration from EEPROM for the last known working configuration.
//
// Valid Values:
//		0 - do not use OTA
//		1 - use OTA
//
// More Information:
//		http://learn.komby.com/Configuration:OTA
//
#define OVER_THE_AIR_CONFIG_ENABLE	0
/********************** END OF REQUIRED CONFIGURATION ************************/



/****************** START OF NON-OTA CONFIGURATION SECTION *******************/
//
// Non-OTA Configuration:
//		If you're using Over-The-Air configuration, please skip to the
//		"OTA Configuration" of the user-customizable options.
//
// For more information on Non-OTA vs. OTA visit:
//		http://learn.komby.com/NON-OTA_vs_OTA

// LISTEN_CHANNEL Description:
//		RF Channel do you want to listen on? This needs to match the channel you've
//		configured your transmitter for broadcasting it's data.  If you're broadcasting
//		multiple universes worth of data, you'll want each transmit/listen channel to be
//		at least X channels away from each other for best results.  There is also a
//		scanner sketch available for your RF1 to log RF activity in the 2.4GHz range
//		to aid in selecting your channels.  Keep in mind, regular WiFi and many other
//		devices are in this range and can cause interference.
//
// Valid Values:
//		1-124
//
// More Information:
//		http://learn.komby.com/Configuration:Listen_Channel
//
#define LISTEN_CHANNEL				100

// HARDCODED_START_CHANNEL Description:
//		This is the start channel for this controller.  If you're transmitting 200
//		channels worth of wireless data and you'd like for this controller to respond
//		to channels 37-43 (2 pixels worth), you would configure HARDCODED_START_CHANNEL
//		to the value 37, and the HARDCODED_NUM_PIXELS to 2.
//
// Valid Values:
//		1-1020
//
// More Information:
//		http://learn.komby.com/Configuration:Hardcoded_Start_Channel
#define HARDCODED_START_CHANNEL		1

// HARDCODED_NUM_PIXELS Description:
//		This is the number of pixels this device will be reading the data from the
//		stream and controlling.  This is NOT the number of channels you need.  The
//		actual number of channels is this value times 3, one each for Red, Green,
//		and blue.
//
// Valid Values:
//		1-340
//
// More Information:
//		http://learn.komby.com/Configuration:Hardcoded_Num_Pixels
#define HARDCODED_NUM_PIXELS		20
/*********************** END OF CONFIGURATION SECTION ************************/



/******************** START OF OTA CONFIGURATION SECTION *********************/
//
// OTA Configuration:
//		If you're not using Over-The-Air configuration, please skip this
//		section, you are done configuring options.
//
// For more information on Non-OTA vs. OTA visit:
//		http://learn.komby.com/NON-OTA_vs_OTA

// RECEIVER_UNIQUE_ID Description:
//		This id should be unique for each receiver in your setup.  This value
//		determines how you will target this node for configuration using the OTA
//		transmit sketch.  You should write this somewhere on the RF1 after
//		programming so in case you need to reprogram the settings, you know which
//		device you're targeting.
//
// Valid Values:
//		1-255
//
// More Information:
//		http://learn.komby.com/Configuration:Receiver_Unique_Id
#define RECEIVER_UNIQUE_ID			33
/********************* END OF OTA CONFIGURATION SECTION **********************/



/******************** START OF ADVANCED SETTINGS SECTION *********************/
//#define DEBUG						1
#define PIXEL_TYPE					FAST_SPI

//How Bright should our LEDs start at
#define LED_BRIGHTNESS				128 //50%
/********************* END OF ADVANCED SETTINGS SECTION **********************/


//Include this after all configuration variables are set
#include "RFPixelControlConfig.h"

CRGB *leds;

//Arduino setup function.
void setup(void)
{
#ifdef DEBUG
	Serial.begin(57600);
	printf_begin();
#endif

	LEDS.setBrightness(LED_BRIGHTNESS);
	// sanity check delay - allows reprogramming if accidently blowing power w/leds
	delay(2000);

	radio.EnableOverTheAirConfiguration(OVER_THE_AIR_CONFIG_ENABLE);

	uint8_t logicalControllerNumber = 0;
	if(!OVER_THE_AIR_CONFIG_ENABLE)
	{
		radio.AddLogicalController(logicalControllerNumber, HARDCODED_START_CHANNEL, HARDCODED_NUM_PIXELS * 3, 0);
	}

	radio.Initialize(radio.RECEIVER, pipes, LISTEN_CHANNEL,DATA_RATE, RECEIVER_UNIQUE_ID);

#ifdef DEBUG
	radio.printDetails();
	Serial.print(F("PixelColorOrder: "));
	printf("%d\n", PIXEL_COLOR_ORDER);
#endif

	LEDS.setBrightness(LED_BRIGHTNESS);

	logicalControllerNumber = 0;
	leds = (CRGB*) radio.GetControllerDataBase(logicalControllerNumber++);
	int countOfPixels = radio.GetNumberOfChannels(0)/3;

#ifdef DEBUG
	Serial.print(F("Number of channels configured "));
	printf("%d\n", countOfPixels);
#endif

	#if (PIXEL_PROTOCOL == LPD_8806)
	LEDS.addLeds(new LPD8806Controller<PIXEL_DATA_PIN, PIXEL_CLOCK_PIN, PIXEL_COLOR_ORDER>(), leds, countOfPixels, 0);
	#elif (PIXEL_PROTOCOL == WS_2801)
	LEDS.addLeds(new WS2801Controller<PIXEL_DATA_PIN, PIXEL_CLOCK_PIN, PIXEL_COLOR_ORDER>(), leds, countOfPixels, 0);
	#elif (PIXEL_PROTOCOL == SM_16716)
	LEDS.addLeds(new SM16716Controller<PIXEL_DATA_PIN, PIXEL_CLOCK_PIN, PIXEL_COLOR_ORDER>(), leds, countOfPixels, 0);
	#elif (PIXEL_PROTOCOL == TM_1809)
	LEDS.addLeds(new TM1809Controller800Khz<PIXEL_DATA_PIN, PIXEL_COLOR_ORDER>(), leds, countOfPixels, 0);
	#elif (PIXEL_PROTOCOL == TM_1803)
	LEDS.addLeds(new TM1803Controller400Khz<PIXEL_DATA_PIN, PIXEL_COLOR_ORDER>(), leds, countOfPixels, 0);
	#elif (PIXEL_PROTOCOL == UCS_1903)
	LEDS.addLeds(new UCS1903Controller400Khz<PIXEL_DATA_PIN, PIXEL_COLOR_ORDER>(), leds, countOfPixels, 0);
	#elif (PIXEL_PROTOCOL == WS_2811)
	LEDS.addLeds(new WS2811Controller800Khz<PIXEL_DATA_PIN, PIXEL_COLOR_ORDER>(), leds, countOfPixels, 0);

	#else
		#error Must define PIXEL_PROTOCOL: (WS_2801,LPD_8806,WS_2811,UCS_1903,TM_1803,SM_16716)
	#endif

	//Initalize the data for LEDs
	//todo eventually this will be a bug
	memset(leds, 0, countOfPixels * sizeof(struct CRGB));
	delay (200);

#ifdef DEBUG
	radio.PrintControllerConfig();
	Serial.print(F("freeMemory()="));
	Serial.println(freeMemory());
#endif
}

void loop(void)
{
	//When Radio.Listen returns true its time to update the LEDs for all controllers, a full update was made
	if (radio.Listen())
	{
		LEDS.show();
	}
}

//+--------------------------------------------------------------------------
//
// Brakelights - (c) 2018 Dave Plummer.  Public domain not for highway use.
//
// File:        Brakelights.ino
//
// Description:
//
//   Runs a strip of WS2812B RGB LED strips.  When switches are activated,
//   offers modes for left turn, right turn, hazard, brake, police, and
//   backup.
//
//   For lab use only; not for highway use.  GPL v3.0
//
// History:     Jul-12-2018         Davepl      Created
//
//---------------------------------------------------------------------------

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <assert.h>

#define LCD_WIDTH 20
#define LCD_HEIGHT 4

LiquidCrystal_I2C lcd(0x27, LCD_WIDTH, LCD_HEIGHT);  // set the LCD address to 0x27 for a 16 chars and 2 line display

#define PIN 6

#define TOTAL_STRIP_PIXELS 144						// How many pixels in entire string
#define NUMBER_USED_PIXELS 144						// The number of pixels that we use (normally, all of them)
#define NUMBER_TURN_PIXELS 50						// How many pixels on the end will be use for turn signals

#define LEFT_TURN_PIN  PIND3						// Digital input pins 
#define RIGHT_TURN_PIN PIND2
#define STOP_PIN       PIND4
#define BACKUP_PIN     PIND5

#define COLOR_BLACK    (Adafruit_NeoPixel::Color(  0,   0,   0))
#define COLOR_WHITE    (Adafruit_NeoPixel::Color(255, 255, 255))
#define COLOR_RED      (Adafruit_NeoPixel::Color(255,   0,   0))
#define COLOR_DARK_RED (Adafruit_NeoPixel::Color( 64,   0,   0))
#define COLOR_BLUE     (Adafruit_NeoPixel::Color(  0,   0, 255))
#define COLOR_AMBER    (Adafruit_NeoPixel::Color(255,  48,   0))
#define COLOR_GREEN    (Adafruit_NeoPixel::Color(  0, 255,   0))
#define COLOR_PURPLE   (Adafruit_NeoPixel::Color(255,   0, 255))
#define COLOR_YELLOW   (Adafruit_NeoPixel::Color(255, 255,   0))

#include "LightingEvents.h"

	// Parameter 1 = number of pixels in strip
	// Parameter 2 = Arduino pin number (most are valid)
	// Parameter 3 = pixel type flags, add together as needed:
	//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
	//   NEO_BRG     Pixels are wired for BRG bitstream order (uncommon I think)

Adafruit_NeoPixel _strip = Adafruit_NeoPixel(TOTAL_STRIP_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

BrakingEvent   * pBraking   = nullptr;
BackupEvent	   * pBackup    = nullptr;
SignalEvent    * pLeftTurn  = nullptr;
SignalEvent    * pRightTurn = nullptr;
SignalEvent    * pHazard    = nullptr;
PoliceLightBar * pPoliceBar = nullptr;


// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

// We keep track of when each vaious feature display started so that we know how
// far we are into it's animation, etc.

unsigned long backupStartTime = 0;
unsigned long brakeStartTime  = 0;
unsigned long turnStartTime   = 0;

void setup()
{
	pBraking   = new BrakingEvent(&_strip);
	pBackup    = new BackupEvent(&_strip);
	pLeftTurn  = new SignalEvent(&_strip, SignalEvent::SIGNAL_STYLE::LEFT_TURN);
	pRightTurn = new SignalEvent(&_strip, SignalEvent::SIGNAL_STYLE::RIGHT_TURN);
	pHazard    = new SignalEvent(&_strip, SignalEvent::SIGNAL_STYLE::HAZARD);
	pPoliceBar = new PoliceLightBar(&_strip);

	Serial.begin(115200);
	Serial.println("BrakeLight Startup");

	_strip.begin();
	_strip.setBrightness(255);
	for (uint16_t i = 0; i < NUMBER_USED_PIXELS; i++)
	{
		_strip.setPixelColor(i, _strip.Color(0, 0, 0));
		_strip.show();
	}
	pinMode(LEFT_TURN_PIN, INPUT_PULLUP);
	pinMode(RIGHT_TURN_PIN, INPUT_PULLUP);
	pinMode(STOP_PIN, INPUT_PULLUP);
	pinMode(BACKUP_PIN, INPUT_PULLUP);

	lcd.init();
	lcd.backlight();
	lcd.setCursor(0, 0);
	lcd.print("Starting...");
}

void loop()
{
	processAndDisplayInputs();
	delay(1);
	return;
}

// processAndDisplayInputs()
// 
// Main update loop

void processAndDisplayInputs()
{
	// Backup

	if (digitalRead(BACKUP_PIN) == 0)
	{
		if (pBackup->GetActive() == false)
			pBackup->Begin();
	}
	else
	{
		if (pBackup->GetActive() == true)
			pBackup->End();
	}
		
	// Hazards

	if (digitalRead(LEFT_TURN_PIN) == 0 && digitalRead(RIGHT_TURN_PIN) == 0 && digitalRead(STOP_PIN) == 0)
	{
		if (pPoliceBar->GetActive() == false)
			pPoliceBar->Begin();
	}
	else
	{
		if (pPoliceBar->GetActive() == true)
			pPoliceBar->End();

		// Braking  

		if (digitalRead(STOP_PIN) == 0)
		{
			if (pBraking->GetActive() == false)
				pBraking->Begin();
		}
		else
		{
			if (pBraking->GetActive() == true)
				pBraking->End();
		}

		if (digitalRead(LEFT_TURN_PIN) == 0 && digitalRead(RIGHT_TURN_PIN) == 0)
		{
			if (pHazard->GetActive() == false)
				pHazard->Begin();
		}
		else
		{
			if (pHazard->GetActive() == true)
				pHazard->End();

			// Left turn

			if (digitalRead(LEFT_TURN_PIN) == 0)
			{
				if (pLeftTurn->GetActive() == false)
					pLeftTurn->Begin();
			}
			else
			{
				if (pLeftTurn->GetActive() == true)
					pLeftTurn->End();
			}

			// Right turn

			if (digitalRead(RIGHT_TURN_PIN) == 0)
			{
				if (pRightTurn->GetActive() == false)
					pRightTurn->Begin();
			}
			else
			{
				if (pRightTurn->GetActive() == true)
					pRightTurn->End();
			}
		}
	}

	pBackup->Draw();
	pLeftTurn->Draw();
	pRightTurn->Draw();
	pBraking->Draw();
	pHazard->Draw();
	pPoliceBar->Draw();

	char szBuf[LCD_WIDTH*LCD_HEIGHT + 1];
	lcd.setCursor(0, 0);
	szBuf[0] = '\0';
	strcpy(szBuf, pBraking->GetActive() ? "STOP" : "    ");
	strcat(szBuf, ":");
	strcat(szBuf, pLeftTurn->GetActive() ? "LEFT" : "    ");
	strcat(szBuf, ":");
	strcat(szBuf, pRightTurn->GetActive() ? "RIGHT" : "     ");
	strcat(szBuf, ":");
	strcat(szBuf, pBackup->GetActive() ? "BACK" : "    ");
	lcd.print(szBuf);
}

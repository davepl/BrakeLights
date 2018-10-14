#pragma once
#include <Adafruit_NeoPixel.h>

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x)/sizeof(*x))
#endif

// LightingEvent
//
// Base class for things like turn signals, braking, backing up, etc.
// All derived classes must implement the abstract base Draw() function.
// All derived methods MUST call their base class implementations properly
// in order for the timers to work, etc.
//
// In short, you create a derived class and then call Begin() when the
// event starts (like braking), and Update keeps track of the currentr
// state.  Draw() actually renders the current state of the effect to
// the light strip.

class LightingEvent
{
	unsigned long       _eventStart;
	bool                _active;

  protected:

	Adafruit_NeoPixel * _pStrip;

  public:

	LightingEvent(Adafruit_NeoPixel * pStrip)  
	{
		_pStrip = pStrip;
		_eventStart = 0;
		_active = false;
	}
	
	float TimeElapsedTotal()						// Total time event has been running in fractional seconds
	{
		return (millis() - _eventStart) / 1000.0f;
	}

	bool GetActive()
	{
		return _active;
	}

	virtual void Begin()    
	{
		_active = true;
		_eventStart = millis();
	};

	virtual void End()      
	{
		_active = false;
		for (int i = 0; i < NUMBER_USED_PIXELS; i++)
			_pStrip->setPixelColor(i, COLOR_BLACK);
		_pStrip->show();
	};

	virtual void Draw()    = 0;
};

class BackupEvent : public LightingEvent
{
	const float BLOOM_TIME = 0.25f;

  public:

	BackupEvent(Adafruit_NeoPixel * pStrip) : LightingEvent(pStrip)
	{
	}

	virtual void Draw() override
	{
		if (false == GetActive())
			return;

		// The backup light illuminates the whole strip in white.  It quickly "blooms"
		// out from the center to fill the strip.

		float fPercentComplete = min(TimeElapsedTotal() / BLOOM_TIME, 1.0f);
		int cLEDs  = NUMBER_USED_PIXELS * fPercentComplete;
		int iFirst = (NUMBER_USED_PIXELS / 2) - (cLEDs / 2);
		int iLast  = (NUMBER_USED_PIXELS / 2) + (cLEDs / 2);
		
		for (int i = 0; i < NUMBER_USED_PIXELS; i++)
		{
			if (i < iFirst || i > iLast)
				_pStrip->setPixelColor(i, COLOR_BLACK);
			else
				_pStrip->setPixelColor(i, COLOR_WHITE);
		}
		_pStrip->show();
	}
};

class BrakingEvent : public LightingEvent
{
	const float BRAKE_STROBE_DURATION = 0.5f;
	const float BLOOM_START_SIZE      = 0.10;
	const float BLOOM_TIME            = 0.50;

  public:

	BrakingEvent(Adafruit_NeoPixel * pStrip) : LightingEvent(pStrip)
	{
	}
	
	// BrakingEvent::Draw
	//
	// The strobe flash happens too fast to wait for the loop pump system, so we do a full 50ms
	// cycle of it here (30 on, 20 off).  That way it is crisp and accurate but we still never
	// block the system for more than 50ms, which is sort of a limit I've set

	virtual void Draw() override
	{
		if (false == GetActive())
			return;

		if (TimeElapsedTotal() < BRAKE_STROBE_DURATION)
		{
			float timeElapsed = TimeElapsedTotal();
			float pctComplete = min(1.0f, (timeElapsed / BLOOM_TIME) + BLOOM_START_SIZE);
			float unusedEachEnd = (1.0f - pctComplete) * NUMBER_USED_PIXELS / 2;

			for (uint16_t i = unusedEachEnd; i < NUMBER_USED_PIXELS - unusedEachEnd; i++)
				_pStrip->setPixelColor(i, COLOR_RED);
			_pStrip->show();
			
			delay(30);

			timeElapsed = TimeElapsedTotal();
			pctComplete = min(1.0f, (timeElapsed / BLOOM_TIME) + BLOOM_START_SIZE);
			unusedEachEnd = (1.0f - pctComplete) * NUMBER_USED_PIXELS / 2;

			for (uint16_t i = unusedEachEnd; i < NUMBER_USED_PIXELS - unusedEachEnd; i++)
				_pStrip->setPixelColor(i, COLOR_DARK_RED);
			_pStrip->show();
			
			delay(20);

			return;
		}
		for (uint16_t i = 0; i < NUMBER_USED_PIXELS; i++)
			_pStrip->setPixelColor(i, COLOR_RED);
		_pStrip->show();
		
	}
};

// SignalEvent
//
// Handles let turns, right turtns, and standard hazards (simply both signals at once)

class SignalEvent : public LightingEvent
{
  public:

	enum SIGNAL_STYLE
	{
		INVALID = 0,
		LEFT_TURN,
		RIGHT_TURN,
		HAZARD
	};

  private:

	const float SequentialBloomStart = 0.00f;
	const float SequentialBloomTime  = 0.50f;

	const float SequentialHoldStart  = SequentialBloomStart + SequentialBloomTime;
	const float SequentialHoldTime   = 0.25f;

	const float SequentialFadeStart  = SequentialHoldStart + SequentialHoldTime;
	const float SequentialFadeTime   = 0.125f;

	const float SequentialOffStart   = SequentialFadeStart + SequentialFadeTime;
	const float SequentialOffTime    = 0.25f;

	const float SequentialCycleTime  = SequentialOffStart + SequentialOffTime;

	// SetTurnLED
	//
	// Depending on which way the signal is turning, light up it's LED on the correct
	// end of the light strip

	void SetTurnLED(int i, uint32_t color)
	{
		if (_style == LEFT_TURN || _style == HAZARD)
			_pStrip->setPixelColor(i, color);

		if (_style == RIGHT_TURN || _style == HAZARD)
			_pStrip->setPixelColor(NUMBER_USED_PIXELS - 1 - i, color);
	}

	SIGNAL_STYLE _style;

  public:

	SignalEvent(Adafruit_NeoPixel * pStrip) 
		: LightingEvent(pStrip)
	{
	}

	SignalEvent(Adafruit_NeoPixel * pStrip, SIGNAL_STYLE style) 
		: LightingEvent(pStrip),
		  _style(style)
	{
	}

	virtual void Draw() override
	{
		if (false == GetActive())
			return;

		float fCyclePosition = fmod(TimeElapsedTotal(), SequentialCycleTime);

		if (fCyclePosition > SequentialOffStart)
		{
			for (int i = 0; i < NUMBER_TURN_PIXELS; i++)
				SetTurnLED(i, COLOR_BLACK);
		}
		else if (fCyclePosition > SequentialFadeStart)
		{
			fCyclePosition -= SequentialFadeStart;
			float pctComplete = fCyclePosition / SequentialFadeTime;
			int cPixelsLit = NUMBER_TURN_PIXELS - (NUMBER_TURN_PIXELS * pctComplete);
			for (int i = 0; i < NUMBER_TURN_PIXELS; i++)
			{
				if (i < cPixelsLit)
					SetTurnLED(i, COLOR_AMBER);
				else
					SetTurnLED(i, COLOR_BLACK);
			}
		}
		else if (fCyclePosition > SequentialHoldStart)
		{
			for (int i = 0; i < NUMBER_TURN_PIXELS; i++)
				SetTurnLED(i, COLOR_AMBER);
		}
		else
		{
			assert(fCyclePosition <= SequentialBloomTime);
			float pctComplete = fCyclePosition / SequentialBloomTime;
			int cPixelsLit = (NUMBER_TURN_PIXELS * pctComplete);
			for (int i = 0; i < NUMBER_TURN_PIXELS; i++)
			{
				if (i >= (NUMBER_TURN_PIXELS - cPixelsLit))
					SetTurnLED(i, COLOR_AMBER);
				else
					SetTurnLED(i, COLOR_BLACK);
			}

		}
		_pStrip->show();

	}
};

// PoliceLightBarState
//
// The police bar breaks the light strip into 8 sections, and then alternates patterns based on a table

struct PoliceLightBarState
{
	uint32_t sectionColor[8];
	uint32_t duration;
};

class PoliceLightBar : public LightingEvent
{
	static const PoliceLightBarState _PoliceBarStates1[11];		// Sadly we must spec arraysize because its a forward declaration

  public:  

	PoliceLightBar(Adafruit_NeoPixel * pStrip)
		: LightingEvent(pStrip)
	{
	}

	virtual void Draw() override
	{
		if (false == GetActive())
			return;

		const size_t sectionSize  = NUMBER_USED_PIXELS / 8;

		for (size_t row = 0; row < ARRAYSIZE(_PoliceBarStates1); row++)
		{
			for (uint16_t i = 0; i < NUMBER_USED_PIXELS; i++)
			{
				int iSection = i / sectionSize;
				const uint32_t color = (_PoliceBarStates1[row].sectionColor[iSection]);
				_pStrip->setPixelColor(i, color);
			}
			_pStrip->show();
			delay(_PoliceBarStates1[row].duration);
		}
	}
};

const PoliceLightBarState PoliceLightBar::_PoliceBarStates1[] =
{
	{  { COLOR_BLUE,  COLOR_BLUE,  COLOR_RED,   COLOR_RED,   COLOR_BLUE,  COLOR_BLUE,  COLOR_RED,   COLOR_RED   }, 200 },
	{  { COLOR_RED,   COLOR_RED,   COLOR_BLUE,  COLOR_BLUE,  COLOR_RED,   COLOR_RED,   COLOR_BLUE,  COLOR_BLUE  }, 200 },
	{  { COLOR_WHITE, COLOR_BLUE,  COLOR_RED,   COLOR_RED,   COLOR_BLUE,  COLOR_BLUE,  COLOR_RED,   COLOR_RED   },  20 },
	{  { COLOR_BLUE,  COLOR_BLUE,  COLOR_RED,   COLOR_RED,   COLOR_BLUE,  COLOR_BLUE,  COLOR_RED,   COLOR_WHITE },  20 },
	{  { COLOR_BLUE,  COLOR_WHITE, COLOR_RED,   COLOR_RED,   COLOR_BLUE,  COLOR_BLUE,  COLOR_RED,   COLOR_RED   },  20 },
	{  { COLOR_BLUE,  COLOR_BLUE,  COLOR_RED,   COLOR_RED,   COLOR_BLUE,  COLOR_BLUE,  COLOR_WHITE, COLOR_RED   },  20 },
	{  { COLOR_BLUE,  COLOR_BLUE,  COLOR_WHITE, COLOR_RED,   COLOR_BLUE,  COLOR_BLUE,  COLOR_RED,   COLOR_RED   },  20 },
	{  { COLOR_BLUE,  COLOR_BLUE,  COLOR_RED,   COLOR_RED,   COLOR_BLUE,  COLOR_WHITE, COLOR_RED,   COLOR_RED   },  20 },
	{  { COLOR_BLUE,  COLOR_BLUE,  COLOR_RED,   COLOR_WHITE, COLOR_BLUE,  COLOR_BLUE,  COLOR_RED,   COLOR_RED   },  20 },
	{  { COLOR_BLUE,  COLOR_BLUE,  COLOR_RED,   COLOR_RED,   COLOR_WHITE, COLOR_BLUE,  COLOR_RED,   COLOR_RED   },  20 },
	{  { COLOR_RED,   COLOR_RED,   COLOR_BLUE,  COLOR_BLUE,  COLOR_RED,   COLOR_RED,   COLOR_BLUE,  COLOR_BLUE  }, 200 },
};


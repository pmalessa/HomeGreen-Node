/*
 * display.c
 *
 *  Created on: 29.01.2019
 *      Author: pmale
 */

#include "display.hpp"

Display::animation_t Display::currentAnimation = ANIMATION_NONE;
bool Display::isInitialized = false;
bool Display::animationDone = false;

uint8_t numToByte[] =
{
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F, // 9
};

uint8_t bootAnimation[] =
{
		0x76, // H
		0x79, // E
		0x38, // L
		0x38, // L
		0x3F, // O
		0x00, // 0
		0x00, // 0
		0x00, // 0
		0x00, // 0
		0x00, // 0
};

#define PUMPANIMATION_FRAMES 12
uint32_t pumpanimation[PUMPANIMATION_FRAMES] = {
		0x01010000,
		0x00010100,
		0x00000101,
		0x00000003,
		0x00000006,
		0x0000000C,
		0x00000808,
		0x00080800,
		0x08080000,
		0x18000000,
		0x30000000,
		0x21000000
};

//---------------------------------------



void Display::Init()
{
	currentAnimation = ANIMATION_NONE;
	tm1637_Init();
	dotmask = 0;
	blinkCounter = 0;
	blinkingEnabled = false;
	Clear();
	SetBrightness(7);
	displayTimer.setTimeStep(REFRESH_RATE);
	timeoutTimer.setTimeStep(DISPLAY_TIMEOUT_S*1000);
	isInitialized = true;
}

void Display::DeInit()
{
	isInitialized = false;
	Clear();
	//display_update();
	tm1637_deInit();
}

void Display::Clear()
{
	for(uint8_t i=0;i<6;i++)
	{
		dig[i]=0;
	}
	dotmask = 0;
}

//val = (0.1 .. 1.0,1.1 .. 10.0,11.0 .. 99.0) * 10 = 1..990
void Display::SetValue(digit_t digit, uint16_t val)
{
	uint8_t d = digit<<1;
	if(val > 990)	//set high limit
	{
		val = 990;
	}
	if(val > 99) //higher than 9.9
	{
		SetByte(d,numToByte[val/100]);
		SetByte(d+1,numToByte[(val%100)/10]);
		setDot(d, 0);
		setDot(d+1, 1);	//dot at second position
	}
	else
	{
		SetByte(d,numToByte[val/10]);
		SetByte(d+1,numToByte[val%10]);
		setDot(d, 1);	//dot at first position
		setDot(d+1, 0);
	}
}

void Display::SetByte(uint8_t pos, uint8_t byte)
{
	if(byte & 0x80)
	{
		setDot(pos,1);
	}
	else
	{
		setDot(pos,0);
	}
	dig[pos]=byte;
}

//0..7
void Display::SetBrightness(uint8_t val)
{
	if(val < 8)
	{
		brightness = val;
		tm1637_setBrightness(val);
	}
}

void Display::EnableBlinking(digit_t digit)
{
	blinkingEnabled = digit+1;
	blinkCounter = 0;
}

void Display::DisableBlinking()
{
	blinkingEnabled = 0;
}

void Display::StartAnimation(animation_t animation)
{
	currentAnimation = animation;
	animationDone = false;
}

void Display::StopAnimation()
{
	currentAnimation = ANIMATION_NONE;
	animationDone = true;
}

bool Display::IsAnimationDone()
{
	return animationDone;
}

void Display::Draw()
{
	static uint8_t state = 0, toggle = 0;
	if(!isInitialized) return;	//dont draw if not initialized

	if(displayTimer.isTimeUp())	//only draw every 100ms
	{
		switch (currentAnimation)
		{
		case ANIMATION_NONE:	//No Animation, Display or Blink values
			if(blinkCounter > 8)
			{
				blinkCounter = 0;
			}
			else
			{
				blinkCounter++;
			}
			if(blinkingEnabled && blinkCounter > 5)	//Blinking
			{
				switch (blinkingEnabled) {
					case 1: //DIGIT_INTERVAL
						dsend(0,0);
						dsend(1,0);
						dsend(2,dig[2]);
						dsend(3,dig[3]);
						dsend(4,dig[4]);
						dsend(5,dig[5]);
						break;
					case 2: //DIGIT_DURATION
						dsend(0,dig[0]);
						dsend(1,dig[1]);
						dsend(2,0);
						dsend(3,0);
						dsend(4,dig[4]);
						dsend(5,dig[5]);
						break;
					case 3: //DIGIT_COUNTDOWN
						dsend(0,dig[0]);
						dsend(1,dig[1]);
						dsend(2,dig[2]);
						dsend(3,dig[3]);
						dsend(4,0);
						dsend(5,0);
						break;
					default:
						break;
				}
			}
			else
			{
				for(uint8_t i=0;i<6;i++)
				{
					dsend(i,dig[i]);	//send all digits
				}
			}
			break;
		
		case ANIMATION_BOOT:
			if(state == 0)	//clear first
			{
				Clear();
			}
			if(state > 11)	//done
			{
				state = 0;
				animationDone = true;
			}
			SetByte(state%5, bootAnimation[state]);
			state++;
			break;

		case ANIMATION_PUMP:
			dotmask = 0;
			dig[0] = (pumpanimation[state]&0xFF000000)>>24;
			dig[1] = (pumpanimation[state]&0x00FF0000)>>16;
			dig[2] = (pumpanimation[state]&0x0000FF00)>>8;
			dig[3] = (pumpanimation[state]&0x000000FF)>>0;
			//SetValue(DIGIT_COUNTDOWN,countdown/6);	//get Pump countdown
			state++;
			if(state == 12)
			{
				state = 0;
				animationDone = true;
			}
			break;

		case ANIMATION_WAKE:
			if(state == 0)	//clear first
			{
				Clear();
				SetBrightness(7);
			}
			if(state > 5)	//done
			{
				state = 0;
				animationDone = true;
			}
			switch (toggle) {
				case 0:
					SetByte(state, 0x30);
					toggle = 1;
					break;
				default:
					SetByte(state, 0x36);
					toggle = 0;
					state++;
					break;
			}
			SetBrightness(7);
			break;

		case ANIMATION_CHARGE:
			static uint8_t speedCounter = 0, speedValue = 4;
			if(state == 1)	//clear first
			{
				Clear();
				SetBrightness(7);
			}
			if(speedCounter < speedValue)
			{
				speedCounter++;
			}
			else
			{
				speedCounter = 0;
				if(state > 6)	//done
				{
					state = 0;
					animationDone = true;
				}
				switch (toggle) {
					case 0:
						SetByte(state-1, 0x30);
						toggle = 1;
						break;
					default:
						SetByte(state-1, 0x36);
						toggle = 0;
						state++;
						break;
				}
				SetBrightness(7);
			}
			break;
		case ANIMATION_FADE:
			static uint8_t min = 1;
			if(state == 0)
			{
				if(brightness > min)
				{
					Display::SetBrightness(brightness-1);
				}
				else
				{
					state = 1;
				}
			}
			else
			{
				if(brightness < 7)
				{
					Display::SetBrightness(brightness+1);
				}
				else //done
				{
					state = 0;
					animationDone = true;
				}
				
			}
			break;
		default:
			break;
		}
	}
}

void Display::ResetTimeout()
{
	timeoutTimer.reset();
}

bool Display::IsTimeout()
{
	return timeoutTimer.isTimeUp();
}
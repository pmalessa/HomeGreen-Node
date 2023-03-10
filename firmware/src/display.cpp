/*
 * display.c
 *
 *  Created on: 29.01.2019
 *      Author: pmale
 */

#include "display.hpp"
#include "pump.hpp"

Display::animation_t Display::currentAnimation = ANIMATION_NONE;
bool Display::isInitialized = false;
bool Display::animationDone = false;
bool Display::resetAnimation = false;
DeltaTimer Display::displayTimer, Display::timeoutTimer;
uint8_t Display::dotmask, Display::brightness, Display::dig[6], Display::blinkingEnabled, Display::blinkCounter;

uint8_t numToByteArray[] =
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
	0x77, // A
	0x7C, // b
	0x58, // c
	0x5E, // d
	0x79, // E
	0x71  // F
};

#define PUMPANIMATION_FRAMES 4
uint16_t pumpanimation[PUMPANIMATION_FRAMES] = {
		0x0108,
		0x0801,
		0x1002,
		0x2004
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
	resetAnimation = false;
}

void Display::Sleep()
{
	isInitialized = false;
	Clear();
	tm1637_deInit();
}

void Display::Wake()
{
	TM_DDR |= TM_BIT_CLK | TM_BIT_DAT;
	TM_OUT |= TM_BIT_CLK;
	isInitialized = true;
}

void Display::Clear()
{
	for(uint8_t i=0;i<6;i++)
	{
		dig[i]=0;
	}
	dotmask = 0;
	for(uint8_t i=0;i<6;i++)
	{
		dsend(i,dig[i]);	//send all digits
	}
}

void Display::Full()
{
	for(uint8_t i=0;i<6;i++)
	{
		dig[i]=0xFF;
	}
	dotmask = 0xFF;
	for(uint8_t i=0;i<6;i++)
	{
		dsend(i,dig[i]);	//send all digits
	}
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
		SetByte(d,numToByte(val/100));
		SetByte(d+1,numToByte((val%100)/10));
		setDot(d, 0);
		setDot(d+1, 1);	//dot at second position
	}
	else
	{
		SetByte(d,numToByte(val/10));
		SetByte(d+1,numToByte(val%10));
		setDot(d, 1);	//dot at first position
		setDot(d+1, 0);
	}
}

//val = +/-(0.1 .. 1.0,1.1 .. 10.0,11.0 .. 99.0) * 10 = +/- 1..999
//position = 0..2 -> 4 digits long
void Display::SetNegValue(uint8_t position, int16_t val)
{
	if(position>2) position = 2;	//high limit
	if(val > 999)	//set high limit
	{
		val = 999;
	}
	if(val < -999)	//set low limit
	{
		val = -999;
	}

	if(val < 0)
	{
		SetByte(position,0x40);	//Negative, minus
		val = abs(val);
	}
	else
	{
		SetByte(position,0x00);	//Positive, nothing
	}
	
	SetByte(position+1,numToByte(val/100));
	SetByte(position+2,numToByte((val%100)/10));
	SetByte(position+3,numToByte(val%10));
	setDot(position+2, 1);	//dot at second position
}

//val = (0.1 .. 9999.9) * 10 = +/- 1..99999
//position = 0..1 -> 5 digits long
void Display::Set4DigValue(uint8_t position, uint32_t val)
{
	if(position>1) position = 1;	//high limit
	if(val > 99999)	//set high limit
	{
		val = 99999;
	}
	SetByte(position,numToByte(val/10000));
	SetByte(position+1,numToByte((val%10000)/1000));
	SetByte(position+2,numToByte((val%1000)/100));
	SetByte(position+3,numToByte((val%100)/10));
	SetByte(position+4,numToByte(val%10));
	setDot(position+3, 1);	//dot at third position
}

void Display::SetByte(uint8_t pos, uint8_t byte)
{
	setDot(pos,byte&DEC_DOT);
	dig[pos]=byte;
}

uint8_t Display::numToByte(uint8_t num)
{
	return numToByteArray[num&0xF];
}

//0..7
void Display::SetBrightness(uint8_t val)
{
	brightness = val&0x7;
	tm1637_setBrightness(brightness);
}

void Display::EnableBlinking(digit_t digit)
{
	blinkingEnabled = digit+1;
	blinkCounter = 6; //switch to off state first
}

void Display::ResetBlinkCounter()
{
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
	resetAnimation = true;
}

void Display::StopAnimation()
{
	currentAnimation = ANIMATION_NONE;
	animationDone = true;
	resetAnimation = true;
}

bool Display::IsAnimationDone()
{
	return animationDone;
}

void Display::ShowError(Data::statusBit_t status)
{
	Display::SetByte(0,0x73); //P
	Display::SetByte(2,0x40); //-
	Display::SetByte(3,0x79); //E
	Display::SetByte(4,0x50); //r
	Display::SetByte(5,0x50); //r

	switch (status)
	{
	case Data::STATUS_PB_ERR:
		Display::SetByte(1,0x7C); //b
		break;
	case Data::STATUS_P1_ERR:
		Display::SetByte(1,Display::numToByte(1)); //1
		break;
	case Data::STATUS_P2_ERR:
		Display::SetByte(1,Display::numToByte(2)); //2
		break;
	case Data::STATUS_P3_ERR:
		Display::SetByte(1,Display::numToByte(3)); //3
		break;
	case Data::STATUS_EP_ERR:
		Display::SetByte(0,0x79); //E
		Display::SetByte(1,0x73); //P
		break;
	}
}

void Display::ForceDraw()
{
	for(uint8_t i=0;i<6;i++)
	{
		dsend(i,dig[i]);	//send all digits
	}
}

void Display::Draw()
{
	static uint8_t state = 0, toggle = 0;
	if(!isInitialized) return;	//dont draw if not initialized

	if(displayTimer.isTimeUp())	//only draw every 100ms
	{
		if(resetAnimation)
		{
			resetAnimation = false;
			state = 0;
			toggle = 0;
		}
		switch (currentAnimation)
		{
		case ANIMATION_NONE:	//No Animation, Display or Blink values
			blinkCounter++;
			if(blinkCounter > 11)	//blinkCounter 0..11
			{
				blinkCounter = 0;
			}
			if(blinkingEnabled && blinkCounter > 5)	//Off 0..5
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
			else	//On 6..11
			{
				for(uint8_t i=0;i<6;i++)
				{
					dsend(i,dig[i]);	//send all digits
				}
			}
			break;

		case ANIMATION_PUMP:
			dotmask = 0;
			dig[0] = 0x73; //P
			dig[1] = numToByte(Pump::getCurrentPump()+1);
			dig[2] = (pumpanimation[state]&0xFF00)>>8;
			dig[3] = (pumpanimation[state]&0x00FF)>>0;
			SetValue(DIGIT_COUNTDOWN,Pump::getCountdown()/6);	//get Pump countdown
			state++;
			if(state == PUMPANIMATION_FRAMES)
			{
				state = 0;
				animationDone = true;
			}
			for(uint8_t i=0;i<6;i++)
			{
				dsend(i,dig[i]);	//send all digits
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
				break;
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
			for(uint8_t i=0;i<6;i++)
			{
				dsend(i,dig[i]);	//send all digits
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
					Timer::shortSleep(400);
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
					break;
				}
				
			}
			for(uint8_t i=0;i<6;i++)
			{
				dsend(i,dig[i]);	//send all digits
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
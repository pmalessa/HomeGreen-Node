//
// AVR TM1637 "Library" v1.01
// Enables control of TM1637 chip based modules, using direct port access.
//
// Copyright (c) 2015 IronCreek Software
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include "PLATFORM.h"
#include "tm1637.h"

#define TM_CLK_LOW()            (TM_OUT &= ~TM_BIT_CLK)
#define TM_CLK_HIGH()           (TM_OUT |= TM_BIT_CLK)
#define TM_DAT_LOW()            (TM_OUT &= ~TM_BIT_DAT)
#define TM_DAT_HIGH()           (TM_OUT |= TM_BIT_DAT)

#define TM_CLK_FLOAT()

// Instructions
#define TM_DATA_CMD             0x40
#define TM_DISP_CTRL            0x80
#define TM_ADDR_CMD             0xC0

// Data command set
#define TM_WRITE_DISP           0x00
#define TM_READ_KEYS            0x02
#define TM_FIXED_ADDR           0x04

// Display control command
#define TM_DISP_PWM_MASK        0x07 // First 3 bits are brightness (PWM controlled)
#define TM_DISP_ENABLE          0x08

#define DELAY_US                50


//      Bits:                 Hex:
//        -- 0 --               -- 01 --
//       |       |             |        |
//       5       1            20        02
//       |       |             |        |
//        -- 6 --               -- 40 --
//       |       |             |        |
//       4       2            10        04
//       |       |             |        |
//        -- 3 --  .7           -- 08 --   .80

void start()
{
	TM_CLK_HIGH();
	TM_DAT_HIGH();
	_delay_us(DELAY_US);

	TM_DAT_LOW();
	_delay_us(DELAY_US);
}

void stop()
{
	TM_CLK_LOW();
	_delay_us(DELAY_US);

	TM_CLK_HIGH();
	TM_DAT_LOW();
	_delay_us(DELAY_US);

	TM_DAT_HIGH();
}

void send(uint8_t b)
{
	// Clock data bits
	for (uint8_t i = 8; i; --i, b >>= 1)
	{
		TM_CLK_LOW();
		if (b & 1)
			TM_DAT_HIGH();
		else
			TM_DAT_LOW();
		_delay_us(DELAY_US);

		TM_CLK_HIGH();
		_delay_us(DELAY_US);
	}

	// Clock out ACK bit; not checking if it worked...
	TM_CLK_LOW();
	TM_DAT_LOW();
	_delay_us(DELAY_US);

	TM_CLK_HIGH();
	_delay_us(DELAY_US);
}

void send_cmd(uint8_t cmd)
{
	start();
	send(cmd);
	stop();
}

void send_data(uint8_t addr, uint8_t data)
{
	send_cmd(TM_DATA_CMD | TM_FIXED_ADDR);

	start();
	send(TM_ADDR_CMD | addr);
	send(data);
	stop();

	_delay_us(DELAY_US);
}

void tm1637_Init()
{
	TM_DDR |= TM_BIT_CLK | TM_BIT_DAT;
	TM_OUT |= TM_BIT_CLK;

    send_cmd(TM_DATA_CMD | TM_WRITE_DISP);
    send_cmd(TM_DISP_CTRL | TM_DISP_ENABLE | TM_DISP_PWM_MASK);
}

void tm1637_deInit()
{
	TM_CLK_LOW();
	TM_DAT_LOW();
}

void tm1637_setByte(uint8_t position, uint8_t b)
{
    send_data(position, b);
}

void tm1637_setBrightness(uint8_t brightness)
{
    send_cmd(TM_DISP_CTRL | TM_DISP_ENABLE | (brightness & TM_DISP_PWM_MASK));
}

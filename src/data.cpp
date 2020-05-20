/*
 * data.c
 *
 *  Created on: 30.01.2019
 *      Author: pmale
 */

#include "data.hpp"
#include "avr/eeprom.h"

uint16_t Data::data[DATA_SIZE] = {0};
int16_t Data::tempdata[TEMPDATA_SIZE] = {0};
uint32_t Data::countdown = 0;
uint8_t Data::status = 0, Data::ignoreStatus = 0;

void Data::Init()
{
	if(!(eeprom_read_dword((uint32_t *)ADR_INIT_CONST) == DATA_INIT_CONST))
	{
		setDefault();
	}
	data[DATA_INTERVAL] = eeprom_read_word((uint16_t *)ADR_INTERVAL);
	data[DATA_DURATION1] = eeprom_read_word((uint16_t *)ADR_DURATION1);
	data[DATA_DURATION2] = eeprom_read_word((uint16_t *)ADR_DURATION2);
	data[DATA_DURATION3] = eeprom_read_word((uint16_t *)ADR_DURATION3);
	data[DATA_TOTAL_RUNTIME] = eeprom_read_word((uint16_t *)ADR_TOTAL_RUNTIME);
	tempdata[DATA_SETUP_TEMP] = eeprom_read_word((uint16_t *)ADR_SETUP_TEMP);
	status = eeprom_read_word((uint16_t *)ADR_STATUS);
	ignoreStatus = eeprom_read_word((uint16_t *)ADR_IGNORE_STATUS);
	resetCountdown();
}

void Data::Increment(data_type_t data_type)
{
	if(data[data_type] < 100)	//smaller 100, +1 steps
	{
		data[data_type] += 1;
	}
	else if(data[data_type] < 990)	//higher 100, +10 steps, smaller 990
	{
		data[data_type] += 10;
	}
}

void Data::Decrement(data_type_t data_type)
{
	if(data[data_type] > 1)
	{
		if(data[data_type] <= 100)	//smaller equal 100, -1 steps
		{
			data[data_type] -= 1;
		}
		else						//higher 100, -10 steps
		{
			data[data_type] -= 10;
		}
	}
}

void Data::Set(data_type_t data_type, uint16_t val)
{
	if(data_type == DATA_TOTAL_RUNTIME)
	{
		data[data_type] = val;
		return;
	}
	if(val > 0 && val <= 990)
	{
		data[data_type] = val;
	}
}

void Data::SetTemp(temp_type_t temp_type, int16_t val)
{
	tempdata[temp_type] = val;
}

int16_t Data::GetTemp(temp_type_t temp_type)
{
	return tempdata[temp_type];
}

uint16_t Data::Get(data_type_t data_type)
{
	return data[data_type];
}

void Data::SetError(statusBit_t bit)
{
	if(ignoreStatus & _BV(bit))	//if Error is ignored, dont set it
	{
		return;
	}
	status |= _BV(bit);
	Save();
}

uint8_t Data::GetErrors()
{
	return status;
}

void Data::ClearError(statusBit_t bit)
{
	status &= ~_BV(bit);
	Save();
}

void Data::SetIgnoreError(statusBit_t bit)
{
	ignoreStatus |= _BV(bit);
	Save();
}

void Data::decCountdown(uint8_t sec)
{
	if(countdown > sec)
	{
		countdown -= sec;
	}
	else
	{
		countdown = 0;
	}
}

void Data::resetCountdown()
{
	countdown = (uint32_t)data[DATA_INTERVAL]*360;	//converted to seconds
}

void Data::setCustomCountdown(uint32_t sec)
{
	countdown = sec;
}

uint32_t Data::getCountdown()
{
	return countdown;
}

uint16_t Data::getCountdownDisplay()
{
	return (countdown/360);
}

void Data::Save()
{
	cli();
	_delay_ms(10);
	eeprom_write_word((uint16_t *)ADR_INTERVAL, data[DATA_INTERVAL]);		//save interval
	eeprom_write_word((uint16_t *)ADR_DURATION1, data[DATA_DURATION1]);		//save duration
	eeprom_write_word((uint16_t *)ADR_DURATION2, data[DATA_DURATION2]);		//save duration
	eeprom_write_word((uint16_t *)ADR_DURATION3, data[DATA_DURATION3]);		//save duration
	eeprom_write_word((uint16_t *)ADR_TOTAL_RUNTIME, data[DATA_TOTAL_RUNTIME]);		//save total runtime
	eeprom_write_word((uint16_t *)ADR_SETUP_TEMP, tempdata[DATA_SETUP_TEMP]);	//save setup temp
	eeprom_write_word((uint16_t *)ADR_STATUS, status);	//save status
	eeprom_write_word((uint16_t *)ADR_IGNORE_STATUS, ignoreStatus);	//save ignoreStatus
	//create CRC
	_delay_ms(10);
	sei();
}

void Data::setDefault()
{
	Set(DATA_INTERVAL,DATA_INTERVAL_DEFAULT);	//set default values
	Set(DATA_DURATION1,DATA_DURATION1_DEFAULT);
	Set(DATA_DURATION2,DATA_DURATION2_DEFAULT);
	Set(DATA_DURATION3,DATA_DURATION3_DEFAULT);
	SetTemp(DATA_SETUP_TEMP,DATA_SETUP_TEMP_DEFAULT);
	status = 0;
	ignoreStatus = 0;
	Save();
	eeprom_write_dword((uint32_t *)ADR_INIT_CONST, DATA_INIT_CONST);	//set init constant
}
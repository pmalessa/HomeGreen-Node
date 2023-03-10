/*
 * data.h
 *
 *  Created on: 30.01.2019
 *      Author: pmale
 */

#ifndef DATA_H_
#define DATA_H_

#include "PLATFORM.h"
#include "power.hpp"

class Data
{
public:
	typedef enum{
		DATA_INTERVAL = DIGIT_INTERVAL,
		DATA_DURATION1 = DIGIT_DURATION,
		DATA_DURATION2,
		DATA_DURATION3,
		DATA_TOTAL_RUNTIME,
		DATA_SIZE
	}data_type_t;

	typedef enum{
		STATUS_PB_ERR = 1,
		STATUS_P1_ERR,
		STATUS_P2_ERR,
		STATUS_P3_ERR,
		STATUS_EP_ERR,
	}statusBit_t;

	union statusAndStrengthUnion
	{
		struct{
			uint8_t strength;
			uint8_t status;
		};
		uint16_t raw;
	};

	static void Init();
	static uint16_t CalcCRC();
	static void Set(data_type_t data_type, uint16_t val);
	static uint16_t Get(data_type_t data_type);
	static void SetError(statusBit_t bit);
	static void ClearError(statusBit_t bit);
	static uint8_t GetErrors();
	static void SetIgnoreError(statusBit_t bit);
	static void SetPumpStrength(uint8_t id, uint8_t strength);
	static uint8_t GetPumpStrength(uint8_t id);
	static void setSavePending();
	static void SaveConfig();
	static void SaveError();
	static void Increment(data_type_t data_type);
	static void Decrement(data_type_t data_type);
	static void decCountdown(uint8_t sec);
	static uint16_t getCountdownDisplay();
	static uint32_t getCountdown();
	static void resetCountdown();
	static void setCustomCountdown(uint32_t sec);
	static void setDefault();
	static void resetFromEEPROM();

private:
	enum data_adr{
		ADR_INIT_CONST = 0x00,
		ADR_INTERVAL = 0x04,
		ADR_DURATION1 = 0x06,
		ADR_DURATION2 = 0x08,
		ADR_DURATION3 = 0x0A,
		ADR_TIME_COUNTER = 0x0C,
		ADR_TOTAL_RUNTIME = 0x14,
		ADR_STATUS = 0x16,
		ADR_IGNORE_STATUS = 0x18,
		ADR_EEP_VERSION = 0x30,
		ADR_CRC = 0x22
	};
	#define DATA_INIT_CONST 0xF00DBABE
	#define DATA_INTERVAL_DEFAULT   77		//1..990 = 0.1..99.0
	#define DATA_DURATION1_DEFAULT  7		//1..990 = 0.1..99.0
	#define DATA_DURATION2_DEFAULT  7		//1..990 = 0.1..99.0
	#define DATA_DURATION3_DEFAULT  7		//1..990 = 0.1..99.0
	#define DATA_EEP_VERSION 11				//1.1

	static uint16_t data[DATA_SIZE];
	static uint32_t countdown;
	static uint8_t ignoreStatus, savePending;
	static statusAndStrengthUnion statusAndStrength;
};


#endif /* DATA_H_ */

/*
 * I2C_temp.c
 *
 *  Created on: Feb 4, 2019
 *      Author: carni
 */

//#define INCLUDE_LOG_DEBUG 1

#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"
#include "I2C_temp.h"
#include "i2cspm.h"
#include "em_i2c.h"
#include "Timer.h"
#include "log.h"
#include "em_core.h"
#include "sleep.h"
#include "em_letimer.h"
#include "ble.h"
#include "gatt_db.h"
#include "infrastructure.h"
#include "gecko_ble_errors.h"
#include "display.h"
#include "ble_device_type.h"

#define SCL_port (gpioPortC)
#define SCL_pin (10)
#define SDA_port (gpioPortC)
#define SDA_pin (11)
#define SE_port (gpioPortD)
#define SE_pin (15)
#define I2CTransferWrite (0)
#define I2CTransferRead (1)

/*Variable Declarations*/
uint8_t write_buffer_data[3] = {0,0,0};
uint8_t write_cmd = 0x07;
uint16_t write_buffer_len = 3;
uint8_t write_buf_index = 0;
//bool readvswrite = I2CTransferRead;
I2C_TransferReturn_TypeDef I2C_Return;
extern server_properties servo_props;

/* Initialize I2C variables*/
I2C_TransferSeq_TypeDef seq;

void I2C_INIT (void)
{

	/*Init I2CSPM variables*/
	I2CSPM_Init_TypeDef init;
	init.port = I2C0;
	init.sclPort = SCL_port;
	init.sclPin = SCL_pin;
	init.sdaPort = SDA_port;
	init.sdaPin = SDA_pin;
	init.portLocationScl = 14;
	init.portLocationSda = 16;
	init.i2cRefFreq = 0;
	init.i2cMaxFreq =I2C_FREQ_STANDARD_MAX;
	init.i2cClhr = i2cClockHLRStandard;

	/*init I2CSPM*/
	I2CSPM_Init(&init);

	//GPIO_PinModeSet(SE_port, SE_pin, gpioModePushPull, false);
	//Enable I2C0 interrupt in NVIC
	NVIC_EnableIRQ(I2C0_IRQn);

}

I2C_TransferReturn_TypeDef I2CWrite(void)
{
	write_cmd = 0x06;

	I2C_TransferReturn_TypeDef Transfer_return;

		seq.addr = 0x00 << 1;
		seq.flags = I2C_FLAG_WRITE_READ;
		seq.buf[0].data = &write_cmd;
		seq.buf[0].len = 1;
		seq.buf[1].data = write_buffer_data;
		seq.buf[1].len = write_buffer_len;

	Transfer_return = I2C_TransferInit(I2C0, &seq);

	/*printf("\r\nI2CWrite Started");
	printf("\r\nTransfer return %d", Transfer_return);*/

	return(Transfer_return);

}
/*I2C_TransferReturn_TypeDef I2CRead(void)
{
	I2C_TransferReturn_TypeDef Transfer_return;

	// Initialize I2C variables
		seq.addr = 0x00 << 1;
		seq.flags = I2C_FLAG_READ;
		seq.buf[0].data = write_buffer_data;
		seq.buf[0].len = write_buffer_len;

	Transfer_return = I2C_TransferInit(I2C0, &seq);
	printf("\r\nI2CRead Started");

		//Transfer_return = I2CSPM_Transfer(I2C0, &seq);

	return(Transfer_return);
}*/

void Temp_Events(struct gecko_cmd_packet* evt)
{
	uint32_t flags = evt->data.evt_system_external_signal.extsignals;
	uint32_t checks;

	for (checks = Temp_Start_PowerUp; checks <= I2CComplete; checks = checks << 1) //switch to for loop to iterate through the bits that can be set - for (i=1; i << 1; i <= I2CComplete)
	{
	if ((checks & flags) == checks)
	{
	switch (checks) //evt->data.evt_system_external_signal.extsignals
	{
	case Temp_Start_PowerUp:
		{
			//GPIO_PinOutSet(SE_port,SE_pin);
			TimerSetEventMs(80);
			printf("\r\nI2C Power up started");
			break;
		}
	case Temp_Wait_I2CWrite:
		{
			SLEEP_SleepBlockBegin(sleepEM2);
			LETIMER_IntDisable(LETIMER0, LETIMER_IEN_COMP1);
			I2C_Return = I2CWrite();

			break;
		}
	case I2CComplete:
	{
		if (I2C_Return != i2cTransferDone)
		{
			printf("\r\nI2CTransfer fail with code: %d", I2C_Return);
		}
		/*if(readvswrite == I2CTransferWrite)
		{
			printf("\r\nI2CWrite Complete");
			//TimerSetEventMs(12);
			LETIMER_IntDisable(LETIMER0, LETIMER_IEN_COMP1);
			ret_status = I2CRead();
			break;
		}*/
		else{
			printf("\r\nI2CRead Complete");

			Temp_Measurement();

			SLEEP_SleepBlockEnd(sleepEM2);			  
			break;
		}
		break;
	default: printf("\r\nUnhandled mask type %x", flags);
	}
	}
	}
	}
}

void Temp_Measurement(void)
{
	  uint16_t Temp_code;
	  uint8_t htmTempBuffer[5]; /* Stores the temperature data in the Health Thermometer (HTM) format. */
	  uint8_t flags = 0x00;   /* HTM flags set as 0 for Celsius, no time stamp and no temperature type. */
	  uint16_t tempData;     /* Stores the Temperature data read from the RHT sensor. */
	  uint32_t temperature;   /* Stores the temperature data read from the sensor in the correct format */
	  uint8_t *p = htmTempBuffer; /* Pointer to HTM temperature buffer needed for converting values to bitstream. */

	  /* Convert flags to bitstream and append them in the HTM temperature data buffer (htmTempBuffer) */
	  UINT8_TO_BITSTREAM(p, flags);

	  Temp_code = (((write_buffer_data[1])) << 8) | write_buffer_data[0];
	  printf("\r\nTemperature = %x \r\n", Temp_code);
	  //printf("\r\nPEC = %x \r\n", write_buffer_data[1]);
	  printf("\r\nPEC = %x \r\n", write_buffer_data[2]);

	  tempData = Temp_code*0.02 - 273.15;
	  printf("\r\nTemperature = %.1d \r\n", tempData);

	  displayPrintf(DISPLAY_ROW_TEMPVALUE  , "\r\nTemp = %.2d", tempData);

	  /* Convert sensor data to correct temperature format */
	  temperature = FLT_TO_UINT32((tempData*1000), -3);
	  /* Convert temperature to bitstream and place it in the HTM temperature data buffer (htmTempBuffer) */
	  UINT32_TO_BITSTREAM(p, temperature);

	  /* Send indication of the temperature in htmTempBuffer to all "listening" clients.
	   * This enables the Health Thermometer in the Blue Gecko app to display the temperature.*/
	  gecko_cmd_gatt_server_send_characteristic_notification(servo_props.connectionHandle, gattdb_temperature_measurement, 5, htmTempBuffer);
}

void I2C0_IRQHandler(void)
{
	I2C_Return = I2C_Transfer(I2C0);

	//printf("\r\nI2C State changed to %d", I2C_Return);

	if (I2C_Return != i2cTransferInProgress)
	{
		CORE_DECLARE_IRQ_STATE;
		CORE_ENTER_CRITICAL();
		//readvswrite = !readvswrite;
		gecko_external_signal(I2CComplete);
		CORE_EXIT_CRITICAL();
		printf("\r\nTemp Sensor State changed to %d", I2CComplete);
		//printf("\r\nReadvsWrite status: %s", (readvswrite)? "Read":"Write");

	}
}

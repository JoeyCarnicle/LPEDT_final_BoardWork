/*
 * ble.h
 *
 *  Created on: Feb 19, 2019
 *      Author: carni
 */

#ifndef SRC_BLE_H_
#define SRC_BLE_H_

#define SCHEDULER_SUPPORTS_DISPLAY_UPDATE_EVENT 1
#define TIMER_SUPPORTS_1HZ_TIMER_EVENT	1

#include "native_gecko.h"
#include <stdbool.h>

enum event_bitmask{

		Temp_Start_PowerUp = 1 << 1,
		Temp_Wait_I2CWrite = 1 << 2,
		I2CComplete = 1 << 3,
		Bonding = 1 << 4,
		Bonded = 1 << 5,
		Bonded_true = 1 << 6,
		Bonded_false = 1 << 7
};

typedef struct {
	uint8_t  connectionHandle;
	uint16_t serverAddress;
	uint32_t thermometerServiceHandle;
	uint16_t thermometerCharacteristicHandle;
	uint32_t ButtonServiceHandle;
	uint16_t ButtonCharacteristicHandle;
	float temperature;
	bool button_state;
}server_properties;

void gecko_events(struct gecko_cmd_packet* evt);
void presskey_confirm(void);
void Button_indication(void);

#endif /* SRC_BLE_H_ */

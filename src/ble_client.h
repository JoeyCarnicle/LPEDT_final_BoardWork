/*
 * ble_client.h
 *
 *  Created on: Feb 28, 2019
 *      Author: carni
 */

#ifndef SRC_BLE_CLIENT_H_
#define SRC_BLE_CLIENT_H_

#include "native_gecko.h"

typedef enum {
  scanning,
  opening,
  discover_service,
  discover_characteristics,
  connected
} connection_state;

void gecko_events_client(struct gecko_cmd_packet* evt);
float gattUint32ToFloat(const uint8_t *value_start_little_endian);

#endif /* SRC_BLE_CLIENT_H_ */

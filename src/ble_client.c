/*
 * ble_client.c
 *
 * Client side bluetooth event handler
 * handles states to discover devices, connect to thermometer UUID, and receive thermometer data from server
 * displayprintf utilizes deisplay.h to print to the LCD
 *
 *  Created on: Feb 28, 2019
 *      Author: carni
 */

/*Includes*/
#include "ble_client.h"
#include "ble.h"
#include "stdint.h"
#include "display.h"
#define INCLUDE_LOG_DEBUG 1
#include "log.h"
#include <stdbool.h>
#include <math.h>
#include "ble_device_type.h"
#include "gecko_ble_errors.h"

/*Variable Declarations*/
connection_state conn_state;
extern server_properties servo_props;
server_properties servo_props_temp;
server_properties servo_props_PB;
bool service_state = false;
bool characteristic_state = false;
bool temp_discovery_done = true;

const uint8_t thermometer_UUID_serv[2] = { 0x09, 0x18 };
const uint8_t thermometer_UUID_char[2] = { 0x1c, 0x2a };
const uint8_t button_UUID_serv[16] = { 0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38, 0x01, 0x00, 0x00, 0x00 };
//00000002-38c8-433e-87ec-652a2d136289
const uint8_t button_UUID_char[16] = { 0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38, 0x02, 0x00, 0x00, 0x00 };
//00000001-38c8-433e-87ec-652a2d136289
extern bool gecko_update(struct gecko_cmd_packet* evt);
extern int bonding_status;

void gecko_events_client(struct gecko_cmd_packet* evt)
{

	gecko_update(evt);

	switch (BGLIB_MSG_ID(evt->header)) {
	case gecko_evt_system_boot_id:
	{
		displayPrintf(DISPLAY_ROW_NAME , "Client");
		displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");

		struct gecko_msg_system_get_bt_address_rsp_t* bt_addr = gecko_cmd_system_get_bt_address();
		displayPrintf(DISPLAY_ROW_BTADDR, "%02X:%02X:%02X:%02X:%02X:%02X", bt_addr->address.addr[0], bt_addr->address.addr[1], bt_addr->address.addr[2], bt_addr->address.addr[3], bt_addr->address.addr[4], bt_addr->address.addr[5]);


		//gecko_cmd_le_gap_set_discovery_type(le_gap_phy_1m, 1);
		gecko_cmd_le_gap_set_discovery_timing(le_gap_phy_1m, 16, 16); //16 = 10ms
		//gecko_cmd_sm_configure(0xF, 1); 	//Display with Keyboard
	    gecko_cmd_le_gap_start_discovery(le_gap_phy_1m, le_gap_discover_generic);

	    gecko_cmd_sm_delete_bondings();
	    gecko_cmd_sm_set_bondable_mode(1); 	//Bondable
	    gecko_cmd_sm_configure(0xF, 1); 	//Display with Keyboard

        conn_state = scanning;
		break;
	}
	case gecko_evt_le_gap_scan_response_id:
	{
		bd_addr bd_address = SERVER_BT_ADDRESS;
		int correct_addr=1;

		for (int i = 0; i<=5; i++)
		{
		if(evt->data.evt_le_gap_scan_response.address.addr[i] != bd_address.addr[i])
			correct_addr = 0;
		}
		if (correct_addr == 1)
		{
		gecko_cmd_le_gap_end_procedure();

		gecko_cmd_le_gap_connect(bd_address,	//bd_addressevt->data.evt_le_gap_scan_response.address
		                         evt->data.evt_le_gap_scan_response.address_type,
		                         le_gap_phy_1m);
		}
		break;
	}
	case gecko_evt_le_connection_opened_id:
	{
		displayPrintf(DISPLAY_ROW_CONNECTION, "Connected");
        gecko_cmd_le_gap_set_conn_parameters(60, 60, 3, 4000);

        servo_props.connectionHandle = evt->data.evt_le_connection_opened.connection;
        struct gecko_msg_sm_increase_security_rsp_t* err = gecko_cmd_sm_increase_security(evt->data.evt_le_connection_opened.connection);
        LOG_DEBUG("error result %d", err->result);

        conn_state = discover_service;

		break;
	}
	case gecko_evt_gatt_service_id:
	{
		if (temp_discovery_done == true) {
	        	LOG_DEBUG("PB service id");
				servo_props_PB.ButtonServiceHandle = evt->data.evt_gatt_service.service;
			}
		else {
			servo_props_temp.thermometerServiceHandle = evt->data.evt_gatt_service.service;
		}
		service_state = true;
		break;
	}
	case gecko_evt_gatt_characteristic_id:
	{
		if (temp_discovery_done == true) {
			LOG_DEBUG("PB char id");
			servo_props_PB.ButtonCharacteristicHandle = evt->data.evt_gatt_characteristic.characteristic;
		}
		else{
			servo_props_temp.thermometerCharacteristicHandle = evt->data.evt_gatt_characteristic.characteristic;
		}
		characteristic_state = true;
		break;
	}
	case gecko_evt_gatt_procedure_completed_id:
	{
		LOG_DEBUG("procedure completed id");
		if (conn_state == discover_service && service_state == true)
		{
			if (temp_discovery_done == true) {
				LOG_DEBUG("discovering PB characteristics by UUID");
				gecko_cmd_gatt_discover_characteristics_by_uuid(evt->data.evt_gatt_procedure_completed.connection,
															servo_props_PB.ButtonServiceHandle,
															16, (const uint8_t*)button_UUID_char);				//need to differentiate b/w these two
			}
			else {
				LOG_DEBUG("discovering temp characteristics by UUID");
				gecko_cmd_gatt_discover_characteristics_by_uuid(evt->data.evt_gatt_procedure_completed.connection,
																servo_props_temp.thermometerServiceHandle,
																2, (const uint8_t*)thermometer_UUID_char);
			}
			service_state = false;
			conn_state = discover_characteristics;
			break;
		}
		if (conn_state == discover_characteristics && characteristic_state == true)
		{
			if (temp_discovery_done == true) {
				LOG_DEBUG("setting pb indications by UUID");
				gecko_cmd_gatt_set_characteristic_notification(evt->data.evt_gatt_procedure_completed.connection,
																servo_props_PB.ButtonCharacteristicHandle,
																gatt_indication);
				temp_discovery_done = false;
				conn_state = discover_service;
			}
			else {
				LOG_DEBUG("setting temp indications by UUID");
				gecko_cmd_gatt_set_characteristic_notification(evt->data.evt_gatt_procedure_completed.connection,
																servo_props_temp.thermometerCharacteristicHandle,
																gatt_indication);

				temp_discovery_done = true;
				conn_state = connected;

			}
			characteristic_state = false;
			break;
		}
		else gecko_cmd_gatt_discover_primary_services_by_uuid(servo_props.connectionHandle, 2,  (const uint8_t*)thermometer_UUID_serv );
		break;
	}
	case gecko_evt_sm_confirm_passkey_id:
	{
		LOG_DEBUG("Passkey confirm");
		uint32_t passkey = evt->data.evt_sm_confirm_passkey.passkey;
		displayPrintf(DISPLAY_ROW_PASSKEY, "Passkey: %d", passkey);
		displayPrintf(DISPLAY_ROW_ACTION, "Confirm with PB0");
		break;
	}
	case gecko_evt_system_external_signal_id:
	{
		if (evt->data.evt_system_external_signal.extsignals == Bonding) {
			presskey_confirm();
			}
		break;
	}
	case gecko_evt_sm_bonded_id:
		LOG_DEBUG("increase security success");
		displayPrintf(DISPLAY_ROW_CONNECTION, "Bonded");
		struct gecko_msg_gatt_discover_primary_services_by_uuid_rsp_t* err = gecko_cmd_gatt_discover_primary_services_by_uuid(servo_props.connectionHandle, 16, button_UUID_serv );
		LOG_DEBUG("discover PB error = %x", err->result);

		break;

	case gecko_evt_sm_bonding_failed_id:
		LOG_DEBUG("increase security failed");
		displayPrintf(DISPLAY_ROW_CONNECTION, "Connected");
		gecko_cmd_sm_delete_bondings();
		displayPrintf(DISPLAY_ROW_PASSKEY, " ");
		displayPrintf(DISPLAY_ROW_ACTION, " ");
		bonding_status = Bonding;
		break;

	case gecko_evt_gatt_characteristic_value_id:
	{
		int8_t *temp_buffer = 0;
		if (conn_state == connected){
		displayPrintf(DISPLAY_ROW_CONNECTION, "Handling Indications");
		if(evt->data.evt_gatt_characteristic_value.characteristic == servo_props_temp.thermometerCharacteristicHandle){
			if(evt->data.evt_gatt_characteristic_value.att_opcode == gatt_handle_value_indication){
				gecko_cmd_gatt_send_characteristic_confirmation(evt->data.evt_gatt_characteristic_value.connection);

		temp_buffer = &evt->data.evt_gatt_characteristic_value.value.data[0];

		servo_props_temp.temperature = gattUint32ToFloat(&temp_buffer[1]);

		displayPrintf(DISPLAY_ROW_TEMPVALUE  , "Temp = %f", servo_props_temp.temperature);
			}
		}

		if(evt->data.evt_gatt_characteristic_value.characteristic == servo_props_PB.ButtonCharacteristicHandle){
			if(evt->data.evt_gatt_characteristic_value.att_opcode == gatt_handle_value_indication){
				gecko_cmd_gatt_send_characteristic_confirmation(evt->data.evt_gatt_characteristic_value.connection);
			if ((temp_buffer = evt->data.evt_gatt_characteristic_value.value.data[0]) == 0x00) displayPrintf(DISPLAY_ROW_ACTION, "Button Released"); //might have to have a GATTunint8toInt function or some shit
			else if ((temp_buffer = evt->data.evt_gatt_characteristic_value.value.data[0]) == 0x01) displayPrintf(DISPLAY_ROW_ACTION, "Button Pressed");
			}
		}
		}
		break;
	}
	case gecko_evt_le_connection_closed_id:
	{
		gecko_cmd_le_gap_start_discovery(le_gap_phy_1m, le_gap_general_discoverable);
		displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");
		conn_state = scanning;
		gecko_cmd_sm_delete_bondings();
		temp_discovery_done = false;
		break;
	}
	}
}

float gattUint32ToFloat(const uint8_t *value_start_little_endian)
{
	int8_t exponent = (int8_t)value_start_little_endian[3];
	uint32_t mantissa = value_start_little_endian[0] +
						(((uint32_t)value_start_little_endian[1]) << 8) +
						(((uint32_t)value_start_little_endian[2]) << 16);
	return (float)mantissa*pow(10,exponent);
}


/*
 * ble.c
 *
 * Handles BLE events per the event scheduler
 * displayprintf utilizes deisplay.h to print to the LCD
 *
 *  Created on: Feb 18, 2019
 *      Author: carni
 */

#include "native_gecko.h"
#include "gecko_ble_errors.h"
#include "I2C_temp.h"
#include "gatt_db.h"
#define INCLUDE_LOG_DEBUG 1
#include "log.h"
#include "em_letimer.h"
#include "ble.h"
#include "display.h"
#include "infrastructure.h"
#include "em_core.h"

server_properties servo_props;

uint32_t mask = 0;

extern bool gecko_update(struct gecko_cmd_packet* evt);
int bonding_status = Bonding;
extern bool button_status;


void gecko_events(struct gecko_cmd_packet* evt)
{

	gecko_update(evt);
	switch (BGLIB_MSG_ID(evt->header)) {
	case gecko_evt_system_boot_id:
		displayPrintf(DISPLAY_ROW_NAME , "Server");
		displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");

		struct gecko_msg_system_get_bt_address_rsp_t* bt_addr = gecko_cmd_system_get_bt_address();
		displayPrintf(DISPLAY_ROW_BTADDR, "%02X:%02X:%02X:%02X:%02X:%02X", bt_addr->address.addr[0], bt_addr->address.addr[1], bt_addr->address.addr[2], bt_addr->address.addr[3], bt_addr->address.addr[4], bt_addr->address.addr[5]);

		BTSTACK_CHECK_RESPONSE(gecko_cmd_le_gap_set_advertise_timing(0, 160, 160, 0, 0));
		BTSTACK_CHECK_RESPONSE(gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable, le_gap_connectable_scannable));

		/*Bonding Setup*/
		gecko_cmd_sm_set_bondable_mode(1); 	//Bondable
		gecko_cmd_sm_configure(0xF, 1); 	//Display with Keyboard

		break;

	case gecko_evt_le_connection_opened_id:
		{
		servo_props.connectionHandle = evt->data.evt_le_connection_opened.connection;
		BTSTACK_CHECK_RESPONSE(gecko_cmd_le_connection_set_parameters (servo_props.connectionHandle, 60, 60, 3, 600));

		displayPrintf(DISPLAY_ROW_CONNECTION, "Connected");

		//Enable interrupts
		LETIMER_IntEnable(LETIMER0, LETIMER_IEN_UF);
		//Start LETIMER0
		LETIMER_Enable(LETIMER0, true);

		break;
		}
	case gecko_evt_sm_confirm_bonding_id:
		gecko_cmd_sm_bonding_confirm(servo_props.connectionHandle, 1);
		bonding_status = Bonding;
		break;

	case gecko_evt_sm_confirm_passkey_id:
	{
		uint32_t passkey = evt->data.evt_sm_confirm_passkey.passkey;
		displayPrintf(DISPLAY_ROW_PASSKEY, "Passkey: %d", passkey);
		displayPrintf(DISPLAY_ROW_ACTION, "Confirm with PB0");
		break;
	}
	case gecko_evt_sm_bonded_id:
		displayPrintf(DISPLAY_ROW_CONNECTION, "Bonded");
		break;

	case gecko_evt_sm_bonding_failed_id:
		displayPrintf(DISPLAY_ROW_CONNECTION, "Connected");
		gecko_cmd_sm_delete_bondings();
		displayPrintf(DISPLAY_ROW_PASSKEY, " ");
		displayPrintf(DISPLAY_ROW_ACTION, " ");
		bonding_status = Bonding;
		break;

	case gecko_evt_le_connection_closed_id:
		LETIMER_IntDisable(LETIMER0, LETIMER_IEN_UF);
		LETIMER_Enable(LETIMER0, false);
		displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");
		gecko_cmd_sm_delete_bondings();
		bonding_status = Bonding;
		break;

	case gecko_evt_gatt_server_characteristic_status_id:
	{
		BTSTACK_CHECK_RESPONSE(gecko_cmd_le_connection_get_rssi(servo_props.connectionHandle));
		break;
	}
	case gecko_evt_system_external_signal_id:
	{

		if (evt->data.evt_system_external_signal.extsignals == Bonding) presskey_confirm();
		if (evt->data.evt_system_external_signal.extsignals == Bonded)
			{
				printf("calling button indication");
				Button_indication();
			}

		Temp_Events(evt);

		break;
	}
	case gecko_evt_le_connection_rssi_id:
	{
		int8_t rssi_input = 0;
		int16_t txPower = 0;
		rssi_input = evt->data.evt_le_connection_rssi.rssi;

		if (rssi_input > -35){
			txPower = -80;
		}
		else if ( ( -35 >= rssi_input) && (rssi_input > -45) ){
			txPower = -200;
		}
		else if ( ( -45 >= rssi_input) && (rssi_input > -55) ){
			txPower = -150;
		}
		else if ( ( -55 >= rssi_input) && (rssi_input > -65) ){
			txPower = -50;
		}
		else if ( ( -65 >= rssi_input) && (rssi_input > -75) ){
			txPower = 0;
		}
		else if ( ( -75 >= rssi_input) && (rssi_input > -85) ){
			txPower = 50;
		}
		else txPower = 80;

		gecko_cmd_system_halt(1);
		gecko_cmd_system_set_tx_power(txPower);
		gecko_cmd_system_halt(0);

		break;
	}
	case gecko_evt_hardware_soft_timer_id:
		displayUpdate();
		break;
	}
}

void presskey_confirm(void)
{
	LOG_DEBUG("presskey function");
	CORE_DECLARE_IRQ_STATE;
	CORE_ENTER_CRITICAL();
	struct gecko_msg_sm_passkey_confirm_rsp_t* err = gecko_cmd_sm_passkey_confirm(servo_props.connectionHandle, 1);
	LOG_DEBUG("error result %d", err->result);
	bonding_status = Bonded;
	CORE_EXIT_CRITICAL();
	NVIC_EnableIRQ(GPIO_EVEN_IRQn);
	displayPrintf(DISPLAY_ROW_PASSKEY, " ");
	displayPrintf(DISPLAY_ROW_ACTION, " ");
}

void Button_indication(void)
{
	  uint8_t button_st[1] = {0x00};
	  uint8_t *p = button_st;
	  uint8_t zero = 0x00;
	  uint8_t one = 0x01;

	  printf("button status = %d ", button_status);

	  if (button_status == 1)
	  {
		  	UINT8_TO_BITSTREAM(p, one);
		  	//displayPrintf(DISPLAY_ROW_TEMPVALUE, "%d", button_st[0]);
	  }
	  if (button_status == 0)
	  {
		  	UINT8_TO_BITSTREAM(p, zero);
		  	//displayPrintf(DISPLAY_ROW_TEMPVALUE, "%d", button_st[1]);
	  }

	  gecko_cmd_gatt_server_send_characteristic_notification(0xff, gattdb_button_state, 1, button_st);
}

/* Board headers */
#include "init_mcu.h"
#include "init_board.h"
#include "init_app.h"
#include "ble-configuration.h"
#include "board_features.h"
#include "Timer.h"
#include "em_letimer.h"
#include "sleep.h"
#include <stdbool.h>
#include "em_core.h"
#include "I2C_temp.h"
#include <stdio.h>
//#define INCLUDE_LOG_DEBUG 1
//#include "log.h"
#include "display.h"
#include "ble.h"
#include "ble_client.h"
#include "ble_device_type.h"

/*Client vs Server build Switch*/
#define device_BLE_Server (1)	//1 for server, 0 for client

/* Bluetooth stack headers */
#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"

/* Libraries containing default Gecko configuration values */
#include "em_emu.h"
#include "em_cmu.h"

/* Device initialization header */
#include "hal-config.h"

#if defined(HAL_CONFIG)
#include "bsphalconfig.h"
#else
#include "bspconfig.h"
#endif

#include "em_device.h"
#include "em_chip.h"
#include "gpio.h"
#ifndef MAX_CONNECTIONS
#define MAX_CONNECTIONS 4
#endif

uint8_t bluetooth_stack_heap[DEFAULT_BLUETOOTH_HEAP(MAX_CONNECTIONS)];

/* Set blocked_mode to the deepest sleep mode to turn off (i.e. sleepEM4 to sleep to EM3)*/
const SLEEP_EnergyMode_t blocked_mode = sleepEM4;

#define period (3000) //period in ms

/*Global Variables*/
int timer_period = 0;


// Gecko configuration parameters (see gecko_configuration.h)
static const gecko_configuration_t config = {
  .config_flags = 0,
  .sleep.flags = SLEEP_FLAGS_DEEP_SLEEP_ENABLE,
  .bluetooth.max_connections = MAX_CONNECTIONS,
  .bluetooth.heap = bluetooth_stack_heap,
  .bluetooth.heap_size = sizeof(bluetooth_stack_heap),
  .bluetooth.sleep_clock_accuracy = 100, // ppm
  .gattdb = &bg_gattdb_data,
  .ota.flags = 0,
  .ota.device_name_len = 3,
  .ota.device_name_ptr = "OTA",
#if (HAL_PA_ENABLE) && defined(FEATURE_PA_HIGH_POWER)
  .pa.config_enable = 1, // Enable high power PA
  .pa.input = GECKO_RADIO_PA_INPUT_VBAT, // Configure PA input to VBAT
#endif // (HAL_PA_ENABLE) && defined(FEATURE_PA_HIGH_POWER)
};


int main(void)
{

  // Initialize device
  initMcu();
  // Initialize board
  initBoard();
  // Initialize application
  initApp();

  gpioInit();

  // Initialize stack
  gecko_init(&config);

  //Init Clock LFA for LETIMER0
  timer_period = Osc_INIT(blocked_mode, period);

  //Init LETIMER0
  LETIMER0_INIT(timer_period);

  //Initialize Logging
   //logInit();

  //Init I2C
  I2C_INIT();

  //Init the display, including starting 1Hz SW timer
  displayInit();

  //Sleep init NULL for this application
  SLEEP_Init_t sleepInitCalls = { 0 };
  //Sleep setup
  SLEEP_InitEx(&sleepInitCalls);
  if ((blocked_mode != sleepEM1))
  {
  SLEEP_SleepBlockBegin(blocked_mode);
  }

  /* Infinite loop */
  while (1) {

		  	/* Check for stack event. */
	  		struct gecko_cmd_packet* evt = gecko_wait_event();
	  		if (IsServerDevice())
	  			{
	  				gecko_events(evt);
	  			}
	  		else if (IsClientDevice())
				{
	  				gecko_events_client(evt);
				}
  }
}



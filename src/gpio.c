/*
 * gpio.c
 *
 *  Created on: Dec 12, 2018
 *      Author: Dan Walkes
 */
#include "gpio.h"
#include "em_gpio.h"
#include <string.h>
#include "ble.h"
#include "gatt_db.h"
#include "em_core.h"
#include <stdbool.h>


/**
 * TODO: define these.  See the radio board user guide at https://www.silabs.com/documents/login/user-guides/ug279-brd4104a-user-guide.pdf
 * and GPIO documentation at https://siliconlabs.github.io/Gecko_SDK_Doc/efm32g/html/group__GPIO.html
 */
#define	LED0_port (gpioPortF)
#define LED0_pin (4)
#define LED1_port (gpioPortF)
#define LED1_pin (5)
#define DISP_EN_port (gpioPortD)
#define DISP_EN_pin (15)
#define DISP_EC_pin (13)
#define PB0_port (gpioPortF)
#define PB0_pin (6)

extern int bonding_status;
bool button_status = false;

void gpioInit()
{
	//GPIO_DriveStrengthSet(LED0_port, gpioDriveStrengthStrongAlternateStrong);
	GPIO_DriveStrengthSet(LED0_port, gpioDriveStrengthWeakAlternateWeak);
	GPIO_PinModeSet(LED0_port, LED0_pin, gpioModePushPull, false);
	//GPIO_DriveStrengthSet(LED1_port, gpioDriveStrengthStrongAlternateStrong);
	GPIO_DriveStrengthSet(LED1_port, gpioDriveStrengthWeakAlternateWeak);
	GPIO_PinModeSet(LED1_port, LED1_pin, gpioModePushPull, false);
	GPIO_PinModeSet(PB0_port, PB0_pin, gpioModeInputPullFilter, true);

	// Enable IRQ for even numbered GPIO pins
	  NVIC_EnableIRQ(GPIO_EVEN_IRQn);

	  // Enable both edge interrupts for PB0 pin
	  GPIO_IntConfig(PB0_port, PB0_pin, 1, 1, true);

}

void gpioLed0SetOn()
{
	GPIO_PinOutSet(LED0_port,LED0_pin);
}
void gpioLed0SetOff()
{
	GPIO_PinOutClear(LED0_port,LED0_pin);
}
void gpioLed1SetOn()
{
	GPIO_PinOutSet(LED1_port,LED1_pin);
}
void gpioLed1SetOff()
{
	GPIO_PinOutClear(LED1_port,LED1_pin);
}

void gpioEnableDisplay()
{
	GPIO_PinModeSet(DISP_EN_port, DISP_EN_pin, gpioModeWiredAndPullUp, false);
	GPIO_PinModeSet(DISP_EN_port, DISP_EC_pin, gpioModeWiredAndPullUp, false);
}

void gpioSetDisplayExtcomin(bool high)
{
	if (high)
	{
	//GPIO_PinModeSet(DISP_EN_port, DISP_EC_pin, gpioModeWiredAndPullUp, true);
	GPIO_PinOutSet(DISP_EN_port,DISP_EC_pin);

	}
	else
	{
	//GPIO_PinModeSet(DISP_EN_port, DISP_EC_pin, gpioModeWiredAndPullUp, true);
	GPIO_PinOutClear(DISP_EN_port,DISP_EC_pin);
	}
}

void GPIO_EVEN_IRQHandler(void)
{
  // Clear all odd pin interrupt flags
  GPIO_IntClear(_GPIO_IF_MASK);

  button_status = !button_status;

  if (bonding_status == Bonding)
  {
	  CORE_DECLARE_IRQ_STATE;
	  CORE_ENTER_CRITICAL();
	  gecko_external_signal(Bonding);
	  CORE_EXIT_CRITICAL();
  }
  else if (bonding_status == Bonded)
  {
	  CORE_DECLARE_IRQ_STATE;
	  CORE_ENTER_CRITICAL();
	  gecko_external_signal(Bonded);
	  CORE_EXIT_CRITICAL();
  }


  // Toggle LED0
  GPIO_PinOutToggle(LED0_port, LED0_pin);
}

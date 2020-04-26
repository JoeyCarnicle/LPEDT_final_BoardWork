/*
 * File used to set up the oscillators and LETIMER0 for assignment 2
 *
 */

#include "Timer.h"
#include <stdbool.h>
#include "em_cmu.h"
#include "em_letimer.h"
#include "sleep.h"
#include "em_core.h"
#include "I2C_temp.h"
#define INCLUDE_LOG_DEBUG 1
#include "log.h"
#include "ble.h"

 extern volatile int LETIMER0_int_count = 0;
 extern int timer_period;


/*Initialize Oscillator*/
int Osc_INIT(SLEEP_EnergyMode_t blocked_mode, int period)
{
	int timer_period;
	/*check blocked_mode for sleep mode*/
	if ((blocked_mode == sleepEM2) | (blocked_mode == sleepEM3)) //if EM1 or EM2, use LFXO
	{
		CMU_OscillatorEnable(cmuOsc_LFXO, true, true);
		CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);
		CMU_ClockEnable(cmuClock_LFA, true);
		CMU_ClockDivSet(cmuClock_LETIMER0, cmuClkDiv_4);
		CMU_ClockEnable(cmuClock_LETIMER0, true);

		timer_period = (65535*period)/8000;
	}
	else														//if EM3, use ULFRCO
		{
			CMU_OscillatorEnable(cmuOsc_ULFRCO, true, true);
			CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_ULFRCO);
			CMU_ClockEnable(cmuClock_LFA, true);
			CMU_ClockEnable(cmuClock_LETIMER0, true);

			timer_period = period;
		}

	CMU_OscillatorEnable(cmuOsc_HFXO, true, true);
	CMU_ClockSelectSet(cmuClock_HFPER, cmuSelect_HFXO);

	return (timer_period);

}


void TimerSetEventMs(int wait_ms)
{
	uint32_t Start_count = LETIMER_CounterGet(LETIMER0);
	int Comp1_value =0;

	if (Start_count < wait_ms)
	{
		wait_ms = wait_ms - Start_count;
		Comp1_value = timer_period - wait_ms;

	}
	else Comp1_value = Start_count - wait_ms;

	LETIMER_IntEnable(LETIMER0, LETIMER_IEN_COMP1);
	LETIMER_CompareSet(LETIMER0, 1, Comp1_value);
}

/*Initialize LETIMER0: COMP) as top, COMP0 and COMP1 values, enable interrupts for UF and COMP1, and NVIC LETIMER0 IRQ*/
void LETIMER0_INIT(int period)
{
	//Set init vars for LETIMER
	LETIMER_Init_TypeDef init;
	init.enable = false;
	init.debugRun = false;
	init.comp0Top = true;
	init.bufTop = false;
	init.out0Pol = 0;        /**< Idle value for output 0. */
	init.out1Pol = 0;        /**< Idle value for output 1. */
	init.ufoa0 = letimerUFOANone;          /**< Underflow output 0 action. */
	init.ufoa1 = letimerUFOANone;          /**< Underflow output 1 action. */
	init.repMode = letimerRepeatFree;        /**< Repeat mode. */
	//init.topValue = 0;


	//Init LETIMER0 with init vars
	LETIMER_Init(LETIMER0, &init);

	//Set Comp0 timer
	LETIMER_CompareSet(LETIMER0, 0, period);

	//Enable LETIMER interrupt in NVIC
	NVIC_EnableIRQ(LETIMER0_IRQn);

}


void LETIMER0_IRQHandler (void)
{

	uint32_t Event_flag = LETIMER_IntGetEnabled(LETIMER0);
	LETIMER_IntClear(LETIMER0, _LETIMER_IF_MASK);

	if (Event_flag & LETIMER_IF_UF)
	{
		LETIMER_IntClear(LETIMER0, LETIMER_IF_UF);
		LETIMER0_int_count++;
		CORE_DECLARE_IRQ_STATE;
		CORE_ENTER_CRITICAL();
		gecko_external_signal(Temp_Start_PowerUp);
		CORE_EXIT_CRITICAL();
		Event_flag = 0;
	}
	if (Event_flag & LETIMER_IF_COMP1)
	{
		LETIMER_IntClear(LETIMER0, LETIMER_IF_COMP1);
			CORE_DECLARE_IRQ_STATE;
			CORE_ENTER_CRITICAL();
			gecko_external_signal(Temp_Wait_I2CWrite);
			CORE_EXIT_CRITICAL();
			Event_flag = 0;

	}

}

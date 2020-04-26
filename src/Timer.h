/*
 * File used to set up the oscillators and LETIMER0 for assignment 2
 *
 */

#include "sleep.h"

int Osc_INIT(SLEEP_EnergyMode_t blocked_mode, int period);
void LETIMER0_INIT(int period);
void waitms(int ms_wait);
void TimerSetEventMs(int wait_ms);

/*
 * File used to set up the I2C communication for the temp sensor
 *
 */

#include "em_i2c.h"
#include "native_gecko.h"

void I2C_INIT(void);
void Temp_Events(struct gecko_cmd_packet* evt);
void I2C0_IRQHandler(void);
void Temp_Measurement(void);
I2C_TransferReturn_TypeDef I2CWrite(void);
I2C_TransferReturn_TypeDef I2CRead(void);

/*
 * ADXL345 helper wrapper over the FreeRTOS I2C device driver.
 */

#ifndef ADXL345_H_
#define ADXL345_H_

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/
#include "main.h"
#include "task_i2c_interface.h"

/********************** macros ***********************************************/
#define ADXL345_I2C_ADDRESS		0x1D
#define ADXL345_REG_DEVID		0x00
#define ADXL345_REG_POWER_CTL	0x2D
#define ADXL345_MEASURE_MODE	0x08

/********************** external functions definition ************************/
static inline void adxl345_write_reg(I2C_HandleTypeDef *h_i2c_device, uint8_t reg, uint8_t data)
{
	uint8_t buffer[2];

	buffer[0] = reg;
	buffer[1] = data;

	write_i2c(h_i2c_device, ADXL345_I2C_ADDRESS, buffer, sizeof(buffer));
}

static inline void adxl345_read_reg(I2C_HandleTypeDef *h_i2c_device, uint8_t reg, uint8_t *data)
{
	read_i2c(h_i2c_device, ADXL345_I2C_ADDRESS, reg, data, sizeof(*data));
}

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* ADXL345_H_ */

/********************** end of file ******************************************/

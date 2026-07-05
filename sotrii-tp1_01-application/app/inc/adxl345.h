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
#define ADXL345_REG_BW_RATE		0x2C
#define ADXL345_REG_POWER_CTL	0x2D
#define ADXL345_REG_DATA_FORMAT	0x31
#define ADXL345_REG_DATA_BASE	0x32
#define ADXL345_DATA_RATE_100HZ	0x0A
#define ADXL345_FULL_RES_2G		0x08
#define ADXL345_MEASURE_MODE	0x08

/********************** typedef **********************************************/
typedef struct
{
	int16_t x;
	int16_t y;
	int16_t z;
} adxl345_raw_t;

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

static inline void adxl345_read_regs(I2C_HandleTypeDef *h_i2c_device, uint8_t reg, uint8_t *data, uint16_t size)
{
	read_i2c(h_i2c_device, ADXL345_I2C_ADDRESS, reg, data, size);
}

static inline void adxl345_init(I2C_HandleTypeDef *h_i2c_device)
{
	adxl345_write_reg(h_i2c_device, ADXL345_REG_POWER_CTL, 0x00u);
	adxl345_write_reg(h_i2c_device, ADXL345_REG_BW_RATE, ADXL345_DATA_RATE_100HZ);
	adxl345_write_reg(h_i2c_device, ADXL345_REG_DATA_FORMAT, ADXL345_FULL_RES_2G);
	adxl345_write_reg(h_i2c_device, ADXL345_REG_POWER_CTL, ADXL345_MEASURE_MODE);
}

static inline void adxl345_read_raw(I2C_HandleTypeDef *h_i2c_device, adxl345_raw_t *raw)
{
	uint8_t data[6];

	adxl345_read_regs(h_i2c_device, ADXL345_REG_DATA_BASE, data, sizeof(data));

	raw->x = (int16_t)((data[1] << 8) | data[0]);
	raw->y = (int16_t)((data[3] << 8) | data[2]);
	raw->z = (int16_t)((data[5] << 8) | data[4]);
}

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* ADXL345_H_ */

/********************** end of file ******************************************/

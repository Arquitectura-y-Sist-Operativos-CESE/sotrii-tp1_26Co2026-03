/*
 * I2C demo peripheral selection.
 */

#ifndef I2C_DEMO_CONFIG_H_
#define I2C_DEMO_CONFIG_H_

#define I2C_DEMO_PERIPHERAL_ADXL345		0
#define I2C_DEMO_PERIPHERAL_LCD			1
#define I2C_DEMO_PERIPHERAL_LEVEL_LCD	2

/* Select the peripheral currently connected to the I2C bus. */
#define I2C_DEMO_PERIPHERAL				I2C_DEMO_PERIPHERAL_LEVEL_LCD

#endif /* I2C_DEMO_CONFIG_H_ */

/********************** end of file ******************************************/

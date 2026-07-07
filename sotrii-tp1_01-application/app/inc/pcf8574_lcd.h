/*
 * PCF8574T LCD helper wrapper over the FreeRTOS I2C device driver.
 *
 * Expected backpack wiring:
 * P0=RS, P1=RW, P2=E, P3=Backlight, P4..P7=LCD D4..D7.
 */

#ifndef PCF8574_LCD_H_
#define PCF8574_LCD_H_

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/
#include "main.h"
#include "cmsis_os.h"
#include "task_i2c_interface.h"

// The Microchipotle/CCS examples use 0x4E as the 8-bit write address.
// This driver receives 7-bit addresses and shifts internally, so 0x4E >> 1 = 0x27.
/********************** macros ***********************************************/
#define PCF8574_LCD_I2C_ADDRESS		0x27

#define PCF8574_LCD_RS				0x01
#define PCF8574_LCD_RW				0x02
#define PCF8574_LCD_EN				0x04
#define PCF8574_LCD_BACKLIGHT		0x08

#define PCF8574_LCD_CMD_CLEAR		0x01
#define PCF8574_LCD_CMD_HOME		0x02
#define PCF8574_LCD_CMD_ENTRY_MODE	0x06
#define PCF8574_LCD_CMD_DISPLAY_ON	0x0C
#define PCF8574_LCD_CMD_FUNCTION	0x28
#define PCF8574_LCD_CMD_LINE_1		0x80
#define PCF8574_LCD_CMD_LINE_2		0xC0

#define PCF8574_LCD_INIT_DELAY_MS	5
#define PCF8574_LCD_STEP_DELAY_MS	1
#define PCF8574_LCD_CLEAR_DELAY_MS	2

/********************** external functions definition ************************/
static inline void pcf8574_lcd_write_raw(I2C_HandleTypeDef *h_i2c_device, uint8_t data)
{
	write_i2c(h_i2c_device, PCF8574_LCD_I2C_ADDRESS, &data, sizeof(data));
}

static inline void pcf8574_lcd_pulse_enable(I2C_HandleTypeDef *h_i2c_device, uint8_t data)
{
	pcf8574_lcd_write_raw(h_i2c_device, data | PCF8574_LCD_EN);
	taskYIELD();
	pcf8574_lcd_write_raw(h_i2c_device, data & (uint8_t)~PCF8574_LCD_EN);
	taskYIELD();
}

static inline void pcf8574_lcd_write4(I2C_HandleTypeDef *h_i2c_device, uint8_t nibble, uint8_t control)
{
	uint8_t data;

	data = (uint8_t)((nibble & 0xF0u) | PCF8574_LCD_BACKLIGHT | control);
	pcf8574_lcd_pulse_enable(h_i2c_device, data);
}

static inline void pcf8574_lcd_send(I2C_HandleTypeDef *h_i2c_device, uint8_t value, uint8_t control)
{
	pcf8574_lcd_write4(h_i2c_device, value & 0xF0u, control);
	pcf8574_lcd_write4(h_i2c_device, (uint8_t)(value << 4), control);
}

static inline void pcf8574_lcd_command(I2C_HandleTypeDef *h_i2c_device, uint8_t command)
{
	pcf8574_lcd_send(h_i2c_device, command, 0u);

	if ((PCF8574_LCD_CMD_CLEAR == command) || (PCF8574_LCD_CMD_HOME == command))
	{
		vTaskDelay(pdMS_TO_TICKS(PCF8574_LCD_CLEAR_DELAY_MS));
	}
}

static inline void pcf8574_lcd_data(I2C_HandleTypeDef *h_i2c_device, uint8_t data)
{
	pcf8574_lcd_send(h_i2c_device, data, PCF8574_LCD_RS);
}

static inline void pcf8574_lcd_init(I2C_HandleTypeDef *h_i2c_device)
{
	vTaskDelay(pdMS_TO_TICKS(PCF8574_LCD_INIT_DELAY_MS));

	/* HD44780 4-bit initialization sequence through the PCF8574 expander. */
	pcf8574_lcd_write4(h_i2c_device, 0x30u, 0u);
	vTaskDelay(pdMS_TO_TICKS(PCF8574_LCD_STEP_DELAY_MS));
	pcf8574_lcd_write4(h_i2c_device, 0x30u, 0u);
	taskYIELD();
	pcf8574_lcd_write4(h_i2c_device, 0x30u, 0u);
	taskYIELD();
	pcf8574_lcd_write4(h_i2c_device, 0x20u, 0u);

	pcf8574_lcd_command(h_i2c_device, PCF8574_LCD_CMD_FUNCTION);
	pcf8574_lcd_command(h_i2c_device, PCF8574_LCD_CMD_DISPLAY_ON);
	pcf8574_lcd_command(h_i2c_device, PCF8574_LCD_CMD_CLEAR);
	pcf8574_lcd_command(h_i2c_device, PCF8574_LCD_CMD_ENTRY_MODE);
}

static inline void pcf8574_lcd_set_cursor(I2C_HandleTypeDef *h_i2c_device, uint8_t row, uint8_t col)
{
	uint8_t base;

	base = (0u == row) ? PCF8574_LCD_CMD_LINE_1 : PCF8574_LCD_CMD_LINE_2;
	pcf8574_lcd_command(h_i2c_device, (uint8_t)(base + col));
}

static inline void pcf8574_lcd_write_text(I2C_HandleTypeDef *h_i2c_device, const char *text)
{
	while ('\0' != *text)
	{
		pcf8574_lcd_data(h_i2c_device, (uint8_t)*text);
		text++;
	}
}

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* PCF8574_LCD_H_ */

/********************** end of file ******************************************/

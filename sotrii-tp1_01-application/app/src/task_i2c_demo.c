/*
 * I2C demo arbiter task.
 *
 * The demo task builds the bytes to write/read and sends generic I2C requests
 * to sender/receiver through FreeRTOS queues.
 */

/********************** inclusions *******************************************/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* Project includes */
#include "main.h"
#include "cmsis_os.h"

/* Demo includes */
#include "logger.h"

/* Application & Tasks includes */
#include "adxl345.h"
#include "i2c_demo_config.h"
#include "pcf8574_lcd.h"
#include "task_i2c_demo.h"

/********************** macros and definitions *******************************/
#define TASK_I2C_DEMO_PERIOD_MS			250
#define TASK_I2C_DEMO_RAD_TO_DEG_X10	572.958f

/********************** internal functions declaration ***********************/
static void task_i2c_demo_adxl345(void);
static void task_i2c_demo_lcd(void);
static void task_i2c_demo_level_lcd(void);
static void task_i2c_demo_send_i2c_write(uint16_t address, const uint8_t *data, uint16_t size);
static task_i2c_status_t task_i2c_demo_request_i2c_read(uint16_t address, uint8_t reg, uint16_t size, i2c_demo_receiver_data_t *data);
static void task_i2c_demo_adxl345_write(uint8_t reg, uint8_t data);
static void task_i2c_demo_adxl345_init(void);
static task_i2c_status_t task_i2c_demo_adxl345_read_raw(int16_t *x, int16_t *y, int16_t *z);
static void task_i2c_demo_lcd_write_buffer(const uint8_t *data, uint16_t size);
static void task_i2c_demo_lcd_pulse_enable(uint8_t data);
static void task_i2c_demo_lcd_write4(uint8_t nibble, uint8_t control);
static void task_i2c_demo_lcd_send(uint8_t value, uint8_t control);
static void task_i2c_demo_lcd_command(uint8_t command);
static void task_i2c_demo_lcd_data(uint8_t data);
static void task_i2c_demo_lcd_init(void);
static void task_i2c_demo_lcd_set_cursor(uint8_t row, uint8_t col);
static void task_i2c_demo_lcd_write_text(const char *text);
static void task_i2c_demo_format_angle(char *buffer, size_t buffer_size, int16_t angle_tenths);

/********************** external functions definition ************************/
void task_i2c_demo(void *parameters)
{
	UNUSED(parameters);

	LOGGER_INFO(" ");
	LOGGER_INFO("%s is running - Tick [mS] = %3d", pcTaskGetName(NULL), (int)xTaskGetTickCount());

	switch (I2C_DEMO_PERIPHERAL)
	{
	case I2C_DEMO_PERIPHERAL_ADXL345:
		task_i2c_demo_adxl345();
		break;

	case I2C_DEMO_PERIPHERAL_LCD:
		task_i2c_demo_lcd();
		break;

	case I2C_DEMO_PERIPHERAL_LEVEL_LCD:
		task_i2c_demo_level_lcd();
		break;

	default:
		configASSERT(0);
		break;
	}

	for (;;)
	{
		vTaskDelay(pdMS_TO_TICKS(TASK_I2C_DEMO_PERIOD_MS));
	}
}

/********************** internal functions definition ************************/
static void task_i2c_demo_adxl345(void)
{
	i2c_demo_receiver_data_t receiver_data;

	for (;;)
	{
		/* Demo 0: build an ADXL345 write frame and request a register read. */
		task_i2c_demo_adxl345_write(ADXL345_REG_POWER_CTL, ADXL345_MEASURE_MODE);
		if (TASK_I2C_STATUS_OK == task_i2c_demo_request_i2c_read(ADXL345_I2C_ADDRESS, ADXL345_REG_DEVID, 1u, &receiver_data))
		{
			LOGGER_INFO("   ==> I2C Demo - ADXL345 DEVID: 0x%02X", receiver_data.data[0]);
		}

		vTaskDelay(pdMS_TO_TICKS(TASK_I2C_DEMO_PERIOD_MS));
	}
}

static void task_i2c_demo_lcd(void)
{
	uint32_t counter;

	counter = 0u;

	task_i2c_demo_lcd_init();
	task_i2c_demo_lcd_command(PCF8574_LCD_CMD_CLEAR);

	for (;;)
	{
		/* Demo 1: build the LCD/PCF8574 byte sequence and send it to sender. */
		task_i2c_demo_lcd_set_cursor(0u, 0u);
		task_i2c_demo_lcd_write_text("RTOS2 I2C");
		task_i2c_demo_lcd_set_cursor(1u, 0u);
		task_i2c_demo_lcd_write_text("Driver OK");
		task_i2c_demo_lcd_set_cursor(1u, 10u);
		task_i2c_demo_lcd_data((uint8_t)('0' + (counter % 10u)));
		counter++;

		vTaskDelay(pdMS_TO_TICKS(TASK_I2C_DEMO_PERIOD_MS));
	}
}

static void task_i2c_demo_level_lcd(void)
{
	char lcd_line[I2C_DEMO_TEXT_LENGTH];
	float angle_rad;
	int16_t angle_tenths;
	int16_t x;
	int16_t y;
	int16_t z;

	task_i2c_demo_adxl345_init();
	task_i2c_demo_lcd_init();
	task_i2c_demo_lcd_command(PCF8574_LCD_CMD_CLEAR);
	task_i2c_demo_lcd_set_cursor(0u, 0u);
	task_i2c_demo_lcd_write_text("Nivel ADXL345");

	for (;;)
	{
		/* Demo 2: request raw sensor bytes, compute angle, then build LCD writes. */
		if (TASK_I2C_STATUS_OK == task_i2c_demo_adxl345_read_raw(&x, &y, &z))
		{
			angle_rad = atan2f((float)x, (float)z);
			angle_tenths = (int16_t)(angle_rad * TASK_I2C_DEMO_RAD_TO_DEG_X10);

			task_i2c_demo_format_angle(lcd_line, sizeof(lcd_line), angle_tenths);
			task_i2c_demo_lcd_set_cursor(1u, 0u);
			task_i2c_demo_lcd_write_text(lcd_line);
		}

		vTaskDelay(pdMS_TO_TICKS(TASK_I2C_DEMO_PERIOD_MS));
	}
}

static void task_i2c_demo_send_i2c_write(uint16_t address, const uint8_t *data, uint16_t size)
{
	i2c_demo_sender_cmd_t cmd = { 0 };
	uint16_t i;

	configASSERT(size <= I2C_DEMO_DATA_LENGTH);

	cmd.address = address;
	cmd.size = size;

	for (i = 0u; i < size; i++)
	{
		cmd.data[i] = data[i];
	}

	xQueueSend(h_queue_i2c_demo_to_sender, &cmd, portMAX_DELAY);
}

static task_i2c_status_t task_i2c_demo_request_i2c_read(uint16_t address, uint8_t reg, uint16_t size, i2c_demo_receiver_data_t *data)
{
	i2c_demo_receiver_cmd_t cmd = { 0 };

	configASSERT(size <= I2C_DEMO_DATA_LENGTH);

	cmd.address = address;
	cmd.reg = reg;
	cmd.size = size;

	xQueueSend(h_queue_i2c_demo_to_receiver, &cmd, portMAX_DELAY);
	xQueueReceive(h_queue_i2c_receiver_to_demo, data, portMAX_DELAY);

	if (TASK_I2C_STATUS_OK != data->status)
	{
		LOGGER_INFO("   ==> I2C Demo - I2C read error status: %d", (int)data->status);
	}

	return data->status;
}

static void task_i2c_demo_adxl345_write(uint8_t reg, uint8_t data)
{
	uint8_t buffer[2];

	buffer[0] = reg;
	buffer[1] = data;

	task_i2c_demo_send_i2c_write(ADXL345_I2C_ADDRESS, buffer, sizeof(buffer));
}

static void task_i2c_demo_adxl345_init(void)
{
	task_i2c_demo_adxl345_write(ADXL345_REG_POWER_CTL, 0x00u);
	task_i2c_demo_adxl345_write(ADXL345_REG_BW_RATE, ADXL345_DATA_RATE_100HZ);
	task_i2c_demo_adxl345_write(ADXL345_REG_DATA_FORMAT, ADXL345_FULL_RES_2G);
	task_i2c_demo_adxl345_write(ADXL345_REG_POWER_CTL, ADXL345_MEASURE_MODE);
}

static task_i2c_status_t task_i2c_demo_adxl345_read_raw(int16_t *x, int16_t *y, int16_t *z)
{
	i2c_demo_receiver_data_t receiver_data;

	if (TASK_I2C_STATUS_OK != task_i2c_demo_request_i2c_read(ADXL345_I2C_ADDRESS, ADXL345_REG_DATA_BASE, 6u, &receiver_data))
	{
		return receiver_data.status;
	}

	*x = (int16_t)((receiver_data.data[1] << 8) | receiver_data.data[0]);
	*y = (int16_t)((receiver_data.data[3] << 8) | receiver_data.data[2]);
	*z = (int16_t)((receiver_data.data[5] << 8) | receiver_data.data[4]);

	return receiver_data.status;
}

static void task_i2c_demo_lcd_write_buffer(const uint8_t *data, uint16_t size)
{
	task_i2c_demo_send_i2c_write(PCF8574_LCD_I2C_ADDRESS, data, size);
}

static void task_i2c_demo_lcd_pulse_enable(uint8_t data)
{
	uint8_t pulse[2];

	pulse[0] = data | PCF8574_LCD_EN;
	pulse[1] = data & (uint8_t)~PCF8574_LCD_EN;

	/* Send E high/low in one I2C transaction to reduce queue and bus overhead. */
	task_i2c_demo_lcd_write_buffer(pulse, sizeof(pulse));
}

static void task_i2c_demo_lcd_write4(uint8_t nibble, uint8_t control)
{
	uint8_t data;

	data = (uint8_t)((nibble & 0xF0u) | PCF8574_LCD_BACKLIGHT | control);
	task_i2c_demo_lcd_pulse_enable(data);
}

static void task_i2c_demo_lcd_send(uint8_t value, uint8_t control)
{
	uint8_t high;
	uint8_t low;
	uint8_t pulse[4];

	high = (uint8_t)((value & 0xF0u) | PCF8574_LCD_BACKLIGHT | control);
	low = (uint8_t)(((uint8_t)(value << 4) & 0xF0u) | PCF8574_LCD_BACKLIGHT | control);

	pulse[0] = high | PCF8574_LCD_EN;
	pulse[1] = high & (uint8_t)~PCF8574_LCD_EN;
	pulse[2] = low | PCF8574_LCD_EN;
	pulse[3] = low & (uint8_t)~PCF8574_LCD_EN;

	/* Normal commands/data are two nibbles; send both pulses as one request. */
	task_i2c_demo_lcd_write_buffer(pulse, sizeof(pulse));
}

static void task_i2c_demo_lcd_command(uint8_t command)
{
	task_i2c_demo_lcd_send(command, 0u);

	if ((PCF8574_LCD_CMD_CLEAR == command) || (PCF8574_LCD_CMD_HOME == command))
	{
		vTaskDelay(pdMS_TO_TICKS(PCF8574_LCD_CLEAR_DELAY_MS));
	}
}

static void task_i2c_demo_lcd_data(uint8_t data)
{
	task_i2c_demo_lcd_send(data, PCF8574_LCD_RS);
}

static void task_i2c_demo_lcd_init(void)
{
	vTaskDelay(pdMS_TO_TICKS(PCF8574_LCD_INIT_DELAY_MS));

	/* HD44780 4-bit initialization sequence built by demo, sent by sender. */
	task_i2c_demo_lcd_write4(0x30u, 0u);
	vTaskDelay(pdMS_TO_TICKS(PCF8574_LCD_STEP_DELAY_MS));
	task_i2c_demo_lcd_write4(0x30u, 0u);
	vTaskDelay(pdMS_TO_TICKS(PCF8574_LCD_STEP_DELAY_MS));
	task_i2c_demo_lcd_write4(0x30u, 0u);
	vTaskDelay(pdMS_TO_TICKS(PCF8574_LCD_STEP_DELAY_MS));
	task_i2c_demo_lcd_write4(0x20u, 0u);

	task_i2c_demo_lcd_command(PCF8574_LCD_CMD_FUNCTION);
	vTaskDelay(pdMS_TO_TICKS(PCF8574_LCD_STEP_DELAY_MS));
	task_i2c_demo_lcd_command(PCF8574_LCD_CMD_DISPLAY_ON);
	vTaskDelay(pdMS_TO_TICKS(PCF8574_LCD_STEP_DELAY_MS));
	task_i2c_demo_lcd_command(PCF8574_LCD_CMD_CLEAR);
	vTaskDelay(pdMS_TO_TICKS(PCF8574_LCD_STEP_DELAY_MS));
	task_i2c_demo_lcd_command(PCF8574_LCD_CMD_ENTRY_MODE);
	vTaskDelay(pdMS_TO_TICKS(PCF8574_LCD_STEP_DELAY_MS));
}

static void task_i2c_demo_lcd_set_cursor(uint8_t row, uint8_t col)
{
	uint8_t base;

	base = (0u == row) ? PCF8574_LCD_CMD_LINE_1 : PCF8574_LCD_CMD_LINE_2;
	task_i2c_demo_lcd_command((uint8_t)(base + col));
}

static void task_i2c_demo_lcd_write_text(const char *text)
{
	while ('\0' != *text)
	{
		task_i2c_demo_lcd_data((uint8_t)*text);
		text++;
	}
}

static void task_i2c_demo_format_angle(char *buffer, size_t buffer_size, int16_t angle_tenths)
{
	int16_t integer;
	int16_t decimal;
	char sign;

	sign = (angle_tenths < 0) ? '-' : ' ';
	angle_tenths = (int16_t)abs(angle_tenths);
	integer = (int16_t)(angle_tenths / 10);
	decimal = (int16_t)(angle_tenths % 10);

	snprintf(buffer, buffer_size, "Ang:%c%3d.%1d deg", sign, integer, decimal);
}

/********************** end of file ******************************************/

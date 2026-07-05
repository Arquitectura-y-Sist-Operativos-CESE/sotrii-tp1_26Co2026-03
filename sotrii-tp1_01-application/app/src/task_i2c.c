/*
 * Copyright (c) 2026 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @author : Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
 */

/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"
#include "cmsis_os.h"

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "app.h"
#include "app_it.h"
#include "task_i2c_attribute.h"

/********************** macros and definitions *******************************/
#define G_TASK_XXXX_CNT_INI			0ul
#define G_TASK_XXXX_RUNTIME_US_INI	0ul

#define TASK_XXXX_DEL_ZERO	(pdMS_TO_TICKS(0ul))
#define TASK_XXXX_DEL_MAX	(pdMS_TO_TICKS(250ul))

/********************** internal data declaration ****************************/

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/
void task_i2c_tx(void *parameters);
void task_i2c_rx(void *parameters);

/********************** internal data definition *****************************/
const char *p_task_i2c_tx_wait_250mS	= "   ==> Task I2C TX - Wait:   250mS";
const char *p_task_i2c_rx_wait_250mS	= "   ==> Task I2C RX - Wait:   250mS";

/********************** external data declaration ****************************/
uint32_t g_task_xxxx_tx_cnt;
uint32_t g_task_xxxx_tx_runtime_us;

uint32_t g_task_xxxx_rx_cnt;
uint32_t g_task_xxxx_rx_runtime_us;

/********************** external functions definition ************************/
/* Task I2C TX thread */
void task_i2c_tx(void *parameters)
{
	/*  Declare & Initialize Task Function variables */
	g_task_xxxx_tx_cnt = G_TASK_XXXX_CNT_INI;
	g_task_xxxx_tx_runtime_us = G_TASK_XXXX_RUNTIME_US_INI;

	task_i2c_dta_t *p_task_i2c_tx_dta = (task_i2c_dta_t *)parameters;

	/* Serial LCD I2C Module–PCF8574
	 * https://alselectro.wordpress.com/2016/05/12/serial-lcd-i2c-module-pcf8574/
	 * https://www.ti.com/product/PCF8574
 	 * i2c1_tx_address_rd_wr = ((address base | jumper less address) << 1) | /write
 	 */

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("%s is running - Tick [mS] = %3d", pcTaskGetName(NULL), (int)xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
	{
		/* Update Task Counter */
		g_task_xxxx_tx_cnt++;

		task_i2c_tx_dta_t task_i2c_tx_dta;

		cycle_counter_reset();

		xQueueReceive(p_task_i2c_tx_dta->queue_tx, &task_i2c_tx_dta, portMAX_DELAY);

		HAL_I2C_Master_Transmit(p_task_i2c_tx_dta->device_id, (task_i2c_tx_dta.address << 1), &task_i2c_tx_dta.data, sizeof(task_i2c_tx_dta.data), HAL_MAX_DELAY);

		g_task_xxxx_tx_runtime_us = cycle_counter_get_time_us();

    	/* Print out: Wait 250mS */
		LOGGER_INFO(p_task_i2c_tx_wait_250mS);
		vTaskDelay(TASK_XXXX_DEL_MAX);
	}
}

/* Task I2C RX thread */
void task_i2c_rx(void *parameters)
{
	/* Declare & Initialize Task Function variables */
	g_task_xxxx_rx_cnt = G_TASK_XXXX_CNT_INI;
	g_task_xxxx_rx_runtime_us = G_TASK_XXXX_RUNTIME_US_INI;

	/* Casteo del parámetro para acceder a la estructura del driver */
	task_i2c_dta_t *p_task_i2c_rx_dta = (task_i2c_dta_t *)parameters;

	/* Dirección I2C del dispositivo esclavo (PCF8574) */
	uint16_t dev_address = 0x27;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("%s is running - Tick [mS] = %3d", pcTaskGetName(NULL), (int)xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
	{
		/* Update Task Counter */
		g_task_xxxx_rx_cnt++;

		uint8_t rx_data = 0; // Variable para almacenar el dato leído

		cycle_counter_reset();

		/* 1. Lectura por Polling (Gatekeeper) con validación de hardware */
				if (HAL_I2C_Master_Receive(p_task_i2c_rx_dta->device_id, (dev_address << 1), &rx_data, sizeof(rx_data), HAL_MAX_DELAY) == HAL_OK)
				{
					/* 2. Validación lógica: Chequear que el dato no esté vacío (0x00) */
					if (rx_data != 0x00)
					{
						/* Solo si la lectura fue exitosa Y el dato tiene contenido útil, lo enviamos a la cola */
						xQueueSend(p_task_i2c_rx_dta->queue_rx, &rx_data, portMAX_DELAY);
					}
					else
					{
						//Loggear que se recibió un dato vacío/nulo
						LOGGER_INFO("   ==> Task I2C RX - Dato vacío descartado");
					}
				}

		g_task_xxxx_rx_runtime_us = cycle_counter_get_time_us();

    	/* Print out: Wait 250mS */
		LOGGER_INFO(p_task_i2c_rx_wait_250mS);
		vTaskDelay(TASK_XXXX_DEL_MAX);
	}
}
/********************** end of file ******************************************/

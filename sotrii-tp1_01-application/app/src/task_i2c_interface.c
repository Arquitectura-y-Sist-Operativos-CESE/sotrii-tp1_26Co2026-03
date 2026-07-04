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
#include "task_i2c.h"
#include "task_i2c_attribute.h"
#include "task_i2c_interface.h"

/********************** macros and definitions *******************************/

/********************** internal data declaration ****************************/

/********************** internal data declaration ****************************/
task_i2c_dta_t task_i2c_dta;

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/

/********************** external functions definition ************************/
/* Interface functions */
void open_i2c(I2C_HandleTypeDef *h_i2c_device)
{
	BaseType_t ret;
	task_i2c_dta_t *p_task_i2c_dta = &task_i2c_dta;

	p_task_i2c_dta->device_id = h_i2c_device;

    /* Before a queue is used it must be explicitly created.
	 * Check the queue was created successfully.
     * Add queue to registry. */
	p_task_i2c_dta->queue_tx = xQueueCreate(5, sizeof(task_i2c_tx_dta_t));
	configASSERT(NULL != p_task_i2c_dta->queue_tx);
	vQueueAddToRegistry(p_task_i2c_dta->queue_tx, "Task I2C Tx Queue Handle");

	p_task_i2c_dta->queue_rx = xQueueCreate(10, sizeof(uint8_t));
	configASSERT(NULL != p_task_i2c_dta->queue_rx);
	vQueueAddToRegistry(p_task_i2c_dta->queue_rx, "Task I2C Rx Queue Handle");

    /* Before a task is executed it must be explicitly created.
	 * Check the task was created successfully. */
    ret = xTaskCreate(task_i2c_tx, "Task I2C Tx", (configMINIMAL_STACK_SIZE),
					  (void *)p_task_i2c_dta,
					  (tskIDLE_PRIORITY + 1ul), &p_task_i2c_dta->task_tx);
    configASSERT(pdPASS == ret);

    ret = xTaskCreate(task_i2c_rx, "Task I2C Rx", (configMINIMAL_STACK_SIZE),
    				  (void *)p_task_i2c_dta,
					  (tskIDLE_PRIORITY + 1ul), &p_task_i2c_dta->task_rx);
    configASSERT(pdPASS == ret);
}

void release_i2c(I2C_HandleTypeDef *h_i2c_device)
{
	task_i2c_dta_t *p_task_i2c_dta = &task_i2c_dta;

	p_task_i2c_dta->device_id = h_i2c_device;

	// Check which version of the i2c triggered this function
	if (p_task_i2c_dta->device_id == h_i2c_device)
	{
	    vQueueUnregisterQueue(p_task_i2c_dta->queue_tx);
		vQueueDelete(p_task_i2c_dta->queue_tx);
	    vQueueUnregisterQueue(p_task_i2c_dta->queue_rx);
		vQueueDelete(p_task_i2c_dta->queue_rx);

		vTaskDelete(p_task_i2c_dta->task_tx);
		vTaskDelete(p_task_i2c_dta->task_rx);
	}
}

void write_i2c(I2C_HandleTypeDef *h_i2c_device, uint16_t dev_address, uint8_t dev_data)
{
	task_i2c_dta_t *p_task_i2c_dta = &task_i2c_dta;

	p_task_i2c_dta->device_id = h_i2c_device;

	// Check which version of the i2c triggered this function
	if (p_task_i2c_dta->device_id == h_i2c_device)
	{
		task_i2c_tx_dta_t task_i2c_tx_dta;

		task_i2c_tx_dta.address = dev_address;
		task_i2c_tx_dta.data = dev_data;

		xQueueSend(p_task_i2c_dta->queue_tx, &task_i2c_tx_dta, portMAX_DELAY);
	}
}

void read_i2c(I2C_HandleTypeDef *h_i2c_device)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(h_i2c_device);
}

void ioctl_i2c(I2C_HandleTypeDef *h_i2c_device)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(h_i2c_device);
}

/********************** end of file ******************************************/

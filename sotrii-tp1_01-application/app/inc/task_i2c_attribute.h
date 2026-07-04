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

#ifndef TASK_I2C_ATTRIBUTE_H_
#define TASK_I2C_ATTRIBUTE_H_

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/

/********************** macros ***********************************************/

/********************** typedef **********************************************/
typedef enum
{
	TASK_I2C_STATUS_OK = 0,
	TASK_I2C_STATUS_ERROR,
	TASK_I2C_STATUS_BUSY,
	TASK_I2C_STATUS_TIMEOUT
} task_i2c_status_t;

/* Structure of Task */
typedef struct
{
	I2C_HandleTypeDef * device_id;
	SemaphoreHandle_t	mutex_bus;

	TaskHandle_t		task_tx;
	QueueHandle_t		queue_tx;

	/* TX synchronization resources: write_i2c() blocks until task_i2c_tx()
	 * completes the HAL polling transfer and gives sem_tx_done. */
	SemaphoreHandle_t	sem_tx_done;
	SemaphoreHandle_t	mutex_tx;
	task_i2c_status_t	tx_status;

	TaskHandle_t		task_rx;
	QueueHandle_t		queue_rx;

	/* RX synchronization resources: read_i2c() blocks until task_i2c_rx()
	 * completes the HAL polling transfer and gives sem_rx_done. */
	SemaphoreHandle_t	sem_rx_done;
	SemaphoreHandle_t	mutex_rx;
	task_i2c_status_t	rx_status;
} task_i2c_dta_t;

/* Structure of I2C Tx */
typedef struct
{
	uint16_t	address;
	uint8_t *	data;
	uint16_t	size;
} task_i2c_tx_dta_t;

/* Structure of I2C Rx */
typedef struct
{
	uint16_t	address;
	uint8_t		reg;
	uint8_t *	data;
	uint16_t	size;
} task_i2c_rx_dta_t;

/********************** external data declaration ****************************/

/********************** external functions declaration ***********************/

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* TASK_I2C_ATTRIBUTE_H_ */

/********************** end of file ******************************************/

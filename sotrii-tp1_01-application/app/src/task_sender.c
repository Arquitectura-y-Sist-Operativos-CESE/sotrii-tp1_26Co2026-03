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
#include "task_i2c_demo.h"
#include "task_i2c_interface.h"

/********************** macros and definitions *******************************/
#define G_TASK_SENDER_CNT_INI	0ul

#define TASK_SENDER_DEL_ZERO	(pdMS_TO_TICKS(0ul))
#define TASK_SENDER_DEL_MAX		(pdMS_TO_TICKS(250ul))

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
const char *p_task_sender_wait_250mS		= "   ==> Task SENDER - Wait:   250mS";
const char *p_task_sender_wait_cmnd		    = "   ==> Task SENDER - Wait:   Command from Demo Task";

/********************** external data declaration ****************************/
uint32_t g_task_sender_cnt;

/********************** external functions definition ************************/
/* Task thread */
void task_sender(void *parameters)
{
	i2c_demo_sender_cmd_t cmd;
	task_i2c_status_t status;

	UNUSED(parameters);

	/*  Declare & Initialize Task Function variables */
	g_task_sender_cnt = G_TASK_SENDER_CNT_INI;

	/* Serial LCD I2C Module–PCF8574
	 * https://alselectro.wordpress.com/2016/05/12/serial-lcd-i2c-module-pcf8574/
	 * https://www.ti.com/product/PCF8574
 	 * dev_address = (address base | jumper less address)
 	 */
	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
	{
		/* Print out: Wait for a command from the demo task */
		/* LOGGER_INFO(p_task_sender_wait_cmnd); */
		xQueueReceive(h_queue_i2c_demo_to_sender, &cmd, portMAX_DELAY);

		/* Update Task Counter */
		g_task_sender_cnt++;

		/* The demo task builds the complete write frame. Sender only executes it. */
		status = write_i2c(&hi2c1, cmd.address, cmd.data, cmd.size);
		if (TASK_I2C_STATUS_OK != status)
		{
			LOGGER_INFO("   ==> Task SENDER - I2C write error status: %d", (int)status);
		}
	}
}

/********************** end of file ******************************************/

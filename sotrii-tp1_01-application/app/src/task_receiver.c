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
#define G_TASK_RECEIVER_CNT_INI	0ul

#define TASK_RECEIVER_DEL_ZERO		(pdMS_TO_TICKS(0ul))
#define TASK_RECEIVER_DEL_MAX		(pdMS_TO_TICKS(250ul))

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
const char *p_task_receiver_wait_250mS		= "   ==> Task RECEIVER - Wait:   250mS";
const char *p_task_receiver_wait_cmnd		= "   ==> Task RECEIVER - Wait:   Command from Demo Task";

/********************** external data declaration ****************************/
uint32_t g_task_receiver_cnt;

/********************** external functions definition ************************/
/* Task thread */
void task_receiver(void *parameters)
{
	i2c_demo_receiver_cmd_t cmd;
	i2c_demo_receiver_data_t data;

	UNUSED(parameters);

	/*  Declare & Initialize Task Function variables */
	g_task_receiver_cnt = G_TASK_RECEIVER_CNT_INI;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
	{
		/* Print out: Wait for a command from the demo task */
		/* LOGGER_INFO(p_task_receiver_wait_cmnd); */
		xQueueReceive(h_queue_i2c_demo_to_receiver, &cmd, portMAX_DELAY);

		/* Update Task Counter */
		g_task_receiver_cnt++;

		/* The demo task defines what to read. Receiver only executes it. */
		data.size = cmd.size;
		data.status = read_i2c(&hi2c1, cmd.address, cmd.reg, data.data, data.size);
		if (TASK_I2C_STATUS_OK != data.status)
		{
			LOGGER_INFO("   ==> Task RECEIVER - I2C read error status: %d", (int)data.status);
		}
		xQueueOverwrite(h_queue_i2c_receiver_to_demo, &data);
	}
}

/********************** end of file ******************************************/

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
#include "task_adc.h"
#include "task_adc_attribute.h"
#include "task_adc_interface.h"

/********************** macros and definitions *******************************/

/********************** internal data declaration ****************************/
adc_device_t g_adc_device_1 = {0};
/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/

/********************** external functions definition ************************/
/* Interface functions */
void open_adc(ADC_HandleTypeDef *h_adc_device)
{
    configASSERT(h_adc_device != NULL);
    if (h_adc_device == NULL) return;

    g_adc_device_1.device_id = 1;
    g_adc_device_1.h_adc = h_adc_device;
    g_adc_device_1.is_initialized = false;

    /* Crear la cola estática */
    g_adc_device_1.device_queue = xQueueCreateStatic(
                                      ADC_QUEUE_LENGTH,
                                      ADC_ITEM_SIZE,
                                      g_adc_device_1.queue_storage,
                                      &g_adc_device_1.queue_cb);

    configASSERT(g_adc_device_1.device_queue != NULL);
    if (g_adc_device_1.device_queue == NULL) return;

    /* Iniciar el hardware (DMA en modo circular llenando el Spooler) */
    HAL_StatusTypeDef hal_status = HAL_ADC_Start_DMA(g_adc_device_1.h_adc,
                                                    (uint32_t*)g_adc_device_1.dma_buffer,
                                                    ADC_DMA_BUFFER_SIZE);

    configASSERT(hal_status == HAL_OK);
    if (hal_status == HAL_OK) {
        g_adc_device_1.is_initialized = true;
    }
}

BaseType_t read_adc(ADC_HandleTypeDef *h_adc_device, uint32_t *out_value)
{
    // Para medir WCET: cycle_counter_reset();
    if ((h_adc_device == NULL) || (out_value == NULL)) return pdFAIL;
    if (!g_adc_device_1.is_initialized || (g_adc_device_1.h_adc != h_adc_device)) return pdFAIL;

    /* Patrón Latest Input Only: timeout cortísimo porque el dato ya debería estar */
    if (xQueueReceive(g_adc_device_1.device_queue, out_value, pdMS_TO_TICKS(1)) == pdPASS) {
        return pdPASS;

	    // Para medir WCET: uint32_t wcet = cycle_counter_get_time_us();
    }
    return pdFAIL;
}

void release_adc(ADC_HandleTypeDef *h_adc_device)
{
    if ((h_adc_device != NULL) && (g_adc_device_1.h_adc == h_adc_device) && g_adc_device_1.is_initialized) {
        HAL_StatusTypeDef hal_status = HAL_ADC_Stop_DMA(g_adc_device_1.h_adc);
        configASSERT(hal_status == HAL_OK);
        if (hal_status == HAL_OK) {
            g_adc_device_1.is_initialized = false;
        }
    }
}
void write_adc(ADC_HandleTypeDef *h_adc_device)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(h_adc_device);
}


void ioctl_adc(ADC_HandleTypeDef *h_adc_device)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(h_adc_device);
}

/********************** end of file ******************************************/

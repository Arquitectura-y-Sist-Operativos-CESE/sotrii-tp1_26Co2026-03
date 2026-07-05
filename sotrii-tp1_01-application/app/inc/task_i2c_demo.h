/*
 * I2C demo task.
 */

#ifndef TASK_I2C_DEMO_H_
#define TASK_I2C_DEMO_H_

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/
#include "cmsis_os.h"
#include "task_i2c_attribute.h"

/********************** macros ***********************************************/
#define I2C_DEMO_QUEUE_LENGTH		1
#define I2C_DEMO_TEXT_LENGTH		17
#define I2C_DEMO_DATA_LENGTH		16

/********************** typedef **********************************************/
typedef struct
{
	uint16_t address;
	uint8_t data[I2C_DEMO_DATA_LENGTH];
	uint16_t size;
} i2c_demo_sender_cmd_t;

typedef struct
{
	uint16_t address;
	uint8_t reg;
	uint16_t size;
} i2c_demo_receiver_cmd_t;

typedef struct
{
	task_i2c_status_t status;
	uint8_t data[I2C_DEMO_DATA_LENGTH];
	uint16_t size;
} i2c_demo_receiver_data_t;

/********************** external data declaration ****************************/
extern QueueHandle_t h_queue_i2c_demo_to_sender;
extern QueueHandle_t h_queue_i2c_demo_to_receiver;
extern QueueHandle_t h_queue_i2c_receiver_to_demo;

/********************** external functions declaration ***********************/
extern void task_i2c_demo(void *parameters);

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* TASK_I2C_DEMO_H_ */

/********************** end of file ******************************************/

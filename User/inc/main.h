
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

void Error_Handler(void);

#define GREEN_LED_Pin GPIO_PIN_12
#define GREEN_LED_GPIO_Port GPIOD
#define ORANGE_LED_Pin GPIO_PIN_13
#define ORANGE_LED_GPIO_Port GPIOD
#define RED_LED_Pin GPIO_PIN_14
#define RED_LED_GPIO_Port GPIOD
#define BLUE_LED_Pin GPIO_PIN_15
#define BLUE_LED_GPIO_Port GPIOD

#define FLASH_PAGE_SIZE		2048 //2 Kbyte per page
#define FLASH_START_ADDR	0x08000000//Origin
#define FLASH_MAX_SIZE      0x00100000// 1 MB Flash

#define FLASH_END_ADDR		   (FLASH_START_ADDR + FLASH_MAX_SIZE)	//FLASH end address
#define FLASH_BOOT_START_ADDR  (FLASH_START_ADDR)//Bootloader start address
#define FLASH_BOOT_SIZE		    0x00020000//64 Kbyte for bootloader

#define FLASH_USER_START_ADDR	(FLASH_BOOT_START_ADDR + FLASH_BOOT_SIZE)	//User application start address

#define FLASH_LAST_ADDR      (FLASH_END_ADDR - 1)
#define FLASH_USER_SIZE		 (FLASH_LAST_ADDR - FLASH_USER_START_ADDR + 1)
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

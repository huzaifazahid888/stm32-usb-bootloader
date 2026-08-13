#ifndef __GPIO_H__
#define __GPIO_H__

#include "main.h"

//-------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

void GPIO_Init(void);
void gpioWritePort(GPIO_TypeDef* GPIOx, uint16_t v);
void gpioWritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint8_t v);
void gpioSetPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void gpioResetPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
uint16_t gpioReadPort(GPIO_TypeDef* GPIOx);
uint8_t gpioReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
IRQn_Type getExtIRQforPin(uint16_t GPIO_Pin);


void setPinMode(GPIO_TypeDef *, uint16_t, uint8_t);
void pinModeInterrupt(GPIO_TypeDef *, uint16_t, uint8_t);
void enablePortsClock(void);
void GPIOE_EnableVeryHighSpeed(void);
void toggleLed();
void setLED(char );

#ifdef __cplusplus
}
#endif


#endif //__ GPIO_H__


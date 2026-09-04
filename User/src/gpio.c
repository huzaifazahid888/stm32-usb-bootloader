#include "io_pins.h"
#include "gpio.h"


void GPIO_Init(void)
{
	//GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	enablePortsClock(); //GPIO Ports Clock Enable
	setPinMode(GPIOD,GPIO_PIN_12,OUTPUT);
	setPinMode(GPIOC,GPIO_PIN_0,OUTPUT);//for usb power related

	gpioWritePin(GPIOD,GPIO_PIN_12,GPIO_PIN_RESET);
	gpioWritePin(GPIOC,GPIO_PIN_0,GPIO_PIN_RESET);
}


void gpioWritePort(GPIO_TypeDef* GPIOx, uint16_t v)
{
	GPIOx->ODR = v;
}

void gpioWritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, uint8_t v)
{
	if(v == 0)	//reset
	{
		GPIOx->BSRR = (uint32_t)GPIO_Pin << 16U;
	}
	else	//set
	{
		GPIOx->BSRR = GPIO_Pin;
	}
}

void gpioSetPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	GPIOx->BSRR = GPIO_Pin;
}

void gpioResetPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	GPIOx->BSRR = (uint32_t)GPIO_Pin << 16U;
}

uint16_t gpioReadPort(GPIO_TypeDef* GPIOx)
{
	return (GPIOx->IDR & 0x0000FFFF);
}

uint8_t gpioReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	if((GPIOx->IDR & GPIO_Pin) != 0)
	{
		return 1;
	}

	return 0;
}

void toggleLed()
{
	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12);
}

void enablePortsClock(void)
{
	//__HAL_RCC_GPIOE_CLK_ENABLE();
	//__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
}
void GPIOE_EnableVeryHighSpeed(void)
{
	GPIOE->OSPEEDR = 0xFFFFFFFF; // Set (PE0..PE15) to very high speed (11b per pin)
}

void setPinMode(GPIO_TypeDef *port, uint16_t pNumber, uint8_t mode)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pNumber;

    switch(mode)
    {
        case OUTPUT:
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // push pull
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
            break;

        case OUTPUT_OpenDrain:
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD; //open drain
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
            break;

        case INPUT:
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
            GPIO_InitStruct.Pull = GPIO_NOPULL;  // floating input
            break;

        case INPUT_PULLUP:
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
            GPIO_InitStruct.Pull = GPIO_PULLUP;
            break;

        case INPUT_PULLDOWN:
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
            GPIO_InitStruct.Pull = GPIO_PULLDOWN;
            break;
    }

    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

void pinModeInterrupt(GPIO_TypeDef *port, uint16_t pNumber, uint8_t edge)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pNumber;

    switch(edge)
    {

    case   INPUT_IT_RISING:
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;           // or GPIO_PULLUP / PULLDOWN
    break;

    case   INPUT_IT_FALLING:
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    break;

    case   INPUT_IT_BOTH:
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    break;

    }

    HAL_GPIO_Init(port, &GPIO_InitStruct);

    // Enable EXTI line in NVIC
    IRQn_Type irq;
    switch(pNumber)
    {
        case GPIO_PIN_0: irq = EXTI0_IRQn; break;
        case GPIO_PIN_1: irq = EXTI1_IRQn; break;
        case GPIO_PIN_2: irq = EXTI2_IRQn; break;
        case GPIO_PIN_3: irq = EXTI3_IRQn; break;
        case GPIO_PIN_4: irq = EXTI4_IRQn; break;
        case GPIO_PIN_5: case GPIO_PIN_6: case GPIO_PIN_7:
        case GPIO_PIN_8: case GPIO_PIN_9: irq = EXTI9_5_IRQn; break;
        default: irq = EXTI15_10_IRQn; break;
    }

    HAL_NVIC_SetPriority(irq, 0, 0);
    HAL_NVIC_EnableIRQ(irq);
}






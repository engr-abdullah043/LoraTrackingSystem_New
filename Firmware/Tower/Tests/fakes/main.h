#ifndef TEST_FAKE_MAIN_H
#define TEST_FAKE_MAIN_H

#include <stdint.h>

typedef struct
{
    uint32_t instance;
} GPIO_TypeDef;

typedef enum
{
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET
} GPIO_PinState;

typedef enum
{
    HAL_OK = 0,
    HAL_ERROR = 1
} HAL_StatusTypeDef;

extern GPIO_TypeDef test_gpio_a;

#define GPIOA ( &test_gpio_a )
#define GPIO_PIN_1 ( ( uint16_t ) 0x0002U )
#define GPIO_PIN_2 ( ( uint16_t ) 0x0004U )
#define GPIO_PIN_3 ( ( uint16_t ) 0x0008U )
#define GPIO_PIN_4 ( ( uint16_t ) 0x0010U )

#define NSS_SPI_Pin GPIO_PIN_1
#define NSS_SPI_GPIO_Port GPIOA
#define BUSY_LoRa_Pin GPIO_PIN_2
#define BUSY_LoRa_GPIO_Port GPIOA
#define RST_Lora_Pin GPIO_PIN_3
#define RST_Lora_GPIO_Port GPIOA
#define DIO1_Pin GPIO_PIN_4
#define DIO1_GPIO_Port GPIOA

GPIO_PinState HAL_GPIO_ReadPin( GPIO_TypeDef* port, uint16_t pin );
void          HAL_GPIO_WritePin( GPIO_TypeDef* port, uint16_t pin, GPIO_PinState state );
uint32_t      HAL_GetTick( void );
void          HAL_Delay( uint32_t delay_ms );

#endif

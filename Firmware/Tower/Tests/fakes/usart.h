#ifndef TEST_FAKE_USART_H
#define TEST_FAKE_USART_H

#include <stdint.h>

typedef struct
{
    uint32_t instance;
} UART_HandleTypeDef;

typedef enum
{
    HAL_OK = 0,
    HAL_ERROR = 1
} HAL_StatusTypeDef;

extern UART_HandleTypeDef huart1;

HAL_StatusTypeDef HAL_UART_Transmit( UART_HandleTypeDef* huart, const uint8_t* data, uint16_t size,
                                     uint32_t timeout );

#endif

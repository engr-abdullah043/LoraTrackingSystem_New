#ifndef TEST_FAKE_SPI_H
#define TEST_FAKE_SPI_H

#include "main.h"

typedef struct
{
    uint32_t instance;
} SPI_HandleTypeDef;

extern SPI_HandleTypeDef hspi1;

HAL_StatusTypeDef HAL_SPI_Transmit( SPI_HandleTypeDef* spi, const uint8_t* data, uint16_t size, uint32_t timeout );
HAL_StatusTypeDef HAL_SPI_Receive( SPI_HandleTypeDef* spi, uint8_t* data, uint16_t size, uint32_t timeout );

#endif

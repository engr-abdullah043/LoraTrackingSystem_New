#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "main.h"
#include "spi.h"
#include "sx1262_board.h"
#include "sx126x_hal.h"

#define TEST_ASSERT( condition, code ) \
    do                                  \
    {                                   \
        if( !( condition ) )            \
        {                               \
            return ( code );            \
        }                               \
    } while( 0 )

GPIO_TypeDef      test_gpio_a = { 1U };
SPI_HandleTypeDef hspi1       = { 1U };

static uint32_t           tick_ms;
static uint32_t           busy_reads_remaining;
static HAL_StatusTypeDef  spi_transmit_result;
static HAL_StatusTypeDef  spi_receive_result;
static uint8_t            transmitted[32];
static size_t             transmitted_length;
static uint8_t            receive_value;
static GPIO_PinState      nss_state;
static GPIO_PinState      reset_state;
static uint32_t           nss_low_count;
static uint32_t           nss_high_count;
static uint32_t           reset_low_count;
static uint32_t           reset_high_count;
static uint32_t           delay_total_ms;

GPIO_PinState HAL_GPIO_ReadPin( GPIO_TypeDef* port, uint16_t pin )
{
    ( void ) port;
    if( ( pin == BUSY_LoRa_Pin ) && ( busy_reads_remaining > 0U ) )
    {
        --busy_reads_remaining;
        return GPIO_PIN_SET;
    }
    return GPIO_PIN_RESET;
}

void HAL_GPIO_WritePin( GPIO_TypeDef* port, uint16_t pin, GPIO_PinState state )
{
    ( void ) port;
    if( pin == NSS_SPI_Pin )
    {
        nss_state = state;
        if( state == GPIO_PIN_RESET )
        {
            ++nss_low_count;
        }
        else
        {
            ++nss_high_count;
        }
    }
    if( pin == RST_Lora_Pin )
    {
        reset_state = state;
        if( state == GPIO_PIN_RESET )
        {
            ++reset_low_count;
        }
        else
        {
            ++reset_high_count;
        }
    }
}

uint32_t HAL_GetTick( void )
{
    return tick_ms++;
}

void HAL_Delay( uint32_t delay_ms )
{
    delay_total_ms += delay_ms;
    tick_ms += delay_ms;
}

HAL_StatusTypeDef HAL_SPI_Transmit( SPI_HandleTypeDef* spi, const uint8_t* data, uint16_t size, uint32_t timeout )
{
    uint16_t index;
    ( void ) timeout;
    if( ( spi != &hspi1 ) || ( spi_transmit_result != HAL_OK ) )
    {
        return HAL_ERROR;
    }
    for( index = 0U; index < size; ++index )
    {
        transmitted[transmitted_length++] = data[index];
    }
    return HAL_OK;
}

HAL_StatusTypeDef HAL_SPI_Receive( SPI_HandleTypeDef* spi, uint8_t* data, uint16_t size, uint32_t timeout )
{
    uint16_t index;
    ( void ) timeout;
    if( ( spi != &hspi1 ) || ( spi_receive_result != HAL_OK ) )
    {
        return HAL_ERROR;
    }
    for( index = 0U; index < size; ++index )
    {
        data[index] = receive_value;
    }
    return HAL_OK;
}

static void reset_fakes( void )
{
    tick_ms                = 0U;
    busy_reads_remaining   = 0U;
    spi_transmit_result    = HAL_OK;
    spi_receive_result     = HAL_OK;
    transmitted_length     = 0U;
    receive_value          = 0xA4U;
    nss_state              = GPIO_PIN_SET;
    reset_state            = GPIO_PIN_SET;
    nss_low_count          = 0U;
    nss_high_count         = 0U;
    reset_low_count        = 0U;
    reset_high_count       = 0U;
    delay_total_ms         = 0U;
}

void HAL_GPIO_EXTI_Rising_Callback( uint16_t GPIO_Pin );

int main( void )
{
    const void*   context = sx1262_board_context();
    const uint8_t command[] = { 0x80U, 0x00U };
    const uint8_t payload[] = { 0x55U };
    uint8_t       received = 0U;

    reset_fakes();
    TEST_ASSERT( sx126x_hal_write( context, command, sizeof( command ), payload, sizeof( payload ) ) ==
                     SX126X_HAL_STATUS_OK,
                 1 );
    TEST_ASSERT( transmitted_length == 3U, 2 );
    TEST_ASSERT( transmitted[0] == 0x80U && transmitted[2] == 0x55U, 3 );
    TEST_ASSERT( nss_low_count == 1U && nss_high_count == 1U && nss_state == GPIO_PIN_SET, 4 );

    reset_fakes();
    busy_reads_remaining = 1000U;
    TEST_ASSERT( sx126x_hal_write( context, command, sizeof( command ), NULL, 0U ) == SX126X_HAL_STATUS_ERROR, 5 );
    TEST_ASSERT( sx1262_board_last_error() == SX1262_BOARD_ERROR_BUSY_TIMEOUT, 6 );
    TEST_ASSERT( nss_low_count == 0U, 7 );

    reset_fakes();
    spi_transmit_result = HAL_ERROR;
    TEST_ASSERT( sx126x_hal_write( context, command, sizeof( command ), NULL, 0U ) == SX126X_HAL_STATUS_ERROR, 8 );
    TEST_ASSERT( sx1262_board_last_error() == SX1262_BOARD_ERROR_SPI, 9 );
    TEST_ASSERT( nss_state == GPIO_PIN_SET && nss_high_count == 1U, 10 );

    reset_fakes();
    TEST_ASSERT( sx126x_hal_read( context, command, 1U, &received, 1U ) == SX126X_HAL_STATUS_OK, 11 );
    TEST_ASSERT( received == 0xA4U && sx1262_board_last_status_byte() == 0xA4U, 12 );

    reset_fakes();
    TEST_ASSERT( sx126x_hal_reset( context ) == SX126X_HAL_STATUS_OK, 13 );
    TEST_ASSERT( reset_low_count == 1U && reset_high_count == 1U && reset_state == GPIO_PIN_SET, 14 );
    TEST_ASSERT( delay_total_ms == 11U, 15 );

    reset_fakes();
    TEST_ASSERT( sx126x_hal_wakeup( context ) == SX126X_HAL_STATUS_OK, 16 );
    TEST_ASSERT( transmitted_length == 2U && transmitted[0] == 0xC0U && transmitted[1] == 0x00U, 17 );
    TEST_ASSERT( nss_state == GPIO_PIN_SET, 18 );

    TEST_ASSERT( sx1262_board_take_dio1_event() == false, 19 );
    HAL_GPIO_EXTI_Rising_Callback( GPIO_PIN_1 );
    TEST_ASSERT( sx1262_board_take_dio1_event() == false, 20 );
    HAL_GPIO_EXTI_Rising_Callback( DIO1_Pin );
    TEST_ASSERT( sx1262_board_take_dio1_event() == true, 21 );
    TEST_ASSERT( sx1262_board_take_dio1_event() == false, 22 );

    return 0;
}

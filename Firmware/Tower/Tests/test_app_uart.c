#include <stddef.h>
#include <stdint.h>

#include "app_uart.h"
#include "usart.h"

#define TEST_ASSERT( condition, code ) \
    do                                  \
    {                                   \
        if( !( condition ) )            \
        {                               \
            return ( code );            \
        }                               \
    } while( 0 )

UART_HandleTypeDef huart1 = { 1U };

static uint8_t            captured[64];
static size_t             captured_length;
static HAL_StatusTypeDef  transmit_result = HAL_OK;
static UART_HandleTypeDef* last_uart;
static uint32_t           last_timeout;

HAL_StatusTypeDef HAL_UART_Transmit( UART_HandleTypeDef* huart, const uint8_t* data, uint16_t size, uint32_t timeout )
{
    uint16_t index;

    last_uart    = huart;
    last_timeout = timeout;
    if( transmit_result != HAL_OK )
    {
        return transmit_result;
    }
    for( index = 0U; index < size; ++index )
    {
        captured[captured_length++] = data[index];
    }
    return HAL_OK;
}

static void reset_fake( void )
{
    captured_length = 0U;
    transmit_result = HAL_OK;
    last_uart        = NULL;
    last_timeout     = 0U;
}

int main( void )
{
    reset_fake();
    TEST_ASSERT( app_uart_write( "boot" ) == APP_UART_OK, 1 );
    TEST_ASSERT( captured_length == 4U, 2 );
    TEST_ASSERT( captured[0] == 'b' && captured[3] == 't', 3 );
    TEST_ASSERT( last_uart == &huart1, 4 );
    TEST_ASSERT( last_timeout > 0U, 5 );

    reset_fake();
    TEST_ASSERT( app_uart_write_line( "ready" ) == APP_UART_OK, 6 );
    TEST_ASSERT( captured_length == 7U, 7 );
    TEST_ASSERT( captured[5] == '\r' && captured[6] == '\n', 8 );

    reset_fake();
    TEST_ASSERT( app_uart_write_hex8( 0xAFU ) == APP_UART_OK, 9 );
    TEST_ASSERT( captured_length == 2U, 10 );
    TEST_ASSERT( captured[0] == 'A' && captured[1] == 'F', 11 );

    TEST_ASSERT( app_uart_write( NULL ) == APP_UART_ERROR, 12 );

    reset_fake();
    transmit_result = HAL_ERROR;
    TEST_ASSERT( app_uart_write( "fail" ) == APP_UART_ERROR, 13 );

    return 0;
}

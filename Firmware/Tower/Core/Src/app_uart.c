#include "app_uart.h"

#include <stddef.h>

#include "usart.h"

#define APP_UART_TIMEOUT_MS ( 100U )

static app_uart_status_t app_uart_write_buffer( const uint8_t* data, size_t length )
{
    if( ( data == NULL ) || ( length > UINT16_MAX ) )
    {
        return APP_UART_ERROR;
    }
    if( length == 0U )
    {
        return APP_UART_OK;
    }

    return ( HAL_UART_Transmit( &huart1, data, ( uint16_t ) length, APP_UART_TIMEOUT_MS ) == HAL_OK ) ? APP_UART_OK
                                                                                                     : APP_UART_ERROR;
}

app_uart_status_t app_uart_write( const char* text )
{
    size_t length = 0U;

    if( text == NULL )
    {
        return APP_UART_ERROR;
    }
    while( text[length] != '\0' )
    {
        ++length;
    }

    return app_uart_write_buffer( ( const uint8_t* ) text, length );
}

app_uart_status_t app_uart_write_line( const char* text )
{
    static const uint8_t line_ending[] = { '\r', '\n' };

    if( app_uart_write( text ) != APP_UART_OK )
    {
        return APP_UART_ERROR;
    }
    return app_uart_write_buffer( line_ending, sizeof( line_ending ) );
}

app_uart_status_t app_uart_write_hex8( uint8_t value )
{
    static const uint8_t hex_digits[] = "0123456789ABCDEF";
    uint8_t              encoded[2];

    encoded[0] = hex_digits[( value >> 4 ) & 0x0FU];
    encoded[1] = hex_digits[value & 0x0FU];
    return app_uart_write_buffer( encoded, sizeof( encoded ) );
}

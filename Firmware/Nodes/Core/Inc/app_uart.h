#ifndef APP_UART_H
#define APP_UART_H

#include <stdint.h>

typedef enum
{
    APP_UART_OK = 0,
    APP_UART_ERROR
} app_uart_status_t;

app_uart_status_t app_uart_write( const char* text );
app_uart_status_t app_uart_write_line( const char* text );
app_uart_status_t app_uart_write_hex8( uint8_t value );
app_uart_status_t app_uart_write_u32( uint32_t value );
app_uart_status_t app_uart_write_i8( int8_t value );
app_uart_status_t app_uart_write_bytes( const uint8_t* data, uint8_t length );

#endif

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

#endif

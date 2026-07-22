#include <stddef.h>
#include <stdint.h>

#include "app_uart.h"
#include "sx1262_board.h"
#include "sx1262_bringup.h"
#include "sx126x.h"

#define TEST_ASSERT( condition, code ) \
    do                                  \
    {                                   \
        if( !( condition ) )            \
        {                               \
            return ( code );            \
        }                               \
    } while( 0 )

static sx126x_status_t       reset_result;
static sx126x_status_t       standby_result;
static sx126x_status_t       get_status_result;
static sx1262_board_error_t  board_error;
static sx126x_chip_status_t  chip_status;
static uint8_t               raw_status;
static app_uart_status_t     uart_result;
static uint8_t               call_order[4];
static size_t                call_count;
static char                  transcript[512];
static size_t                transcript_length;

static void append_character( char value )
{
    if( transcript_length < ( sizeof( transcript ) - 1U ) )
    {
        transcript[transcript_length++] = value;
        transcript[transcript_length]   = '\0';
    }
}

app_uart_status_t app_uart_write( const char* text )
{
    size_t index = 0U;
    if( ( uart_result != APP_UART_OK ) || ( text == NULL ) )
    {
        return APP_UART_ERROR;
    }
    while( text[index] != '\0' )
    {
        append_character( text[index++] );
    }
    return APP_UART_OK;
}

app_uart_status_t app_uart_write_line( const char* text )
{
    if( app_uart_write( text ) != APP_UART_OK )
    {
        return APP_UART_ERROR;
    }
    append_character( '\r' );
    append_character( '\n' );
    return APP_UART_OK;
}

app_uart_status_t app_uart_write_hex8( uint8_t value )
{
    static const char digits[] = "0123456789ABCDEF";
    if( uart_result != APP_UART_OK )
    {
        return APP_UART_ERROR;
    }
    append_character( digits[( value >> 4 ) & 0x0FU] );
    append_character( digits[value & 0x0FU] );
    return APP_UART_OK;
}

const void* sx1262_board_context( void )
{
    static uint8_t context;
    return &context;
}

sx1262_board_error_t sx1262_board_last_error( void )
{
    return board_error;
}

uint8_t sx1262_board_last_status_byte( void )
{
    return raw_status;
}

bool sx1262_board_take_dio1_event( void )
{
    return false;
}

sx126x_status_t sx126x_reset( const void* context )
{
    ( void ) context;
    call_order[call_count++] = 1U;
    return reset_result;
}

sx126x_status_t sx126x_set_standby( const void* context, sx126x_standby_cfg_t cfg )
{
    ( void ) context;
    if( cfg != SX126X_STANDBY_CFG_RC )
    {
        return SX126X_STATUS_UNKNOWN_VALUE;
    }
    call_order[call_count++] = 2U;
    return standby_result;
}

sx126x_status_t sx126x_get_status( const void* context, sx126x_chip_status_t* radio_status )
{
    ( void ) context;
    call_order[call_count++] = 3U;
    if( radio_status != NULL )
    {
        radio_status->chip_mode  = chip_status.chip_mode;
        radio_status->cmd_status = chip_status.cmd_status;
    }
    return get_status_result;
}

static void reset_fakes( void )
{
    reset_result              = SX126X_STATUS_OK;
    standby_result            = SX126X_STATUS_OK;
    get_status_result         = SX126X_STATUS_OK;
    board_error               = SX1262_BOARD_ERROR_NONE;
    chip_status.chip_mode     = SX126X_CHIP_MODE_STBY_RC;
    chip_status.cmd_status    = SX126X_CMD_STATUS_DATA_AVAILABLE;
    raw_status                = 0xA4U;
    uart_result               = APP_UART_OK;
    call_count                = 0U;
    transcript_length         = 0U;
    transcript[0]             = '\0';
}

static bool transcript_contains( const char* expected )
{
    size_t start;
    for( start = 0U; start < transcript_length; ++start )
    {
        size_t index = 0U;
        while( ( expected[index] != '\0' ) && ( ( start + index ) < transcript_length ) &&
               ( transcript[start + index] == expected[index] ) )
        {
            ++index;
        }
        if( expected[index] == '\0' )
        {
            return true;
        }
    }
    return false;
}

int main( void )
{
    reset_fakes();
    TEST_ASSERT( sx1262_bringup_run() == SX1262_BRINGUP_OK, 1 );
    TEST_ASSERT( call_count == 3U && call_order[0] == 1U && call_order[1] == 2U && call_order[2] == 3U, 2 );
    TEST_ASSERT( transcript_contains( "Status: 0xA4" ), 3 );
    TEST_ASSERT( transcript_contains( "SX1262 BRING-UP: PASS" ), 4 );

    reset_fakes();
    reset_result = SX126X_STATUS_ERROR;
    board_error  = SX1262_BOARD_ERROR_BUSY_TIMEOUT;
    TEST_ASSERT( sx1262_bringup_run() == SX1262_BRINGUP_BUSY_TIMEOUT, 5 );
    TEST_ASSERT( call_count == 1U && transcript_contains( "FAIL (BUSY timeout)" ), 6 );

    reset_fakes();
    standby_result = SX126X_STATUS_ERROR;
    board_error    = SX1262_BOARD_ERROR_SPI;
    TEST_ASSERT( sx1262_bringup_run() == SX1262_BRINGUP_SPI_ERROR, 7 );
    TEST_ASSERT( call_count == 2U, 8 );

    reset_fakes();
    get_status_result = SX126X_STATUS_ERROR;
    TEST_ASSERT( sx1262_bringup_run() == SX1262_BRINGUP_DRIVER_ERROR, 9 );

    reset_fakes();
    chip_status.chip_mode = SX126X_CHIP_MODE_RX;
    TEST_ASSERT( sx1262_bringup_run() == SX1262_BRINGUP_INVALID_STATUS, 10 );

    reset_fakes();
    chip_status.cmd_status = SX126X_CMD_STATUS_CMD_PROCESS_ERROR;
    TEST_ASSERT( sx1262_bringup_run() == SX1262_BRINGUP_INVALID_STATUS, 11 );

    reset_fakes();
    uart_result = APP_UART_ERROR;
    TEST_ASSERT( sx1262_bringup_run() == SX1262_BRINGUP_UART_ERROR, 12 );
    TEST_ASSERT( call_count == 0U, 13 );

    return 0;
}

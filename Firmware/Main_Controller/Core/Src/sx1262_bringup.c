#include "sx1262_bringup.h"

#include "app_uart.h"
#include "sx1262_board.h"
#include "sx126x.h"

static sx1262_bringup_result_t sx1262_bringup_driver_failure( void )
{
    switch( sx1262_board_last_error() )
    {
    case SX1262_BOARD_ERROR_BUSY_TIMEOUT:
        return SX1262_BRINGUP_BUSY_TIMEOUT;
    case SX1262_BOARD_ERROR_SPI:
        return SX1262_BRINGUP_SPI_ERROR;
    default:
        return SX1262_BRINGUP_DRIVER_ERROR;
    }
}

static const char* sx1262_bringup_reason( sx1262_bringup_result_t result )
{
    switch( result )
    {
    case SX1262_BRINGUP_BUSY_TIMEOUT:
        return "BUSY timeout";
    case SX1262_BRINGUP_SPI_ERROR:
        return "SPI error";
    case SX1262_BRINGUP_INVALID_STATUS:
        return "invalid status";
    case SX1262_BRINGUP_DRIVER_ERROR:
        return "driver error";
    default:
        return "unknown error";
    }
}

static sx1262_bringup_result_t sx1262_bringup_report_failure( sx1262_bringup_result_t result )
{
    if( ( app_uart_write( "SX1262 BRING-UP: FAIL (" ) != APP_UART_OK ) ||
        ( app_uart_write( sx1262_bringup_reason( result ) ) != APP_UART_OK ) ||
        ( app_uart_write_line( ")" ) != APP_UART_OK ) )
    {
        return SX1262_BRINGUP_UART_ERROR;
    }
    return result;
}

static bool sx1262_bringup_status_is_valid( const sx126x_chip_status_t* status )
{
    return ( status->chip_mode == SX126X_CHIP_MODE_STBY_RC ) &&
           ( status->cmd_status == SX126X_CMD_STATUS_DATA_AVAILABLE );
}

static bool sx1262_bringup_report_status( const char* label, const sx126x_chip_status_t* status )
{
    return ( app_uart_write( label ) == APP_UART_OK ) &&
           ( app_uart_write_hex8( sx1262_board_last_status_byte() ) == APP_UART_OK ) &&
           ( app_uart_write( ", chip_mode=0x" ) == APP_UART_OK ) &&
           ( app_uart_write_hex8( ( uint8_t ) status->chip_mode ) == APP_UART_OK ) &&
           ( app_uart_write( ", cmd_status=0x" ) == APP_UART_OK ) &&
           ( app_uart_write_hex8( ( uint8_t ) status->cmd_status ) == APP_UART_OK ) &&
           ( app_uart_write_line( "" ) == APP_UART_OK );
}

sx1262_bringup_result_t sx1262_bringup_run( void )
{
    const void*          context = sx1262_board_context();
    sx126x_chip_status_t status  = { 0 };
    sx1262_bringup_result_t failure;

    if( app_uart_write_line( "=== SX1262 UART BRING-UP ===" ) != APP_UART_OK )
    {
        return SX1262_BRINGUP_UART_ERROR;
    }

    if( sx126x_reset( context ) != SX126X_STATUS_OK )
    {
        failure = sx1262_bringup_driver_failure();
        return sx1262_bringup_report_failure( failure );
    }
    if( app_uart_write_line( "Reset: OK" ) != APP_UART_OK )
    {
        return SX1262_BRINGUP_UART_ERROR;
    }

    if( sx126x_set_standby( context, SX126X_STANDBY_CFG_RC ) != SX126X_STATUS_OK )
    {
        failure = sx1262_bringup_driver_failure();
        return sx1262_bringup_report_failure( failure );
    }
    if( app_uart_write_line( "Standby RC: OK" ) != APP_UART_OK )
    {
        return SX1262_BRINGUP_UART_ERROR;
    }

    if( sx126x_get_status( context, &status ) != SX126X_STATUS_OK )
    {
        failure = sx1262_bringup_driver_failure();
        return sx1262_bringup_report_failure( failure );
    }

    if( !sx1262_bringup_report_status( "Status #1: 0x", &status ) )
    {
        return SX1262_BRINGUP_UART_ERROR;
    }

    if( status.cmd_status == SX126X_CMD_STATUS_RFU )
    {
        if( sx126x_get_status( context, &status ) != SX126X_STATUS_OK )
        {
            failure = sx1262_bringup_driver_failure();
            return sx1262_bringup_report_failure( failure );
        }
        if( !sx1262_bringup_report_status( "Status #2: 0x", &status ) )
        {
            return SX1262_BRINGUP_UART_ERROR;
        }
    }

    if( !sx1262_bringup_status_is_valid( &status ) )
    {
        return sx1262_bringup_report_failure( SX1262_BRINGUP_INVALID_STATUS );
    }

    if( app_uart_write_line( "SX1262 BRING-UP: PASS" ) != APP_UART_OK )
    {
        return SX1262_BRINGUP_UART_ERROR;
    }
    return SX1262_BRINGUP_OK;
}
